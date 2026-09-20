#include "InventoryManagement/FastArray/MIS_FastArray.h"

#include "MIS_MessageKeys.h"

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
		// [解耦重构] 复制回调不再直接调委托, 改为广播消息, 由 UI 自行决定是否响应。
		// ⚠ 同 Added: 必须传裸指针而不是 TObjectPtr, 否则签名校验失败、消息被丢弃。
		MIS::Emit(MSGKEY(MIS_MSG_ITEM_REMOVED), GMP::FSigSource(Component), Entries[Index].Item.Get());
	}
}

void FMIS_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UMIS_InventoryComponent* Component = Cast<UMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return;

	for (int32 Index : AddedIndices)
	{
		// [解耦重构] 复制回调不再直接调委托, 改为广播消息。
		// [服务端权威] 位置随条目一起下发, UI 据此在指定格子渲染, 不再自己找位置。
		// ⚠ 必须传裸指针: GMP 会对消息参数做签名校验, TObjectPtr<T> 与监听端声明的 T*
		//   类型名不一致, 会导致 "SignatureMismatch On Send" 并且消息被直接丢弃。
		MIS::Emit(MSGKEY(MIS_MSG_ITEM_ADDED), GMP::FSigSource(Component),
			Entries[Index].Item.Get(), Entries[Index].UpperLeftIndex);
	}
}

UMIS_InventoryItem* FMIS_InventoryFastArray::AddEntry(UMIS_ItemComponent* ItemComponent, int32 UpperLeftIndex)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	UMIS_InventoryComponent* Component = Cast<UMIS_InventoryComponent>(OwnerComponent);
	if (!IsValid(Component)) return nullptr;

	FMIS_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	FMIS_ItemManifest ManifestCopy = ItemComponent->GetItemManifest();
	NewEntry.Item = ManifestCopy.Manifest(OwningActor);
	NewEntry.UpperLeftIndex = UpperLeftIndex;

	Component->AddRepSubObj(NewEntry.Item);
	MarkItemDirty(NewEntry);

	return NewEntry.Item;
}

UMIS_InventoryItem* FMIS_InventoryFastArray::AddEntry(UMIS_InventoryItem* Item, int32 UpperLeftIndex)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FMIS_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;
	NewEntry.UpperLeftIndex = UpperLeftIndex;

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
