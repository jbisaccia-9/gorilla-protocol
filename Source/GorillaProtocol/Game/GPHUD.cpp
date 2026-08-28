#include "GPHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "../Player/GPBrunoCharacter.h"
#include "GPGameModeBase.h"

void AGPHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas)
    {
        return;
    }

    AGPBrunoCharacter* Bruno = Cast<AGPBrunoCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    AGPGameModeBase* Mode = GetWorld()->GetAuthGameMode<AGPGameModeBase>();
    if (!Bruno || !Mode)
    {
        return;
    }

    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    const FLinearColor Ivory(0.92f, 0.91f, 0.82f, 1.0f);
    const FLinearColor Amber(1.0f, 0.42f, 0.08f, 1.0f);
    const FLinearColor Red(0.95f, 0.05f, 0.025f, 1.0f);
    const FLinearColor Green(0.1f, 0.9f, 0.34f, 1.0f);

    DrawLine(Width * 0.5f - 9.0f, Height * 0.5f, Width * 0.5f - 2.0f, Height * 0.5f, Ivory, 1.5f);
    DrawLine(Width * 0.5f + 2.0f, Height * 0.5f, Width * 0.5f + 9.0f, Height * 0.5f, Ivory, 1.5f);
    DrawLine(Width * 0.5f, Height * 0.5f - 9.0f, Width * 0.5f, Height * 0.5f - 2.0f, Ivory, 1.5f);
    DrawLine(Width * 0.5f, Height * 0.5f + 2.0f, Width * 0.5f, Height * 0.5f + 9.0f, Ivory, 1.5f);

    DrawRect(FLinearColor(0.01f, 0.012f, 0.015f, 0.82f), 28.0f, Height - 98.0f, 280.0f, 60.0f);
    DrawText(TEXT("BRUNO"), Ivory, 42.0f, Height - 91.0f, GEngine->GetSmallFont(), 1.0f, false);
    const float HealthRatio = Bruno->GetHealth() / Bruno->GetMaxHealth();
    DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f), 42.0f, Height - 64.0f, 185.0f, 13.0f);
    DrawRect(HealthRatio > 0.3f ? Amber : Red, 42.0f, Height - 64.0f, 185.0f * HealthRatio, 13.0f);
    DrawText(FString::Printf(TEXT("%02d / %02d"), Bruno->GetAmmo(), Bruno->GetMagazineSize()),
        Ivory, 241.0f, Height - 73.0f, GEngine->GetMediumFont(), 1.05f, false);

    DrawRect(FLinearColor(0.01f, 0.012f, 0.015f, 0.78f), 28.0f, 24.0f, 570.0f, 62.0f);
    DrawText(TEXT("OPERAZIONE SCIMMIA DI MARE"), Amber, 43.0f, 32.0f,
        GEngine->GetSmallFont(), 1.0f, false);
    DrawText(Mode->GetObjectiveText(), Mode->IsMissionComplete() ? Green : Ivory,
        43.0f, 55.0f, GEngine->GetMediumFont(), 0.86f, false);
    DrawText(FString::Printf(TEXT("GUARDIE ATTIVE: %d"), Mode->GetGuardsRemaining()),
        Ivory, Width - 210.0f, 35.0f, GEngine->GetSmallFont(), 0.9f, false);

    DrawText(TEXT("WASD MUOVI  |  SHIFT CORRI  |  LMB SPARA  |  RMB PUGNO  |  E INTERAGISCI  |  Q PARLA"),
        FLinearColor(0.72f, 0.72f, 0.67f, 1.0f), 34.0f, Height - 24.0f,
        GEngine->GetSmallFont(), 0.75f, false);

    if (Bruno->IsSubtitleVisible())
    {
        float TextWidth = 0.0f;
        float TextHeight = 0.0f;
        GetTextSize(Bruno->GetSubtitle(), TextWidth, TextHeight, GEngine->GetMediumFont(), 1.0f);
        DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.76f), Width * 0.5f - TextWidth * 0.5f - 18.0f,
            Height - 175.0f, TextWidth + 36.0f, 38.0f);
        DrawText(Bruno->GetSubtitle(), Ivory, Width * 0.5f - TextWidth * 0.5f,
            Height - 168.0f, GEngine->GetMediumFont(), 1.0f, false);
    }

    if (Mode->IsMissionComplete())
    {
        DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f), Width * 0.5f - 330.0f,
            Height * 0.5f - 80.0f, 660.0f, 160.0f);
        DrawText(TEXT("MISSIONE COMPIUTA"), Green, Width * 0.5f - 205.0f,
            Height * 0.5f - 38.0f, GEngine->GetLargeFont(), 1.5f, false);
        DrawText(TEXT("Il gorilla e sparito nella notte."), Ivory, Width * 0.5f - 170.0f,
            Height * 0.5f + 18.0f, GEngine->GetMediumFont(), 1.0f, false);
    }
}
