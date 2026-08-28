#include "GPMissionZone.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "../Player/GPBrunoCharacter.h"
#include "GPGameModeBase.h"

AGPMissionZone::AGPMissionZone()
{
    PrimaryActorTick.bCanEverTick = false;
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(125.0f, 125.0f, 115.0f));
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGPMissionZone::OnOverlap);

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Marker->SetupAttachment(Trigger);
    Marker->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
    Glow->SetupAttachment(Trigger);
    Glow->SetIntensity(6500.0f);
    Glow->SetAttenuationRadius(520.0f);
}

void AGPMissionZone::Configure(bool bInExtraction)
{
    bExtraction = bInExtraction;
    if (UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr,
        bExtraction ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Marker->SetStaticMesh(Shape);
    }
    Glow->SetLightColor(bExtraction ? FLinearColor(0.05f, 1.0f, 0.22f) : FLinearColor(1.0f, 0.04f, 0.02f));
    Marker->SetRelativeScale3D(bExtraction ? FVector(2.1f, 2.1f, 0.06f) : FVector(0.42f));
}

void AGPMissionZone::SetActive(bool bInActive)
{
    bActive = bInActive;
    SetActorHiddenInGame(!bActive);
    SetActorEnableCollision(bActive);
    Glow->SetIntensity(bActive ? 6500.0f : 0.0f);
}

void AGPMissionZone::NotifyActorOnClicked(FKey ButtonPressed)
{
    Super::NotifyActorOnClicked(ButtonPressed);
    if (!bActive || bExtraction)
    {
        return;
    }
    if (AGPGameModeBase* Mode = GetWorld()->GetAuthGameMode<AGPGameModeBase>())
    {
        Mode->CollectLedger();
        SetActive(false);
    }
}

void AGPMissionZone::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!bActive || !bExtraction || !Cast<AGPBrunoCharacter>(OtherActor))
    {
        return;
    }
    if (AGPGameModeBase* Mode = GetWorld()->GetAuthGameMode<AGPGameModeBase>())
    {
        Mode->TryExtract();
    }
}
