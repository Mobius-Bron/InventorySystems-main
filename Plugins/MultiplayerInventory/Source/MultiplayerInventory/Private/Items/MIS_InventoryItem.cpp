#include "Items/MIS_InventoryItem.h"

#include "Net/UnrealNetwork.h"
#include "Items/Fragments/MIS_ItemFragment.h"
#include "Items/Fragments/MIS_FragmentTags.h"

void UMIS_InventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemManifest);
	DOREPLIFETIME(ThisClass, TotalStackCount);
}

void UMIS_InventoryItem::SetItemManifest(const FMIS_ItemManifest& Manifest)
{
	ItemManifest = FInstancedStruct::Make(Manifest);
}

bool UMIS_InventoryItem::IsStackable() const
{
	if (!ItemManifest.IsValid()) return false;
	const FMIS_ItemManifest& Manifest = GetItemManifest();
	return Manifest.GetFragmentOfType<FMIS_StackableFragment>() != nullptr;
}

bool UMIS_InventoryItem::IsConsumable() const
{
	if (!ItemManifest.IsValid()) return false;
	const FMIS_ItemManifest& Manifest = GetItemManifest();
	return Manifest.GetFragmentOfType<FMIS_ConsumableFragment>() != nullptr;
}
