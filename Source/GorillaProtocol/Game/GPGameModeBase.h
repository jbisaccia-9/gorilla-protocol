#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GPGameModeBase.generated.h"

class AGPMissionZone;
class UMaterialInterface;
class UStaticMesh;

UCLASS()
class GORILLAPROTOCOL_API AGPGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGPGameModeBase();
    virtual void BeginPlay() override;

    void NotifyGuardDefeated();
    void CollectLedger();
    void TryExtract();
    void NotifyLure(const FVector& Location);

    int32 GetGuardsRemaining() const { return GuardsRemaining; }
    bool HasLedger() const { return bHasLedger; }
    bool IsMissionComplete() const { return bMissionComplete; }
    FString GetObjectiveText() const;

private:
    void BuildMissionSpace();
    void SpawnCombatants();
    AActor* SpawnBlock(const FVector& Location, const FVector& Size,
        const FLinearColor& Color, const TCHAR* Label);
    UMaterialInterface* CreateColorMaterial(UObject* Outer, const FLinearColor& Color) const;

    TObjectPtr<UStaticMesh> CubeMesh;
    TObjectPtr<UStaticMesh> PlaneMesh;
    TObjectPtr<AGPMissionZone> ExtractionZone;
    int32 GuardsRemaining = 0;
    bool bHasLedger = false;
    bool bMissionComplete = false;
};
