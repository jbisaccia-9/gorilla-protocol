#include "World/GPWorldDirector.h"

#include "Characters/GPGuardCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Interaction/GPExtractionZone.h"
#include "Interaction/GPObjectiveActor.h"
#include "Engine/World.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

AGPWorldDirector::AGPWorldDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Architecture = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Architecture"));
    Architecture->SetupAttachment(SceneRoot);
    Architecture->SetCollisionProfileName(TEXT("BlockAll"));
    Architecture->SetCanEverAffectNavigation(true);
    Architecture->SetCastShadow(true);

    CoverProps = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CoverProps"));
    CoverProps->SetupAttachment(SceneRoot);
    CoverProps->SetCollisionProfileName(TEXT("BlockAll"));
    CoverProps->SetCanEverAffectNavigation(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Architecture->SetStaticMesh(CubeMesh.Object);
        CoverProps->SetStaticMesh(CubeMesh.Object);
    }

    MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
    MoonLight->SetupAttachment(SceneRoot);
    MoonLight->SetRelativeRotation(FRotator(-38.0f, -24.0f, 0.0f));
    MoonLight->SetLightColor(FLinearColor(0.46f, 0.62f, 1.0f));
    MoonLight->SetIntensity(3.2f);
    MoonLight->SetAtmosphereSunLight(true);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetIntensity(0.7f);
    SkyLight->bRealTimeCapture = true;

    SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
    SkyAtmosphere->SetupAttachment(SceneRoot);

    HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
    HeightFog->SetupAttachment(SceneRoot);
    HeightFog->SetFogDensity(0.022f);
    HeightFog->SetFogHeightFalloff(0.16f);
    HeightFog->SetFogInscatteringColor(FLinearColor(0.03f, 0.10f, 0.13f));
    HeightFog->SetVolumetricFog(true);

    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
    PostProcess->SetupAttachment(SceneRoot);
    PostProcess->bUnbound = true;
    PostProcess->Settings.bOverride_BloomIntensity = true;
    PostProcess->Settings.BloomIntensity = 0.18f;
    PostProcess->Settings.bOverride_VignetteIntensity = true;
    PostProcess->Settings.VignetteIntensity = 0.22f;
    PostProcess->Settings.bOverride_MotionBlurAmount = true;
    PostProcess->Settings.MotionBlurAmount = 0.18f;
}

void AGPWorldDirector::BeginPlay()
{
    Super::BeginPlay();
    BuildGraybox();
    SpawnMissionActors();
}

void AGPWorldDirector::BuildGraybox()
{
    AddBox(Architecture, FVector(1800.0f, 0.0f, -50.0f), FVector(7200.0f, 4400.0f, 100.0f));
    AddBox(Architecture, FVector(1800.0f, -2200.0f, 300.0f), FVector(7200.0f, 100.0f, 700.0f));
    AddBox(Architecture, FVector(1800.0f, 2200.0f, 300.0f), FVector(7200.0f, 100.0f, 700.0f));
    AddBox(Architecture, FVector(-1800.0f, 0.0f, 300.0f), FVector(100.0f, 4400.0f, 700.0f));
    AddBox(Architecture, FVector(5400.0f, 0.0f, 300.0f), FVector(100.0f, 4400.0f, 700.0f));

    AddBox(Architecture, FVector(800.0f, -900.0f, 300.0f), FVector(100.0f, 2400.0f, 700.0f));
    AddBox(Architecture, FVector(2600.0f, 900.0f, 300.0f), FVector(100.0f, 2600.0f, 700.0f));
    AddBox(Architecture, FVector(4000.0f, -600.0f, 300.0f), FVector(100.0f, 3000.0f, 700.0f));

    const TArray<FVector> CoverLocations = {
        FVector(200.0f, 700.0f, 70.0f), FVector(1300.0f, -1500.0f, 70.0f),
        FVector(2200.0f, 200.0f, 70.0f), FVector(3300.0f, 1500.0f, 70.0f),
        FVector(4500.0f, -1400.0f, 70.0f), FVector(4700.0f, 900.0f, 70.0f)
    };
    for (const FVector& Location : CoverLocations)
    {
        AddBox(CoverProps, Location, FVector(220.0f, 120.0f, 140.0f), FRotator(0.0f, Location.X * 0.03f, 0.0f));
    }

    if (ANavMeshBoundsVolume* NavBounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(FVector(1800.0f, 0.0f, 250.0f), FRotator::ZeroRotator))
    {
        NavBounds->SetActorScale3D(FVector(38.0f, 24.0f, 5.0f));
        if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            Navigation->OnNavigationBoundsUpdated(NavBounds);
        }
    }
}

void AGPWorldDirector::SpawnMissionActors()
{
    GetWorld()->SpawnActor<AGPObjectiveActor>(FVector(4900.0f, 1550.0f, 110.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AGPExtractionZone>(FVector(-1200.0f, 0.0f, 10.0f), FRotator::ZeroRotator);

    const TArray<FVector> GuardLocations = {
        FVector(900.0f, 1300.0f, 90.0f), FVector(1500.0f, -1500.0f, 90.0f),
        FVector(2500.0f, -300.0f, 90.0f), FVector(3300.0f, 1450.0f, 90.0f),
        FVector(4200.0f, -1500.0f, 90.0f), FVector(4700.0f, 800.0f, 90.0f)
    };
    for (const FVector& Location : GuardLocations)
    {
        GetWorld()->SpawnActor<AGPGuardCharacter>(Location, FRotator(0.0f, 180.0f, 0.0f));
    }
}

void AGPWorldDirector::AddBox(UInstancedStaticMeshComponent* Component, const FVector& Location,
    const FVector& Size, const FRotator& Rotation)
{
    if (!Component) return;
    const FTransform Transform(Rotation, Location, Size / 100.0f);
    Component->AddInstance(Transform);
}
