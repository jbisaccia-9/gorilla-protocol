#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "GPGuardCharacter.generated.h"

class UGPHealthComponent;
class UGPWeaponComponent;
class UBoxComponent;
class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGPGuardCombatCue : uint8
{
    None,
    Alert,
    Telegraph,
    Firing,
    Flanking,
    Reloading,
    Melee,
    Hit
};

UCLASS(Blueprintable)
class GORILLAPROTOCOL_API AGPGuardCharacter : public ACharacter, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AGPGuardCharacter();
    virtual void BeginPlay() override;
    virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
    virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(1); }

    UFUNCTION(BlueprintPure, Category="Components")
    UGPHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category="Components")
    UGPWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

    UFUNCTION(BlueprintCallable, Category="Combat")
    void SetCombatCue(EGPGuardCombatCue NewCue);

    UFUNCTION(BlueprintPure, Category="Combat")
    EGPGuardCombatCue GetCombatCue() const { return CombatCue; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UGPHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UGPWeaponComponent> WeaponComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<USphereComponent> HeadHitZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UBoxComponent> TorsoHitZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> ProxyBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMeshComponent> ProxyHead;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UTextRenderComponent> CombatBark;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UPointLightComponent> CombatCueLight;

    UFUNCTION(BlueprintImplementableEvent, Category="Presentation")
    void BP_OnGuardDied();

    UFUNCTION(BlueprintImplementableEvent, Category="Presentation")
    void BP_OnGuardHit(float HealthNormalized, AActor* DamageCauser);

    UFUNCTION(BlueprintImplementableEvent, Category="Presentation")
    void BP_OnCombatCueChanged(EGPGuardCombatCue NewCue);

private:
    UFUNCTION()
    void HandleHealthChanged(float CurrentHealth, float MaxHealth, AActor* DamageCauser);

    UFUNCTION()
    void HandleDeath(AActor* DamageCauser);

    void FinishHitReaction();

    FTimerHandle HitReactionTimer;
    EGPGuardCombatCue CombatCue = EGPGuardCombatCue::None;
};
