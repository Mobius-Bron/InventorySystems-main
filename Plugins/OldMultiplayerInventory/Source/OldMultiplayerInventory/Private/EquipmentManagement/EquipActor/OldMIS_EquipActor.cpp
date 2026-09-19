#include "EquipmentManagement/EquipActor/OldMIS_EquipActor.h"

AOldMIS_EquipActor::AOldMIS_EquipActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
	SetReplicatingMovement(false);
}
