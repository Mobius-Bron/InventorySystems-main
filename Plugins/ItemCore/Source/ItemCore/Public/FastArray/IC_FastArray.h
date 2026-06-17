// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "IC_FastArray.generated.h"

struct FGameplayTag;
class UIC_InventoryItem;
class UIC_ItemComponent;

/**
 * 库存条目 - FastArray 中的单个条目
 */
USTRUCT(BlueprintType)
struct FIC_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FIC_InventoryEntry() {}

private:
	friend struct FIC_InventoryFastArray;

	UPROPERTY()
	TObjectPtr<UIC_InventoryItem> Item = nullptr;
};

/**
 * 库存快速数组 - 使用 UE5 FastArray 机制进行高效的网络增量复制
 *
 * 设计要点:
 * - 通过构造函数或 Init() 绑定 OwnerComponent,不依赖特定组件类型
 * - PreReplicatedRemove/PostReplicatedAdd 通过委托通知外部
 * - 任何实现了委托绑定的 UActorComponent 均可使用
 */
USTRUCT(BlueprintType)
struct FIC_InventoryFastArray : public FFastArraySerializer
{
	GENERATED_BODY()

	FIC_InventoryFastArray() : OwnerComponent(nullptr) {}
	FIC_InventoryFastArray(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}

	/** 通过 Init() 在任何位置完成初始化 */
	void Init(UActorComponent* InOwnerComponent) { OwnerComponent = InOwnerComponent; }

	TArray<UIC_InventoryItem*> GetAllItems() const;

	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FIC_InventoryEntry, FIC_InventoryFastArray>(Entries, DeltaParams, *this);
	}

	UIC_InventoryItem* AddEntry(UIC_ItemComponent* ItemComponent);
	UIC_InventoryItem* AddEntry(UIC_InventoryItem* Item);
	void RemoveEntry(UIC_InventoryItem* Item);
	UIC_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType);

	/** 物品添加/移除委托 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemChanged, UIC_InventoryItem*);
	FOnItemChanged OnItemAdded;
	FOnItemChanged OnItemRemoved;

private:
	UPROPERTY()
	TArray<FIC_InventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FIC_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FIC_InventoryFastArray>
{
	enum { WithNetDeltaSerializer = true };
};