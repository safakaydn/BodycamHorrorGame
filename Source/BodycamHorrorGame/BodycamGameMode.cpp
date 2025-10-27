// Copyright belongs to Real Interactive Studio, 2025


#include "BodycamGameMode.h"
#include "BodycamCharacter.h"

ABodycamGameMode::ABodycamGameMode(const FObjectInitializer &ObjectInitializer)
{
    // set default pawn class to our character
    DefaultPawnClass = ABodycamCharacter::StaticClass();
}
