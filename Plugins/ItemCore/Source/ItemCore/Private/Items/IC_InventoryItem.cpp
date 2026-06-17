// Copyright AmberAeolian. All Rights Reserved.

#include "Items/IC_InventoryItem.h"

#include "Net/UnrealNetwork.h"
#include "Items/IC_ItemFragment.h"
#include "Items/IC_ItemTags.h"

void UIC_InventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemManifest);
	DOREPLIFETIME(ThisClass, TotalStackCount);
}

void UIC_InventoryItem::SetItemManifest(const FIC_ItemManifest& Manifest)
{
	ItemManifest = FInstancedStruct::Make(Manifest);
}

bool UIC_InventoryItem::IsStackable() const
{
	if (!ItemManifest.IsValid()) return false;
	const FIC_ItemManifest& Manifest = GetItemManifest();
	return Manifest.GetFragmentOfType<FIC_StackableFragment>() != nullptr;
}

FGameplayTag UIC_InventoryItem::GetItemType() const
{
	if (!ItemManifest.IsValid()) return FGameplayTag::EmptyTag;
	return GetItemManifest().GetItemType();
}