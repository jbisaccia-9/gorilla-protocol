#include "AI/GPGuardAIController.h"

#include "AI/GPEncounterSubsystem.h"
#include "Characters/GPAgentCharacter.h"
#include "Characters/GPGuardCharacter.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
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
    EnterState(EGPGuardState::Patrol);
}

void AGPGuardAIController::OnUnPossess()
{
    ReleaseFireToken();
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
        case EGPGuardState::Telegraph:
            if (!CanSeeTarget()) EnterState(EGPGuardState::Investigate);
            else if (Now >= StateDeadline)
            {
                BurstRoundsRemaining = 3;
                EnterState(EGPGuardState::FireBurst);
                NextDecisionTime = Now;
            }
            break;
        case EGPGuardState::FireBurst:
            if (Now >= NextDecisionTime && BurstRoundsRemaining > 0)
            {
                GuardPawn->GetWeaponComponent()->FireSingleShot();
                --BurstRoundsRemaining;
                NextDecisionTime = Now + 0.11f;
            }
            if (BurstRoundsRemaining <= 0)
            {
                ReleaseFireToken();
                NextAttackTime = Now + FMath::FRandRange(1.0f, 1.6f);
                EnterState(EGPGuardState::Engage);
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
                NextAttackTime = Now + 1.0f;
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
    }
    ReleaseFireToken();
    EnterState(EGPGuardState::Stagger, 0.22f);
}

void AGPGuardAIController::NotifyGuardDeath()
{
    ReleaseFireToken();
    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);
    bHasSightStimulus = false;
    EnterState(EGPGuardState::Dead);
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
    GuardState = NewState;
    StateDeadline = GetWorld() ? GetWorld()->GetTimeSeconds() + Duration : Duration;
    if (NewState == EGPGuardState::Investigate || NewState == EGPGuardState::Patrol)
    {
        ReleaseFireToken();
        ClearFocus(EAIFocusPriority::Gameplay);
    }
}

void AGPGuardAIController::TickPatrol(float Now)
{
    if (Now < NextDecisionTime || !GuardPawn) return;
    NextDecisionTime = Now + FMath::FRandRange(3.0f, 5.0f);
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

    if (Distance < 130.0f && Now >= NextAttackTime)
    {
        StopMovement();
        EnterState(EGPGuardState::MeleeWindup, 0.55f);
        return;
    }
    if (Distance > 1800.0f)
    {
        MoveToActor(TargetActor, 1200.0f, true);
    }
    else if (Distance < 700.0f)
    {
        const FVector RetreatDirection = (GuardPawn->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
        MoveToLocation(GuardPawn->GetActorLocation() + RetreatDirection * 450.0f, 70.0f, true);
    }
    else
    {
        StopMovement();
    }

    if (Now >= NextAttackTime)
    {
        if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
        {
            bOwnsFireToken = Encounter->RequestFireToken(GuardPawn);
            if (bOwnsFireToken)
            {
                StopMovement();
                EnterState(EGPGuardState::Telegraph, 0.45f);
            }
        }
    }
}

void AGPGuardAIController::ReleaseFireToken()
{
    if (!bOwnsFireToken || !GetWorld()) return;
    if (UGPEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<UGPEncounterSubsystem>())
    {
        Encounter->ReleaseFireToken(GuardPawn);
    }
    bOwnsFireToken = false;
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
