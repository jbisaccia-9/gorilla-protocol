#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GPGameModeBase.generated.h"

class UGPVerticalSliceDefinition;

UCLASS(Abstract, Blueprintable)
class GORILLAPROTOCOL_API AGPGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGPGameModeBase();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gorilla Protocol|Experience")
    TSoftObjectPtr<UGPVerticalSliceDefinition> VerticalSliceDefinition;

private:
    UPROPERTY(Transient)
    TObjectPtr<UGPVerticalSliceDefinition> LoadedVerticalSlice;
};
