#include "InventoryManagement/FastArray/OldMIS_FastArray.h"

#include "InventoryManagement/Components/OldMIS_InventoryComponent.h"
#include "Items/OldMIS_InventoryItem.h"
#include "Items/Components/OldMIS_ItemComponent.h"

TArray<UOldMIS_InventoryItem*> FOldMIS_InventoryFastArray::GetAllItems() const
{
	TArray<UOldMIS_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const auto& Entry : Entries)
	{
		if (!IsValid(Entry.Item)) continue;
		Results.Add(Entry.Item);
	}
	return Results;
}

void FOldMIS_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	UOldMIS_InventoryComponent* Component = Cast<UOldMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return;

	for (int32 Index : RemovedIndices)
	{
		Component->OnItemRemoved.Broadcast(Entries[Index].Item);
	}
}

void FOldMIS_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UOldMIS_InventoryComponent* Component = Cast<UOldMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return;

	for (int32 Index : AddedIndices)
	{
		Component->OnItemAdded.Broadcast(Entries[Index].Item);
	}
}

UOldMIS_InventoryItem* FOldMIS_InventoryFastArray::AddEntry(UOldMIS_ItemComponent* ItemComponent)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	UOldMIS_InventoryComponent* Component = Cast<UOldMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return nullptr;

	FOldMIS_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	FOldMIS_ItemManifest ManifestCopy = ItemComponent->GetItemManifest();
	NewEntry.Item = ManifestCopy.Manifest(OwningActor);

	Component->AddRepSubObj(NewEntry.Item);
	MarkItemDirty(NewEntry);

	return NewEntry.Item;
}

UOldMIS_InventoryItem* FOldMIS_InventoryFastArray::AddEntry(UOldMIS_InventoryItem* Item)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FOldMIS_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;

	MarkItemDirty(NewEntry);
	return Item;
}

void FOldMIS_InventoryFastArray::RemoveEntry(UOldMIS_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FOldMIS_InventoryEntry& Entry = *EntryIt;
		if (Entry.Item == Item)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

UOldMIS_InventoryItem* FOldMIS_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType)
{
	auto* FoundItem = Entries.FindByPredicate([ItemType = ItemType](const FOldMIS_InventoryEntry& Entry)
	{
		return IsValid(Entry.Item) && Entry.Item->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
	});
	return FoundItem ? FoundItem->Item : nullptr;
}
