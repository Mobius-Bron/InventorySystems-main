#include "Items/OldMIS_InventoryItem.h"

#include "Net/UnrealNetwork.h"
#include "Items/Fragments/OldMIS_ItemFragment.h"
#include "Items/Fragments/OldMIS_FragmentTags.h"

void UOldMIS_InventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemManifest);
	DOREPLIFETIME(ThisClass, TotalStackCount);
}

void UOldMIS_InventoryItem::SetItemManifest(const FOldMIS_ItemManifest& Manifest)
{
	ItemManifest = FInstancedStruct::Make(Manifest);
}

bool UOldMIS_InventoryItem::IsStackable() const
{
	if (!ItemManifest.IsValid()) return false;
	const FOldMIS_ItemManifest& Manifest = GetItemManifest();
	return Manifest.GetFragmentOfType<FOldMIS_StackableFragment>() != nullptr;
}

bool UOldMIS_InventoryItem::IsConsumable() const
{
	if (!ItemManifest.IsValid()) return false;
	const FOldMIS_ItemManifest& Manifest = GetItemManifest();
	return Manifest.GetFragmentOfType<FOldMIS_ConsumableFragment>() != nullptr;
}
