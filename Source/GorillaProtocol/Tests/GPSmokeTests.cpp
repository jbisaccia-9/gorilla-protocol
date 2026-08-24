#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/GPHealthComponent.h"
#include "Game/GPMissionSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGPCoreDefaultsTest,
    "GorillaProtocol.Smoke.CoreDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPCoreDefaultsTest::RunTest(const FString& Parameters)
{
    const UGPHealthComponent* Health = NewObject<UGPHealthComponent>();
    TestNotNull(TEXT("Health component can be constructed"), Health);
    TestEqual(TEXT("Default health is 100"), Health->GetHealth(), 100.0f);
    TestFalse(TEXT("New health component is alive"), Health->IsDead());
    TestEqual(TEXT("Extraction phase enum is stable"), static_cast<uint8>(EGPMissionPhase::Extraction), static_cast<uint8>(2));
    return true;
}

#endif
