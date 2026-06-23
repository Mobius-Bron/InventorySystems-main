// Copyright AmberAeolian. All Rights Reserved.

#include "SBI_GameMode.h"

#include "SBI_PlayerCharacter.h"
#include "SBI_PlayerController.h"

ASBI_GameMode::ASBI_GameMode()
{
	DefaultPawnClass = ASBI_PlayerCharacter::StaticClass();
	PlayerControllerClass = ASBI_PlayerController::StaticClass();
}