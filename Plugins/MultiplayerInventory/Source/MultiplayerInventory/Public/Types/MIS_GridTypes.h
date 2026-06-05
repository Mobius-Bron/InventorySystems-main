#pragma once

#include "MIS_GridTypes.generated.h"

class UMIS_InventoryItem;

/**
 * 瓦片象限 - 鼠标在单个格子的哪个四分之一区域内
 * 用于大物品拖放时确定锚点位置
 */
UENUM(BlueprintType)
enum class EMIS_TileQuadrant : uint8
{
	TopLeft,        // 左上象限
	TopRight,       // 右上象限
	BottomLeft,     // 左下象限
	BottomRight,    // 右下象限
	None            // 无/未确定
};

/**
 * 瓦片参数 - 描述当前鼠标悬停的网格位置信息
 */
USTRUCT(BlueprintType)
struct FMIS_TileParameters
{
	GENERATED_BODY()

	/** 瓦片在网格中的二维坐标 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FIntPoint TileCoordinats{};

	/** 瓦片的一维索引 (Row * Columns + Col) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	int32 TileIndex{INDEX_NONE};

	/** 鼠标在瓦片内的象限 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	EMIS_TileQuadrant TileQuadrant{EMIS_TileQuadrant::None};
};

inline bool operator==(const FMIS_TileParameters& A, const FMIS_TileParameters& B)
{
	return A.TileCoordinats == B.TileCoordinats && A.TileIndex == B.TileIndex && A.TileQuadrant == B.TileQuadrant;
}

/**
 * 槽位可用性 - 描述某个位置的容纳能力
 */
USTRUCT()
struct FMIS_SlotAvailability
{
	GENERATED_BODY()

	FMIS_SlotAvailability() {}
	FMIS_SlotAvailability(int32 ItemIndex, int32 Room, bool bHasItem) : Index(ItemIndex), AmountToFill(Room), bItemAtIndex(bHasItem) {}

	int32 Index{INDEX_NONE};        // 槽位索引
	int32 AmountToFill{0};         // 可填充的数量
	bool bItemAtIndex{false};      // 该位置是否已有物品
};

/**
 * 槽位可用性结果 - 空间查询的完整结果
 */
USTRUCT()
struct FMIS_SlotAvailabilityResult
{
	GENERATED_BODY()

	FMIS_SlotAvailabilityResult() {}

	TWeakObjectPtr<UMIS_InventoryItem> Item;   // 被查询的物品
	int32 TotalRoomToFill{0};                   // 总共可容纳的数量
	int32 Remainder{0};                          // 无法容纳的剩余数量
	bool bStackable{false};                      // 是否可堆叠
	TArray<FMIS_SlotAvailability> SlotAvailabilities; // 每个可用槽位的详细信息
};

/**
 * 空间查询结果 - 检查某个位置是否可放置物品
 */
USTRUCT()
struct FMIS_SpaceQueryResult
{
	GENERATED_BODY()

	bool bHasSpace{false};  // 是否有足够空间放置

	TWeakObjectPtr<UMIS_InventoryItem> ValidItem = nullptr; // 可交换的单个物品(如果有)

	int32 UpperLeftIndex{INDEX_NONE}; // 该物品的左上角索引
};
