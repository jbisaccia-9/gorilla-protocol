#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GPInteractable.generated.h"

UINTERFACE(BlueprintType)
class GORILLAPROTOCOL_API UGPInteractable : public UInterface
{
    GENERATED_BODY()
};

class GORILLAPROTOCOL_API IGPInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
    bool Interact(APawn* InteractingPawn);
};
