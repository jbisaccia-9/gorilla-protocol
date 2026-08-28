#include "GPVerticalSliceDefinition.h"

namespace
{
bool RequireReference(const FSoftObjectPath& Reference, const TCHAR* Label, FString& OutFailure)
{
    if (Reference.IsValid())
    {
        return true;
    }

    OutFailure = FString::Printf(TEXT("Vertical slice definition is missing %s."), Label);
    return false;
}
}

FPrimaryAssetId UGPVerticalSliceDefinition::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("GPVerticalSlice"), GetFName());
}

bool UGPVerticalSliceDefinition::ValidateRequiredContent(FString& OutFailure) const
{
    OutFailure.Reset();

    if (!RequireReference(MissionMap.ToSoftObjectPath(), TEXT("the authored mission map"), OutFailure) ||
        !RequireReference(PlayerPawnClass.ToSoftObjectPath(), TEXT("the authored player pawn"), OutFailure) ||
        !RequireReference(RootHudWidgetClass.ToSoftObjectPath(), TEXT("the authored HUD"), OutFailure) ||
        !RequireReference(PlayerInputMapping.ToSoftObjectPath(), TEXT("the input mapping asset"), OutFailure) ||
        !RequireReference(FirstPersonGorillaMesh.ToSoftObjectPath(), TEXT("the first-person gorilla mesh"), OutFailure) ||
        !RequireReference(FullBodyGorillaMesh.ToSoftObjectPath(), TEXT("the full-body gorilla mesh"), OutFailure) ||
        !RequireReference(SuppressedPistolMesh.ToSoftObjectPath(), TEXT("the suppressed pistol mesh"), OutFailure) ||
        !RequireReference(GuardMesh.ToSoftObjectPath(), TEXT("the guard mesh"), OutFailure) ||
        !RequireReference(WatchmanClass.ToSoftObjectPath(), TEXT("the watchman class"), OutFailure) ||
        !RequireReference(RadioOperatorClass.ToSoftObjectPath(), TEXT("the radio operator class"), OutFailure) ||
        !RequireReference(EnforcerClass.ToSoftObjectPath(), TEXT("the enforcer class"), OutFailure))
    {
        return false;
    }

    if (RequiredPresentationAssets.IsEmpty())
    {
        OutFailure = TEXT("Vertical slice definition has no required presentation assets.");
        return false;
    }

    for (int32 Index = 0; Index < RequiredPresentationAssets.Num(); ++Index)
    {
        if (RequiredPresentationAssets[Index].IsNull())
        {
            OutFailure = FString::Printf(TEXT("Required presentation asset %d is empty."), Index);
            return false;
        }
    }

    return true;
}
