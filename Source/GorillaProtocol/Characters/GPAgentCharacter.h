#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "Game/GPMissionSubsystem.h"
#include "InputActionValue.h"
#include "GPAgentCharacter.generated.h"

class UCameraComponent;
class UGPHealthComponent;
class UGPWeaponComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class GORILLAPROTOCOL_API AGPAgentCharacter : public ACharacter, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AGPAgentCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void PawnClientRestart() override;
    virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(0); }

    UFUNCTION(BlueprintCallable, Category="Dialogue")
    void SayItalianLine(const FText& ItalianLine);

    UFUNCTION(BlueprintPure, Category="Components")
    UGPHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category="Components")
    UGPWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

    UFUNCTION(BlueprintPure, Category="Presentation")
    bool IsAiming() const { return bAiming; }

    UFUNCTION(BlueprintPure, Category="Presentation")
    bool IsSprinting() const { return bSprinting; }

    UFUNCTION(BlueprintPure, Category="Presentation")
    bool HasInteractableFocus() const { return bHasInteractableFocus; }

    UFUNCTION(BlueprintPure, Category="Presentation")
    float GetDamageFeedbackAlpha() const { return DamageFeedbackAlpha; }

    UFUNCTION(BlueprintPure, Category="Presentation")
    float GetMovementPresentationAlpha() const { return MovementPresentationAlpha; }

    UFUNCTION(BlueprintPure, Category="Presentation")
    FText GetCurrentItalianLine() const;

    float GetDefaultFieldOfView() const { return DefaultFieldOfView; }
    float GetAimingFieldOfView() const { return AimingFieldOfView; }
    float GetSprintFieldOfView() const { return SprintFieldOfView; }

    static float ResolveTargetFieldOfView(bool bIsAiming, bool bIsSprinting, float DefaultFov,
        float AimFov, float SprintFov);
    static float CalculateLandingKick(float VerticalVelocity);

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<USkeletalMeshComponent> FirstPersonArms;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<USceneComponent> ViewModelRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> LeftForearm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> LeftFist;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> RightForearm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> RightFist;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> PistolBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> PistolBarrel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> BananaCharm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UGPHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UGPWeaponComponent> WeaponComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
    float WalkSpeed = 440.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
    float SprintSpeed = 700.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
    float DefaultFieldOfView = 82.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
    float AimingFieldOfView = 66.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
    float SprintFieldOfView = 90.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction")
    float InteractionRange = 350.0f;

    UFUNCTION(BlueprintImplementableEvent, Category="Dialogue", meta=(DisplayName="On Italian Line Requested"))
    void BP_OnItalianLineRequested(const FText& ItalianLine);

    UFUNCTION(BlueprintImplementableEvent, Category="Presentation")
    void BP_OnAgentDied();

private:
    void EnsureRuntimeInput();
    void InstallInputMapping();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();
    void StartAim();
    void StopAim();
    void Interact();
    void UpdatePresentation(float DeltaSeconds);
    void UpdateInteractionFocus();
    void ApplyViewModelMaterials();

    UFUNCTION()
    void HandleDeath(AActor* DamageCauser);

    UFUNCTION()
    void HandleHealthChanged(float CurrentHealth, float MaxHealth, AActor* DamageCauser);

    UFUNCTION()
    void HandleMissionPresentation(EGPMissionPhase NewPhase, EGPMissionAlertState NewAlertState);

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> RuntimeMapping;
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> LookAction;
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> FireAction;
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> AimAction;
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> ReloadAction;
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SprintAction;
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> ViewModelBaseMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FurMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> WeaponMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BananaMaterial;

    bool bAiming = false;
    bool bSprinting = false;
    bool bWasFalling = false;
    bool bHasInteractableFocus = false;
    float MovementClock = 0.0f;
    float LandingKick = 0.0f;
    float ShotKick = 0.0f;
    float ViewModelKick = 0.0f;
    float DamageFeedbackAlpha = 0.0f;
    float MovementPresentationAlpha = 0.0f;
    float PreviousVerticalVelocity = 0.0f;
    float LastRecordedHealth = 100.0f;
    float LastPainLineTime = -100.0f;
    float LastReloadLineTime = -100.0f;
    float ItalianLineExpireTime = -1.0f;
    float InteractionTraceAccumulator = 0.0f;
    int32 PreviousMagazineAmmo = INDEX_NONE;
    int32 DamageBarkIndex = 0;
    EGPMissionPhase LastAnnouncedPhase = EGPMissionPhase::Infiltration;
    EGPMissionAlertState LastAnnouncedAlert = EGPMissionAlertState::Covert;
    FVector BaseCameraLocation = FVector(-8.0f, 0.0f, 72.0f);
    FVector CurrentCameraOffset = FVector::ZeroVector;
    FRotator CurrentCameraRotation = FRotator::ZeroRotator;
    FVector2D LookSway = FVector2D::ZeroVector;
    FText CurrentItalianLine;
};
