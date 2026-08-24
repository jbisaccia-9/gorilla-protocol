#include "Interaction/GPObjectiveActor.h"

#include "Characters/GPAgentCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/GPMissionSubsystem.h"
#include "UObject/ConstructorHelpers.h"

AGPObjectiveActor::AGPObjectiveActor()
{
    PrimaryActorTick.bCanEverTick = false;
    ObjectiveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectiveMesh"));
    SetRootComponent(ObjectiveMesh);
    ObjectiveMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        ObjectiveMesh->SetStaticMesh(CubeMesh.Object);
        ObjectiveMesh->SetRelativeScale3D(FVector(0.36f, 0.26f, 0.10f));
    }

    ObjectiveLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ObjectiveLight"));
    ObjectiveLight->SetupAttachment(ObjectiveMesh);
    ObjectiveLight->SetLightColor(FLinearColor(0.1f, 0.8f, 0.65f));
    ObjectiveLight->SetIntensity(1800.0f);
    ObjectiveLight->SetAttenuationRadius(450.0f);
}

bool AGPObjectiveActor::Interact_Implementation(APawn* InteractingPawn)
{
    if (bCollected || !InteractingPawn) return false;
    bCollected = true;
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->RecoverCipher();
    }
    if (AGPAgentCharacter* Agent = Cast<AGPAgentCharacter>(InteractingPawn))
    {
        Agent->SayItalianLine(NSLOCTEXT("GorillaProtocol", "CipherRecovered", "Cifrario recuperato. Ora sparisco nella nebbia."));
    }
    ObjectiveMesh->SetVisibility(false, true);
    ObjectiveMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    return true;
}
