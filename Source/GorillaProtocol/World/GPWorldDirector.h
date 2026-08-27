#pragma once

#include "CoreMinimal.h"
#include "Game/GPMissionSubsystem.h"
#include "GameFramework/Actor.h"
#include "GPWorldDirector.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
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
    virtual void Tick(float DeltaSeconds) override;

    static FLinearColor GetCovertAccentColor();
    static FLinearColor GetAlarmAccentColor();
    static FLinearColor GetEscapeAccentColor();
    static FLinearColor ResolveSignalColor(EGPMissionAlertState AlertState);
    static float CalculateAlarmPulse(float TimeSeconds);

protected:
    virtual void BeginPlay() override;

private:
    void BuildCoastalFacility();
    void BuildPalm(const FVector& Location, float Scale, float Yaw);
    void BuildBananaMark(const FVector& Location, const FRotator& Rotation, float Scale);
    void ApplyRuntimeMaterials();
    void SpawnMissionActors();
    void AddBox(UInstancedStaticMeshComponent* Component, const FVector& Location, const FVector& Size,
        const FRotator& Rotation = FRotator::ZeroRotator);
    void AddPrimitive(UInstancedStaticMeshComponent* Component, const FVector& Location, const FVector& Scale,
        const FRotator& Rotation = FRotator::ZeroRotator);

    UFUNCTION()
    void HandleMissionPresentation(EGPMissionPhase NewPhase, EGPMissionAlertState NewAlertState);

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Ground;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Stucco;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Structure;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> BananaAccent;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> SignalProps;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> CoverProps;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Foliage;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> RoundProps;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> GlobeProps;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> ScenicWater;
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

    UPROPERTY()
    TObjectPtr<UPointLightComponent> CourtyardLight;
    UPROPERTY()
    TObjectPtr<UPointLightComponent> DockLight;
    UPROPERTY()
    TObjectPtr<UPointLightComponent> ObjectiveLight;
    UPROPERTY()
    TObjectPtr<UPointLightComponent> AlarmLightWest;
    UPROPERTY()
    TObjectPtr<UPointLightComponent> AlarmLightCenter;
    UPROPERTY()
    TObjectPtr<UPointLightComponent> AlarmLightEast;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> PrimitiveMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GroundMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> StuccoMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> StructureMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BananaMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> SignalMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CoverMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FoliageMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> WaterMaterial;

    EGPMissionPhase CurrentPhase = EGPMissionPhase::Infiltration;
    EGPMissionAlertState CurrentAlertState = EGPMissionAlertState::Covert;
    float AlarmClock = 0.0f;
};
