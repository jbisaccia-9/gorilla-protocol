#include "GPGameModeBase.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "../AI/GPGuardCharacter.h"
#include "../Player/GPBrunoCharacter.h"
#include "GPHUD.h"
#include "GPMissionZone.h"

AGPGameModeBase::AGPGameModeBase()
{
    DefaultPawnClass = AGPBrunoCharacter::StaticClass();
    HUDClass = AGPHUD::StaticClass();
}

void AGPGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    BuildMissionSpace();
    SpawnCombatants();

    if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (!Player->GetPawn())
        {
            RestartPlayerAtTransform(Player,
                FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 145.0f)));
        }
    }
}

UMaterialInterface* AGPGameModeBase::CreateColorMaterial(UObject* Outer, const FLinearColor& Color) const
{
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Base)
    {
        return nullptr;
    }
    UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(Base, Outer);
    Dynamic->SetVectorParameterValue(TEXT("Color"), Color);
    return Dynamic;
}

AActor* AGPGameModeBase::SpawnBlock(const FVector& Location, const FVector& Size,
    const FLinearColor& Color, const TCHAR* Label)
{
    if (!CubeMesh)
    {
        return nullptr;
    }
    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    if (!Block)
    {
        return nullptr;
    }
#if WITH_EDITOR
    Block->SetActorLabel(Label);
#endif
    Block->SetActorScale3D(Size / 100.0f);
    UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
    Mesh->SetStaticMesh(CubeMesh);
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetVisibility(false, true);
    return Block;
}

void AGPGameModeBase::BuildMissionSpace()
{
    const FLinearColor WetDeck(0.055f, 0.07f, 0.085f);
    const FLinearColor Concrete(0.13f, 0.15f, 0.17f);
    const FLinearColor Glass(0.025f, 0.11f, 0.14f);
    const FLinearColor Brass(0.28f, 0.17f, 0.055f);

    SpawnBlock(FVector(1700.0f, 0.0f, -45.0f), FVector(4100.0f, 1900.0f, 90.0f), WetDeck, TEXT("Wet yacht deck"));
    SpawnBlock(FVector(1700.0f, -980.0f, 90.0f), FVector(4100.0f, 45.0f, 280.0f), Concrete, TEXT("Port rail"));
    SpawnBlock(FVector(1700.0f, 980.0f, 90.0f), FVector(4100.0f, 45.0f, 280.0f), Concrete, TEXT("Starboard rail"));
    SpawnBlock(FVector(3750.0f, 0.0f, 220.0f), FVector(80.0f, 1960.0f, 520.0f), Concrete, TEXT("Bow bulkhead"));
    SpawnBlock(FVector(2250.0f, 0.0f, 250.0f), FVector(900.0f, 1000.0f, 520.0f), Concrete, TEXT("Glass lounge"));
    SpawnBlock(FVector(1790.0f, 0.0f, 250.0f), FVector(45.0f, 920.0f, 420.0f), Glass, TEXT("Lounge glass"));
    SpawnBlock(FVector(2500.0f, -520.0f, 185.0f), FVector(1350.0f, 55.0f, 390.0f), Concrete, TEXT("Cabin port wall"));
    SpawnBlock(FVector(2500.0f, 520.0f, 185.0f), FVector(1350.0f, 55.0f, 390.0f), Concrete, TEXT("Cabin starboard wall"));
    SpawnBlock(FVector(800.0f, -430.0f, 80.0f), FVector(330.0f, 180.0f, 170.0f), Concrete, TEXT("Aft cover A"));
    SpawnBlock(FVector(1180.0f, 360.0f, 95.0f), FVector(260.0f, 240.0f, 200.0f), Concrete, TEXT("Aft cover B"));
    SpawnBlock(FVector(1670.0f, -620.0f, 70.0f), FVector(420.0f, 150.0f, 150.0f), Concrete, TEXT("Planter cover"));
    SpawnBlock(FVector(3050.0f, 390.0f, 85.0f), FVector(300.0f, 200.0f, 180.0f), Brass, TEXT("Ledger console"));

    ADirectionalLight* Moon = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 1200.0f), FRotator(-48.0f, -32.0f, 0.0f));
    if (Moon)
    {
        Moon->GetLightComponent()->SetIntensity(5.2f);
        Moon->GetLightComponent()->SetLightColor(FLinearColor(0.28f, 0.48f, 0.82f));
        Moon->GetLightComponent()->SetCastShadows(true);
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky)
    {
        Sky->GetLightComponent()->SetIntensity(0.65f);
        Sky->GetLightComponent()->SetRealTimeCapture(false);
        Sky->GetLightComponent()->RecaptureSky();
    }

    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    if (Fog)
    {
        Fog->GetComponent()->SetFogDensity(0.018f);
        Fog->GetComponent()->SetFogHeightFalloff(0.22f);
        Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.025f, 0.08f, 0.14f));
    }

    APostProcessVolume* Post = GetWorld()->SpawnActor<APostProcessVolume>();
    if (Post)
    {
        Post->bUnbound = true;
        Post->Settings.bOverride_BloomIntensity = true;
        Post->Settings.BloomIntensity = 0.9f;
        Post->Settings.bOverride_VignetteIntensity = true;
        Post->Settings.VignetteIntensity = 0.34f;
        Post->Settings.bOverride_AutoExposureBias = true;
        Post->Settings.AutoExposureBias = -0.45f;
    }

    const TArray<FVector> WarmLights = {
        FVector(720.0f, -760.0f, 250.0f), FVector(1250.0f, 720.0f, 260.0f),
        FVector(1870.0f, -380.0f, 330.0f), FVector(2550.0f, 390.0f, 320.0f),
        FVector(3220.0f, -620.0f, 270.0f)
    };
    for (const FVector& Position : WarmLights)
    {
        APointLight* Light = GetWorld()->SpawnActor<APointLight>(Position, FRotator::ZeroRotator);
        if (Light)
        {
            if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(Light->GetLightComponent()))
            {
                PointLight->SetIntensity(4800.0f);
                PointLight->SetAttenuationRadius(720.0f);
                PointLight->SetLightColor(FLinearColor(1.0f, 0.38f, 0.12f));
            }
        }
    }
}

void AGPGameModeBase::SpawnCombatants()
{
    const TArray<FVector> GuardLocations = {
        FVector(760.0f, 310.0f, 110.0f), FVector(1320.0f, -470.0f, 110.0f),
        FVector(1760.0f, 650.0f, 110.0f), FVector(2880.0f, -630.0f, 110.0f)
    };
    for (const FVector& Location : GuardLocations)
    {
        if (GetWorld()->SpawnActor<AGPGuardCharacter>(Location, FRotator(0.0f, 180.0f, 0.0f)))
        {
            ++GuardsRemaining;
        }
    }

    AGPMissionZone* Ledger = GetWorld()->SpawnActor<AGPMissionZone>(FVector(3100.0f, 390.0f, 205.0f), FRotator::ZeroRotator);
    if (Ledger)
    {
        Ledger->Configure(false);
    }
    ExtractionZone = GetWorld()->SpawnActor<AGPMissionZone>(FVector(-150.0f, 0.0f, 85.0f), FRotator::ZeroRotator);
    if (ExtractionZone)
    {
        ExtractionZone->Configure(true);
        ExtractionZone->SetActive(false);
    }
}

void AGPGameModeBase::NotifyGuardDefeated()
{
    GuardsRemaining = FMath::Max(0, GuardsRemaining - 1);
}

void AGPGameModeBase::CollectLedger()
{
    if (bHasLedger)
    {
        return;
    }
    bHasLedger = true;
    if (ExtractionZone)
    {
        ExtractionZone->SetActive(true);
    }
    if (AGPBrunoCharacter* Bruno = Cast<AGPBrunoCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        Bruno->SpeakObjective();
    }
}

void AGPGameModeBase::TryExtract()
{
    if (!bHasLedger || bMissionComplete)
    {
        return;
    }
    bMissionComplete = true;
    if (AGPBrunoCharacter* Bruno = Cast<AGPBrunoCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        Bruno->SpeakComplete();
        Bruno->DisableInput(UGameplayStatics::GetPlayerController(this, 0));
    }
}

void AGPGameModeBase::NotifyLure(const FVector& Location)
{
    for (TActorIterator<AGPGuardCharacter> It(GetWorld()); It; ++It)
    {
        It->HearLure(Location);
    }
}

FString AGPGameModeBase::GetObjectiveText() const
{
    if (bMissionComplete)
    {
        return TEXT("MISSIONE COMPIUTA - OPERAZIONE SCIMMIA DI MARE");
    }
    if (bHasLedger)
    {
        return TEXT("TORNA A POPPA PER L'ESTRAZIONE");
    }
    return TEXT("RUBA IL REGISTRO CIFRATO NEL SALONE");
}
