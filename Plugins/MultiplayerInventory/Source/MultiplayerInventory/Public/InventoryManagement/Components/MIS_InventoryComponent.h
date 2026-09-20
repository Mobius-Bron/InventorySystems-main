#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CollisionProfile.h"
#include "InventoryManagement/FastArray/MIS_FastArray.h"

#include "MIS_InventoryComponent.generated.h"

class UMIS_InventoryItem;
class UMIS_ItemComponent;
class UMIS_InventoryWidget;
class APlayerController;
class AActor;
struct FMIS_ItemManifest;
struct FMIS_SlotAvailabilityResult;

/**
 * 库存核心组件 (ViewModel)
 *
 * 初始化: 外部调用 Init(PC) 传入 PlayerController,不依赖 Owner 类型,
 * 可挂在 Character / PlayerController 等任意 Actor 上。
 *
 * [解耦重构] 本组件不声明任何多播委托, 也不再持有 HUD 指针:
 *   - 状态通知: 通过 GMP 消息 (MIS.Inv.*) 广播, SigSource = 本组件, 键见 MIS_MessageKeys.h
 *   - 意图命令: 在 BeginPlay 监听 UI 发来的 (MIS.Cmd.*) 消息
 *
 * [服务端权威] 物品的落点 (UpperLeftIndex) 已纳入 FastArray 复制, 服务端据此持有一份
 * 独立的位置表, 可校验客户端上报的放置是否合法; UI 上报网格布局后, 专用服务器同样能校验。
 * 仍然保留 SetInventoryWidget 注入: 客户端的空间查询 HasRoomForItem 需要 UI 侧的
 * 槽位占用视图来给出即时反馈 (服务端校验是事后的第二道关)。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class MULTIPLAYERINVENTORY_API UMIS_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIS_InventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 初始化 — 传入 PlayerController 引用 */
	void Init(APlayerController* InPC);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void TryAddItem(UMIS_ItemComponent* ItemComponent);

	/**
	 * @param TargetGridIndex  客户端 UI 计算出的落点 (物品左上角格子索引)。
	 *                         服务端会用自己维护的位置表复核; INDEX_NONE 表示未指定。
	 */
	UFUNCTION(Server, Reliable)
	void Server_AddNewItem(AActor* ItemActor, int32 StackCount, int32 Remainder, int32 TargetGridIndex);

	UFUNCTION(Server, Reliable)
	void Server_AddStacksToItem(AActor* ItemActor, int32 StackCount, int32 Remainder);

	UFUNCTION(Server, Reliable)
	void Server_DropItem(UMIS_InventoryItem* Item, int32 StackCount);

	UFUNCTION(Server, Reliable)
	void Server_ConsumeItem(UMIS_InventoryItem* Item);

	UFUNCTION(Server, Reliable)
	void Server_EquipSlotClicked(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EquipSlotClicked(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip);

	/**
	 * [服务端权威] 拥有者的 UI 上报网格布局 (列/行)。
	 * 专用服务器没有 UI, 需改为在蓝图里配置 GridColumns/GridRows 才会启用校验。
	 */
	UFUNCTION(Server, Reliable)
	void Server_SetGridLayout(int32 InColumns, int32 InRows);

	/** [服务端权威] 落点被拒绝时回告客户端, 由 UI 提示"背包已满"。 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyPlacementRejected();

	void RequestEquipSlotClicked(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip);
	void RequestDropItem(UMIS_InventoryItem* ItemToDrop, int32 StackCount = 1);
	void RequestConsumeItem(UMIS_InventoryItem* ItemToConsume);
	
	void PrimaryInteract();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void TraceForItem();

	void AddRepSubObj(UObject* SubObj);
	void SpawnDroppedItem(UMIS_InventoryItem* Item, int32 StackCount);

	TArray<UMIS_InventoryItem*> GetAllItems();
	UMIS_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType);

	/** [解耦遗留] 网格空间查询需要 UI 侧的槽位占用视图, 暂保留单向注入。 */
	void SetInventoryWidget(UMIS_InventoryWidget* InInventoryWidget) { InventoryWidget = InInventoryWidget; }
	bool IsInventoryOpen() const;

	// ------------------------------------------------------------------
	// [服务端权威] 网格位置相关
	// ------------------------------------------------------------------

	/**
	 * 校验"把尺寸为 Manifest 的物品放在 UpperLeftIndex"是否合法 (范围 + 冲突)。
	 * 未配置布局 (列/行 <= 0) 时返回 true —— 避免在专用服务器尚未配置网格时误拒正常操作。
	 */
	bool ValidatePlacement(const FMIS_ItemManifest& Manifest, int32 UpperLeftIndex, const UMIS_InventoryItem* IgnoreItem = nullptr) const;

	/** 构建网格占用表: 下标为格子索引, 值为该格上的物品 (可为空)。 */
	void BuildOccupancyMap(TArray<UMIS_InventoryItem*>& OutSlotItems) const;

	/** 记录/更新某物品的落点并触发增量复制 (仅服务端生效)。 */
	void SetItemGridIndex(UMIS_InventoryItem* Item, int32 UpperLeftIndex);

	/** 取某物品当前记录的落点 (INDEX_NONE 表示未记录)。 */
	int32 GetItemGridIndex(const UMIS_InventoryItem* Item) const;

	int32 GetGridColumns() const { return GridColumns; }
	int32 GetGridRows() const { return GridRows; }

private:
	TWeakObjectPtr<APlayerController> OwningController;

	UPROPERTY(Replicated)
	FMIS_InventoryFastArray InventoryList;

	UPROPERTY()
	TObjectPtr<UMIS_InventoryWidget> InventoryWidget;

	/** [服务端权威] 网格列数。拥有者 UI 会通过 Server_SetGridLayout 上报; 专用服务器请在此配置。 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Grid")
	int32 GridColumns{0};

	/** [服务端权威] 网格行数。 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Grid")
	int32 GridRows{0};

	/** 从 Manifest 的 GridFragment 取物品占用的格子尺寸, 缺省 1x1。 */
	FIntPoint GetManifestGridSize(const FMIS_ItemManifest& Manifest) const;

	/** 判断以 Index 为左上角、尺寸为 Dim 的物品是否完整落在网格内 (含跨行检查)。 */
	bool IsIndexInBounds(int32 Index, const FIntPoint& Dim) const;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Trace")
	float TraceLength{500.f};

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Trace")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel{ECC_GameTraceChannel1};

	UPROPERTY(EditAnywhere, Category = "Inventory|Drop")
	float DropSpawnAngleMin = -85.f;

	UPROPERTY(EditAnywhere, Category = "Inventory|Drop")
	float DropSpawnAngleMax = 85.f;

	UPROPERTY(EditAnywhere, Category = "Inventory|Drop")
	float DropSpawnDistanceMin = 10.f;

	UPROPERTY(EditAnywhere, Category = "Inventory|Drop")
	float DropSpawnDistanceMax = 50.f;

	UPROPERTY(EditAnywhere, Category = "Inventory|Drop")
	float RelativeSpawnElevation = 70.f;

	bool bInventoryOpen{false};

	TWeakObjectPtr<AActor> ThisActor;
	TWeakObjectPtr<AActor> LastActor;
};
