#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GPHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGPHealthChanged, float, CurrentHealth, float, MaxHealth, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPDeathEvent, AActor*, DamageCauser);

UCLASS(ClassGroup=(GorillaProtocol), meta=(BlueprintSpawnableComponent))
class GORILLAPROTOCOL_API UGPHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGPHealthComponent();

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool ReceiveDamage(float Damage, AActor* DamageCauser);

    UFUNCTION(BlueprintCallable, Category="Combat")
    void RestoreHealth(float Amount);

    UFUNCTION(BlueprintPure, Category="Combat")
    float GetHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category="Combat")
    float GetHealthNormalized() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

    UFUNCTION(BlueprintPure, Category="Combat")
    bool IsDead() const { return CurrentHealth <= 0.0f; }

    UPROPERTY(BlueprintAssignable, Category="Combat")
    FGPHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category="Combat")
    FGPDeathEvent OnDeath;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat", meta=(ClampMin="1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat")
    float CurrentHealth = 100.0f;
};
