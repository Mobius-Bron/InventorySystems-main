// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FastArray/IC_FastArray.h"
#include "Types/SBI_Types.h"

#include "SBI_InventoryComponent.generated.h"

class UIC_InventoryItem;
class UIC_ItemComponent;
class USBI_InventoryWidget;
class APlayerController;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBI_ItemChange, UIC_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSBI_NoRoom);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBI_StackChange, const FSBI_SlotAvailabilityResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBI_EquipStatusChanged, UIC_InventoryItem*, Item);

/**
 * 沙盒库存核心组件 (ViewModel) - 固定格子背包 (1x1 物品)
 *
 * 初始化: 外部调用 Init(PC) 传入 PlayerController,不依赖 Owner 类型,
 * 可挂在 Character / PlayerController 等任意 Actor 上。
 * 后续通过 SetHUDWidget / SetInventoryWidget 绑定 UI。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SANDBOXINVENTORY_API USBI_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USBI_InventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 初始化 — 传入 PlayerController 引用 */
	void Init(APlayerController* InPC);

	/** 拾取物品 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SandboxInventory")
	void TryAddItem(UIC_ItemComponent* ItemComponent);

	/** 服务端: 创建新物品条目 */
	UFUNCTION(Server, Reliable)
	void Server_AddNewItem(AActor* ItemActor, int32 StackCount, int32 Remainder);

	/** 服务端: 堆叠到已有物品 */
	UFUNCTION(Server, Reliable)
	void Server_AddStacksToItem(AActor* ItemActor, int32 StackCount, int32 Remainder);

	/** 服务端: 丢弃物品 */
	UFUNCTION(Server, Reliable)
	void Server_DropItem(UIC_InventoryItem* Item, int32 StackCount);

	/** 服务端: 消耗物品 */
	UFUNCTION(Server, Reliable)
	void Server_ConsumeItem(UIC_InventoryItem* Item);

	/** 请求拾取交互 */
	void PrimaryInteract();

	/** 请求丢弃物品 */
	void RequestDropItem(UIC_InventoryItem* ItemToDrop, int32 StackCount = 1);

	/** 请求消耗物品 */
	void RequestConsumeItem(UIC_InventoryItem* ItemToConsume);

	/** 装备交互 — 客户端请求服务器处理装备/卸下 */
	UFUNCTION(Server, Reliable)
	void Server_EquipSlotClicked(UIC_InventoryItem* ItemToEquip, UIC_InventoryItem* ItemToUnequip);

	/** 装备交互 — 多播通知所有客户端 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EquipSlotClicked(UIC_InventoryItem* ItemToEquip, UIC_InventoryItem* ItemToUnequip);

	/** 请求装备槽位点击 (客户端 —> 服务器) */
	void RequestEquipSlotClicked(UIC_InventoryItem* ItemToEquip, UIC_InventoryItem* ItemToUnequip);

	/** 打开外部容器 (箱子、其他角色、宠物等) */
	UFUNCTION(BlueprintCallable, Category = "SandboxInventory|Container")
	void OpenExternalContainer(USBI_InventoryComponent* ExternalInventory);

	/** 关闭外部容器 */
	UFUNCTION(BlueprintCallable, Category = "SandboxInventory|Container")
	void CloseExternalContainer();

	/** 是否正在与外部容器交互 */
	bool IsInteractingWithContainer() const { return IsValid(ExternalInventoryComponent); }

	/** 获取当前外部容器 */
	USBI_InventoryComponent* GetExternalContainer() const { return ExternalInventoryComponent.Get(); }

	/** 将物品转移到外部容器 */
	void TransferItemToExternal(UIC_InventoryItem* Item, int32 StackCount = 1);

	/** 从外部容器转移物品 */
	void TransferItemFromExternal(UIC_InventoryItem* Item, int32 StackCount = 1);

	/** 开/关背包 */
	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	void ToggleInventory();

	/** 视线检测可拾取物品 */
	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	void TraceForItem();

	/** 在世界中生成丢弃的物品 */
	void SpawnDroppedItem(UIC_InventoryItem* Item, int32 StackCount);

	/** 获取所有物品 */
	TArray<UIC_InventoryItem*> GetAllItems();

	/** 按类型查找物品 */
	UIC_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType);

	/** 设置 UI 引用 */
	void SetInventoryWidget(USBI_InventoryWidget* InInventoryWidget) { InventoryWidget = InInventoryWidget; }
	bool IsInventoryOpen() const { return bInventoryOpen; }

	/** 网格配置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Grid")
	int32 Columns = 9;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Grid")
	int32 Rows = 4;

	/** 拾取检测距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Interaction")
	float TraceLength = 500.f;

	/** 丢弃角度范围 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Drop")
	float DropSpawnAngleMin = -45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Drop")
	float DropSpawnAngleMax = 45.f;

	/** 委托 */
	FSBI_ItemChange OnItemAdded;
	FSBI_ItemChange OnItemRemoved;
	FSBI_NoRoom NoRoomInInventory;
	FSBI_StackChange OnStackChange;

	FSBI_EquipStatusChanged OnItemEquipped;
	FSBI_EquipStatusChanged OnItemUnequipped;

	/** 空间查询 (供 UI 层调用) */
	FSBI_SlotAvailabilityResult HasRoomForItem(UIC_ItemComponent* ItemComponent) const;
	FSBI_SlotAvailabilityResult HasRoomForItem(UIC_InventoryItem* Item, int32 StackAmountOverride = -1) const;

private:
	TWeakObjectPtr<APlayerController> OwningController;

	UPROPERTY(Replicated)
	FIC_InventoryFastArray InventoryList;

	UPROPERTY()
	TObjectPtr<USBI_InventoryWidget> InventoryWidget;

	/** 外部容器引用 (箱子、其他角色、宠物等) */
	UPROPERTY(Replicated)
	TObjectPtr<USBI_InventoryComponent> ExternalInventoryComponent;

	/** 槽位到物品的映射 (线性索引 -> 物品) */
	TMap<int32, TObjectPtr<UIC_InventoryItem>> SlotToItem;

	/** 当前视线中的 Actor */
	TWeakObjectPtr<AActor> LastActor;
	TWeakObjectPtr<AActor> ThisActor;

	bool bInventoryOpen{false};

	/** 查找第一个空槽位索引 */
	int32 FindFirstEmptySlot() const;

	/** 槽位线性索引 */
	int32 SlotIndex(int32 Row, int32 Col) const { return Row * Columns + Col; }
	void IndexToSlot(int32 Idx, int32& Row, int32& Col) const { Row = Idx / Columns; Col = Idx % Columns; }
};