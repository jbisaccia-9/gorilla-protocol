#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GPGameMode.generated.h"

UCLASS()
class GORILLAPROTOCOL_API AGPGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGPGameMode();
    virtual void RestartPlayer(AController* NewPlayer) override;

protected:
    virtual void BeginPlay() override;
};
