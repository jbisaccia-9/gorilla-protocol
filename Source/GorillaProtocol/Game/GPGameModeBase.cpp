#include "GPGameModeBase.h"

#include "../Core/GPVerticalSliceDefinition.h"
#include "../GorillaProtocol.h"

AGPGameModeBase::AGPGameModeBase()
{
    DefaultPawnClass = nullptr;
    HUDClass = nullptr;
}

void AGPGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    LoadedVerticalSlice = VerticalSliceDefinition.LoadSynchronous();
    if (!LoadedVerticalSlice)
    {
        ErrorMessage = TEXT("No authored vertical-slice definition is assigned to the game mode.");
        UE_LOG(LogGorillaProtocol, Error, TEXT("%s Refusing to create fallback gameplay."), *ErrorMessage);
        return;
    }

    FString ContentFailure;
    if (!LoadedVerticalSlice->ValidateRequiredContent(ContentFailure))
    {
        ErrorMessage = ContentFailure;
        UE_LOG(LogGorillaProtocol, Error, TEXT("%s Refusing to create fallback gameplay."), *ErrorMessage);
        return;
    }

    DefaultPawnClass = LoadedVerticalSlice->PlayerPawnClass.LoadSynchronous();
    if (!DefaultPawnClass)
    {
        ErrorMessage = TEXT("The authored player pawn could not be loaded.");
        UE_LOG(LogGorillaProtocol, Error, TEXT("%s"), *ErrorMessage);
    }
}
