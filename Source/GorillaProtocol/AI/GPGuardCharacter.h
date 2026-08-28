#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GPGuardCharacter.generated.h"

class UPointLightComponent;

UCLASS()
class GORILLAPROTOCOL_API AGPGuardCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGPGuardCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    void HearLure(const FVector& Location);

private:
    bool CanSeeBruno(const ACharacter* Bruno) const;
    void FireAtBruno(ACharacter* Bruno);
    void Die();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> AlertLight;

    FVector PatrolOrigin = FVector::ZeroVector;
    FVector PatrolTarget = FVector::ZeroVector;
    float Health = 100.0f;
    float NextShotTime = 0.0f;
    float NextPatrolChange = 0.0f;
    float NextMoveUpdate = 0.0f;
    float AlertedAt = 0.0f;
    FVector InvestigationTarget = FVector::ZeroVector;
    bool bAlerted = false;
    bool bInvestigating = false;
    bool bDead = false;
};
