#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/SlottedItems/OldMIS_SlottedItem.h"
#include "GameplayTagContainer.h"

#include "OldMIS_EquippedSlottedItem.generated.h"

class UOldMIS_InventoryItem;
class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOldMIS_OnEquippedSlottedItemClicked, UOldMIS_EquippedSlottedItem*, EquippedSlottedItem);

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_EquippedSlottedItem : public UOldMIS_SlottedItem
{
	GENERATED_BODY()
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FOldMIS_OnEquippedSlottedItemClicked OnEquippedSlottedItemClicked;

	void SetImage(UTexture2D* Icon) const;
	void SetEquipmentTypeTag(const FGameplayTag& Tag) { EquipmentTypeTag = Tag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }

private:
	FGameplayTag EquipmentTypeTag;
};
