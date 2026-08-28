#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GPMissionZone.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class GORILLAPROTOCOL_API AGPMissionZone : public AActor
{
    GENERATED_BODY()

public:
    AGPMissionZone();

    void Configure(bool bInExtraction);
    void SetActive(bool bInActive);
    virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

private:
    UFUNCTION()
    void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Marker;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> Glow;

    bool bExtraction = false;
    bool bActive = true;
};
