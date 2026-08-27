#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GPVerticalSliceDefinition.generated.h"

class AActor;
class APawn;
class UInputMappingContext;
class USkeletalMesh;
class UStaticMesh;
class UUserWidget;
class UWorld;

UCLASS(BlueprintType, Const)
class GORILLAPROTOCOL_API UGPVerticalSliceDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    bool ValidateRequiredContent(FString& OutFailure) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
    TSoftObjectPtr<UWorld> MissionMap;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
    TSoftClassPtr<APawn> PlayerPawnClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
    TSoftClassPtr<UUserWidget> RootHudWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
    TSoftObjectPtr<UInputMappingContext> PlayerInputMapping;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Presentation")
    TSoftObjectPtr<USkeletalMesh> FirstPersonGorillaMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Presentation")
    TSoftObjectPtr<USkeletalMesh> FullBodyGorillaMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Presentation")
    TSoftObjectPtr<UStaticMesh> SuppressedPistolMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemies")
    TSoftObjectPtr<USkeletalMesh> GuardMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemies")
    TSoftClassPtr<AActor> WatchmanClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemies")
    TSoftClassPtr<AActor> RadioOperatorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemies")
    TSoftClassPtr<AActor> EnforcerClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Validation")
    TArray<TSoftObjectPtr<UObject>> RequiredPresentationAssets;
};
