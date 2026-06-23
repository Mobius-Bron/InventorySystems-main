// Copyright AmberAeolian. All Rights Reserved.

#include "EquipmentManagement/EquipActor/SBI_EquipActor.h"

ASBI_EquipActor::ASBI_EquipActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
	SetReplicatingMovement(false);
}