#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "GPAgentCharacter.generated.h"

class UCameraComponent;
class UGPHealthComponent;
class UGPWeaponComponent;
class UInputAction;
class UInputMappingContext;
class USkeletalMeshComponent;

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

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<USkeletalMeshComponent> FirstPersonArms;

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

    UFUNCTION()
    void HandleDeath(AActor* DamageCauser);

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

    bool bAiming = false;
};
