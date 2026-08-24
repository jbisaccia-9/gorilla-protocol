#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GPHUD.generated.h"

UCLASS()
class GORILLAPROTOCOL_API AGPHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
