#include "Characters/GPGuardCharacter.h"

#include "AI/GPGuardAIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/GPMissionSubsystem.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

AGPGuardCharacter::AGPGuardCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    AIControllerClass = AGPGuardAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCharacterMovement()->MaxWalkSpeed = 360.0f;

    HealthComponent = CreateDefaultSubobject<UGPHealthComponent>(TEXT("HealthComponent"));
    WeaponComponent = CreateDefaultSubobject<UGPWeaponComponent>(TEXT("WeaponComponent"));
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    TorsoHitZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TorsoHitZone"));
    TorsoHitZone->SetupAttachment(GetCapsuleComponent());
    TorsoHitZone->SetBoxExtent(FVector(32.0f, 25.0f, 45.0f));
    TorsoHitZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    TorsoHitZone->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TorsoHitZone->ComponentTags.Add(TEXT("HitZone.Torso"));

    HeadHitZone = CreateDefaultSubobject<USphereComponent>(TEXT("HeadHitZone"));
    HeadHitZone->SetupAttachment(GetCapsuleComponent());
    HeadHitZone->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
    HeadHitZone->SetSphereRadius(18.0f);
    HeadHitZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    HeadHitZone->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    HeadHitZone->ComponentTags.Add(TEXT("HitZone.Head"));

    ProxyBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProxyBody"));
    ProxyBody->SetupAttachment(GetCapsuleComponent());
    ProxyBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProxyBody->SetRelativeScale3D(FVector(0.65f, 0.48f, 1.25f));
    ProxyHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProxyHead"));
    ProxyHead->SetupAttachment(GetCapsuleComponent());
    ProxyHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProxyHead->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
    ProxyHead->SetRelativeScale3D(FVector(0.38f));

    CombatBark = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CombatBark"));
    CombatBark->SetupAttachment(GetCapsuleComponent());
    CombatBark->SetRelativeLocation(FVector(0.0f, 0.0f, 132.0f));
    CombatBark->SetHorizontalAlignment(EHTA_Center);
    CombatBark->SetVerticalAlignment(EVRTA_TextCenter);
    CombatBark->SetWorldSize(26.0f);
    CombatBark->SetTextRenderColor(FColor(255, 205, 58));
    CombatBark->SetHiddenInGame(true);
    CombatBark->SetCastShadow(false);

    CombatCueLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CombatCueLight"));
    CombatCueLight->SetupAttachment(GetCapsuleComponent());
    CombatCueLight->SetRelativeLocation(FVector(20.0f, 0.0f, 82.0f));
    CombatCueLight->SetAttenuationRadius(180.0f);
    CombatCueLight->SetIntensity(0.0f);
    CombatCueLight->SetCastShadows(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CubeMesh.Succeeded()) ProxyBody->SetStaticMesh(CubeMesh.Object);
    if (SphereMesh.Succeeded()) ProxyHead->SetStaticMesh(SphereMesh.Object);
}

void AGPGuardCharacter::BeginPlay()
{
    Super::BeginPlay();
    FGPWeaponTuning GuardWeapon;
    GuardWeapon.Damage = 8.0f;
    GuardWeapon.HeadshotMultiplier = 2.0f;
    GuardWeapon.Range = 12000.0f;
    GuardWeapon.FireInterval = 0.11f;
    GuardWeapon.HipSpreadDegrees = 2.4f;
    GuardWeapon.AimSpreadDegrees = 1.2f;
    GuardWeapon.SpreadPerShotDegrees = 0.28f;
    GuardWeapon.MaxBloomDegrees = 1.8f;
    GuardWeapon.BloomRecoveryDegreesPerSecond = 2.8f;
    GuardWeapon.RecoilPitchDegrees = 0.0f;
    GuardWeapon.RecoilYawDegrees = 0.0f;
    GuardWeapon.MagazineSize = 24;
    GuardWeapon.ReloadDuration = 2.1f;
    WeaponComponent->ConfigureWeapon(GuardWeapon, 96);
    WeaponComponent->SetSpreadSeed(GetTypeHash(GetFName()));
    HealthComponent->OnHealthChanged.AddDynamic(this, &AGPGuardCharacter::HandleHealthChanged);
    HealthComponent->OnDeath.AddDynamic(this, &AGPGuardCharacter::HandleDeath);
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->RegisterGuard(this);
    }
}

void AGPGuardCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
    OutLocation = GetActorLocation() + FVector(0.0f, 0.0f, BaseEyeHeight);
    OutRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
}

void AGPGuardCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth, AActor* DamageCauser)
{
    if (CurrentHealth > 0.0f)
    {
        SetCombatCue(EGPGuardCombatCue::Hit);
        const FVector DamageDirection = DamageCauser
            ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D()
            : -GetActorForwardVector();
        const float ReactionSide = FVector::DotProduct(GetActorRightVector(), DamageDirection) >= 0.0f ? 1.0f : -1.0f;
        ProxyBody->SetRelativeRotation(FRotator(-5.0f, 0.0f, ReactionSide * 8.0f));
        ProxyHead->SetRelativeRotation(FRotator(5.0f, 0.0f, -ReactionSide * 12.0f));
        GetWorldTimerManager().SetTimer(HitReactionTimer, this, &AGPGuardCharacter::FinishHitReaction, 0.18f, false);
        BP_OnGuardHit(MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f, DamageCauser);
    }
    if (AGPGuardAIController* GuardController = Cast<AGPGuardAIController>(Controller))
    {
        GuardController->NotifyDamaged(DamageCauser);
    }
}

void AGPGuardCharacter::HandleDeath(AActor* DamageCauser)
{
    GetWorldTimerManager().ClearTimer(HitReactionTimer);
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->NotifyGuardEliminated(this);
    }
    if (AGPGuardAIController* GuardController = Cast<AGPGuardAIController>(Controller))
    {
        GuardController->NotifyGuardDeath();
    }
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TorsoHitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadHitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();
    ProxyBody->SetRelativeRotation(FRotator::ZeroRotator);
    ProxyHead->SetRelativeRotation(FRotator::ZeroRotator);
    BP_OnGuardDied();
    SetLifeSpan(12.0f);
}

void AGPGuardCharacter::SetCombatCue(EGPGuardCombatCue NewCue)
{
    if (CombatCue == NewCue || !CombatBark || !CombatCueLight)
    {
        return;
    }

    CombatCue = NewCue;
    FText CueText = FText::GetEmpty();
    FColor CueColor = FColor(255, 205, 58);
    float LightIntensity = 0.0f;

    switch (NewCue)
    {
        case EGPGuardCombatCue::Alert:
            CueText = FText::FromString(TEXT("ALLARME! GORILLA!"));
            LightIntensity = 950.0f;
            break;
        case EGPGuardCombatCue::Telegraph:
            CueText = FText::FromString(TEXT("FUOCO!"));
            CueColor = FColor(255, 74, 54);
            LightIntensity = 2600.0f;
            break;
        case EGPGuardCombatCue::Firing:
            CueText = FText::FromString(TEXT("RAT-TA-TA!"));
            CueColor = FColor(255, 125, 42);
            LightIntensity = 1500.0f;
            break;
        case EGPGuardCombatCue::Flanking:
            CueText = FText::FromString(TEXT("LO AGGIRO!"));
            CueColor = FColor(80, 210, 255);
            LightIntensity = 750.0f;
            break;
        case EGPGuardCombatCue::Reloading:
            CueText = FText::FromString(TEXT("RICARICO LE BANANE!"));
            CueColor = FColor(255, 230, 95);
            LightIntensity = 500.0f;
            break;
        case EGPGuardCombatCue::Melee:
            CueText = FText::FromString(TEXT("PRESA BANANA!"));
            CueColor = FColor(255, 58, 58);
            LightIntensity = 2200.0f;
            break;
        case EGPGuardCombatCue::Hit:
            CueText = FText::FromString(TEXT("MAMMA MIA!"));
            CueColor = FColor::White;
            LightIntensity = 1100.0f;
            break;
        default:
            break;
    }

    CombatBark->SetText(CueText);
    CombatBark->SetTextRenderColor(CueColor);
    CombatBark->SetHiddenInGame(NewCue == EGPGuardCombatCue::None);
    CombatCueLight->SetLightColor(FLinearColor(CueColor));
    CombatCueLight->SetIntensity(LightIntensity);
    BP_OnCombatCueChanged(NewCue);
}

void AGPGuardCharacter::FinishHitReaction()
{
    ProxyBody->SetRelativeRotation(FRotator::ZeroRotator);
    ProxyHead->SetRelativeRotation(FRotator::ZeroRotator);
    if (CombatCue == EGPGuardCombatCue::Hit)
    {
        SetCombatCue(EGPGuardCombatCue::Alert);
    }
}
