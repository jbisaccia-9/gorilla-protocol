#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GPWorldDirector.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UPostProcessComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

UCLASS()
class GORILLAPROTOCOL_API AGPWorldDirector : public AActor
{
    GENERATED_BODY()

public:
    AGPWorldDirector();

protected:
    virtual void BeginPlay() override;

private:
    void BuildGraybox();
    void SpawnMissionActors();
    void AddBox(UInstancedStaticMeshComponent* Component, const FVector& Location, const FVector& Size,
        const FRotator& Rotation = FRotator::ZeroRotator);

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Architecture;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> CoverProps;
    UPROPERTY()
    TObjectPtr<UDirectionalLightComponent> MoonLight;
    UPROPERTY()
    TObjectPtr<USkyLightComponent> SkyLight;
    UPROPERTY()
    TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;
    UPROPERTY()
    TObjectPtr<UExponentialHeightFogComponent> HeightFog;
    UPROPERTY()
    TObjectPtr<UPostProcessComponent> PostProcess;
};
