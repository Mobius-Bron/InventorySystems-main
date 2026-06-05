#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/SlottedItems/MIS_SlottedItem.h"
#include "GameplayTagContainer.h"

#include "MIS_EquippedSlottedItem.generated.h"

class UMIS_InventoryItem;
class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMIS_OnEquippedSlottedItemClicked, UMIS_EquippedSlottedItem*, EquippedSlottedItem);

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_EquippedSlottedItem : public UMIS_SlottedItem
{
	GENERATED_BODY()
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FMIS_OnEquippedSlottedItemClicked OnEquippedSlottedItemClicked;

	void SetImage(UTexture2D* Icon) const;
	void SetEquipmentTypeTag(const FGameplayTag& Tag) { EquipmentTypeTag = Tag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }

private:
	FGameplayTag EquipmentTypeTag;
};
