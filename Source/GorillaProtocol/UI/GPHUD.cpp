#include "UI/GPHUD.h"

#include "Characters/GPAgentCharacter.h"
#include "Components/GPHealthComponent.h"
#include "Components/GPWeaponComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Game/GPMissionSubsystem.h"
#include "GameFramework/PlayerController.h"

void AGPHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner || !GEngine) return;

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    const FLinearColor Accent(0.20f, 0.95f, 0.68f, 1.0f);
    DrawLine(CenterX - 10.0f, CenterY, CenterX - 3.0f, CenterY, Accent, 1.5f);
    DrawLine(CenterX + 3.0f, CenterY, CenterX + 10.0f, CenterY, Accent, 1.5f);
    DrawLine(CenterX, CenterY - 10.0f, CenterX, CenterY - 3.0f, Accent, 1.5f);
    DrawLine(CenterX, CenterY + 3.0f, CenterX, CenterY + 10.0f, Accent, 1.5f);

    const AGPAgentCharacter* Agent = Cast<AGPAgentCharacter>(PlayerOwner->GetPawn());
    if (!Agent) return;
    const UGPHealthComponent* Health = Agent->GetHealthComponent();
    const UGPWeaponComponent* Weapon = Agent->GetWeaponComponent();
    const float HealthValue = Health ? Health->GetHealth() : 0.0f;
    const int32 Magazine = Weapon ? Weapon->GetMagazineAmmo() : 0;
    const int32 Reserve = Weapon ? Weapon->GetReserveAmmo() : 0;

    DrawText(FString::Printf(TEXT("SALUTE  %03.0f"), HealthValue), FLinearColor::White,
        42.0f, Canvas->ClipY - 74.0f, GEngine->GetMediumFont(), 1.0f, false);
    DrawText(FString::Printf(TEXT("MUNIZIONI  %02d / %03d"), Magazine, Reserve), FLinearColor::White,
        Canvas->ClipX - 270.0f, Canvas->ClipY - 74.0f, GEngine->GetMediumFont(), 1.0f, false);

    FString Objective = TEXT("INFILTRAZIONE");
    int32 GuardsRemaining = 0;
    if (const UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        GuardsRemaining = Mission->GetGuardsRemaining();
        switch (Mission->GetPhase())
        {
            case EGPMissionPhase::RecoverCipher: Objective = TEXT("RECUPERA IL CIFRARIO"); break;
            case EGPMissionPhase::Extraction: Objective = TEXT("RAGGIUNGI L'ESTRAZIONE"); break;
            case EGPMissionPhase::Complete: Objective = TEXT("MISSIONE COMPIUTA"); break;
            case EGPMissionPhase::Failed: Objective = TEXT("MISSIONE FALLITA"); break;
            default: break;
        }
    }
    DrawText(FString::Printf(TEXT("GORILLA PROTOCOL  |  %s  |  OSTILI %02d"), *Objective, GuardsRemaining),
        Accent, 42.0f, 34.0f, GEngine->GetSmallFont(), 1.15f, false);
}
