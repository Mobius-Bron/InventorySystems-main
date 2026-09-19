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

/**
 * 库存核心组件 (ViewModel)
 *
 * 初始化: 外部调用 Init(PC) 传入 PlayerController,不依赖 Owner 类型,
 * 可挂在 Character / PlayerController 等任意 Actor 上。
 *
 * [解耦重构] 本组件已不再声明任何多播委托,也不再持有 HUD 指针:
 *   - 状态通知: 通过 GMP 消息 (MIS.Inv.*) 广播, SigSource = 本组件, 键定义见 MIS_MessageKeys.h
 *   - 意图命令: 在 BeginPlay 监听 UI 发来的 (MIS.Cmd.*) 消息
 *   - 唯一例外: SetInventoryWidget 注入的网格视图, 用于同步空间查询 HasRoomForItem,
 *     因为槽位占用数据当前仅存在于 UI 侧; 待位置数据化后一并移除。
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

	UFUNCTION(Server, Reliable)
	void Server_AddNewItem(AActor* ItemActor, int32 StackCount, int32 Remainder);

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

	/** [解耦遗留] 网格空间查询需要 UI 侧的槽位占用信息, 暂保留单向注入。 */
	void SetInventoryWidget(UMIS_InventoryWidget* InInventoryWidget) { InventoryWidget = InInventoryWidget; }
	bool IsInventoryOpen() const;

private:
	TWeakObjectPtr<APlayerController> OwningController;

	UPROPERTY(Replicated)
	FMIS_InventoryFastArray InventoryList;

	UPROPERTY()
	TObjectPtr<UMIS_InventoryWidget> InventoryWidget;

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
