#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "OldMIS_FastArray.generated.h"

struct FGameplayTag;
class UOldMIS_InventoryComponent;
class UOldMIS_InventoryItem;
class UOldMIS_ItemComponent;

/**
 * 库存条目 - FastArray 中的单个条目
 * 每个条目持有一个 InventoryItem 的引用
 */
USTRUCT(BlueprintType)
struct FOldMIS_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FOldMIS_InventoryEntry() {}

private:
	friend struct FOldMIS_InventoryFastArray;
	friend UOldMIS_InventoryComponent;

	/** 物品引用 */
	UPROPERTY()
	TObjectPtr<UOldMIS_InventoryItem> Item = nullptr;
};

/**
 * 库存快速数组 - 使用 UE5 FastArray 机制进行高效的网络增量复制
 * 服务端修改后自动同步到所有客户端,通过 PreReplicatedAdd/Remove 触发 UI 更新
 */
USTRUCT(BlueprintType)
struct FOldMIS_InventoryFastArray : public FFastArraySerializer
{
	GENERATED_BODY()

	FOldMIS_InventoryFastArray() : OwnerComponent(nullptr) {}
	FOldMIS_InventoryFastArray(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}

	/** 获取所有物品列表 */
	TArray<UOldMIS_InventoryItem*> GetAllItems() const;

	/** 条目被移除前的回调 - 广播 OnItemRemoved */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/** 条目被添加后的回调 - 注册复制子对象 + 广播 OnItemAdded */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/** FastArray 增量序列化 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FOldMIS_InventoryEntry, FOldMIS_InventoryFastArray>(Entries, DeltaParams, *this);
	}

	/** 从 ItemComponent 创建新条目 (拾取新物品) */
	UOldMIS_InventoryItem* AddEntry(UOldMIS_ItemComponent* ItemComponent);

	/** 从已有的 InventoryItem 添加条目 */
	UOldMIS_InventoryItem* AddEntry(UOldMIS_InventoryItem* Item);

	/** 移除指定物品的条目 */
	void RemoveEntry(UOldMIS_InventoryItem* Item);

	/** 按物品类型查找第一个匹配的物品 (用于堆叠合并) */
	UOldMIS_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType);

private:
	friend UOldMIS_InventoryComponent;

	/** 条目列表 - 网络复制 */
	UPROPERTY()
	TArray<FOldMIS_InventoryEntry> Entries;

	/** 所有者组件引用 - 不复制 */
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

/** 启用 NetDeltaSerialize */
template<>
struct TStructOpsTypeTraits<FOldMIS_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FOldMIS_InventoryFastArray>
{
	enum { WithNetDeltaSerializer = true };
};
