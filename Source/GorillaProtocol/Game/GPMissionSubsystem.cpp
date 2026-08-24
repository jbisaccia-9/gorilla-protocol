#include "Game/GPMissionSubsystem.h"

#include "Interaction/GPExtractionZone.h"

void UGPMissionSubsystem::BeginMission()
{
    ActiveGuards.Reset();
    bCipherRecovered = false;
    Phase = EGPMissionPhase::RecoverCipher;
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
    BroadcastState();
}

void UGPMissionSubsystem::RecoverCipher()
{
    if (bCipherRecovered || Phase == EGPMissionPhase::Complete) return;
    bCipherRecovered = true;
    Phase = EGPMissionPhase::Extraction;
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
    BroadcastState();
}

void UGPMissionSubsystem::FailMission()
{
    if (Phase == EGPMissionPhase::Complete) return;
    Phase = EGPMissionPhase::Failed;
    BroadcastState();
}

void UGPMissionSubsystem::BroadcastState()
{
    for (auto It = ActiveGuards.CreateIterator(); It; ++It)
    {
        if (!It->IsValid()) It.RemoveCurrent();
    }
    OnMissionUpdated.Broadcast(Phase, ActiveGuards.Num(), bCipherRecovered);
}
