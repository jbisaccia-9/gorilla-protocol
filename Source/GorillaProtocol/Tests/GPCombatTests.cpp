#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/GPEncounterSubsystem.h"
#include "AI/GPGuardAIController.h"
#include "Components/GPWeaponComponent.h"

namespace GPCombatTests
{
    constexpr uint32 TestFlags = EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPCombatRoleAssignmentTest,
    "GorillaProtocol.Combat.Coordination.RoleAssignment",
    GPCombatTests::TestFlags)

bool FGPCombatRoleAssignmentTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("First guard pressures"), static_cast<uint8>(UGPEncounterSubsystem::GetRoleForOrdinal(0)),
        static_cast<uint8>(EGPGuardTacticalRole::Pressure));
    TestEqual(TEXT("Second guard flanks left"), static_cast<uint8>(UGPEncounterSubsystem::GetRoleForOrdinal(1)),
        static_cast<uint8>(EGPGuardTacticalRole::FlankLeft));
    TestEqual(TEXT("Third guard flanks right"), static_cast<uint8>(UGPEncounterSubsystem::GetRoleForOrdinal(2)),
        static_cast<uint8>(EGPGuardTacticalRole::FlankRight));
    TestEqual(TEXT("Fourth guard supports"), static_cast<uint8>(UGPEncounterSubsystem::GetRoleForOrdinal(3)),
        static_cast<uint8>(EGPGuardTacticalRole::Support));
    TestEqual(TEXT("Roles cycle for larger squads"), static_cast<uint8>(UGPEncounterSubsystem::GetRoleForOrdinal(4)),
        static_cast<uint8>(EGPGuardTacticalRole::Pressure));
    TestEqual(TEXT("Negative ordinals safely resolve to pressure"),
        static_cast<uint8>(UGPEncounterSubsystem::GetRoleForOrdinal(-2)),
        static_cast<uint8>(EGPGuardTacticalRole::Pressure));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPCombatFormationTest,
    "GorillaProtocol.Combat.Coordination.FormationGeometry",
    GPCombatTests::TestFlags)

bool FGPCombatFormationTest::RunTest(const FString& Parameters)
{
    const FVector Left = UGPEncounterSubsystem::CalculateFormationOffset(
        EGPGuardTacticalRole::FlankLeft, FVector::ForwardVector, 1000.0f, 600.0f);
    const FVector Right = UGPEncounterSubsystem::CalculateFormationOffset(
        EGPGuardTacticalRole::FlankRight, FVector::ForwardVector, 1000.0f, 600.0f);
    const FVector Pressure = UGPEncounterSubsystem::CalculateFormationOffset(
        EGPGuardTacticalRole::Pressure, FVector::ForwardVector, 1000.0f, 600.0f);
    const FVector Support = UGPEncounterSubsystem::CalculateFormationOffset(
        EGPGuardTacticalRole::Support, FVector::ForwardVector, 1000.0f, 600.0f);

    TestTrue(TEXT("Flanks have matching forward depth"), FMath::IsNearlyEqual(Left.X, Right.X));
    TestTrue(TEXT("Flanks mirror laterally"), FMath::IsNearlyEqual(Left.Y, -Right.Y));
    TestTrue(TEXT("Flanks use the requested width"), FMath::IsNearlyEqual(FMath::Abs(Left.Y), 600.0f));
    TestTrue(TEXT("Pressure stays closer than support"), Pressure.Size2D() < Support.Size2D());
    TestTrue(TEXT("Zero direction still creates a useful offset"),
        !UGPEncounterSubsystem::CalculateFormationOffset(EGPGuardTacticalRole::Pressure,
            FVector::ZeroVector, 1000.0f, 600.0f).IsNearlyZero());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPCombatFireTokenPolicyTest,
    "GorillaProtocol.Combat.Coordination.FireTokenPolicy",
    GPCombatTests::TestFlags)

bool FGPCombatFireTokenPolicyTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Current shooter keeps its token"), FGPFireTokenPolicy::CanGrant(true, 2, 2, false));
    TestTrue(TEXT("Front of queue enters an open firing slot"), FGPFireTokenPolicy::CanGrant(false, 1, 2, true));
    TestFalse(TEXT("Full firing slots deny another shooter"), FGPFireTokenPolicy::CanGrant(false, 2, 2, true));
    TestFalse(TEXT("Later guards cannot jump the queue"), FGPFireTokenPolicy::CanGrant(false, 0, 2, false));
    TestTrue(TEXT("Invalid zero limit still permits one readable shooter"),
        FGPFireTokenPolicy::CanGrant(false, 0, 0, true));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPCombatTacticalTuningTest,
    "GorillaProtocol.Combat.Guards.TacticalTuning",
    GPCombatTests::TestFlags)

bool FGPCombatTacticalTuningTest::RunTest(const FString& Parameters)
{
    const float PressureRange = AGPGuardAIController::GetDesiredRangeForRole(EGPGuardTacticalRole::Pressure);
    const float FlankRange = AGPGuardAIController::GetDesiredRangeForRole(EGPGuardTacticalRole::FlankLeft);
    const float SupportRange = AGPGuardAIController::GetDesiredRangeForRole(EGPGuardTacticalRole::Support);
    TestTrue(TEXT("Pressure role fights closest"), PressureRange < FlankRange);
    TestTrue(TEXT("Support role fights farthest"), SupportRange > FlankRange);
    TestEqual(TEXT("Close bursts stay short and dodgeable"), AGPGuardAIController::GetBurstSizeForDistance(400.0f), 2);
    TestEqual(TEXT("Midrange bursts apply normal pressure"), AGPGuardAIController::GetBurstSizeForDistance(900.0f), 3);
    TestEqual(TEXT("Long-range bursts compensate for lower accuracy"), AGPGuardAIController::GetBurstSizeForDistance(1700.0f), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPCombatWeaponPatternTest,
    "GorillaProtocol.Combat.Weapon.DeterministicPattern",
    GPCombatTests::TestFlags)

bool FGPCombatWeaponPatternTest::RunTest(const FString& Parameters)
{
    FGPWeaponTuning Tuning;
    Tuning.HipSpreadDegrees = 1.2f;
    Tuning.AimSpreadDegrees = 0.2f;
    Tuning.SpreadPerShotDegrees = 0.3f;
    Tuning.MaxBloomDegrees = 0.8f;

    TestTrue(TEXT("Aiming produces a tighter first shot"),
        UGPWeaponComponent::CalculateSpreadDegrees(Tuning, true, 0.0f) <
        UGPWeaponComponent::CalculateSpreadDegrees(Tuning, false, 0.0f));
    TestTrue(TEXT("Bloom is added to base spread"), FMath::IsNearlyEqual(
        UGPWeaponComponent::CalculateSpreadDegrees(Tuning, true, 0.5f), 0.7f));
    TestTrue(TEXT("Bloom clamps at the configured cap"), FMath::IsNearlyEqual(
        UGPWeaponComponent::CalculateBloomAfterShot(Tuning, 0.7f), 0.8f));

    const FVector First = UGPWeaponComponent::CalculateShotDirection(FVector::ForwardVector, 2.0f, 3, 7331);
    const FVector Repeat = UGPWeaponComponent::CalculateShotDirection(FVector::ForwardVector, 2.0f, 3, 7331);
    const FVector Next = UGPWeaponComponent::CalculateShotDirection(FVector::ForwardVector, 2.0f, 4, 7331);
    TestTrue(TEXT("Shot direction remains normalized"), FMath::IsNearlyEqual(First.Size(), 1.0f, KINDA_SMALL_NUMBER));
    TestTrue(TEXT("Same seed and index reproduce exactly"), First.Equals(Repeat, KINDA_SMALL_NUMBER));
    TestFalse(TEXT("A later shot advances the recoil pattern"), First.Equals(Next, KINDA_SMALL_NUMBER));

    const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
        FVector::DotProduct(First, FVector::ForwardVector), -1.0f, 1.0f)));
    TestTrue(TEXT("Generated direction stays inside the requested cone"), AngleDegrees <= 2.0f + KINDA_SMALL_NUMBER);
    return true;
}

#endif
