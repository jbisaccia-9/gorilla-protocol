#include "AI/GPGuardAIController.h"

#include "AI/GPEncounterSubsystem.h"
#include "Characters/GPAgentCharacter.h"
#include "Characters/GPGuardCharacter.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/GPMissionSubsystem.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"

AGPGuardAIController::AGPGuardAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickInterval(0.08f);
    SetGenericTeamId(FGenericTeamId(1));

    GuardPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("GuardPerceptionComponent"));
    SetPerceptionComponent(*GuardPerceptionComponent);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 2200.0f;
    SightConfig->LoseSightRadius = 2800.0f;
    SightConfig->PeripheralVisionAngleDegrees = 58.0f;
    SightConfig->SetMaxAge(8.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = false;

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 3500.0f;
    HearingConfig->SetMaxAge(6.0f);
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;

    GuardPerceptionComponent->ConfigureSense(*SightConfig);
    GuardPerceptionComponent->ConfigureSense(*HearingConfig);
    GuardPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    GuardPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AGPGuardAIController::HandleTargetPerception);
}

void AGPGuardAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    GuardPawn = Cast<AGPGuardCharacter>(InPawn);
    PatrolOrigin = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
    CombatRandom.Initialize(InPawn ? static_cast<int32>(GetTypeHash(InPawn->GetFName())) : 1337);
    if (GuardPawn && GetWorld())
    {
        if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
        {
            TacticalRole = Encounter->RegisterCombatant(GuardPawn);
            LastObservedSuppressionSerial = Encounter->GetSuppressionSerial();
        }
        GuardPawn->GetWeaponComponent()->SetSpreadSeed(CombatRandom.GetInitialSeed());
    }
    EnterState(EGPGuardState::Patrol);
}

void AGPGuardAIController::OnUnPossess()
{
    ReleaseFireToken();
    ReleaseMeleeToken();
    if (GuardPawn && GetWorld())
    {
        if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
        {
            Encounter->UnregisterCombatant(GuardPawn);
        }
    }
    GuardPawn = nullptr;
    TargetActor = nullptr;
    bHasSightStimulus = false;
    Super::OnUnPossess();
}

void AGPGuardAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!GuardPawn || !GetWorld() || GuardState == EGPGuardState::Dead)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    TryAcquireSharedContact();
    switch (GuardState)
    {
        case EGPGuardState::Patrol:
            TickPatrol(Now);
            break;
        case EGPGuardState::Suspicious:
            if (Now >= StateDeadline) EnterState(CanSeeTarget() ? EGPGuardState::Engage : EGPGuardState::Investigate);
            break;
        case EGPGuardState::Investigate:
            TickInvestigate(Now);
            break;
        case EGPGuardState::Engage:
            TickCombat(Now);
            break;
        case EGPGuardState::Reposition:
            TickReposition(Now);
            break;
        case EGPGuardState::Telegraph:
            if (!CanSeeTarget())
            {
                EnterState(EGPGuardState::Investigate);
            }
            else if (Now >= StateDeadline)
            {
                const float Distance = FVector::Dist(GuardPawn->GetActorLocation(), TargetActor->GetActorLocation());
                BurstRoundsRemaining = GetBurstSizeForDistance(Distance);
                EnterState(EGPGuardState::FireBurst);
                NextDecisionTime = Now;
            }
            break;
        case EGPGuardState::FireBurst:
            if (!CanSeeTarget())
            {
                EnterState(EGPGuardState::Investigate);
                break;
            }
            if (Now >= NextDecisionTime && BurstRoundsRemaining > 0)
            {
                UGPWeaponComponent* Weapon = GuardPawn->GetWeaponComponent();
                if (Weapon->FireSingleShot())
                {
                    --BurstRoundsRemaining;
                }
                else if (Weapon->IsReloading() || Weapon->GetMagazineAmmo() <= 0)
                {
                    EnterState(EGPGuardState::Reload, Weapon->GetReloadDuration());
                    break;
                }
                NextDecisionTime = Now + Weapon->GetFireInterval();
            }
            if (BurstRoundsRemaining <= 0)
            {
                if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
                {
                    Encounter->ReportSuppression(GuardPawn, LastKnownLocation);
                    LastObservedSuppressionSerial = Encounter->GetSuppressionSerial();
                }
                ReleaseFireToken();
                ++CompletedBursts;
                NextAttackTime = Now + CombatRandom.FRandRange(0.85f, 1.25f);
                if (CompletedBursts % 2 == 0)
                {
                    BeginReposition(Now, false);
                }
                else
                {
                    EnterState(EGPGuardState::Engage);
                }
            }
            break;
        case EGPGuardState::Reload:
            TickReposition(Now);
            if (!GuardPawn->GetWeaponComponent()->IsReloading())
            {
                NextAttackTime = Now + 0.35f;
                EnterState(HasValidTarget() ? EGPGuardState::Engage : EGPGuardState::Investigate);
            }
            break;
        case EGPGuardState::Stagger:
            if (Now >= StateDeadline) EnterState(HasValidTarget() ? EGPGuardState::Engage : EGPGuardState::Investigate);
            break;
        case EGPGuardState::MeleeWindup:
            if (Now >= StateDeadline)
            {
                if (HasValidTarget() && FVector::DistSquared(TargetActor->GetActorLocation(), GuardPawn->GetActorLocation()) < FMath::Square(150.0f))
                {
                    if (UGPHealthComponent* Health = TargetActor->FindComponentByClass<UGPHealthComponent>())
                    {
                        Health->ReceiveDamage(25.0f, GuardPawn);
                    }
                }
                ReleaseMeleeToken();
                NextAttackTime = Now + 1.15f;
                EnterState(EGPGuardState::Engage);
            }
            break;
        default:
            break;
    }
}

void AGPGuardAIController::HandleTargetPerception(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor || !Actor->IsA<AGPAgentCharacter>() || GuardState == EGPGuardState::Dead || !GetWorld())
    {
        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        LastKnownLocation = Stimulus.StimulusLocation;
        LastContactTime = GetWorld()->GetTimeSeconds();
        if (Stimulus.Type == SightConfig->GetSenseID())
        {
            bHasSightStimulus = true;
            SetTarget(Actor);
            ReportContactToSquad();
            EnterState(EGPGuardState::Suspicious, 0.65f);
        }
        else if (GuardState == EGPGuardState::Patrol)
        {
            TargetActor = Actor;
            EnterState(EGPGuardState::Investigate);
        }
    }
    else if (Actor == TargetActor)
    {
        if (Stimulus.Type == SightConfig->GetSenseID()) bHasSightStimulus = false;
        LastKnownLocation = Stimulus.StimulusLocation;
        LastContactTime = GetWorld()->GetTimeSeconds();
        EnterState(EGPGuardState::Investigate);
    }
}

void AGPGuardAIController::NotifyDamaged(AActor* DamageCauser)
{
    if (GuardState == EGPGuardState::Dead || !GetWorld()) return;
    if (DamageCauser)
    {
        SetTarget(DamageCauser);
        bHasSightStimulus = LineOfSightTo(DamageCauser);
        ReportContactToSquad();
    }
    ReleaseFireToken();
    EnterState(EGPGuardState::Stagger, 0.22f);
}

void AGPGuardAIController::NotifyGuardDeath()
{
    ReleaseFireToken();
    ReleaseMeleeToken();
    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);
    bHasSightStimulus = false;
    EnterState(EGPGuardState::Dead);
    if (GuardPawn && GetWorld())
    {
        if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
        {
            Encounter->UnregisterCombatant(GuardPawn);
        }
    }
}

void AGPGuardAIController::SetTarget(AActor* NewTarget)
{
    TargetActor = NewTarget;
    if (NewTarget)
    {
        LastKnownLocation = NewTarget->GetActorLocation();
        SetFocus(NewTarget, EAIFocusPriority::Gameplay);
    }
}

void AGPGuardAIController::EnterState(EGPGuardState NewState, float Duration)
{
    if (NewState != EGPGuardState::Telegraph && NewState != EGPGuardState::FireBurst)
    {
        ReleaseFireToken();
    }
    if (NewState != EGPGuardState::MeleeWindup)
    {
        ReleaseMeleeToken();
    }
    GuardState = NewState;
    StateDeadline = GetWorld() ? GetWorld()->GetTimeSeconds() + Duration : Duration;
    if (!GuardPawn)
    {
        return;
    }

    UGPWeaponComponent* Weapon = GuardPawn->GetWeaponComponent();
    Weapon->SetAiming(NewState != EGPGuardState::Patrol && NewState != EGPGuardState::Suspicious &&
        NewState != EGPGuardState::Dead);
    if (NewState == EGPGuardState::Engage || NewState == EGPGuardState::Telegraph ||
        NewState == EGPGuardState::FireBurst)
    {
        GuardPawn->GetCharacterMovement()->MaxWalkSpeed = 360.0f;
    }
    if (NewState == EGPGuardState::Engage && GetWorld())
    {
        if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
        {
            Mission->RaiseAlert(EGPMissionAlertState::Alarm);
        }
    }
    switch (NewState)
    {
        case EGPGuardState::Patrol:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::None);
            break;
        case EGPGuardState::Suspicious:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Alert);
            break;
        case EGPGuardState::Reposition:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Flanking);
            break;
        case EGPGuardState::Telegraph:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Telegraph);
            break;
        case EGPGuardState::FireBurst:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Firing);
            break;
        case EGPGuardState::Reload:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Reloading);
            break;
        case EGPGuardState::MeleeWindup:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Melee);
            break;
        case EGPGuardState::Stagger:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Hit);
            break;
        case EGPGuardState::Dead:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::None);
            break;
        default:
            GuardPawn->SetCombatCue(EGPGuardCombatCue::Alert);
            break;
    }

    if (NewState == EGPGuardState::Investigate || NewState == EGPGuardState::Patrol)
    {
        ClearFocus(EAIFocusPriority::Gameplay);
    }
}

void AGPGuardAIController::TickPatrol(float Now)
{
    if (Now < NextDecisionTime || !GuardPawn) return;
    NextDecisionTime = Now + CombatRandom.FRandRange(3.0f, 5.0f);
    if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation Destination;
        if (Navigation->GetRandomReachablePointInRadius(PatrolOrigin, 900.0f, Destination))
        {
            MoveToLocation(Destination.Location, 80.0f, true);
        }
    }
}

void AGPGuardAIController::TickInvestigate(float Now)
{
    MoveToLocation(LastKnownLocation, 100.0f, true);
    if (CanSeeTarget())
    {
        EnterState(EGPGuardState::Engage);
    }
    else if (Now - LastContactTime > 8.0f ||
        (GuardPawn && FVector::DistSquared(GuardPawn->GetActorLocation(), LastKnownLocation) < FMath::Square(140.0f)))
    {
        TargetActor = nullptr;
        bHasSightStimulus = false;
        EnterState(EGPGuardState::Patrol);
    }
}

void AGPGuardAIController::TickCombat(float Now)
{
    if (!HasValidTarget())
    {
        EnterState(EGPGuardState::Investigate);
        return;
    }
    if (!CanSeeTarget())
    {
        LastContactTime = Now;
        EnterState(EGPGuardState::Investigate);
        return;
    }

    const float Distance = FVector::Dist(GuardPawn->GetActorLocation(), TargetActor->GetActorLocation());
    LastKnownLocation = TargetActor->GetActorLocation();
    LastContactTime = Now;
    SetFocus(TargetActor, EAIFocusPriority::Gameplay);
    ReportContactToSquad();

    if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
    {
        const uint32 SuppressionSerial = Encounter->GetSuppressionSerial();
        if (SuppressionSerial != LastObservedSuppressionSerial)
        {
            LastObservedSuppressionSerial = SuppressionSerial;
            if (!bOwnsFireToken && TacticalRole != EGPGuardTacticalRole::Pressure)
            {
                BeginReposition(Now, true);
                return;
            }
        }
    }

    if (Distance < 130.0f && Now >= NextAttackTime)
    {
        if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
        {
            bOwnsMeleeToken = Encounter->RequestMeleeToken(GuardPawn);
        }
        if (bOwnsMeleeToken)
        {
            StopMovement();
            EnterState(EGPGuardState::MeleeWindup, 0.55f);
        }
        else
        {
            BeginReposition(Now, true);
        }
        return;
    }

    UGPWeaponComponent* Weapon = GuardPawn->GetWeaponComponent();
    if (Weapon->GetMagazineAmmo() <= 0)
    {
        Weapon->Reload();
        BeginReposition(Now, false);
        EnterState(EGPGuardState::Reload, Weapon->GetReloadDuration());
        return;
    }

    const float DesiredRange = GetDesiredRangeForRole(TacticalRole);
    if (Now >= NextRepositionTime || Distance > DesiredRange + 650.0f || Distance < DesiredRange - 450.0f)
    {
        BeginReposition(Now, false);
        return;
    }

    StopMovement();

    if (Now >= NextAttackTime)
    {
        if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
        {
            bOwnsFireToken = Encounter->RequestFireToken(GuardPawn);
            if (bOwnsFireToken)
            {
                StopMovement();
                const float TelegraphDuration = TacticalRole == EGPGuardTacticalRole::Support ? 0.62f : 0.48f;
                EnterState(EGPGuardState::Telegraph, TelegraphDuration);
            }
        }
    }
}

void AGPGuardAIController::TickReposition(float Now)
{
    if (!HasValidTarget())
    {
        EnterState(EGPGuardState::Investigate);
        return;
    }

    if (CanSeeTarget())
    {
        LastKnownLocation = TargetActor->GetActorLocation();
        LastContactTime = Now;
        SetFocus(TargetActor, EAIFocusPriority::Gameplay);
    }
    if (GuardState == EGPGuardState::Reposition &&
        (Now >= StateDeadline || GetMoveStatus() == EPathFollowingStatus::Idle))
    {
        EnterState(CanSeeTarget() ? EGPGuardState::Engage : EGPGuardState::Investigate);
    }
}

void AGPGuardAIController::BeginReposition(float Now, bool bForcedBySuppression)
{
    if (!GuardPawn || !HasValidTarget())
    {
        return;
    }

    const FVector TargetLocation = TargetActor->GetActorLocation();
    FVector AwayFromTarget = GuardPawn->GetActorLocation() - TargetLocation;
    if (AwayFromTarget.IsNearlyZero())
    {
        AwayFromTarget = -TargetActor->GetActorForwardVector();
    }
    const float DesiredRange = GetDesiredRangeForRole(TacticalRole);
    const float FlankWidth = bForcedBySuppression ? 760.0f : 590.0f;
    FVector Destination = TargetLocation + UGPEncounterSubsystem::CalculateFormationOffset(
        TacticalRole, AwayFromTarget, DesiredRange, FlankWidth);

    const FVector Right = FVector::CrossProduct(FVector::UpVector, AwayFromTarget.GetSafeNormal2D());
    Destination += Right * CombatRandom.FRandRange(-90.0f, 90.0f);
    if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation ProjectedDestination;
        if (Navigation->ProjectPointToNavigation(Destination, ProjectedDestination, FVector(240.0f, 240.0f, 400.0f)))
        {
            Destination = ProjectedDestination.Location;
        }
    }

    GuardPawn->GetCharacterMovement()->MaxWalkSpeed = bForcedBySuppression ? 470.0f : 420.0f;
    MoveToLocation(Destination, 90.0f, true);
    NextRepositionTime = Now + CombatRandom.FRandRange(3.2f, 4.4f);
    EnterState(EGPGuardState::Reposition, bForcedBySuppression ? 1.75f : 1.45f);
}

void AGPGuardAIController::TryAcquireSharedContact()
{
    if (HasValidTarget() || GuardState == EGPGuardState::Dead || !GetWorld())
    {
        return;
    }

    if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
    {
        AActor* SharedTarget = nullptr;
        FVector SharedLocation;
        if (Encounter->GetSharedContact(SharedTarget, SharedLocation))
        {
            TargetActor = SharedTarget;
            LastKnownLocation = SharedLocation;
            LastContactTime = GetWorld()->GetTimeSeconds();
            bHasSightStimulus = false;
            EnterState(EGPGuardState::Investigate);
        }
    }
}

void AGPGuardAIController::ReportContactToSquad()
{
    if (!GuardPawn || !HasValidTarget() || !GetWorld())
    {
        return;
    }
    if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
    {
        Encounter->ReportContact(GuardPawn, TargetActor, LastKnownLocation);
    }
}

void AGPGuardAIController::ReleaseFireToken()
{
    if (!GetWorld()) return;
    if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
    {
        Encounter->ReleaseFireToken(GuardPawn);
    }
    bOwnsFireToken = false;
}

void AGPGuardAIController::ReleaseMeleeToken()
{
    if (!GetWorld()) return;
    if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
    {
        Encounter->ReleaseMeleeToken(GuardPawn);
    }
    bOwnsMeleeToken = false;
}

bool AGPGuardAIController::HasValidTarget() const
{
    if (!IsValid(TargetActor)) return false;
    const UGPHealthComponent* Health = TargetActor->FindComponentByClass<UGPHealthComponent>();
    return !Health || !Health->IsDead();
}

bool AGPGuardAIController::CanSeeTarget() const
{
    return bHasSightStimulus && HasValidTarget() && LineOfSightTo(TargetActor);
}

float AGPGuardAIController::GetDesiredRangeForRole(EGPGuardTacticalRole Role)
{
    switch (Role)
    {
        case EGPGuardTacticalRole::Pressure:
            return 760.0f;
        case EGPGuardTacticalRole::FlankLeft:
        case EGPGuardTacticalRole::FlankRight:
            return 1050.0f;
        case EGPGuardTacticalRole::Support:
            return 1325.0f;
        default:
            return 1000.0f;
    }
}

int32 AGPGuardAIController::GetBurstSizeForDistance(float Distance)
{
    if (Distance < 650.0f)
    {
        return 2;
    }
    if (Distance > 1400.0f)
    {
        return 4;
    }
    return 3;
}
