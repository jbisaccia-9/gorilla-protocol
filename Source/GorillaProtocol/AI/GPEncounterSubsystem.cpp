#include "AI/GPEncounterSubsystem.h"

bool FGPFireTokenPolicy::CanGrant(bool bRequesterAlreadyActive, int32 ActiveCount, int32 Limit,
    bool bRequesterIsNext)
{
    return bRequesterAlreadyActive || (bRequesterIsNext && ActiveCount < FMath::Max(1, Limit));
}

void UGPEncounterSubsystem::Tick(float DeltaTime)
{
    PruneInvalidCombatants();

    if (SharedTarget.IsValid() && GetWorld() &&
        GetWorld()->GetTimeSeconds() - SharedContactTime > SharedContactMemorySeconds)
    {
        SharedTarget.Reset();
        ContactReporter.Reset();
    }
}

TStatId UGPEncounterSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGPEncounterSubsystem, STATGROUP_Tickables);
}

EGPGuardTacticalRole UGPEncounterSubsystem::RegisterCombatant(AActor* Combatant)
{
    if (!IsValid(Combatant))
    {
        return EGPGuardTacticalRole::Pressure;
    }

    const TWeakObjectPtr<AActor> CombatantPtr(Combatant);
    if (const EGPGuardTacticalRole* ExistingRole = TacticalRoles.Find(CombatantPtr))
    {
        return *ExistingRole;
    }

    const EGPGuardTacticalRole Role = GetRoleForOrdinal(NextRoleOrdinal++);
    TacticalRoles.Add(CombatantPtr, Role);
    return Role;
}

void UGPEncounterSubsystem::UnregisterCombatant(AActor* Combatant)
{
    if (!Combatant)
    {
        return;
    }

    const TWeakObjectPtr<AActor> CombatantPtr(Combatant);
    ActiveShooters.Remove(CombatantPtr);
    FireQueue.Remove(CombatantPtr);
    if (MeleeAttacker == CombatantPtr)
    {
        MeleeAttacker.Reset();
    }
    TacticalRoles.Remove(CombatantPtr);
    if (ContactReporter == CombatantPtr)
    {
        ContactReporter.Reset();
    }
    if (LastSuppressor == CombatantPtr)
    {
        LastSuppressor.Reset();
    }
}

bool UGPEncounterSubsystem::RequestFireToken(AActor* Requester)
{
    if (!IsValid(Requester))
    {
        return false;
    }
    PruneInvalidCombatants();
    const TWeakObjectPtr<AActor> RequesterPtr(Requester);
    const bool bAlreadyActive = ActiveShooters.Contains(RequesterPtr);
    if (!bAlreadyActive && !FireQueue.Contains(RequesterPtr))
    {
        FireQueue.Add(RequesterPtr);
    }

    const bool bRequesterIsNext = FireQueue.Num() > 0 && FireQueue[0] == RequesterPtr;
    if (!FGPFireTokenPolicy::CanGrant(bAlreadyActive, ActiveShooters.Num(), MaxConcurrentShooters,
        bRequesterIsNext))
    {
        return false;
    }

    FireQueue.Remove(RequesterPtr);
    ActiveShooters.Add(RequesterPtr);
    return true;
}

void UGPEncounterSubsystem::ReleaseFireToken(AActor* Requester)
{
    const TWeakObjectPtr<AActor> RequesterPtr(Requester);
    ActiveShooters.Remove(RequesterPtr);
    FireQueue.Remove(RequesterPtr);
}

bool UGPEncounterSubsystem::RequestMeleeToken(AActor* Requester)
{
    if (!IsValid(Requester))
    {
        return false;
    }
    if (!MeleeAttacker.IsValid() || MeleeAttacker.Get() == Requester)
    {
        MeleeAttacker = Requester;
        return true;
    }
    return false;
}

void UGPEncounterSubsystem::ReleaseMeleeToken(AActor* Requester)
{
    if (MeleeAttacker.Get() == Requester)
    {
        MeleeAttacker.Reset();
    }
}

void UGPEncounterSubsystem::ReportContact(AActor* Reporter, AActor* Target, const FVector& ContactLocation)
{
    if (!IsValid(Reporter) || !IsValid(Target))
    {
        return;
    }

    SharedTarget = Target;
    ContactReporter = Reporter;
    SharedContactLocation = ContactLocation;
    SharedContactTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

bool UGPEncounterSubsystem::GetSharedContact(AActor*& OutTarget, FVector& OutLocation) const
{
    OutTarget = nullptr;
    OutLocation = FVector::ZeroVector;
    if (!SharedTarget.IsValid())
    {
        return false;
    }
    if (GetWorld() && GetWorld()->GetTimeSeconds() - SharedContactTime > SharedContactMemorySeconds)
    {
        return false;
    }

    OutTarget = SharedTarget.Get();
    OutLocation = SharedContactLocation;
    return true;
}

void UGPEncounterSubsystem::ReportSuppression(AActor* Shooter, const FVector& InSuppressedLocation)
{
    if (!IsValid(Shooter))
    {
        return;
    }
    LastSuppressor = Shooter;
    SuppressedLocation = InSuppressedLocation;
    ++SuppressionSerial;
}

EGPGuardTacticalRole UGPEncounterSubsystem::GetTacticalRole(AActor* Combatant) const
{
    if (const EGPGuardTacticalRole* Role = TacticalRoles.Find(TWeakObjectPtr<AActor>(Combatant)))
    {
        return *Role;
    }
    return EGPGuardTacticalRole::Pressure;
}

EGPGuardTacticalRole UGPEncounterSubsystem::GetRoleForOrdinal(int32 Ordinal)
{
    static constexpr EGPGuardTacticalRole RoleOrder[] = {
        EGPGuardTacticalRole::Pressure,
        EGPGuardTacticalRole::FlankLeft,
        EGPGuardTacticalRole::FlankRight,
        EGPGuardTacticalRole::Support
    };
    const int32 SafeOrdinal = FMath::Max(0, Ordinal);
    return RoleOrder[SafeOrdinal % UE_ARRAY_COUNT(RoleOrder)];
}

FVector UGPEncounterSubsystem::CalculateFormationOffset(EGPGuardTacticalRole Role,
    const FVector& AwayFromTarget, float DesiredRange, float FlankWidth)
{
    FVector Away = AwayFromTarget.GetSafeNormal2D();
    if (Away.IsNearlyZero())
    {
        Away = FVector::BackwardVector;
    }
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Away).GetSafeNormal();
    const float Range = FMath::Max(100.0f, DesiredRange);
    const float Width = FMath::Max(0.0f, FlankWidth);

    switch (Role)
    {
        case EGPGuardTacticalRole::Pressure:
            return Away * Range * 0.72f;
        case EGPGuardTacticalRole::FlankLeft:
            return Away * Range - Right * Width;
        case EGPGuardTacticalRole::FlankRight:
            return Away * Range + Right * Width;
        case EGPGuardTacticalRole::Support:
            return Away * Range * 1.22f;
        default:
            return Away * Range;
    }
}

void UGPEncounterSubsystem::PruneInvalidCombatants()
{
    if (!MeleeAttacker.IsValid())
    {
        MeleeAttacker.Reset();
    }
    for (auto It = ActiveShooters.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
        {
            It.RemoveCurrent();
        }
    }
    FireQueue.RemoveAll([](const TWeakObjectPtr<AActor>& Combatant) { return !Combatant.IsValid(); });
    for (auto It = TacticalRoles.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
