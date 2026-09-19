#include "Items/Components/OldMIS_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Items/Manifest/OldMIS_ItemManifest.h"

UOldMIS_ItemComponent::UOldMIS_ItemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UOldMIS_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemManifest);
}

void UOldMIS_ItemComponent::InitItemManifest(FOldMIS_ItemManifest CopyOfManifest)
{
	ItemManifest = FInstancedStruct::Make<FOldMIS_ItemManifest>(MoveTemp(CopyOfManifest));
}

const FOldMIS_ItemManifest& UOldMIS_ItemComponent::GetItemManifest() const
{
	return ItemManifest.Get<FOldMIS_ItemManifest>();
}

FOldMIS_ItemManifest& UOldMIS_ItemComponent::GetItemManifestMutable()
{
	return ItemManifest.GetMutable<FOldMIS_ItemManifest>();
}

void UOldMIS_ItemComponent::PickedUp()
{
	OnPickedUp();
	GetOwner()->Destroy();
}
