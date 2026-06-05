#include "MIS_GameMode.h"

#include "MIS_PlayerCharacter.h"
#include "MIS_PlayerController.h"

AMIS_GameMode::AMIS_GameMode()
{
	DefaultPawnClass = AMIS_PlayerCharacter::StaticClass();
	PlayerControllerClass = AMIS_PlayerController::StaticClass();
}
