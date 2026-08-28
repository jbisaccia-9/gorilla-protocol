#include "GPBrunoCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "../Game/GPGameModeBase.h"

AGPBrunoCharacter::AGPBrunoCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(48.0f, 104.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->JumpZVelocity = 620.0f;
    GetCharacterMovement()->AirControl = 0.35f;
    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(-8.0f, 0.0f, 78.0f));
    Camera->bUsePawnControlRotation = true;

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("P9"));
    WeaponMesh->SetupAttachment(Camera);
    WeaponMesh->SetRelativeLocation(FVector(35.0f, 18.0f, -24.0f));
    WeaponMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    WeaponMesh->SetRelativeScale3D(FVector(0.85f));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->bCastDynamicShadow = false;

    LeftForearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftForearm"));
    LeftForearm->SetupAttachment(Camera);
    LeftForearm->SetRelativeLocation(FVector(24.0f, -23.0f, -27.0f));
    LeftForearm->SetRelativeRotation(FRotator(72.0f, 0.0f, 0.0f));
    LeftForearm->SetRelativeScale3D(FVector(0.14f, 0.18f, 0.48f));
    LeftForearm->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    RightForearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightForearm"));
    RightForearm->SetupAttachment(Camera);
    RightForearm->SetRelativeLocation(FVector(24.0f, 19.0f, -28.0f));
    RightForearm->SetRelativeRotation(FRotator(72.0f, 0.0f, 0.0f));
    RightForearm->SetRelativeScale3D(FVector(0.14f, 0.18f, 0.48f));
    RightForearm->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    LeftFist = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFist"));
    LeftFist->SetupAttachment(Camera);
    LeftFist->SetRelativeLocation(FVector(51.0f, -23.0f, -25.0f));
    LeftFist->SetRelativeScale3D(FVector(0.22f, 0.28f, 0.25f));
    LeftFist->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    RightFist = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFist"));
    RightFist->SetupAttachment(Camera);
    RightFist->SetRelativeLocation(FVector(50.0f, 18.0f, -24.0f));
    RightFist->SetRelativeScale3D(FVector(0.22f, 0.28f, 0.25f));
    RightFist->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
    MuzzleLight->SetupAttachment(WeaponMesh);
    MuzzleLight->SetRelativeLocation(FVector(0.0f, 55.0f, 0.0f));
    MuzzleLight->SetLightColor(FLinearColor(1.0f, 0.35f, 0.05f));
    MuzzleLight->SetIntensity(0.0f);
    MuzzleLight->SetAttenuationRadius(420.0f);
}

void AGPBrunoCharacter::BeginPlay()
{
    Super::BeginPlay();

    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    LeftForearm->SetStaticMesh(Cylinder);
    RightForearm->SetStaticMesh(Cylinder);
    LeftFist->SetStaticMesh(Sphere);
    RightFist->SetStaticMesh(Sphere);

    if (UStaticMesh* P9 = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/GorillaProtocol/Weapons/P9/SM_P9.SM_P9")))
    {
        WeaponMesh->SetStaticMesh(P9);
    }

    if (UMaterialInterface* Fur = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/GorillaProtocol/Materials/M_BrunoFur.M_BrunoFur")))
    {
        LeftForearm->SetMaterial(0, Fur);
        RightForearm->SetMaterial(0, Fur);
        LeftFist->SetMaterial(0, Fur);
        RightFist->SetMaterial(0, Fur);
    }

    GetWorldTimerManager().SetTimerForNextTick(this, &AGPBrunoCharacter::SpeakMissionStart);
}

void AGPBrunoCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Bob = FMath::Sin(GetWorld()->GetTimeSeconds() * 7.0f) *
        FMath::Clamp(GetVelocity().Size() / SprintSpeed, 0.0f, 1.0f) * 1.4f;
    WeaponMesh->SetRelativeLocation(FVector(35.0f, 18.0f, -24.0f + Bob));
}

void AGPBrunoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGPBrunoCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGPBrunoCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AGPBrunoCharacter::StartFire);
    PlayerInputComponent->BindAction(TEXT("Punch"), IE_Pressed, this, &AGPBrunoCharacter::Punch);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AGPBrunoCharacter::Interact);
    PlayerInputComponent->BindAction(TEXT("BrunoVoice"), IE_Pressed, this, &AGPBrunoCharacter::BrunoVoice);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AGPBrunoCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AGPBrunoCharacter::StopSprint);
    PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AGPBrunoCharacter::Reload);
}

void AGPBrunoCharacter::MoveForward(float Value)
{
    if (!bDead && Controller && !FMath::IsNearlyZero(Value))
    {
        const FRotator Yaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
    }
}

void AGPBrunoCharacter::MoveRight(float Value)
{
    if (!bDead && Controller && !FMath::IsNearlyZero(Value))
    {
        const FRotator Yaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
    }
}

void AGPBrunoCharacter::StartSprint()
{
    bSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AGPBrunoCharacter::StopSprint()
{
    bSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AGPBrunoCharacter::StartFire()
{
    if (bDead || bReloading || !bCanFire)
    {
        return;
    }
    if (Ammo <= 0)
    {
        Reload();
        return;
    }

    --Ammo;
    bCanFire = false;
    GetWorldTimerManager().SetTimer(FireTimer, this, &AGPBrunoCharacter::ResetFire, 0.19f, false);

    FVector Start = Camera->GetComponentLocation();
    FVector Direction = Camera->GetForwardVector();
    Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(0.65f));
    const FVector End = Start + Direction * 12000.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BrunoFire), true, this);
    FVector TraceEnd = End;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        TraceEnd = Hit.ImpactPoint;
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), 34.0f, Direction, Hit,
            GetController(), this, nullptr);
    }

    DrawDebugLine(GetWorld(), Start, TraceEnd, FColor(255, 195, 85), false, 0.07f, 0, 1.25f);
    MuzzleLight->SetIntensity(9000.0f);
    GetWorldTimerManager().SetTimer(MuzzleTimer,
        FTimerDelegate::CreateWeakLambda(this, [this]() { MuzzleLight->SetIntensity(0.0f); }),
        0.055f, false);
    AddControllerPitchInput(-0.38f);
}

void AGPBrunoCharacter::ResetFire()
{
    bCanFire = true;
}

void AGPBrunoCharacter::Punch()
{
    if (bDead || !bCanPunch)
    {
        return;
    }

    bCanPunch = false;
    GetWorldTimerManager().SetTimer(PunchTimer, this, &AGPBrunoCharacter::ResetPunch,
        bSprinting ? 1.1f : 0.55f, false);
    Speak(TEXT("punch"), TEXT("Permesso!"));
    const FVector Start = Camera->GetComponentLocation();
    const float Reach = bSprinting ? 430.0f : 240.0f;
    const FVector End = Start + Camera->GetForwardVector() * Reach;
    if (bSprinting)
    {
        LaunchCharacter(Camera->GetForwardVector() * 1050.0f + FVector(0.0f, 0.0f, 90.0f), true, true);
    }
    FHitResult Hit;
    FCollisionShape Shape = FCollisionShape::MakeSphere(bSprinting ? 92.0f : 52.0f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BrunoPunch), false, this);
    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
        ECC_Pawn, Shape, Params))
    {
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), bSprinting ? 125.0f : 82.0f, Camera->GetForwardVector(),
            Hit, GetController(), this, nullptr);
        if (UPrimitiveComponent* Component = Hit.GetComponent())
        {
            Component->AddImpulseAtLocation(Camera->GetForwardVector() * 85000.0f, Hit.ImpactPoint);
        }
    }
}

void AGPBrunoCharacter::ResetPunch()
{
    bCanPunch = true;
}

void AGPBrunoCharacter::Interact()
{
    if (bDead)
    {
        return;
    }

    const FVector Start = Camera->GetComponentLocation();
    const FVector End = Start + Camera->GetForwardVector() * 350.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BrunoInteract), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor())
    {
        Hit.GetActor()->NotifyActorOnClicked(FKey(TEXT("E")));
    }
}

void AGPBrunoCharacter::Reload()
{
    if (bDead || bReloading || Ammo == MagazineSize)
    {
        return;
    }
    bReloading = true;
    GetWorldTimerManager().SetTimer(ReloadTimer, this, &AGPBrunoCharacter::FinishReload, 1.05f, false);
}

void AGPBrunoCharacter::FinishReload()
{
    Ammo = MagazineSize;
    bReloading = false;
}

void AGPBrunoCharacter::BrunoVoice()
{
    Speak(TEXT("banana"), TEXT("Una banana tattica. Tecnologia italiana."));
    if (AGPGameModeBase* Mode = GetWorld()->GetAuthGameMode<AGPGameModeBase>())
    {
        Mode->NotifyLure(GetActorLocation());
    }
}

void AGPBrunoCharacter::SpeakMissionStart()
{
    Speak(TEXT("mission_start"), TEXT("Operazione Scimmia di Mare. Entriamo piano... piu o meno."));
}

void AGPBrunoCharacter::SpeakObjective()
{
    Speak(TEXT("objective"), TEXT("Documento preso. Adesso, fuga elegante."));
}

void AGPBrunoCharacter::SpeakComplete()
{
    Speak(TEXT("complete"), TEXT("Missione compiuta. Nessuno sospettava del gorilla."));
}

void AGPBrunoCharacter::Speak(const TCHAR* AssetName, const TCHAR* Subtitle)
{
    const FString Path = FString::Printf(TEXT("/Game/GorillaProtocol/Audio/Bruno/%s.%s"), AssetName, AssetName);
    if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path))
    {
        UGameplayStatics::PlaySound2D(this, Sound, 1.0f);
    }
    CurrentSubtitle = Subtitle;
    SubtitleUntil = GetWorld()->GetTimeSeconds() + 3.4f;
}

bool AGPBrunoCharacter::IsSubtitleVisible() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() < SubtitleUntil && !CurrentSubtitle.IsEmpty();
}

float AGPBrunoCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bDead || Applied <= 0.0f)
    {
        return Applied;
    }

    Health = FMath::Max(0.0f, Health - Applied);
    if (Health <= 0.0f)
    {
        Die();
    }
    else if (FMath::FRand() < 0.35f)
    {
        Speak(TEXT("hurt"), TEXT("Ah! La pelliccia era nuova!"));
    }
    return Applied;
}

void AGPBrunoCharacter::Die()
{
    bDead = true;
    DisableInput(Cast<APlayerController>(GetController()));
    CurrentSubtitle = TEXT("Bruno e caduto. Riavvio missione...");
    SubtitleUntil = GetWorld()->GetTimeSeconds() + 5.0f;
    FTimerHandle RestartTimer;
    GetWorldTimerManager().SetTimer(RestartTimer,
        FTimerDelegate::CreateWeakLambda(this, [this]() {
            UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/GorillaProtocol/Maps/L_ScimmiaDiMare")));
        }),
        3.0f, false);
}
