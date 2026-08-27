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

    static float CalculateLayoutScale(float ViewportWidth, float ViewportHeight);
    static float CalculateCrosshairGap(bool bIsAiming, float MovementAlpha, float LayoutScale);

private:
    void DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Accent,
        float Scale, float Opacity = 0.78f);
    void DrawMeter(float X, float Y, float Width, float Height, float NormalizedValue,
        const FLinearColor& FillColor);
};
