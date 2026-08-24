#include "Game/GPGameMode.h"

#include "Characters/GPAgentCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Game/GPMissionSubsystem.h"
#include "UI/GPHUD.h"
#include "World/GPWorldDirector.h"
#include "Kismet/GameplayStatics.h"

AGPGameMode::AGPGameMode()
{
    DefaultPawnClass = AGPAgentCharacter::StaticClass();
    HUDClass = AGPHUD::StaticClass();
}

void AGPGameMode::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer) return;
    if (FindPlayerStart(NewPlayer))
    {
        Super::RestartPlayer(NewPlayer);
        return;
    }

    const FTransform FallbackSpawn(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 110.0f));
    if (APawn* NewPawn = SpawnDefaultPawnAtTransform(NewPlayer, FallbackSpawn))
    {
        NewPlayer->Possess(NewPawn);
    }
}

void AGPGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (UGPMissionSubsystem* Mission = GetWorld()->GetSubsystem<UGPMissionSubsystem>())
    {
        Mission->BeginMission();
    }
    if (!UGameplayStatics::GetActorOfClass(this, AGPWorldDirector::StaticClass()))
    {
        GetWorld()->SpawnActor<AGPWorldDirector>(FVector::ZeroVector, FRotator::ZeroRotator);
    }
}
