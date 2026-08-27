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

UENUM(BlueprintType)
enum class EGPMissionAlertState : uint8
{
    Covert,
    Suspicious,
    Alarm,
    Escape
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGPMissionUpdated, EGPMissionPhase, Phase, int32, GuardsRemaining, bool, bCipherRecovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPMissionPresentationUpdated, EGPMissionPhase, Phase,
    EGPMissionAlertState, AlertState);

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

    UFUNCTION(BlueprintCallable, Category="Mission|Presentation")
    void RaiseAlert(EGPMissionAlertState RequestedState);

    UFUNCTION(BlueprintPure, Category="Mission")
    EGPMissionPhase GetPhase() const { return Phase; }

    UFUNCTION(BlueprintPure, Category="Mission")
    int32 GetGuardsRemaining() const { return ActiveGuards.Num(); }

    UFUNCTION(BlueprintPure, Category="Mission")
    bool IsCipherRecovered() const { return bCipherRecovered; }

    UFUNCTION(BlueprintPure, Category="Mission|Presentation")
    EGPMissionAlertState GetAlertState() const { return AlertState; }

    UFUNCTION(BlueprintPure, Category="Mission|Presentation")
    bool IsAlarmActive() const;

    UFUNCTION(BlueprintPure, Category="Mission|Presentation")
    FText GetObjectiveText() const;

    static bool CanRecoverCipher(EGPMissionPhase CurrentPhase, bool bAlreadyRecovered);
    static FText ResolveObjectiveText(EGPMissionPhase CurrentPhase);
    static EGPMissionAlertState ResolveAlertEscalation(EGPMissionAlertState CurrentState,
        EGPMissionAlertState RequestedState);

    UPROPERTY(BlueprintAssignable, Category="Mission")
    FGPMissionUpdated OnMissionUpdated;

    UPROPERTY(BlueprintAssignable, Category="Mission|Presentation")
    FGPMissionPresentationUpdated OnPresentationUpdated;

private:
    void BroadcastState();

    EGPMissionPhase Phase = EGPMissionPhase::Infiltration;
    EGPMissionAlertState AlertState = EGPMissionAlertState::Covert;
    bool bCipherRecovered = false;
    TSet<TWeakObjectPtr<AActor>> ActiveGuards;
    TArray<TWeakObjectPtr<AGPExtractionZone>> ExtractionZones;
};
