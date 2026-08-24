#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GPMissionSubsystem.generated.h"

class AGPExtractionZone;

UENUM(BlueprintType)
enum class EGPMissionPhase : uint8
{
    Infiltration,
    RecoverCipher,
    Extraction,
    Complete,
    Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGPMissionUpdated, EGPMissionPhase, Phase, int32, GuardsRemaining, bool, bCipherRecovered);

UCLASS()
class GORILLAPROTOCOL_API UGPMissionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Mission")
    void BeginMission();

    void RegisterGuard(AActor* Guard);
    void RegisterExtractionZone(AGPExtractionZone* Zone);
    void NotifyGuardEliminated(AActor* Guard);

    UFUNCTION(BlueprintCallable, Category="Mission")
    void RecoverCipher();

    UFUNCTION(BlueprintCallable, Category="Mission")
    void CompleteExtraction();

    UFUNCTION(BlueprintCallable, Category="Mission")
    void FailMission();

    UFUNCTION(BlueprintPure, Category="Mission")
    EGPMissionPhase GetPhase() const { return Phase; }

    UFUNCTION(BlueprintPure, Category="Mission")
    int32 GetGuardsRemaining() const { return ActiveGuards.Num(); }

    UFUNCTION(BlueprintPure, Category="Mission")
    bool IsCipherRecovered() const { return bCipherRecovered; }

    UPROPERTY(BlueprintAssignable, Category="Mission")
    FGPMissionUpdated OnMissionUpdated;

private:
    void BroadcastState();

    EGPMissionPhase Phase = EGPMissionPhase::Infiltration;
    bool bCipherRecovered = false;
    TSet<TWeakObjectPtr<AActor>> ActiveGuards;
    TArray<TWeakObjectPtr<AGPExtractionZone>> ExtractionZones;
};
