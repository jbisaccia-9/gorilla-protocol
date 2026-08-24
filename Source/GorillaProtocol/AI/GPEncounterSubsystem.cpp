#include "AI/GPEncounterSubsystem.h"

void UGPEncounterSubsystem::Tick(float DeltaTime)
{
    for (auto It = ActiveShooters.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
        {
            It.RemoveCurrent();
        }
    }
}

TStatId UGPEncounterSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGPEncounterSubsystem, STATGROUP_Tickables);
}

bool UGPEncounterSubsystem::RequestFireToken(AActor* Requester)
{
    if (!IsValid(Requester))
    {
        return false;
    }
    const TWeakObjectPtr<AActor> RequesterPtr(Requester);
    if (ActiveShooters.Contains(RequesterPtr))
    {
        return true;
    }
    if (ActiveShooters.Num() >= MaxConcurrentShooters)
    {
        return false;
    }
    ActiveShooters.Add(RequesterPtr);
    return true;
}

void UGPEncounterSubsystem::ReleaseFireToken(AActor* Requester)
{
    ActiveShooters.Remove(TWeakObjectPtr<AActor>(Requester));
}
