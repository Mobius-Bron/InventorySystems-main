#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CollisionProfile.h"
#include "InventoryManagement/FastArray/OldMIS_FastArray.h"

#include "OldMIS_InventoryComponent.generated.h"

class UOldMIS_InventoryItem;
class UOldMIS_ItemComponent;
class UOldMIS_HUDWidget;
class UOldMIS_InventoryWidget;
class APlayerController;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOldMIS_ItemChange, UOldMIS_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOldMIS_NoRoom);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOldMIS_StackChange, const FOldMIS_SlotAvailabilityResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOldMIS_EquipStatusChanged, UOldMIS_InventoryItem*, Item);

/**
 * 库存核心组件 (ViewModel)
 *
 * 初始化: 外部调用 Init(PC) 传入 PlayerController,不依赖 Owner 类型,
 * 可挂在 Character / PlayerController 等任意 Actor 上。
 * 后续通过 SetHUDWidget / SetInventoryWidget 绑定 UI。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class OLDMULTIPLAYERINVENTORY_API UOldMIS_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOldMIS_InventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 初始化 — 传入 PlayerController 引用 */
	void Init(APlayerController* InPC);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void TryAddItem(UOldMIS_ItemComponent* ItemComponent);

	UFUNCTION(Server, Reliable)
	void Server_AddNewItem(AActor* ItemActor, int32 StackCount, int32 Remainder);

	UFUNCTION(Server, Reliable)
	void Server_AddStacksToItem(AActor* ItemActor, int32 StackCount, int32 Remainder);

	UFUNCTION(Server, Reliable)
	void Server_DropItem(UOldMIS_InventoryItem* Item, int32 StackCount);

	UFUNCTION(Server, Reliable)
	void Server_ConsumeItem(UOldMIS_InventoryItem* Item);

	UFUNCTION(Server, Reliable)
	void Server_EquipSlotClicked(UOldMIS_InventoryItem* ItemToEquip, UOldMIS_InventoryItem* ItemToUnequip);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EquipSlotClicked(UOldMIS_InventoryItem* ItemToEquip, UOldMIS_InventoryItem* ItemToUnequip);

	void RequestEquipSlotClicked(UOldMIS_InventoryItem* ItemToEquip, UOldMIS_InventoryItem* ItemToUnequip);
	void RequestDropItem(UOldMIS_InventoryItem* ItemToDrop, int32 StackCount = 1);
	void RequestConsumeItem(UOldMIS_InventoryItem* ItemToConsume);
	
	void PrimaryInteract();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void TraceForItem();

	void AddRepSubObj(UObject* SubObj);
	void SpawnDroppedItem(UOldMIS_InventoryItem* Item, int32 StackCount);

	TArray<UOldMIS_InventoryItem*> GetAllItems();
	UOldMIS_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType);

	void SetHUDWidget(UOldMIS_HUDWidget* InHUDWidget) { HUDWidget = InHUDWidget; }
	void SetInventoryWidget(UOldMIS_InventoryWidget* InInventoryWidget) { InventoryWidget = InInventoryWidget; }
	bool IsInventoryOpen() const;

	FOldMIS_ItemChange OnItemAdded;
	FOldMIS_ItemChange OnItemRemoved;
	FOldMIS_NoRoom NoRoomInInventory;
	FOldMIS_StackChange OnStackChange;
	FOldMIS_EquipStatusChanged OnItemEquipped;
	FOldMIS_EquipStatusChanged OnItemUnequipped;

private:
	TWeakObjectPtr<APlayerController> OwningController;

	UPROPERTY(Replicated)
	FOldMIS_InventoryFastArray InventoryList;

	UPROPERTY()
	TObjectPtr<UOldMIS_HUDWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<UOldMIS_InventoryWidget> InventoryWidget;

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
