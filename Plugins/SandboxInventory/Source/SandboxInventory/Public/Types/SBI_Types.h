// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SBI_Types.generated.h"

class UIC_InventoryItem;

/**
 * 槽位可用性 - 描述某个槽位的容纳能力
 */
USTRUCT()
struct FSBI_SlotAvailability
{
	GENERATED_BODY()

	FSBI_SlotAvailability() {}
	FSBI_SlotAvailability(int32 InSlotIndex, int32 InRoom, bool bInHasItem) :
		SlotIndex(InSlotIndex), AmountToFill(InRoom), bHasItem(bInHasItem) {}

	int32 SlotIndex{INDEX_NONE};    // 槽位索引
	int32 AmountToFill{0};          // 可填充的数量
	bool bHasItem{false};           // 该位置是否已有物品
};

/**
 * 槽位可用性结果 - 空间查询的完整结果
 */
USTRUCT()
struct FSBI_SlotAvailabilityResult
{
	GENERATED_BODY()

	FSBI_SlotAvailabilityResult() {}

	TWeakObjectPtr<UIC_InventoryItem> Item;          // 被查询的物品
	int32 TotalRoomToFill{0};                         // 总共可容纳的数量
	int32 Remainder{0};                               // 无法容纳的剩余数量
	bool bStackable{false};                           // 是否可堆叠
	TArray<FSBI_SlotAvailability> SlotAvailabilities; // 每个可用槽位的详细信息
};