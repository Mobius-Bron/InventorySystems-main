#include "EquipmentManagement/EquipActor/MIS_EquipActor.h"

AMIS_EquipActor::AMIS_EquipActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
	SetReplicatingMovement(false);
}
