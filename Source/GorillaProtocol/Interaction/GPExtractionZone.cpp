#include "Interaction/GPExtractionZone.h"

#include "Characters/GPAgentCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/GPMissionSubsystem.h"
#include "UObject/ConstructorHelpers.h"

AGPExtractionZone::AGPExtractionZone()
{
    PrimaryActorTick.bCanEverTick = true;
    TriggerZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerZone"));
    SetRootComponent(TriggerZone);
    TriggerZone->SetBoxExtent(FVector(220.0f, 220.0f, 140.0f));
    TriggerZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    MarkerMesh->SetupAttachment(TriggerZone);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        MarkerMesh->SetStaticMesh(CylinderMesh.Object);
        MarkerMesh->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.03f));
    }

    MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
    MarkerLight->SetupAttachment(TriggerZone);
    MarkerLight->SetLightColor(FLinearColor(0.1f, 0.9f, 0.55f));
    MarkerLight->SetIntensity(3500.0f);
    MarkerLight->SetAttenuationRadius(900.0f);
}

void AGPExtractionZone::BeginPlay()
{
    Super::BeginPlay();
    TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &AGPExtractionZone::HandleBeginOverlap);
    TriggerZone->OnComponentEndOverlap.AddDynamic(this, &AGPExtractionZone::HandleEndOverlap);
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->RegisterExtractionZone(this);
    }
    SetExtractionActive(false);
}

void AGPExtractionZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bActive || !ExtractingPawn.IsValid()) return;
    HoldProgress += DeltaSeconds;
    if (HoldProgress >= ExtractionHoldSeconds)
    {
        if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
        {
            Mission->CompleteExtraction();
        }
        if (AGPAgentCharacter* Agent = Cast<AGPAgentCharacter>(ExtractingPawn.Get()))
        {
            Agent->SayItalianLine(NSLOCTEXT("GorillaProtocol", "MissionComplete", "Missione compiuta. Eleganza, forza e silenzio."));
        }
        ExtractingPawn.Reset();
    }
}

void AGPExtractionZone::SetExtractionActive(bool bNewActive)
{
    bActive = bNewActive;
    MarkerMesh->SetVisibility(bActive);
    MarkerLight->SetVisibility(bActive);
    HoldProgress = 0.0f;
}

void AGPExtractionZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bActive && OtherActor && OtherActor->IsA<AGPAgentCharacter>())
    {
        ExtractingPawn = Cast<APawn>(OtherActor);
        HoldProgress = 0.0f;
    }
}

void AGPExtractionZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
    if (OtherActor == ExtractingPawn.Get())
    {
        ExtractingPawn.Reset();
        HoldProgress = 0.0f;
    }
}
