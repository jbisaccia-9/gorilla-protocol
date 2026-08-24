#include "Characters/GPGuardCharacter.h"

#include "AI/GPGuardAIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/GPMissionSubsystem.h"
#include "UObject/ConstructorHelpers.h"

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
    GuardWeapon.MagazineSize = 24;
    GuardWeapon.ReloadDuration = 2.1f;
    WeaponComponent->ConfigureWeapon(GuardWeapon, 96);
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
    if (AGPGuardAIController* GuardController = Cast<AGPGuardAIController>(Controller))
    {
        GuardController->NotifyDamaged(DamageCauser);
    }
}

void AGPGuardCharacter::HandleDeath(AActor* DamageCauser)
{
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
    BP_OnGuardDied();
    SetLifeSpan(12.0f);
}
