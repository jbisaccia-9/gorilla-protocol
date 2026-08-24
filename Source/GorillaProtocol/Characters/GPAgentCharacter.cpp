#include "Characters/GPAgentCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Game/GPMissionSubsystem.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interaction/GPInteractable.h"

AGPAgentCharacter::AGPAgentCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(-8.0f, 0.0f, 72.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->SetFieldOfView(DefaultFieldOfView);

    FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
    FirstPersonArms->SetupAttachment(FirstPersonCamera);
    FirstPersonArms->SetOnlyOwnerSee(true);
    FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonArms->CastShadow = false;

    GetMesh()->SetOwnerNoSee(true);
    HealthComponent = CreateDefaultSubobject<UGPHealthComponent>(TEXT("HealthComponent"));
    WeaponComponent = CreateDefaultSubobject<UGPWeaponComponent>(TEXT("WeaponComponent"));
}

void AGPAgentCharacter::BeginPlay()
{
    Super::BeginPlay();
    EnsureRuntimeInput();
    HealthComponent->OnDeath.AddDynamic(this, &AGPAgentCharacter::HandleDeath);
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
    const float TargetFov = bAiming ? AimingFieldOfView : DefaultFieldOfView;
    FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFov, DeltaSeconds, 12.0f));
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
}

void AGPAgentCharacter::StartSprint()
{
    if (!bAiming) GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AGPAgentCharacter::StopSprint()
{
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
    BP_OnItalianLineRequested(ItalianLine);
}

void AGPAgentCharacter::HandleDeath(AActor* DamageCauser)
{
    WeaponComponent->StopFire();
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->FailMission();
    }
    DisableInput(Cast<APlayerController>(Controller));
    BP_OnAgentDied();
}
