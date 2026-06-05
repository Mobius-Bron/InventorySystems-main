#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/GridSlots/MIS_GridSlot.h"
#include "GameplayTagContainer.h"
#include "Components/Overlay.h"

#include "MIS_EquippedGridSlot.generated.h"

class UMIS_EquippedSlottedItem;
class UMIS_InventoryItem;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMIS_EquippedGridSlotClicked, UMIS_EquippedGridSlot*, GridSlot, const FGameplayTag&, EquipmentTypeTag);

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_EquippedGridSlot : public UMIS_GridSlot
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	UMIS_EquippedSlottedItem* OnItemEquipped(UMIS_InventoryItem* Item, const FGameplayTag& EquipmentTag, float TileSize);
	void SetEquippedSlottedItem(UMIS_EquippedSlottedItem* Item) { EquippedSlottedItem = Item; }
	void SetEquipmentTypeTag(const FGameplayTag& Tag) { EquipmentTypeTag = Tag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }

	FMIS_EquippedGridSlotClicked EquippedGridSlotClicked;

private:
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (Categories = "GameItems.Equipment"))
	FGameplayTag EquipmentTypeTag;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GrayedOutIcon;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UMIS_EquippedSlottedItem> EquippedSlottedItemClass;

	UPROPERTY()
	TObjectPtr<UMIS_EquippedSlottedItem> EquippedSlottedItem;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Root;
};
