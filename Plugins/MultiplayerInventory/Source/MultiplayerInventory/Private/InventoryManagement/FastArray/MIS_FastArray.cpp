#include "InventoryManagement/FastArray/MIS_FastArray.h"

#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "Items/MIS_InventoryItem.h"
#include "Items/Components/MIS_ItemComponent.h"

TArray<UMIS_InventoryItem*> FMIS_InventoryFastArray::GetAllItems() const
{
	TArray<UMIS_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const auto& Entry : Entries)
	{
		if (!IsValid(Entry.Item)) continue;
		Results.Add(Entry.Item);
	}
	return Results;
}

void FMIS_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	UMIS_InventoryComponent* Component = Cast<UMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return;

	for (int32 Index : RemovedIndices)
	{
		Component->OnItemRemoved.Broadcast(Entries[Index].Item);
	}
}

void FMIS_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UMIS_InventoryComponent* Component = Cast<UMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return;

	for (int32 Index : AddedIndices)
	{
		Component->OnItemAdded.Broadcast(Entries[Index].Item);
	}
}

UMIS_InventoryItem* FMIS_InventoryFastArray::AddEntry(UMIS_ItemComponent* ItemComponent)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	UMIS_InventoryComponent* Component = Cast<UMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return nullptr;

	FMIS_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	FMIS_ItemManifest ManifestCopy = ItemComponent->GetItemManifest();
	NewEntry.Item = ManifestCopy.Manifest(OwningActor);

	Component->AddRepSubObj(NewEntry.Item);
	MarkItemDirty(NewEntry);

	return NewEntry.Item;
}

UMIS_InventoryItem* FMIS_InventoryFastArray::AddEntry(UMIS_InventoryItem* Item)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FMIS_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;

	MarkItemDirty(NewEntry);
	return Item;
}

void FMIS_InventoryFastArray::RemoveEntry(UMIS_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FMIS_InventoryEntry& Entry = *EntryIt;
		if (Entry.Item == Item)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

UMIS_InventoryItem* FMIS_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType)
{
	auto* FoundItem = Entries.FindByPredicate([ItemType = ItemType](const FMIS_InventoryEntry& Entry)
	{
		return IsValid(Entry.Item) && Entry.Item->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
	});
	return FoundItem ? FoundItem->Item : nullptr;
}
