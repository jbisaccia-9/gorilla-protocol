#include "UI/GPHUD.h"

#include "Characters/GPAgentCharacter.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Game/GPMissionSubsystem.h"
#include "GameFramework/PlayerController.h"

namespace
{
const FLinearColor Ink(0.008f, 0.018f, 0.026f, 1.0f);
const FLinearColor Paper(0.88f, 0.94f, 0.91f, 1.0f);
const FLinearColor CovertGreen(0.13f, 0.95f, 0.61f, 1.0f);
const FLinearColor BananaGold(1.0f, 0.58f, 0.025f, 1.0f);
const FLinearColor SuspiciousGold(1.0f, 0.76f, 0.12f, 1.0f);
const FLinearColor AlarmRed(1.0f, 0.10f, 0.055f, 1.0f);
const FLinearColor EscapeCyan(0.05f, 0.86f, 1.0f, 1.0f);

FLinearColor ResolveAlertColor(EGPMissionAlertState AlertState)
{
    switch (AlertState)
    {
        case EGPMissionAlertState::Suspicious: return SuspiciousGold;
        case EGPMissionAlertState::Alarm: return AlarmRed;
        case EGPMissionAlertState::Escape: return EscapeCyan;
        default: return CovertGreen;
    }
}

FString ResolveAlertLabel(EGPMissionAlertState AlertState)
{
    switch (AlertState)
    {
        case EGPMissionAlertState::Suspicious: return TEXT("SOSPETTO");
        case EGPMissionAlertState::Alarm: return TEXT("ALLARME");
        case EGPMissionAlertState::Escape: return TEXT("FUGA");
        default: return TEXT("INCOGNITO");
    }
}
}

void AGPHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner || !GEngine) return;

    const float Scale = CalculateLayoutScale(Canvas->ClipX, Canvas->ClipY);
    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;

    const AGPAgentCharacter* Agent = Cast<AGPAgentCharacter>(PlayerOwner->GetPawn());
    if (!Agent) return;
    const UGPHealthComponent* Health = Agent->GetHealthComponent();
    const UGPWeaponComponent* Weapon = Agent->GetWeaponComponent();
    const float HealthValue = Health ? Health->GetHealth() : 0.0f;
    const float HealthNormalized = Health ? Health->GetHealthNormalized() : 0.0f;
    const int32 Magazine = Weapon ? Weapon->GetMagazineAmmo() : 0;
    const int32 Reserve = Weapon ? Weapon->GetReserveAmmo() : 0;

    FString Objective = TEXT("INFILTRAZIONE");
    int32 GuardsRemaining = 0;
    EGPMissionPhase Phase = EGPMissionPhase::Infiltration;
    EGPMissionAlertState AlertState = EGPMissionAlertState::Covert;
    if (const UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        GuardsRemaining = Mission->GetGuardsRemaining();
        Phase = Mission->GetPhase();
        AlertState = Mission->GetAlertState();
        Objective = Mission->GetObjectiveText().ToString();
    }
    FLinearColor Accent = ResolveAlertColor(AlertState);
    if (AlertState == EGPMissionAlertState::Alarm)
    {
        const float Pulse = 0.72f + FMath::Sin(GetWorld()->GetTimeSeconds() * 7.0f) * 0.28f;
        Accent *= Pulse;
        Accent.A = 1.0f;
    }

    const float Margin = 34.0f * Scale;
    const float TopWidth = FMath::Min(760.0f * Scale, Canvas->ClipX - Margin * 2.0f);
    DrawPanel(Margin, Margin, TopWidth, 92.0f * Scale, Accent, Scale);
    DrawText(TEXT("GORILLA PROTOCOL"), BananaGold, Margin + 22.0f * Scale,
        Margin + 12.0f * Scale, GEngine->GetMediumFont(), 1.05f * Scale, false);
    DrawText(TEXT("AGENTE BRUNO // PRIMATE D'ASSALTO"), Paper * 0.78f, Margin + 22.0f * Scale,
        Margin + 45.0f * Scale, GEngine->GetSmallFont(), 0.82f * Scale, false);

    const float AlertChipWidth = 156.0f * Scale;
    DrawRect(Accent.CopyWithNewOpacity(0.16f), Margin + TopWidth - AlertChipWidth - 16.0f * Scale,
        Margin + 17.0f * Scale, AlertChipWidth, 44.0f * Scale);
    DrawText(ResolveAlertLabel(AlertState), Accent, Margin + TopWidth - AlertChipWidth,
        Margin + 27.0f * Scale, GEngine->GetSmallFont(), 0.92f * Scale, false);

    const float ObjectiveY = Margin + 105.0f * Scale;
    DrawRect(Ink.CopyWithNewOpacity(0.66f), Margin, ObjectiveY, TopWidth, 42.0f * Scale);
    DrawRect(Accent, Margin, ObjectiveY, 5.0f * Scale, 42.0f * Scale);
    DrawText(FString::Printf(TEXT("OBIETTIVO  //  %s"), *Objective), Paper, Margin + 18.0f * Scale,
        ObjectiveY + 10.0f * Scale, GEngine->GetSmallFont(), 0.86f * Scale, false);

    const float CrosshairGap = CalculateCrosshairGap(Agent->IsAiming(),
        Agent->GetMovementPresentationAlpha(), Scale);
    const float CrosshairLength = (Agent->IsAiming() ? 7.0f : 10.0f) * Scale;
    const float CrosshairWidth = Agent->IsAiming() ? 1.2f : 1.8f;
    DrawLine(CenterX - CrosshairGap - CrosshairLength, CenterY, CenterX - CrosshairGap, CenterY,
        Accent, CrosshairWidth * Scale);
    DrawLine(CenterX + CrosshairGap, CenterY, CenterX + CrosshairGap + CrosshairLength, CenterY,
        Accent, CrosshairWidth * Scale);
    DrawLine(CenterX, CenterY - CrosshairGap - CrosshairLength, CenterX, CenterY - CrosshairGap,
        Accent, CrosshairWidth * Scale);
    DrawLine(CenterX, CenterY + CrosshairGap, CenterX, CenterY + CrosshairGap + CrosshairLength,
        Accent, CrosshairWidth * Scale);
    DrawRect(Accent, CenterX - Scale, CenterY - Scale, 2.0f * Scale, 2.0f * Scale);

    if (Agent->HasInteractableFocus())
    {
        const float PromptWidth = 228.0f * Scale;
        const float PromptY = CenterY + 70.0f * Scale;
        DrawRect(Ink.CopyWithNewOpacity(0.88f), CenterX - PromptWidth * 0.5f, PromptY,
            PromptWidth, 38.0f * Scale);
        DrawRect(BananaGold, CenterX - PromptWidth * 0.5f + 8.0f * Scale,
            PromptY + 7.0f * Scale, 24.0f * Scale, 24.0f * Scale);
        DrawText(TEXT("E"), Ink, CenterX - PromptWidth * 0.5f + 15.0f * Scale,
            PromptY + 10.0f * Scale, GEngine->GetSmallFont(), 0.72f * Scale, false);
        DrawText(TEXT("INTERAGISCI"), Paper, CenterX - PromptWidth * 0.5f + 45.0f * Scale,
            PromptY + 9.0f * Scale, GEngine->GetSmallFont(), 0.86f * Scale, false);
    }

    const float BottomY = Canvas->ClipY - Margin - 98.0f * Scale;
    const float StatusWidth = 310.0f * Scale;
    DrawPanel(Margin, BottomY, StatusWidth, 98.0f * Scale,
        HealthNormalized < 0.3f ? AlarmRed : CovertGreen, Scale);
    DrawText(TEXT("SALUTE DEL PRIMATE"), Paper * 0.72f, Margin + 18.0f * Scale,
        BottomY + 12.0f * Scale, GEngine->GetSmallFont(), 0.72f * Scale, false);
    DrawText(FString::Printf(TEXT("%03.0f"), HealthValue), Paper, Margin + 232.0f * Scale,
        BottomY + 7.0f * Scale, GEngine->GetMediumFont(), 1.15f * Scale, false);
    DrawMeter(Margin + 18.0f * Scale, BottomY + 57.0f * Scale, 268.0f * Scale,
        13.0f * Scale, HealthNormalized, HealthNormalized < 0.3f ? AlarmRed : CovertGreen);

    const float AmmoX = Canvas->ClipX - Margin - StatusWidth;
    DrawPanel(AmmoX, BottomY, StatusWidth, 98.0f * Scale, BananaGold, Scale);
    DrawText(TEXT("PISTOLA BANANA // 9MM"), Paper * 0.72f, AmmoX + 18.0f * Scale,
        BottomY + 12.0f * Scale, GEngine->GetSmallFont(), 0.72f * Scale, false);
    DrawText(FString::Printf(TEXT("%02d"), Magazine), Magazine <= 5 ? AlarmRed : Paper,
        AmmoX + 18.0f * Scale, BottomY + 37.0f * Scale, GEngine->GetMediumFont(), 1.45f * Scale, false);
    DrawText(FString::Printf(TEXT("/ %03d"), Reserve), Paper * 0.65f,
        AmmoX + 94.0f * Scale, BottomY + 51.0f * Scale, GEngine->GetSmallFont(), 0.88f * Scale, false);
    DrawText(FString::Printf(TEXT("OSTILI  %02d"), GuardsRemaining), Accent,
        AmmoX + 202.0f * Scale, BottomY + 51.0f * Scale, GEngine->GetSmallFont(), 0.76f * Scale, false);

    const FText ItalianLine = Agent->GetCurrentItalianLine();
    if (!ItalianLine.IsEmpty())
    {
        const FString Line = FString::Printf(TEXT("BRUNO:  %s"), *ItalianLine.ToString());
        float TextWidth = 0.0f;
        float TextHeight = 0.0f;
        Canvas->StrLen(GEngine->GetMediumFont(), Line, TextWidth, TextHeight);
        TextWidth *= 0.95f * Scale;
        const float SubtitleWidth = FMath::Min(TextWidth + 72.0f * Scale, Canvas->ClipX - 80.0f * Scale);
        const float SubtitleY = Canvas->ClipY - 190.0f * Scale;
        DrawRect(Ink.CopyWithNewOpacity(0.90f), CenterX - SubtitleWidth * 0.5f, SubtitleY,
            SubtitleWidth, 47.0f * Scale);
        DrawRect(BananaGold, CenterX - SubtitleWidth * 0.5f, SubtitleY, 6.0f * Scale, 47.0f * Scale);
        DrawText(Line, Paper, CenterX - TextWidth * 0.5f, SubtitleY + 10.0f * Scale,
            GEngine->GetMediumFont(), 0.95f * Scale, false);
    }

    const float DamageAlpha = Agent->GetDamageFeedbackAlpha();
    if (DamageAlpha > 0.01f)
    {
        const FLinearColor DamageColor = AlarmRed.CopyWithNewOpacity(DamageAlpha * 0.28f);
        const float Edge = 18.0f * Scale;
        DrawRect(DamageColor, 0.0f, 0.0f, Canvas->ClipX, Edge);
        DrawRect(DamageColor, 0.0f, Canvas->ClipY - Edge, Canvas->ClipX, Edge);
        DrawRect(DamageColor, 0.0f, 0.0f, Edge, Canvas->ClipY);
        DrawRect(DamageColor, Canvas->ClipX - Edge, 0.0f, Edge, Canvas->ClipY);
    }

    if (Phase == EGPMissionPhase::Complete || Phase == EGPMissionPhase::Failed)
    {
        const FLinearColor ResultColor = Phase == EGPMissionPhase::Complete ? CovertGreen : AlarmRed;
        const float BannerWidth = FMath::Min(720.0f * Scale, Canvas->ClipX - 80.0f * Scale);
        DrawRect(Ink.CopyWithNewOpacity(0.94f), CenterX - BannerWidth * 0.5f,
            CenterY - 70.0f * Scale, BannerWidth, 140.0f * Scale);
        DrawRect(ResultColor, CenterX - BannerWidth * 0.5f,
            CenterY - 70.0f * Scale, BannerWidth, 8.0f * Scale);
        DrawText(Phase == EGPMissionPhase::Complete ? TEXT("MISSIONE COMPIUTA") : TEXT("MISSIONE FALLITA"),
            ResultColor, CenterX - 190.0f * Scale, CenterY - 30.0f * Scale,
            GEngine->GetLargeFont(), 1.15f * Scale, false);
    }
}

float AGPHUD::CalculateLayoutScale(float ViewportWidth, float ViewportHeight)
{
    if (ViewportWidth <= 0.0f || ViewportHeight <= 0.0f) return 1.0f;
    return FMath::Clamp(FMath::Min(ViewportWidth / 1920.0f, ViewportHeight / 1080.0f), 0.70f, 1.35f);
}

float AGPHUD::CalculateCrosshairGap(bool bIsAiming, float MovementAlpha, float LayoutScale)
{
    const float SafeMovementAlpha = FMath::Clamp(MovementAlpha, 0.0f, 1.0f);
    const float SafeScale = FMath::Max(0.0f, LayoutScale);
    return ((bIsAiming ? 3.0f : 6.0f) + SafeMovementAlpha * 8.0f) * SafeScale;
}

void AGPHUD::DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Accent,
    float Scale, float Opacity)
{
    DrawRect(Ink.CopyWithNewOpacity(Opacity), X, Y, Width, Height);
    DrawRect(Accent.CopyWithNewOpacity(0.96f), X, Y, 5.0f * Scale, Height);
    DrawLine(X, Y, X + Width, Y, Accent.CopyWithNewOpacity(0.42f), Scale);
    DrawLine(X, Y + Height, X + Width, Y + Height, Accent.CopyWithNewOpacity(0.22f), Scale);
}

void AGPHUD::DrawMeter(float X, float Y, float Width, float Height, float NormalizedValue,
    const FLinearColor& FillColor)
{
    const float SafeValue = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
    DrawRect(Paper.CopyWithNewOpacity(0.10f), X, Y, Width, Height);
    DrawRect(FillColor, X, Y, Width * SafeValue, Height);
    const float MarkerStep = Width / 10.0f;
    for (int32 Index = 1; Index < 10; ++Index)
    {
        DrawRect(Ink.CopyWithNewOpacity(0.52f), X + MarkerStep * Index, Y, 1.0f, Height);
    }
}
