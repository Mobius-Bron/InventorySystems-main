// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "SBI_EquipmentSlot.generated.h"

class UIC_InventoryItem;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSBI_EquipmentSlotClicked, const FGameplayTag&, EquipmentTypeTag, const FPointerEvent&, MouseEvent);

UCLASS()
class SANDBOXINVENTORY_API USBI_EquipmentSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 设置装备类型标签 */
	void SetEquipmentTypeTag(const FGameplayTag& InTag) { EquipmentTypeTag = InTag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }

	/** 设置当前装备的物品 */
	void SetInventoryItem(UIC_InventoryItem* Item);
	UIC_InventoryItem* GetInventoryItem() const { return InventoryItem.Get(); }

	/** 设置槽位背景纹理 */
	void SetUnoccupiedTexture();
	void SetOccupiedTexture();

	FSBI_EquipmentSlotClicked EquipmentSlotClicked;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_EquipmentSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SlotLabel;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FSlateBrush Brush_Unoccupied;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FSlateBrush Brush_Occupied;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FSlateBrush Brush_Selected;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FGameplayTag EquipmentTypeTag = FGameplayTag::EmptyTag;

	TWeakObjectPtr<UIC_InventoryItem> InventoryItem;
};