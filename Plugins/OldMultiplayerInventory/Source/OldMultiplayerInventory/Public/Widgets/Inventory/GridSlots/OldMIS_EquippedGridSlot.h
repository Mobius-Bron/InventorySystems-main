#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/GridSlots/OldMIS_GridSlot.h"
#include "GameplayTagContainer.h"
#include "Components/Overlay.h"

#include "OldMIS_EquippedGridSlot.generated.h"

class UOldMIS_EquippedSlottedItem;
class UOldMIS_InventoryItem;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOldMIS_EquippedGridSlotClicked, UOldMIS_EquippedGridSlot*, GridSlot, const FGameplayTag&, EquipmentTypeTag);

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_EquippedGridSlot : public UOldMIS_GridSlot
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	UOldMIS_EquippedSlottedItem* OnItemEquipped(UOldMIS_InventoryItem* Item, const FGameplayTag& EquipmentTag, float TileSize);
	void SetEquippedSlottedItem(UOldMIS_EquippedSlottedItem* Item) { EquippedSlottedItem = Item; }
	void SetEquipmentTypeTag(const FGameplayTag& Tag) { EquipmentTypeTag = Tag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }
	void ClearEquippedState();

	FOldMIS_EquippedGridSlotClicked EquippedGridSlotClicked;

private:
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (Categories = "GameItems.Equipment"))
	FGameplayTag EquipmentTypeTag;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GrayedOutIcon;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOldMIS_EquippedSlottedItem> EquippedSlottedItemClass;

	UPROPERTY()
	TObjectPtr<UOldMIS_EquippedSlottedItem> EquippedSlottedItem;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Root;
};
