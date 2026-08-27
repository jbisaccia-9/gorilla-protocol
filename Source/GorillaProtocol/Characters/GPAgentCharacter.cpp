#include "Characters/GPAgentCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Game/GPMissionSubsystem.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interaction/GPInteractable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureViewModelPrimitive(UStaticMeshComponent* Component, USceneComponent* Parent,
    const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator)
{
    Component->SetupAttachment(Parent);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeScale3D(Scale);
    Component->SetRelativeRotation(Rotation);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetOnlyOwnerSee(true);
    Component->SetCastShadow(false);
}
}

AGPAgentCharacter::AGPAgentCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxAcceleration = 2600.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1900.0f;
    GetCharacterMovement()->GroundFriction = 7.5f;
    GetCharacterMovement()->AirControl = 0.32f;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(BaseCameraLocation);
    FirstPersonCamera->bUsePawnControlRotation = false;
    FirstPersonCamera->SetFieldOfView(DefaultFieldOfView);

    FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
    FirstPersonArms->SetupAttachment(FirstPersonCamera);
    FirstPersonArms->SetOnlyOwnerSee(true);
    FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonArms->CastShadow = false;

    ViewModelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ViewModelRoot"));
    ViewModelRoot->SetupAttachment(FirstPersonCamera);

    LeftForearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftForearm"));
    ConfigureViewModelPrimitive(LeftForearm, ViewModelRoot, FVector(29.0f, -19.0f, -23.0f),
        FVector(0.24f, 0.10f, 0.10f), FRotator(0.0f, -10.0f, 8.0f));
    LeftFist = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFist"));
    ConfigureViewModelPrimitive(LeftFist, ViewModelRoot, FVector(45.0f, -20.0f, -20.0f),
        FVector(0.15f, 0.13f, 0.13f));
    RightForearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightForearm"));
    ConfigureViewModelPrimitive(RightForearm, ViewModelRoot, FVector(29.0f, 17.0f, -23.0f),
        FVector(0.24f, 0.10f, 0.10f), FRotator(0.0f, 10.0f, -8.0f));
    RightFist = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFist"));
    ConfigureViewModelPrimitive(RightFist, ViewModelRoot, FVector(43.0f, 16.0f, -19.0f),
        FVector(0.15f, 0.13f, 0.13f));
    PistolBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PistolBody"));
    ConfigureViewModelPrimitive(PistolBody, ViewModelRoot, FVector(56.0f, 15.0f, -12.0f),
        FVector(0.26f, 0.065f, 0.095f), FRotator(0.0f, -2.0f, 0.0f));
    PistolBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PistolBarrel"));
    ConfigureViewModelPrimitive(PistolBarrel, ViewModelRoot, FVector(75.0f, 15.0f, -10.0f),
        FVector(0.055f, 0.055f, 0.23f), FRotator(90.0f, 0.0f, 0.0f));
    BananaCharm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BananaCharm"));
    ConfigureViewModelPrimitive(BananaCharm, ViewModelRoot, FVector(54.0f, 20.0f, -21.0f),
        FVector(0.035f, 0.025f, 0.075f), FRotator(22.0f, 0.0f, 18.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (CubeMesh.Succeeded())
    {
        LeftForearm->SetStaticMesh(CubeMesh.Object);
        RightForearm->SetStaticMesh(CubeMesh.Object);
        PistolBody->SetStaticMesh(CubeMesh.Object);
    }
    if (SphereMesh.Succeeded())
    {
        LeftFist->SetStaticMesh(SphereMesh.Object);
        RightFist->SetStaticMesh(SphereMesh.Object);
        BananaCharm->SetStaticMesh(SphereMesh.Object);
    }
    if (CylinderMesh.Succeeded()) PistolBarrel->SetStaticMesh(CylinderMesh.Object);
    if (ShapeMaterial.Succeeded()) ViewModelBaseMaterial = ShapeMaterial.Object;

    GetMesh()->SetOwnerNoSee(true);
    HealthComponent = CreateDefaultSubobject<UGPHealthComponent>(TEXT("HealthComponent"));
    WeaponComponent = CreateDefaultSubobject<UGPWeaponComponent>(TEXT("WeaponComponent"));
}

void AGPAgentCharacter::BeginPlay()
{
    Super::BeginPlay();
    EnsureRuntimeInput();
    ApplyViewModelMaterials();
    LastRecordedHealth = HealthComponent->GetHealth();
    PreviousMagazineAmmo = WeaponComponent->GetMagazineAmmo();
    HealthComponent->OnHealthChanged.AddDynamic(this, &AGPAgentCharacter::HandleHealthChanged);
    HealthComponent->OnDeath.AddDynamic(this, &AGPAgentCharacter::HandleDeath);
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->OnPresentationUpdated.AddDynamic(this, &AGPAgentCharacter::HandleMissionPresentation);
        LastAnnouncedPhase = Mission->GetPhase();
        LastAnnouncedAlert = Mission->GetAlertState();
    }
    InstallInputMapping();
    SayItalianLine(NSLOCTEXT("GorillaProtocol", "MissionStart", "Agente Bruno operativo. Nessuno mi ferma."));
}

void AGPAgentCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    EnsureRuntimeInput();
    InstallInputMapping();
}

void AGPAgentCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdatePresentation(DeltaSeconds);

    InteractionTraceAccumulator += DeltaSeconds;
    if (InteractionTraceAccumulator >= 0.08f)
    {
        InteractionTraceAccumulator = 0.0f;
        UpdateInteractionFocus();
    }
}

void AGPAgentCharacter::EnsureRuntimeInput()
{
    if (RuntimeMapping)
    {
        return;
    }

    RuntimeMapping = NewObject<UInputMappingContext>(this, TEXT("IMC_RuntimeAgent"));
    MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
    MoveAction->ValueType = EInputActionValueType::Axis2D;
    LookAction = NewObject<UInputAction>(this, TEXT("IA_Look"));
    LookAction->ValueType = EInputActionValueType::Axis2D;
    FireAction = NewObject<UInputAction>(this, TEXT("IA_Fire"));
    AimAction = NewObject<UInputAction>(this, TEXT("IA_Aim"));
    ReloadAction = NewObject<UInputAction>(this, TEXT("IA_Reload"));
    SprintAction = NewObject<UInputAction>(this, TEXT("IA_Sprint"));
    InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact"));

    auto AddNegate = [this](FEnhancedActionKeyMapping& Mapping)
    {
        Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(RuntimeMapping));
    };
    auto AddSwizzleY = [this](FEnhancedActionKeyMapping& Mapping)
    {
        UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(RuntimeMapping);
        Swizzle->Order = EInputAxisSwizzle::YXZ;
        Mapping.Modifiers.Add(Swizzle);
    };

    AddSwizzleY(RuntimeMapping->MapKey(MoveAction, EKeys::W));
    FEnhancedActionKeyMapping& Backward = RuntimeMapping->MapKey(MoveAction, EKeys::S);
    AddNegate(Backward); AddSwizzleY(Backward);
    RuntimeMapping->MapKey(MoveAction, EKeys::D);
    FEnhancedActionKeyMapping& Left = RuntimeMapping->MapKey(MoveAction, EKeys::A);
    AddNegate(Left);

    RuntimeMapping->MapKey(LookAction, EKeys::MouseX);
    FEnhancedActionKeyMapping& MouseY = RuntimeMapping->MapKey(LookAction, EKeys::MouseY);
    AddNegate(MouseY); AddSwizzleY(MouseY);
    RuntimeMapping->MapKey(FireAction, EKeys::LeftMouseButton);
    RuntimeMapping->MapKey(AimAction, EKeys::RightMouseButton);
    RuntimeMapping->MapKey(ReloadAction, EKeys::R);
    RuntimeMapping->MapKey(SprintAction, EKeys::LeftShift);
    RuntimeMapping->MapKey(InteractAction, EKeys::E);
}

void AGPAgentCharacter::InstallInputMapping()
{
    const APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !RuntimeMapping) return;
    if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Subsystem->RemoveMappingContext(RuntimeMapping);
            Subsystem->AddMappingContext(RuntimeMapping, 0);
        }
    }
}

void AGPAgentCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    EnsureRuntimeInput();
    UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGPAgentCharacter::Move);
    EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGPAgentCharacter::Look);
    EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, WeaponComponent, &UGPWeaponComponent::StartFire);
    EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, WeaponComponent, &UGPWeaponComponent::StopFire);
    EnhancedInput->BindAction(AimAction, ETriggerEvent::Started, this, &AGPAgentCharacter::StartAim);
    EnhancedInput->BindAction(AimAction, ETriggerEvent::Completed, this, &AGPAgentCharacter::StopAim);
    EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, WeaponComponent, &UGPWeaponComponent::Reload);
    EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AGPAgentCharacter::StartSprint);
    EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AGPAgentCharacter::StopSprint);
    EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AGPAgentCharacter::Interact);
}

void AGPAgentCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    const FRotator YawRotation(0.0f, Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AGPAgentCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
    LookSway = FVector2D(FMath::Clamp(Axis.X, -4.0f, 4.0f), FMath::Clamp(Axis.Y, -4.0f, 4.0f));
}

void AGPAgentCharacter::StartSprint()
{
    if (!bAiming)
    {
        bSprinting = true;
        GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
    }
}

void AGPAgentCharacter::StopSprint()
{
    bSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AGPAgentCharacter::StartAim()
{
    bAiming = true;
    StopSprint();
    WeaponComponent->SetAiming(true);
}

void AGPAgentCharacter::StopAim()
{
    bAiming = false;
    WeaponComponent->SetAiming(false);
}

void AGPAgentCharacter::Interact()
{
    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractionRange;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GPInteractionTrace), false, this);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) &&
        Hit.GetActor() && Hit.GetActor()->Implements<UGPInteractable>())
    {
        IGPInteractable::Execute_Interact(Hit.GetActor(), this);
    }
}

void AGPAgentCharacter::SayItalianLine(const FText& ItalianLine)
{
    CurrentItalianLine = ItalianLine;
    ItalianLineExpireTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 4.2f : -1.0f;
    BP_OnItalianLineRequested(ItalianLine);
}

FText AGPAgentCharacter::GetCurrentItalianLine() const
{
    if (CurrentItalianLine.IsEmpty() || !GetWorld() || GetWorld()->GetTimeSeconds() > ItalianLineExpireTime)
    {
        return FText::GetEmpty();
    }
    return CurrentItalianLine;
}

void AGPAgentCharacter::UpdatePresentation(float DeltaSeconds)
{
    const float Speed = GetVelocity().Size2D();
    const bool bMovingOnGround = Speed > 8.0f && GetCharacterMovement()->IsMovingOnGround();
    const bool bFalling = GetCharacterMovement()->IsFalling();
    const float SpeedReference = bSprinting ? SprintSpeed : WalkSpeed;
    MovementPresentationAlpha = FMath::FInterpTo(MovementPresentationAlpha,
        bMovingOnGround ? FMath::Clamp(Speed / SpeedReference, 0.0f, 1.0f) : 0.0f, DeltaSeconds, 8.0f);

    if (bMovingOnGround)
    {
        MovementClock += DeltaSeconds * (bSprinting ? 13.0f : 9.2f) * FMath::Max(0.55f, MovementPresentationAlpha);
        MovementClock = FMath::Fmod(MovementClock, 2.0f * UE_PI);
    }
    if (bWasFalling && !bFalling)
    {
        LandingKick = CalculateLandingKick(PreviousVerticalVelocity);
    }
    if (bFalling) PreviousVerticalVelocity = GetVelocity().Z;
    bWasFalling = bFalling;

    const int32 MagazineAmmo = WeaponComponent ? WeaponComponent->GetMagazineAmmo() : 0;
    if (PreviousMagazineAmmo != INDEX_NONE && MagazineAmmo < PreviousMagazineAmmo)
    {
        ShotKick = FMath::Clamp(ShotKick + (bAiming ? 0.42f : 0.78f), 0.0f, 2.4f);
        ViewModelKick = 3.6f;
        if (MagazineAmmo == 0)
        {
            SayItalianLine(NSLOCTEXT("GorillaProtocol", "MagazineEmpty", "Caricatore vuoto. Che figuraccia."));
        }
    }
    else if (PreviousMagazineAmmo != INDEX_NONE && MagazineAmmo > PreviousMagazineAmmo && GetWorld())
    {
        const float Now = GetWorld()->GetTimeSeconds();
        if (Now - LastReloadLineTime > 5.0f)
        {
            SayItalianLine(NSLOCTEXT("GorillaProtocol", "ReloadBark", "Ricaricato. Eleganza, potenza, banana."));
            LastReloadLineTime = Now;
        }
    }
    PreviousMagazineAmmo = MagazineAmmo;

    LandingKick = FMath::FInterpTo(LandingKick, 0.0f, DeltaSeconds, 11.0f);
    ShotKick = FMath::FInterpTo(ShotKick, 0.0f, DeltaSeconds, 15.0f);
    ViewModelKick = FMath::FInterpTo(ViewModelKick, 0.0f, DeltaSeconds, 18.0f);
    DamageFeedbackAlpha = FMath::FInterpTo(DamageFeedbackAlpha, 0.0f, DeltaSeconds, 3.8f);
    LookSway = FMath::Vector2DInterpTo(LookSway, FVector2D::ZeroVector, DeltaSeconds, 11.0f);

    const float StepSin = FMath::Sin(MovementClock);
    const float StepCos = FMath::Cos(MovementClock * 0.5f);
    const float BobScale = bSprinting ? 1.65f : 1.0f;
    const FVector TargetCameraOffset(
        0.0f,
        StepCos * 0.75f * MovementPresentationAlpha * BobScale,
        FMath::Abs(StepSin) * 1.45f * MovementPresentationAlpha * BobScale - LandingKick);
    const FRotator TargetCameraRotation(-ShotKick - LookSway.Y * 0.08f, 0.0f,
        StepCos * 0.55f * MovementPresentationAlpha + LookSway.X * 0.10f);
    CurrentCameraOffset = FMath::VInterpTo(CurrentCameraOffset, TargetCameraOffset, DeltaSeconds, 13.0f);
    CurrentCameraRotation = FMath::RInterpTo(CurrentCameraRotation, TargetCameraRotation, DeltaSeconds, 16.0f);
    FirstPersonCamera->SetRelativeLocation(BaseCameraLocation + CurrentCameraOffset);
    const FRotator ViewRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
    FirstPersonCamera->SetWorldRotation(ViewRotation + CurrentCameraRotation);

    const float TargetFov = ResolveTargetFieldOfView(bAiming, bSprinting, DefaultFieldOfView,
        AimingFieldOfView, SprintFieldOfView);
    FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFov,
        DeltaSeconds, bAiming ? 14.0f : 9.0f));

    FVector ViewModelTarget(-ViewModelKick, -LookSway.X * 0.55f, LookSway.Y * 0.35f);
    FRotator ViewModelRotation(-LookSway.Y * 0.8f, LookSway.X * 0.9f,
        StepCos * MovementPresentationAlpha * 1.1f);
    if (bAiming)
    {
        ViewModelTarget += FVector(7.0f, -14.5f, 8.5f);
        ViewModelRotation *= 0.25f;
    }
    else if (bSprinting)
    {
        ViewModelTarget += FVector(-6.0f, 4.0f, -6.0f);
        ViewModelRotation += FRotator(9.0f, -5.0f, 8.0f);
    }
    ViewModelRoot->SetRelativeLocation(FMath::VInterpTo(ViewModelRoot->GetRelativeLocation(),
        ViewModelTarget, DeltaSeconds, 12.0f));
    ViewModelRoot->SetRelativeRotation(FMath::RInterpTo(ViewModelRoot->GetRelativeRotation(),
        ViewModelRotation, DeltaSeconds, 12.0f));
}

float AGPAgentCharacter::ResolveTargetFieldOfView(bool bIsAiming, bool bIsSprinting, float DefaultFov,
    float AimFov, float SprintFov)
{
    return bIsAiming ? AimFov : (bIsSprinting ? SprintFov : DefaultFov);
}

float AGPAgentCharacter::CalculateLandingKick(float VerticalVelocity)
{
    return VerticalVelocity < -120.0f ? FMath::Clamp(-VerticalVelocity / 155.0f, 2.0f, 7.0f) : 0.0f;
}

void AGPAgentCharacter::UpdateInteractionFocus()
{
    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractionRange;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GPInteractionFocusTrace), false, this);
    FHitResult Hit;
    bHasInteractableFocus = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) &&
        Hit.GetActor() && Hit.GetActor()->Implements<UGPInteractable>();
}

void AGPAgentCharacter::ApplyViewModelMaterials()
{
    if (!ViewModelBaseMaterial) return;
    FurMaterial = UMaterialInstanceDynamic::Create(ViewModelBaseMaterial, this);
    WeaponMaterial = UMaterialInstanceDynamic::Create(ViewModelBaseMaterial, this);
    BananaMaterial = UMaterialInstanceDynamic::Create(ViewModelBaseMaterial, this);
    FurMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.095f, 0.035f, 0.018f));
    WeaponMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.018f, 0.035f, 0.050f));
    BananaMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.58f, 0.025f));
    LeftForearm->SetMaterial(0, FurMaterial);
    LeftFist->SetMaterial(0, FurMaterial);
    RightForearm->SetMaterial(0, FurMaterial);
    RightFist->SetMaterial(0, FurMaterial);
    PistolBody->SetMaterial(0, WeaponMaterial);
    PistolBarrel->SetMaterial(0, WeaponMaterial);
    BananaCharm->SetMaterial(0, BananaMaterial);
}

void AGPAgentCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth, AActor* DamageCauser)
{
    if (CurrentHealth < LastRecordedHealth)
    {
        DamageFeedbackAlpha = 1.0f;
        if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
        {
            Mission->RaiseAlert(EGPMissionAlertState::Alarm);
        }

        const float Now = GetWorld()->GetTimeSeconds();
        if (Now - LastPainLineTime > 4.0f)
        {
            static const FText DamageLines[] = {
                NSLOCTEXT("GorillaProtocol", "DamageBarkA", "Ahi! Mi hai spettinato il pelo."),
                NSLOCTEXT("GorillaProtocol", "DamageBarkB", "Pessima idea. Il gorilla e' arrabbiato."),
                NSLOCTEXT("GorillaProtocol", "DamageBarkC", "Mamma mia, quello era il mio completo buono!")
            };
            SayItalianLine(DamageLines[DamageBarkIndex % UE_ARRAY_COUNT(DamageLines)]);
            ++DamageBarkIndex;
            LastPainLineTime = Now;
        }
    }
    LastRecordedHealth = CurrentHealth;
}

void AGPAgentCharacter::HandleMissionPresentation(EGPMissionPhase NewPhase, EGPMissionAlertState NewAlertState)
{
    const bool bPhaseChanged = NewPhase != LastAnnouncedPhase;
    LastAnnouncedPhase = NewPhase;
    if (bPhaseChanged && NewPhase == EGPMissionPhase::Complete)
    {
        LastAnnouncedAlert = NewAlertState;
        SayItalianLine(NSLOCTEXT("GorillaProtocol", "MissionCompleteBark",
            "Missione compiuta. Banane per tutti."));
        return;
    }
    if (bPhaseChanged && NewPhase == EGPMissionPhase::Failed)
    {
        LastAnnouncedAlert = NewAlertState;
        return;
    }
    if (NewAlertState == LastAnnouncedAlert) return;
    LastAnnouncedAlert = NewAlertState;
    switch (NewAlertState)
    {
        case EGPMissionAlertState::Suspicious:
            SayItalianLine(NSLOCTEXT("GorillaProtocol", "SuspiciousBark", "Piano, Bruno. Faccia innocente."));
            break;
        case EGPMissionAlertState::Alarm:
            SayItalianLine(NSLOCTEXT("GorillaProtocol", "AlarmBark", "Allarme? Perfetto. Adesso si balla."));
            break;
        case EGPMissionAlertState::Escape:
            SayItalianLine(NSLOCTEXT("GorillaProtocol", "EscapeBark", "Cifrario in tasca. Corri al motoscafo!"));
            break;
        default:
            break;
    }
}

void AGPAgentCharacter::HandleDeath(AActor* DamageCauser)
{
    WeaponComponent->StopFire();
    SayItalianLine(NSLOCTEXT("GorillaProtocol", "AgentDeath", "Mamma mia... missione molto discreta."));
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->FailMission();
    }
    DisableInput(Cast<APlayerController>(Controller));
    BP_OnAgentDied();
}
