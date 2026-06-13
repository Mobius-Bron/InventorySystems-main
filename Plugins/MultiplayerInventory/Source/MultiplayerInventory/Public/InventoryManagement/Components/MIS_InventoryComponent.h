#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CollisionProfile.h"
#include "InventoryManagement/FastArray/MIS_FastArray.h"

#include "MIS_InventoryComponent.generated.h"

class UMIS_InventoryItem;
class UMIS_ItemComponent;
class UMIS_HUDWidget;
class UMIS_InventoryWidget;
class APlayerController;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMIS_ItemChange, UMIS_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMIS_NoRoom);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMIS_StackChange, const FMIS_SlotAvailabilityResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMIS_EquipStatusChanged, UMIS_InventoryItem*, Item);

/**
 * 库存核心组件 (ViewModel)
 *
 * 初始化: 外部调用 Init(PC) 传入 PlayerController,不依赖 Owner 类型,
 * 可挂在 Character / PlayerController 等任意 Actor 上。
 * 后续通过 SetHUDWidget / SetInventoryWidget 绑定 UI。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class MULTIPLAYERINVENTORY_API UMIS_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIS_InventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	void SetHUDWidget(UMIS_HUDWidget* InHUDWidget) { HUDWidget = InHUDWidget; }
	void SetInventoryWidget(UMIS_InventoryWidget* InInventoryWidget) { InventoryWidget = InInventoryWidget; }
	bool IsInventoryOpen() const;

	FMIS_ItemChange OnItemAdded;
	FMIS_ItemChange OnItemRemoved;
	FMIS_NoRoom NoRoomInInventory;
	FMIS_StackChange OnStackChange;
	FMIS_EquipStatusChanged OnItemEquipped;
	FMIS_EquipStatusChanged OnItemUnequipped;

private:
	TWeakObjectPtr<APlayerController> OwningController;

	UPROPERTY(Replicated)
	FMIS_InventoryFastArray InventoryList;

	UPROPERTY()
	TObjectPtr<UMIS_HUDWidget> HUDWidget;

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
