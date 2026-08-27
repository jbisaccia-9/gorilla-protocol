#include "Game/GPMissionSubsystem.h"

#include "Interaction/GPExtractionZone.h"

void UGPMissionSubsystem::BeginMission()
{
    ActiveGuards.Reset();
    bCipherRecovered = false;
    Phase = EGPMissionPhase::RecoverCipher;
    AlertState = EGPMissionAlertState::Covert;
    for (const TWeakObjectPtr<AGPExtractionZone>& Zone : ExtractionZones)
    {
        if (Zone.IsValid()) Zone->SetExtractionActive(false);
    }
    BroadcastState();
}

void UGPMissionSubsystem::RegisterGuard(AActor* Guard)
{
    if (IsValid(Guard))
    {
        ActiveGuards.Add(TWeakObjectPtr<AActor>(Guard));
        BroadcastState();
    }
}

void UGPMissionSubsystem::RegisterExtractionZone(AGPExtractionZone* Zone)
{
    if (IsValid(Zone))
    {
        ExtractionZones.AddUnique(TWeakObjectPtr<AGPExtractionZone>(Zone));
        Zone->SetExtractionActive(Phase == EGPMissionPhase::Extraction);
    }
}

void UGPMissionSubsystem::NotifyGuardEliminated(AActor* Guard)
{
    ActiveGuards.Remove(TWeakObjectPtr<AActor>(Guard));
    const EGPMissionAlertState PreviousAlertState = AlertState;
    RaiseAlert(EGPMissionAlertState::Suspicious);
    if (AlertState == PreviousAlertState)
    {
        BroadcastState();
    }
}

void UGPMissionSubsystem::RecoverCipher()
{
    if (!CanRecoverCipher(Phase, bCipherRecovered)) return;
    bCipherRecovered = true;
    Phase = EGPMissionPhase::Extraction;
    AlertState = EGPMissionAlertState::Escape;
    for (const TWeakObjectPtr<AGPExtractionZone>& Zone : ExtractionZones)
    {
        if (Zone.IsValid()) Zone->SetExtractionActive(true);
    }
    BroadcastState();
}

void UGPMissionSubsystem::CompleteExtraction()
{
    if (Phase != EGPMissionPhase::Extraction) return;
    Phase = EGPMissionPhase::Complete;
    AlertState = EGPMissionAlertState::Covert;
    BroadcastState();
}

void UGPMissionSubsystem::FailMission()
{
    if (Phase == EGPMissionPhase::Complete || Phase == EGPMissionPhase::Failed) return;
    Phase = EGPMissionPhase::Failed;
    AlertState = EGPMissionAlertState::Alarm;
    BroadcastState();
}

void UGPMissionSubsystem::RaiseAlert(EGPMissionAlertState RequestedState)
{
    if (Phase == EGPMissionPhase::Complete || Phase == EGPMissionPhase::Failed ||
        Phase == EGPMissionPhase::Extraction)
    {
        return;
    }

    const EGPMissionAlertState NewState = ResolveAlertEscalation(AlertState, RequestedState);
    if (NewState != AlertState)
    {
        AlertState = NewState;
        BroadcastState();
    }
}

bool UGPMissionSubsystem::IsAlarmActive() const
{
    return AlertState == EGPMissionAlertState::Alarm || AlertState == EGPMissionAlertState::Escape;
}

FText UGPMissionSubsystem::GetObjectiveText() const
{
    return ResolveObjectiveText(Phase);
}

bool UGPMissionSubsystem::CanRecoverCipher(EGPMissionPhase CurrentPhase, bool bAlreadyRecovered)
{
    return CurrentPhase == EGPMissionPhase::RecoverCipher && !bAlreadyRecovered;
}

FText UGPMissionSubsystem::ResolveObjectiveText(EGPMissionPhase CurrentPhase)
{
    switch (CurrentPhase)
    {
        case EGPMissionPhase::RecoverCipher:
            return NSLOCTEXT("GorillaProtocol", "ObjectiveRecoverCipher", "RECUPERA IL CIFRARIO BANANA");
        case EGPMissionPhase::Extraction:
            return NSLOCTEXT("GorillaProtocol", "ObjectiveExtraction", "RAGGIUNGI IL MOLO D'ESTRAZIONE");
        case EGPMissionPhase::Complete:
            return NSLOCTEXT("GorillaProtocol", "ObjectiveComplete", "MISSIONE COMPIUTA, SCIMMIONE");
        case EGPMissionPhase::Failed:
            return NSLOCTEXT("GorillaProtocol", "ObjectiveFailed", "MISSIONE FALLITA");
        default:
            return NSLOCTEXT("GorillaProtocol", "ObjectiveInfiltration", "INFILTRAZIONE");
    }
}

EGPMissionAlertState UGPMissionSubsystem::ResolveAlertEscalation(EGPMissionAlertState CurrentState,
    EGPMissionAlertState RequestedState)
{
    if (CurrentState == EGPMissionAlertState::Escape || RequestedState == EGPMissionAlertState::Escape)
    {
        return EGPMissionAlertState::Escape;
    }
    return static_cast<uint8>(RequestedState) > static_cast<uint8>(CurrentState) ? RequestedState : CurrentState;
}

void UGPMissionSubsystem::BroadcastState()
{
    for (auto It = ActiveGuards.CreateIterator(); It; ++It)
    {
        if (!It->IsValid()) It.RemoveCurrent();
    }
    OnMissionUpdated.Broadcast(Phase, ActiveGuards.Num(), bCipherRecovered);
    OnPresentationUpdated.Broadcast(Phase, AlertState);
}
