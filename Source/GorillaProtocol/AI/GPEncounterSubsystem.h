#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GPEncounterSubsystem.generated.h"

UCLASS(Config=Game)
class GORILLAPROTOCOL_API UGPEncounterSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    bool RequestFireToken(AActor* Requester);
    void ReleaseFireToken(AActor* Requester);

    UFUNCTION(BlueprintPure, Category="Encounter")
    int32 GetActiveShooterCount() const { return ActiveShooters.Num(); }

protected:
    UPROPERTY(Config, EditAnywhere, Category="Encounter", meta=(ClampMin="1", ClampMax="6"))
    int32 MaxConcurrentShooters = 2;

private:
    TSet<TWeakObjectPtr<AActor>> ActiveShooters;
};
