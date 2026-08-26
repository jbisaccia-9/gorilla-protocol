#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GPEncounterSubsystem.generated.h"

UENUM(BlueprintType)
enum class EGPGuardTacticalRole : uint8
{
    Pressure,
    FlankLeft,
    FlankRight,
    Support
};

struct GORILLAPROTOCOL_API FGPFireTokenPolicy
{
    static bool CanGrant(bool bRequesterAlreadyActive, int32 ActiveCount, int32 Limit, bool bRequesterIsNext);
};

UCLASS(Config=Game)
class GORILLAPROTOCOL_API UGPEncounterSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    EGPGuardTacticalRole RegisterCombatant(AActor* Combatant);
    void UnregisterCombatant(AActor* Combatant);

    bool RequestFireToken(AActor* Requester);
    void ReleaseFireToken(AActor* Requester);
    bool RequestMeleeToken(AActor* Requester);
    void ReleaseMeleeToken(AActor* Requester);

    void ReportContact(AActor* Reporter, AActor* Target, const FVector& ContactLocation);
    bool GetSharedContact(AActor*& OutTarget, FVector& OutLocation) const;
    void ReportSuppression(AActor* Shooter, const FVector& SuppressedLocation);

    EGPGuardTacticalRole GetTacticalRole(AActor* Combatant) const;
    static EGPGuardTacticalRole GetRoleForOrdinal(int32 Ordinal);
    static FVector CalculateFormationOffset(EGPGuardTacticalRole Role, const FVector& AwayFromTarget,
        float DesiredRange, float FlankWidth);

    UFUNCTION(BlueprintPure, Category="Encounter")
    int32 GetActiveShooterCount() const { return ActiveShooters.Num(); }

    UFUNCTION(BlueprintPure, Category="Encounter")
    int32 GetQueuedShooterCount() const { return FireQueue.Num(); }

    uint32 GetSuppressionSerial() const { return SuppressionSerial; }
    FVector GetSuppressedLocation() const { return SuppressedLocation; }

protected:
    UPROPERTY(Config, EditAnywhere, Category="Encounter", meta=(ClampMin="1", ClampMax="6"))
    int32 MaxConcurrentShooters = 2;

    UPROPERTY(Config, EditAnywhere, Category="Encounter", meta=(ClampMin="1.0", ClampMax="20.0"))
    float SharedContactMemorySeconds = 7.0f;

private:
    void PruneInvalidCombatants();

    TSet<TWeakObjectPtr<AActor>> ActiveShooters;
    TArray<TWeakObjectPtr<AActor>> FireQueue;
    TWeakObjectPtr<AActor> MeleeAttacker;
    TMap<TWeakObjectPtr<AActor>, EGPGuardTacticalRole> TacticalRoles;
    TWeakObjectPtr<AActor> SharedTarget;
    TWeakObjectPtr<AActor> ContactReporter;
    TWeakObjectPtr<AActor> LastSuppressor;
    FVector SharedContactLocation = FVector::ZeroVector;
    FVector SuppressedLocation = FVector::ZeroVector;
    float SharedContactTime = -100.0f;
    uint32 SuppressionSerial = 0;
    int32 NextRoleOrdinal = 0;
};
