#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/GPAgentCharacter.h"
#include "Game/GPMissionSubsystem.h"
#include "UI/GPHUD.h"
#include "World/GPWorldDirector.h"

namespace GPPresentationTests
{
constexpr uint32 TestFlags = EAutomationTestFlags::EditorContext |
    EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPPresentationLayoutTest,
    "GorillaProtocol.Presentation.HUD.ResponsiveLayout",
    GPPresentationTests::TestFlags)

bool FGPPresentationLayoutTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("1080p is the baseline scale"),
        FMath::IsNearlyEqual(AGPHUD::CalculateLayoutScale(1920.0f, 1080.0f), 1.0f));
    TestTrue(TEXT("720p remains readable at the lower clamp"),
        FMath::IsNearlyEqual(AGPHUD::CalculateLayoutScale(1280.0f, 720.0f), 0.70f));
    TestTrue(TEXT("Ultrawide layouts scale from height"),
        FMath::IsNearlyEqual(AGPHUD::CalculateLayoutScale(2560.0f, 1080.0f), 1.0f));
    TestTrue(TEXT("4K layouts respect the upper clamp"),
        FMath::IsNearlyEqual(AGPHUD::CalculateLayoutScale(3840.0f, 2160.0f), 1.35f));
    TestTrue(TEXT("Invalid dimensions fall back safely"),
        FMath::IsNearlyEqual(AGPHUD::CalculateLayoutScale(0.0f, 1080.0f), 1.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPPresentationCrosshairTest,
    "GorillaProtocol.Presentation.HUD.CrosshairFeedback",
    GPPresentationTests::TestFlags)

bool FGPPresentationCrosshairTest::RunTest(const FString& Parameters)
{
    const float HipGap = AGPHUD::CalculateCrosshairGap(false, 0.0f, 1.0f);
    const float AimGap = AGPHUD::CalculateCrosshairGap(true, 0.0f, 1.0f);
    const float MovingGap = AGPHUD::CalculateCrosshairGap(false, 1.0f, 1.0f);
    TestTrue(TEXT("Aiming tightens the reticle"), AimGap < HipGap);
    TestTrue(TEXT("Movement opens the reticle"), MovingGap > HipGap);
    TestTrue(TEXT("Movement input is clamped"), FMath::IsNearlyEqual(
        MovingGap, AGPHUD::CalculateCrosshairGap(false, 4.0f, 1.0f)));
    TestTrue(TEXT("Negative layout scale cannot invert the reticle"), FMath::IsNearlyZero(
        AGPHUD::CalculateCrosshairGap(false, 1.0f, -1.0f)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPPresentationCameraTest,
    "GorillaProtocol.Presentation.Agent.CameraResponse",
    GPPresentationTests::TestFlags)

bool FGPPresentationCameraTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Default movement uses the base FOV"), FMath::IsNearlyEqual(
        AGPAgentCharacter::ResolveTargetFieldOfView(false, false, 82.0f, 66.0f, 90.0f), 82.0f));
    TestTrue(TEXT("Sprinting expands the FOV"), FMath::IsNearlyEqual(
        AGPAgentCharacter::ResolveTargetFieldOfView(false, true, 82.0f, 66.0f, 90.0f), 90.0f));
    TestTrue(TEXT("Aiming takes priority over sprint state"), FMath::IsNearlyEqual(
        AGPAgentCharacter::ResolveTargetFieldOfView(true, true, 82.0f, 66.0f, 90.0f), 66.0f));
    TestTrue(TEXT("Small vertical movement does not kick the camera"), FMath::IsNearlyZero(
        AGPAgentCharacter::CalculateLandingKick(-100.0f)));
    TestTrue(TEXT("Hard landings clamp to a comfortable maximum"), FMath::IsNearlyEqual(
        AGPAgentCharacter::CalculateLandingKick(-4000.0f), 7.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPPresentationMissionRulesTest,
    "GorillaProtocol.Presentation.Mission.StateRules",
    GPPresentationTests::TestFlags)

bool FGPPresentationMissionRulesTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("The cipher is recoverable during its objective"),
        UGPMissionSubsystem::CanRecoverCipher(EGPMissionPhase::RecoverCipher, false));
    TestFalse(TEXT("The cipher cannot be recovered twice"),
        UGPMissionSubsystem::CanRecoverCipher(EGPMissionPhase::RecoverCipher, true));
    TestFalse(TEXT("Terminal missions cannot return to extraction"),
        UGPMissionSubsystem::CanRecoverCipher(EGPMissionPhase::Failed, false));
    TestEqual(TEXT("Alert escalation cannot move backward"),
        static_cast<uint8>(UGPMissionSubsystem::ResolveAlertEscalation(
            EGPMissionAlertState::Alarm, EGPMissionAlertState::Suspicious)),
        static_cast<uint8>(EGPMissionAlertState::Alarm));
    TestEqual(TEXT("Escape remains the terminal alert presentation"),
        static_cast<uint8>(UGPMissionSubsystem::ResolveAlertEscalation(
            EGPMissionAlertState::Escape, EGPMissionAlertState::Covert)),
        static_cast<uint8>(EGPMissionAlertState::Escape));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPPresentationObjectiveCopyTest,
    "GorillaProtocol.Presentation.Mission.ItalianObjectives",
    GPPresentationTests::TestFlags)

bool FGPPresentationObjectiveCopyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Cipher objective stays in Italian"),
        UGPMissionSubsystem::ResolveObjectiveText(EGPMissionPhase::RecoverCipher).ToString(),
        FString(TEXT("RECUPERA IL CIFRARIO BANANA")));
    TestEqual(TEXT("Extraction objective stays in Italian"),
        UGPMissionSubsystem::ResolveObjectiveText(EGPMissionPhase::Extraction).ToString(),
        FString(TEXT("RAGGIUNGI IL MOLO D'ESTRAZIONE")));
    TestFalse(TEXT("Every mission phase has HUD copy"),
        UGPMissionSubsystem::ResolveObjectiveText(EGPMissionPhase::Complete).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPPresentationWorldPaletteTest,
    "GorillaProtocol.Presentation.World.AlertPalette",
    GPPresentationTests::TestFlags)

bool FGPPresentationWorldPaletteTest::RunTest(const FString& Parameters)
{
    const FLinearColor Covert = AGPWorldDirector::ResolveSignalColor(EGPMissionAlertState::Covert);
    const FLinearColor Alarm = AGPWorldDirector::ResolveSignalColor(EGPMissionAlertState::Alarm);
    const FLinearColor Escape = AGPWorldDirector::ResolveSignalColor(EGPMissionAlertState::Escape);
    TestTrue(TEXT("Covert state reads green"), Covert.G > Covert.R && Covert.G > Covert.B);
    TestTrue(TEXT("Alarm state reads red"), Alarm.R > Alarm.G && Alarm.R > Alarm.B);
    TestTrue(TEXT("Escape state reads cyan"), Escape.G > Escape.R && Escape.B > Escape.R);

    for (int32 Sample = 0; Sample < 32; ++Sample)
    {
        const float Pulse = AGPWorldDirector::CalculateAlarmPulse(static_cast<float>(Sample) * 0.1f);
        TestTrue(TEXT("Alarm pulse remains in its authored intensity range"), Pulse >= 0.58f && Pulse <= 1.0f);
    }
    return true;
}

#endif
