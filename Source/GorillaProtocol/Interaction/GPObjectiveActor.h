#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GPInteractable.h"
#include "GPObjectiveActor.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class GORILLAPROTOCOL_API AGPObjectiveActor : public AActor, public IGPInteractable
{
    GENERATED_BODY()

public:
    AGPObjectiveActor();
    virtual bool Interact_Implementation(APawn* InteractingPawn) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> ObjectiveMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UPointLightComponent> ObjectiveLight;

private:
    bool bCollected = false;
};
