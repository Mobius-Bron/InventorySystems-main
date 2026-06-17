// Copyright AmberAeolian. All Rights Reserved.

#include "FastArray/IC_FastArray.h"

#include "DS_DebugFunctionLibrary.h"
#include "Items/IC_InventoryItem.h"
#include "Components/IC_ItemComponent.h"

TArray<UIC_InventoryItem*> FIC_InventoryFastArray::GetAllItems() const
{
	TArray<UIC_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const auto& Entry : Entries)
	{
		if (!IsValid(Entry.Item)) continue;
		Results.Add(Entry.Item);
	}
	return Results;
}

void FIC_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		UIC_InventoryItem* Item = Entries[Index].Item;
		DS_LOG(Network, "FastArray::PreReplicatedRemove | %s", IsValid(Item) ? *Item->GetName() : TEXT("null"));
		OnItemRemoved.Broadcast(Item);
	}
}

void FIC_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (!IsValid(OwnerComponent)) return;
	AActor* OwningActor = OwnerComponent->GetOwner();
	if (!IsValid(OwningActor)) return;

	for (int32 Index : AddedIndices)
	{
		UIC_InventoryItem* Item = Entries[Index].Item;
		DS_LOG(Network, "FastArray::PostReplicatedAdd | %s", IsValid(Item) ? *Item->GetName() : TEXT("null"));
		OnItemAdded.Broadcast(Item);
	}
}

UIC_InventoryItem* FIC_InventoryFastArray::AddEntry(UIC_ItemComponent* ItemComponent)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FIC_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	FIC_ItemManifest ManifestCopy = ItemComponent->GetItemManifest();
	NewEntry.Item = ManifestCopy.Manifest(OwningActor);

	MarkItemDirty(NewEntry);

	DS_PRINT(Inventory, 2.f, DSColors::Green, "FastArray::AddEntry | %s", *NewEntry.Item->GetItemType().ToString());

	return NewEntry.Item;
}

UIC_InventoryItem* FIC_InventoryFastArray::AddEntry(UIC_InventoryItem* Item)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FIC_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;

	MarkItemDirty(NewEntry);
	return Item;
}

void FIC_InventoryFastArray::RemoveEntry(UIC_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FIC_InventoryEntry& Entry = *EntryIt;
		if (Entry.Item == Item)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

UIC_InventoryItem* FIC_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType)
{
	auto* FoundItem = Entries.FindByPredicate([ItemType = ItemType](const FIC_InventoryEntry& Entry)
	{
		return IsValid(Entry.Item) && Entry.Item->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
	});
	return FoundItem ? FoundItem->Item : nullptr;
}