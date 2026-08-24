#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GPExtractionZone.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class GORILLAPROTOCOL_API AGPExtractionZone : public AActor
{
    GENERATED_BODY()

public:
    AGPExtractionZone();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void SetExtractionActive(bool bNewActive);

    UFUNCTION(BlueprintPure, Category="Mission")
    float GetExtractionProgress() const { return ExtractionHoldSeconds > 0.0f ? HoldProgress / ExtractionHoldSeconds : 0.0f; }

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> TriggerZone;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MarkerMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> MarkerLight;

    UPROPERTY(EditDefaultsOnly, Category="Mission", meta=(ClampMin="1.0"))
    float ExtractionHoldSeconds = 5.0f;

private:
    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

    TWeakObjectPtr<APawn> ExtractingPawn;
    float HoldProgress = 0.0f;
    bool bActive = false;
};
