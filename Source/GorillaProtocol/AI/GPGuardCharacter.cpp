#include "GPGuardCharacter.h"

#include "AIController.h"
#include "Animation/AnimationAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Game/GPGameModeBase.h"

AGPGuardCharacter::AGPGuardCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);
    GetCharacterMovement()->MaxWalkSpeed = 310.0f;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    bUseControllerRotationYaw = false;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    AlertLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AlertLight"));
    AlertLight->SetupAttachment(GetCapsuleComponent());
    AlertLight->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    AlertLight->SetLightColor(FLinearColor(1.0f, 0.02f, 0.01f));
    AlertLight->SetIntensity(0.0f);
    AlertLight->SetAttenuationRadius(240.0f);
}

void AGPGuardCharacter::BeginPlay()
{
    Super::BeginPlay();
    PatrolOrigin = GetActorLocation();
    PatrolTarget = PatrolOrigin + GetActorRightVector() * 350.0f;
    NextPatrolChange = GetWorld()->GetTimeSeconds() + FMath::FRandRange(2.0f, 4.0f);

    if (USkeletalMesh* GuardMesh = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/GorillaProtocol/Characters/Guards/SK_Guard.SK_Guard")))
    {
        GetMesh()->SetSkeletalMesh(GuardMesh);
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
        GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (UAnimationAsset* Idle = LoadObject<UAnimationAsset>(nullptr,
        TEXT("/Game/GorillaProtocol/Characters/Guards/AN_GuardIdle.AN_GuardIdle")))
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->PlayAnimation(Idle, true);
    }
}

void AGPGuardCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead)
    {
        return;
    }

    ACharacter* Bruno = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!Bruno)
    {
        return;
    }

    const FVector ToBruno = Bruno->GetActorLocation() - GetActorLocation();
    const float Distance = ToBruno.Size();
    if (!bAlerted && Distance < 2600.0f && CanSeeBruno(Bruno))
    {
        bAlerted = true;
        bInvestigating = false;
        AlertedAt = GetWorld()->GetTimeSeconds();
        NextShotTime = AlertedAt + 0.55f;
    }

    if (bAlerted)
    {
        const FRotator Desired = ToBruno.Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(),
            FRotator(0.0f, Desired.Yaw, 0.0f), DeltaSeconds, 7.0f));

        if (Distance > 720.0f)
        {
            if (GetWorld()->GetTimeSeconds() >= NextMoveUpdate)
            {
                NextMoveUpdate = GetWorld()->GetTimeSeconds() + 0.35f;
                if (AAIController* AI = Cast<AAIController>(GetController()))
                {
                    AI->MoveToLocation(Bruno->GetActorLocation(), 620.0f);
                }
            }
            AddMovementInput(ToBruno.GetSafeNormal2D(), 0.45f);
        }
        else if (CanSeeBruno(Bruno) && GetWorld()->GetTimeSeconds() >= NextShotTime)
        {
            FireAtBruno(Bruno);
        }
    }
    else
    {
        if (bInvestigating)
        {
            if (FVector::DistSquared2D(GetActorLocation(), InvestigationTarget) < FMath::Square(110.0f))
            {
                bInvestigating = false;
            }
            else if (GetWorld()->GetTimeSeconds() >= NextMoveUpdate)
            {
                NextMoveUpdate = GetWorld()->GetTimeSeconds() + 0.35f;
                if (AAIController* AI = Cast<AAIController>(GetController()))
                {
                    AI->MoveToLocation(InvestigationTarget, 80.0f);
                }
            }
            AddMovementInput((InvestigationTarget - GetActorLocation()).GetSafeNormal2D(), 0.35f);
            return;
        }
        if (GetWorld()->GetTimeSeconds() >= NextPatrolChange ||
            FVector::DistSquared2D(GetActorLocation(), PatrolTarget) < FMath::Square(90.0f))
        {
            const FVector Offset = FMath::VRand() * FMath::FRandRange(240.0f, 520.0f);
            PatrolTarget = PatrolOrigin + FVector(Offset.X, Offset.Y, 0.0f);
            NextPatrolChange = GetWorld()->GetTimeSeconds() + FMath::FRandRange(3.0f, 6.0f);
        }
        const FVector ToPatrol = (PatrolTarget - GetActorLocation()).GetSafeNormal2D();
        if (GetWorld()->GetTimeSeconds() >= NextMoveUpdate)
        {
            NextMoveUpdate = GetWorld()->GetTimeSeconds() + 0.5f;
            if (AAIController* AI = Cast<AAIController>(GetController()))
            {
                AI->MoveToLocation(PatrolTarget, 70.0f);
            }
        }
        AddMovementInput(ToPatrol, 0.2f);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToPatrol.Rotation(), DeltaSeconds, 3.0f));
    }
}

void AGPGuardCharacter::HearLure(const FVector& Location)
{
    if (bDead || bAlerted || FVector::DistSquared(GetActorLocation(), Location) > FMath::Square(1900.0f))
    {
        return;
    }
    bInvestigating = true;
    InvestigationTarget = Location;
    NextMoveUpdate = 0.0f;
}

bool AGPGuardCharacter::CanSeeBruno(const ACharacter* Bruno) const
{
    const FVector Eye = GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);
    const FVector Target = Bruno->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
    const FVector Direction = (Target - Eye).GetSafeNormal();
    if (!bAlerted && FVector::DotProduct(GetActorForwardVector(), Direction) < 0.2f)
    {
        return false;
    }

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GuardSight), true, this);
    Params.AddIgnoredActor(this);
    return GetWorld()->LineTraceSingleByChannel(Hit, Eye, Target, ECC_Visibility, Params) &&
        Hit.GetActor() == Bruno;
}

void AGPGuardCharacter::FireAtBruno(ACharacter* Bruno)
{
    NextShotTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(0.72f, 1.15f);
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 58.0f) + GetActorForwardVector() * 35.0f;
    FVector Direction = (Bruno->GetActorLocation() + FVector(0.0f, 0.0f, 38.0f) - Start).GetSafeNormal();
    Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(4.0f));
    const FVector End = Start + Direction * 5000.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GuardFire), true, this);
    FVector TraceEnd = End;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        TraceEnd = Hit.ImpactPoint;
        if (Hit.GetActor() == Bruno)
        {
            UGameplayStatics::ApplyDamage(Bruno, FMath::FRandRange(7.0f, 12.0f),
                GetController(), this, nullptr);
        }
    }
    DrawDebugLine(GetWorld(), Start, TraceEnd, FColor(255, 35, 20), false, 0.12f, 0, 1.7f);
}

float AGPGuardCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bDead || Applied <= 0.0f)
    {
        return Applied;
    }
    bAlerted = true;
    bInvestigating = false;
    AlertedAt = GetWorld()->GetTimeSeconds();
    NextShotTime = FMath::Max(NextShotTime, AlertedAt + 0.55f);
    Health -= Applied;
    if (Health <= 0.0f)
    {
        Die();
    }
    return Applied;
}

void AGPGuardCharacter::Die()
{
    bDead = true;
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();
    AlertLight->SetIntensity(0.0f);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    if (AGPGameModeBase* Mode = GetWorld()->GetAuthGameMode<AGPGameModeBase>())
    {
        Mode->NotifyGuardDefeated();
    }
    SetLifeSpan(8.0f);
}
