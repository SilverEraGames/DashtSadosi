#include "DashtSadosiGameMode.h"
#include "HumanPlayerCharacter.h"

ADashtSadosiGameMode::ADashtSadosiGameMode()
{
    DefaultPawnClass = AHumanPlayerCharacter::StaticClass();
}
