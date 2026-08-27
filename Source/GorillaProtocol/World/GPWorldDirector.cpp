#include "World/GPWorldDirector.h"

#include "Characters/GPGuardCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "Interaction/GPExtractionZone.h"
#include "Interaction/GPObjectiveActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor MidnightConcrete(0.018f, 0.040f, 0.055f);
const FLinearColor MediterraneanStucco(0.48f, 0.24f, 0.12f);
const FLinearColor InkBlue(0.012f, 0.030f, 0.044f);
const FLinearColor BananaGold(1.0f, 0.48f, 0.018f);
const FLinearColor CargoOrange(0.52f, 0.105f, 0.035f);
const FLinearColor PalmGreen(0.025f, 0.19f, 0.10f);
const FLinearColor SeaBlue(0.005f, 0.10f, 0.16f);

void ConfigureInstancedMesh(UInstancedStaticMeshComponent* Component, USceneComponent* Parent,
    bool bCollision, bool bNavigation, bool bShadow)
{
    Component->SetupAttachment(Parent);
    Component->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    Component->SetCanEverAffectNavigation(bNavigation);
    Component->SetCastShadow(bShadow);
}

void ConfigureAccentLight(UPointLightComponent* Light, USceneComponent* Parent, const FVector& Location,
    const FLinearColor& Color, float Intensity, float Radius)
{
    Light->SetupAttachment(Parent);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetRelativeLocation(Location);
    Light->SetLightColor(Color);
    Light->SetIntensity(Intensity);
    Light->SetAttenuationRadius(Radius);
    Light->SetCastShadows(false);
    Light->SetUseInverseSquaredFalloff(false);
}
}

AGPWorldDirector::AGPWorldDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Ground = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ground"));
    Stucco = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stucco"));
    Structure = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Structure"));
    BananaAccent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BananaAccent"));
    SignalProps = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SignalProps"));
    CoverProps = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CoverProps"));
    Foliage = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Foliage"));
    RoundProps = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RoundProps"));
    GlobeProps = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GlobeProps"));
    ScenicWater = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ScenicWater"));

    ConfigureInstancedMesh(Ground, SceneRoot, true, true, true);
    ConfigureInstancedMesh(Stucco, SceneRoot, true, true, true);
    ConfigureInstancedMesh(Structure, SceneRoot, true, true, true);
    ConfigureInstancedMesh(BananaAccent, SceneRoot, false, false, false);
    ConfigureInstancedMesh(SignalProps, SceneRoot, false, false, false);
    ConfigureInstancedMesh(CoverProps, SceneRoot, true, true, true);
    ConfigureInstancedMesh(Foliage, SceneRoot, false, false, true);
    ConfigureInstancedMesh(RoundProps, SceneRoot, false, false, true);
    ConfigureInstancedMesh(GlobeProps, SceneRoot, false, false, false);
    ConfigureInstancedMesh(ScenicWater, SceneRoot, false, false, false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (CubeMesh.Succeeded())
    {
        Ground->SetStaticMesh(CubeMesh.Object);
        Stucco->SetStaticMesh(CubeMesh.Object);
        Structure->SetStaticMesh(CubeMesh.Object);
        BananaAccent->SetStaticMesh(CubeMesh.Object);
        SignalProps->SetStaticMesh(CubeMesh.Object);
        CoverProps->SetStaticMesh(CubeMesh.Object);
        Foliage->SetStaticMesh(CubeMesh.Object);
        ScenicWater->SetStaticMesh(CubeMesh.Object);
    }
    if (CylinderMesh.Succeeded()) RoundProps->SetStaticMesh(CylinderMesh.Object);
    if (SphereMesh.Succeeded()) GlobeProps->SetStaticMesh(SphereMesh.Object);
    if (ShapeMaterial.Succeeded()) PrimitiveMaterial = ShapeMaterial.Object;

    MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
    MoonLight->SetupAttachment(SceneRoot);
    MoonLight->SetMobility(EComponentMobility::Movable);
    MoonLight->SetRelativeRotation(FRotator(-31.0f, -34.0f, 0.0f));
    MoonLight->SetLightColor(FLinearColor(0.38f, 0.60f, 1.0f));
    MoonLight->SetIntensity(4.4f);
    MoonLight->SetAtmosphereSunLight(true);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->SetIntensity(0.85f);
    SkyLight->bRealTimeCapture = false;

    SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
    SkyAtmosphere->SetupAttachment(SceneRoot);

    HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
    HeightFog->SetupAttachment(SceneRoot);
    HeightFog->SetFogDensity(0.014f);
    HeightFog->SetFogHeightFalloff(0.22f);
    HeightFog->SetFogInscatteringColor(FLinearColor(0.018f, 0.085f, 0.12f));
    HeightFog->SetVolumetricFog(false);

    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
    PostProcess->SetupAttachment(SceneRoot);
    PostProcess->bUnbound = true;
    PostProcess->Settings.bOverride_BloomIntensity = true;
    PostProcess->Settings.BloomIntensity = 0.34f;
    PostProcess->Settings.bOverride_VignetteIntensity = true;
    PostProcess->Settings.VignetteIntensity = 0.28f;
    PostProcess->Settings.bOverride_MotionBlurAmount = true;
    PostProcess->Settings.MotionBlurAmount = 0.0f;
    PostProcess->Settings.bOverride_ColorSaturation = true;
    PostProcess->Settings.ColorSaturation = FVector4(1.03f, 1.02f, 0.98f, 1.0f);
    PostProcess->Settings.bOverride_ColorContrast = true;
    PostProcess->Settings.ColorContrast = FVector4(1.08f, 1.06f, 1.04f, 1.0f);
    PostProcess->Settings.bOverride_AutoExposureBias = true;
    PostProcess->Settings.AutoExposureBias = 0.2f;

    CourtyardLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CourtyardLight"));
    DockLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DockLight"));
    ObjectiveLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ObjectiveLight"));
    AlarmLightWest = CreateDefaultSubobject<UPointLightComponent>(TEXT("AlarmLightWest"));
    AlarmLightCenter = CreateDefaultSubobject<UPointLightComponent>(TEXT("AlarmLightCenter"));
    AlarmLightEast = CreateDefaultSubobject<UPointLightComponent>(TEXT("AlarmLightEast"));
    ConfigureAccentLight(CourtyardLight, SceneRoot, FVector(2050.0f, -180.0f, 460.0f),
        FLinearColor(1.0f, 0.34f, 0.10f), 950.0f, 1050.0f);
    ConfigureAccentLight(DockLight, SceneRoot, FVector(-1200.0f, 0.0f, 260.0f),
        GetCovertAccentColor(), 1100.0f, 850.0f);
    ConfigureAccentLight(ObjectiveLight, SceneRoot, FVector(4860.0f, 1430.0f, 280.0f),
        GetCovertAccentColor(), 1500.0f, 780.0f);
    ConfigureAccentLight(AlarmLightWest, SceneRoot, FVector(950.0f, -1040.0f, 520.0f),
        GetAlarmAccentColor(), 0.0f, 800.0f);
    ConfigureAccentLight(AlarmLightCenter, SceneRoot, FVector(2950.0f, 90.0f, 510.0f),
        GetAlarmAccentColor(), 0.0f, 900.0f);
    ConfigureAccentLight(AlarmLightEast, SceneRoot, FVector(4700.0f, 1580.0f, 610.0f),
        GetAlarmAccentColor(), 0.0f, 820.0f);
}

void AGPWorldDirector::BeginPlay()
{
    Super::BeginPlay();
    ApplyRuntimeMaterials();
    BuildCoastalFacility();
    SpawnMissionActors();

    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->OnPresentationUpdated.AddDynamic(this, &AGPWorldDirector::HandleMissionPresentation);
        HandleMissionPresentation(Mission->GetPhase(), Mission->GetAlertState());
    }
}

void AGPWorldDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AlarmClock += DeltaSeconds;
    const float Pulse = CalculateAlarmPulse(AlarmClock);
    const FLinearColor SignalColor = ResolveSignalColor(CurrentAlertState);
    const bool bEscape = CurrentAlertState == EGPMissionAlertState::Escape;
    const float Intensity = (bEscape ? 1700.0f : 2300.0f) * Pulse;
    AlarmLightWest->SetIntensity(Intensity);
    AlarmLightCenter->SetIntensity(Intensity * 1.08f);
    AlarmLightEast->SetIntensity(Intensity * 0.92f);
    AlarmLightWest->SetLightColor(SignalColor);
    AlarmLightCenter->SetLightColor(SignalColor);
    AlarmLightEast->SetLightColor(SignalColor);
    if (SignalMaterial)
    {
        FLinearColor PulsedColor = SignalColor * (0.72f + Pulse * 0.45f);
        PulsedColor.A = 1.0f;
        SignalMaterial->SetVectorParameterValue(TEXT("Color"), PulsedColor);
    }
}

FLinearColor AGPWorldDirector::GetCovertAccentColor()
{
    return FLinearColor(0.08f, 0.92f, 0.53f);
}

FLinearColor AGPWorldDirector::GetAlarmAccentColor()
{
    return FLinearColor(1.0f, 0.035f, 0.018f);
}

FLinearColor AGPWorldDirector::GetEscapeAccentColor()
{
    return FLinearColor(0.02f, 0.78f, 1.0f);
}

FLinearColor AGPWorldDirector::ResolveSignalColor(EGPMissionAlertState AlertState)
{
    switch (AlertState)
    {
        case EGPMissionAlertState::Suspicious: return BananaGold;
        case EGPMissionAlertState::Alarm: return GetAlarmAccentColor();
        case EGPMissionAlertState::Escape: return GetEscapeAccentColor();
        default: return GetCovertAccentColor();
    }
}

float AGPWorldDirector::CalculateAlarmPulse(float TimeSeconds)
{
    return 0.58f + 0.42f * FMath::Square(FMath::Sin(TimeSeconds * 5.8f));
}

void AGPWorldDirector::ApplyRuntimeMaterials()
{
    if (!PrimitiveMaterial) return;
    auto MakeMaterial = [this](const TCHAR* Name, const FLinearColor& Color)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(PrimitiveMaterial, this, FName(Name));
        Material->SetVectorParameterValue(TEXT("Color"), Color);
        return Material;
    };

    GroundMaterial = MakeMaterial(TEXT("MI_Ground"), MidnightConcrete);
    StuccoMaterial = MakeMaterial(TEXT("MI_Stucco"), MediterraneanStucco);
    StructureMaterial = MakeMaterial(TEXT("MI_Structure"), InkBlue);
    BananaMaterial = MakeMaterial(TEXT("MI_Banana"), BananaGold);
    SignalMaterial = MakeMaterial(TEXT("MI_Signal"), GetCovertAccentColor());
    CoverMaterial = MakeMaterial(TEXT("MI_Cover"), CargoOrange);
    FoliageMaterial = MakeMaterial(TEXT("MI_Foliage"), PalmGreen);
    WaterMaterial = MakeMaterial(TEXT("MI_Water"), SeaBlue);

    Ground->SetMaterial(0, GroundMaterial);
    Stucco->SetMaterial(0, StuccoMaterial);
    Structure->SetMaterial(0, StructureMaterial);
    BananaAccent->SetMaterial(0, BananaMaterial);
    SignalProps->SetMaterial(0, SignalMaterial);
    CoverProps->SetMaterial(0, CoverMaterial);
    Foliage->SetMaterial(0, FoliageMaterial);
    RoundProps->SetMaterial(0, StructureMaterial);
    GlobeProps->SetMaterial(0, SignalMaterial);
    ScenicWater->SetMaterial(0, WaterMaterial);
}

void AGPWorldDirector::BuildCoastalFacility()
{
    Ground->ClearInstances();
    Stucco->ClearInstances();
    Structure->ClearInstances();
    BananaAccent->ClearInstances();
    SignalProps->ClearInstances();
    CoverProps->ClearInstances();
    Foliage->ClearInstances();
    RoundProps->ClearInstances();
    GlobeProps->ClearInstances();
    ScenicWater->ClearInstances();

    // Island slab, open sea, cliff wall and a low sea wall frame the route without hiding the horizon.
    AddBox(Ground, FVector(1800.0f, 0.0f, -45.0f), FVector(7200.0f, 4400.0f, 90.0f));
    AddBox(ScenicWater, FVector(1800.0f, 3150.0f, -90.0f), FVector(9800.0f, 1900.0f, 35.0f));
    AddBox(Structure, FVector(1800.0f, -2190.0f, 235.0f), FVector(7200.0f, 120.0f, 560.0f));
    AddBox(Stucco, FVector(-1780.0f, -550.0f, 170.0f), FVector(110.0f, 3300.0f, 430.0f));
    AddBox(Stucco, FVector(5380.0f, -350.0f, 170.0f), FVector(110.0f, 3700.0f, 430.0f));
    for (float X = -1500.0f; X <= 5100.0f; X += 600.0f)
    {
        AddBox(Stucco, FVector(X, 2170.0f, 65.0f), FVector(420.0f, 90.0f, 130.0f));
    }

    // Arrival dock and extraction plaza.
    AddBox(Structure, FVector(-1220.0f, 0.0f, 10.0f), FVector(900.0f, 900.0f, 110.0f));
    AddBox(BananaAccent, FVector(-1220.0f, -430.0f, 72.0f), FVector(900.0f, 24.0f, 18.0f));
    AddBox(BananaAccent, FVector(-1220.0f, 430.0f, 72.0f), FVector(900.0f, 24.0f, 18.0f));
    for (float Y : {-340.0f, 340.0f})
    {
        AddPrimitive(RoundProps, FVector(-1530.0f, Y, 120.0f), FVector(0.11f, 0.11f, 0.55f));
        AddPrimitive(GlobeProps, FVector(-1530.0f, Y, 190.0f), FVector(0.13f));
    }
    BuildBananaMark(FVector(-1730.0f, 0.0f, 195.0f), FRotator(0.0f, 90.0f, 0.0f), 1.15f);

    // South villa: warm stucco massing, dark roof and banana-gold spy windows.
    AddBox(Stucco, FVector(850.0f, -1580.0f, 190.0f), FVector(1120.0f, 940.0f, 380.0f));
    AddBox(Structure, FVector(850.0f, -1580.0f, 405.0f), FVector(1240.0f, 1050.0f, 55.0f));
    AddBox(Structure, FVector(400.0f, -1098.0f, 220.0f), FVector(220.0f, 18.0f, 150.0f));
    AddBox(Structure, FVector(850.0f, -1098.0f, 220.0f), FVector(220.0f, 18.0f, 150.0f));
    AddBox(Structure, FVector(1300.0f, -1098.0f, 220.0f), FVector(220.0f, 18.0f, 150.0f));
    AddBox(BananaAccent, FVector(850.0f, -1085.0f, 392.0f), FVector(990.0f, 16.0f, 20.0f));
    AddPrimitive(RoundProps, FVector(960.0f, -1580.0f, 610.0f), FVector(0.08f, 0.08f, 3.4f));
    AddBox(SignalProps, FVector(960.0f, -1580.0f, 780.0f), FVector(80.0f, 80.0f, 30.0f));

    // North operations wing leaves a broad tactical courtyard in front of it.
    AddBox(Stucco, FVector(2550.0f, 1740.0f, 165.0f), FVector(1500.0f, 690.0f, 330.0f));
    AddBox(Structure, FVector(2550.0f, 1740.0f, 360.0f), FVector(1620.0f, 800.0f, 60.0f));
    for (float X : {2050.0f, 2400.0f, 2750.0f, 3100.0f})
    {
        AddBox(Structure, FVector(X, 1390.0f, 190.0f), FVector(210.0f, 20.0f, 130.0f));
    }
    AddBox(BananaAccent, FVector(2550.0f, 1378.0f, 345.0f), FVector(1320.0f, 18.0f, 18.0f));
    BuildBananaMark(FVector(2550.0f, 1360.0f, 205.0f), FRotator::ZeroRotator, 0.72f);

    // Control tower and cipher pavilion terminate the combat route with a strong silhouette.
    AddBox(Stucco, FVector(4760.0f, 1860.0f, 270.0f), FVector(1040.0f, 520.0f, 540.0f));
    AddBox(Structure, FVector(4760.0f, 1860.0f, 575.0f), FVector(1160.0f, 640.0f, 70.0f));
    AddBox(Structure, FVector(4520.0f, 1590.0f, 335.0f), FVector(180.0f, 20.0f, 220.0f));
    AddBox(Structure, FVector(4760.0f, 1590.0f, 335.0f), FVector(180.0f, 20.0f, 220.0f));
    AddBox(Structure, FVector(5000.0f, 1590.0f, 335.0f), FVector(180.0f, 20.0f, 220.0f));
    AddPrimitive(RoundProps, FVector(4760.0f, 1860.0f, 820.0f), FVector(0.10f, 0.10f, 4.1f));
    AddBox(SignalProps, FVector(4760.0f, 1860.0f, 1015.0f), FVector(95.0f, 95.0f, 38.0f));
    AddBox(Structure, FVector(4860.0f, 1360.0f, 115.0f), FVector(680.0f, 460.0f, 40.0f));
    AddBox(BananaAccent, FVector(4860.0f, 1360.0f, 145.0f), FVector(600.0f, 380.0f, 18.0f));

    // Tactical rhythm: checkpoint slats and offset cargo create three readable combat lanes.
    AddBox(Structure, FVector(1670.0f, -510.0f, 175.0f), FVector(90.0f, 920.0f, 350.0f));
    AddBox(Structure, FVector(1670.0f, 830.0f, 175.0f), FVector(90.0f, 780.0f, 350.0f));
    AddBox(Structure, FVector(3320.0f, -1020.0f, 175.0f), FVector(90.0f, 900.0f, 350.0f));
    AddBox(Structure, FVector(3320.0f, 510.0f, 175.0f), FVector(90.0f, 1020.0f, 350.0f));
    AddBox(BananaAccent, FVector(1685.0f, -42.0f, 260.0f), FVector(120.0f, 18.0f, 18.0f));
    AddBox(BananaAccent, FVector(3335.0f, -555.0f, 260.0f), FVector(120.0f, 18.0f, 18.0f));

    const TArray<FVector> CargoLocations = {
        FVector(150.0f, 690.0f, 70.0f), FVector(1180.0f, 480.0f, 70.0f),
        FVector(2110.0f, -760.0f, 70.0f), FVector(2820.0f, 720.0f, 70.0f),
        FVector(3820.0f, -1440.0f, 70.0f), FVector(4140.0f, 760.0f, 70.0f)
    };
    for (int32 Index = 0; Index < CargoLocations.Num(); ++Index)
    {
        const FVector& Location = CargoLocations[Index];
        AddBox(CoverProps, Location, FVector(230.0f, 150.0f, 140.0f),
            FRotator(0.0f, Index % 2 == 0 ? 8.0f : -12.0f, 0.0f));
        AddBox(BananaAccent, Location + FVector(0.0f, 0.0f, 73.0f), FVector(170.0f, 95.0f, 8.0f));
    }

    for (const FVector& LampLocation : {
        FVector(-450.0f, 1200.0f, 0.0f), FVector(2050.0f, -160.0f, 0.0f),
        FVector(2950.0f, 1130.0f, 0.0f), FVector(4380.0f, -1080.0f, 0.0f)})
    {
        AddPrimitive(RoundProps, LampLocation + FVector(0.0f, 0.0f, 205.0f), FVector(0.075f, 0.075f, 4.1f));
        AddPrimitive(GlobeProps, LampLocation + FVector(0.0f, 0.0f, 425.0f), FVector(0.14f));
    }

    BuildPalm(FVector(-720.0f, 1830.0f, 0.0f), 0.95f, 8.0f);
    BuildPalm(FVector(620.0f, 1850.0f, 0.0f), 1.15f, -18.0f);
    BuildPalm(FVector(3700.0f, 1830.0f, 0.0f), 1.05f, 16.0f);
    BuildPalm(FVector(5150.0f, -1820.0f, 0.0f), 0.90f, -12.0f);

    if (ANavMeshBoundsVolume* NavBounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(
        FVector(1800.0f, 0.0f, 250.0f), FRotator::ZeroRotator))
    {
        NavBounds->SetActorScale3D(FVector(38.0f, 24.0f, 5.0f));
        if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            Navigation->OnNavigationBoundsUpdated(NavBounds);
        }
    }
}

void AGPWorldDirector::BuildPalm(const FVector& Location, float Scale, float Yaw)
{
    AddPrimitive(RoundProps, Location + FVector(0.0f, 0.0f, 190.0f * Scale),
        FVector(0.13f * Scale, 0.13f * Scale, 3.8f * Scale), FRotator(0.0f, Yaw, 4.0f));
    for (int32 Leaf = 0; Leaf < 6; ++Leaf)
    {
        const float Angle = Yaw + Leaf * 60.0f;
        const FVector Direction = FRotator(0.0f, Angle, 0.0f).Vector();
        AddBox(Foliage, Location + FVector(0.0f, 0.0f, 405.0f * Scale) + Direction * 92.0f * Scale,
            FVector(255.0f, 38.0f, 20.0f) * Scale, FRotator(-10.0f, Angle, 0.0f));
    }
}

void AGPWorldDirector::BuildBananaMark(const FVector& Location, const FRotator& Rotation, float Scale)
{
    const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
    const FVector Up = Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));
    AddBox(BananaAccent, Location - Right * 44.0f * Scale, FVector(34.0f, 14.0f, 100.0f) * Scale,
        Rotation + FRotator(0.0f, 0.0f, -24.0f));
    AddBox(BananaAccent, Location + Up * 22.0f * Scale, FVector(34.0f, 14.0f, 120.0f) * Scale, Rotation);
    AddBox(BananaAccent, Location + Right * 44.0f * Scale, FVector(34.0f, 14.0f, 100.0f) * Scale,
        Rotation + FRotator(0.0f, 0.0f, 24.0f));
}

void AGPWorldDirector::SpawnMissionActors()
{
    GetWorld()->SpawnActor<AGPObjectiveActor>(FVector(4860.0f, 1360.0f, 190.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AGPExtractionZone>(FVector(-1220.0f, 0.0f, 70.0f), FRotator::ZeroRotator);

    const TArray<FVector> GuardLocations = {
        FVector(620.0f, 760.0f, 90.0f), FVector(1320.0f, -500.0f, 90.0f),
        FVector(2180.0f, 870.0f, 90.0f), FVector(2850.0f, -720.0f, 90.0f),
        FVector(3900.0f, 520.0f, 90.0f), FVector(4550.0f, 1120.0f, 90.0f)
    };
    for (int32 Index = 0; Index < GuardLocations.Num(); ++Index)
    {
        const float Yaw = Index % 2 == 0 ? 195.0f : 150.0f;
        GetWorld()->SpawnActor<AGPGuardCharacter>(GuardLocations[Index], FRotator(0.0f, Yaw, 0.0f));
    }
}

void AGPWorldDirector::HandleMissionPresentation(EGPMissionPhase NewPhase,
    EGPMissionAlertState NewAlertState)
{
    CurrentPhase = NewPhase;
    CurrentAlertState = NewAlertState;
    const bool bAnimatedAlarm = NewAlertState == EGPMissionAlertState::Alarm ||
        NewAlertState == EGPMissionAlertState::Escape;
    SetActorTickEnabled(bAnimatedAlarm);

    const FLinearColor SignalColor = ResolveSignalColor(NewAlertState);
    const float StaticAlarmIntensity = NewAlertState == EGPMissionAlertState::Suspicious ? 520.0f : 0.0f;

    if (SignalMaterial) SignalMaterial->SetVectorParameterValue(TEXT("Color"), SignalColor);
    AlarmLightWest->SetIntensity(StaticAlarmIntensity);
    AlarmLightCenter->SetIntensity(StaticAlarmIntensity);
    AlarmLightEast->SetIntensity(StaticAlarmIntensity);
    AlarmLightWest->SetLightColor(SignalColor);
    AlarmLightCenter->SetLightColor(SignalColor);
    AlarmLightEast->SetLightColor(SignalColor);
    ObjectiveLight->SetLightColor(NewPhase == EGPMissionPhase::Extraction ? GetEscapeAccentColor() : SignalColor);
    DockLight->SetLightColor(NewPhase == EGPMissionPhase::Extraction ? GetEscapeAccentColor() : GetCovertAccentColor());
    ObjectiveLight->SetIntensity(NewPhase == EGPMissionPhase::Extraction ? 480.0f : 1500.0f);
    DockLight->SetIntensity(NewPhase == EGPMissionPhase::Extraction ? 1900.0f : 1100.0f);

    HeightFog->SetFogInscatteringColor(NewAlertState == EGPMissionAlertState::Alarm
        ? FLinearColor(0.11f, 0.022f, 0.018f)
        : FLinearColor(0.018f, 0.085f, 0.12f));
    PostProcess->Settings.VignetteIntensity = NewAlertState == EGPMissionAlertState::Alarm ? 0.36f : 0.28f;
}

void AGPWorldDirector::AddBox(UInstancedStaticMeshComponent* Component, const FVector& Location,
    const FVector& Size, const FRotator& Rotation)
{
    AddPrimitive(Component, Location, Size / 100.0f, Rotation);
}

void AGPWorldDirector::AddPrimitive(UInstancedStaticMeshComponent* Component, const FVector& Location,
    const FVector& Scale, const FRotator& Rotation)
{
    if (!Component) return;
    Component->AddInstance(FTransform(Rotation, Location, Scale));
}
