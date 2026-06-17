// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IC_GridTypes.generated.h"

class UIC_InventoryItem;

/**
 * 瓦片象限 - 鼠标在单个格子的哪个四分之一区域内
 * 用于大物品拖放时确定锚点位置
 */
UENUM(BlueprintType)
enum class EIC_TileQuadrant : uint8
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	None
};

/**
 * 瓦片参数 - 描述当前鼠标悬停的网格位置信息
 */
USTRUCT(BlueprintType)
struct FIC_TileParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemCore")
	FIntPoint TileCoordinats{};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemCore")
	int32 TileIndex{INDEX_NONE};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemCore")
	EIC_TileQuadrant TileQuadrant{EIC_TileQuadrant::None};
};

inline bool operator==(const FIC_TileParameters& A, const FIC_TileParameters& B)
{
	return A.TileCoordinats == B.TileCoordinats && A.TileIndex == B.TileIndex && A.TileQuadrant == B.TileQuadrant;
}

/**
 * 槽位可用性 - 描述某个位置的容纳能力
 */
USTRUCT()
struct FIC_SlotAvailability
{
	GENERATED_BODY()

	FIC_SlotAvailability() {}
	FIC_SlotAvailability(int32 ItemIndex, int32 Room, bool bHasItem) : Index(ItemIndex), AmountToFill(Room), bItemAtIndex(bHasItem) {}

	int32 Index{INDEX_NONE};
	int32 AmountToFill{0};
	bool bItemAtIndex{false};
};

/**
 * 槽位可用性结果 - 空间查询的完整结果
 */
USTRUCT()
struct FIC_SlotAvailabilityResult
{
	GENERATED_BODY()

	FIC_SlotAvailabilityResult() {}

	TWeakObjectPtr<UIC_InventoryItem> Item;
	int32 TotalRoomToFill{0};
	int32 Remainder{0};
	bool bStackable{false};
	TArray<FIC_SlotAvailability> SlotAvailabilities;
};

/**
 * 空间查询结果 - 检查某个位置是否可放置物品
 */
USTRUCT()
struct FIC_SpaceQueryResult
{
	GENERATED_BODY()

	bool bHasSpace{false};
	TWeakObjectPtr<UIC_InventoryItem> ValidItem = nullptr;
	int32 UpperLeftIndex{INDEX_NONE};
};