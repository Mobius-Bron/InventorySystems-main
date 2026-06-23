// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Equipment/SBI_EquipmentSlot.h"

#include "DS_DebugFunctionLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/IC_InventoryItem.h"
#include "Items/IC_ItemFragment.h"
#include "Items/Fragments/SBI_ItemFragment.h"

void USBI_EquipmentSlot::SetInventoryItem(UIC_InventoryItem* Item)
{
	InventoryItem = Item;

	if (IsValid(Item))
	{
		SetOccupiedTexture();

		// 设置图标
		const FIC_ImageFragment* ImageFragment = GetFragment<FIC_ImageFragment>(Item, FGameplayTag::EmptyTag);
		if (ImageFragment && Image_Icon)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(ImageFragment->GetIcon());
			Brush.ImageSize = ImageFragment->GetIconDimensions();
			Image_Icon->SetBrush(Brush);
			Image_Icon->SetVisibility(ESlateVisibility::Visible);
		}

		DS_LOG(Inventory, "装备槽位 [%s] 已装备: %s", *EquipmentTypeTag.ToString(), *Item->GetName());
	}
	else
	{
		SetUnoccupiedTexture();

		if (Image_Icon)
		{
			Image_Icon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void USBI_EquipmentSlot::SetUnoccupiedTexture()
{
	if (Image_EquipmentSlot)
	{
		Image_EquipmentSlot->SetBrush(Brush_Unoccupied);
	}
}

void USBI_EquipmentSlot::SetOccupiedTexture()
{
	if (Image_EquipmentSlot)
	{
		Image_EquipmentSlot->SetBrush(Brush_Occupied);
	}
}

FReply USBI_EquipmentSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	DS_LOG(Inventory, "装备槽位点击: %s | 按键=%s",
		*EquipmentTypeTag.ToString(), *InMouseEvent.GetEffectingButton().ToString());

	EquipmentSlotClicked.Broadcast(EquipmentTypeTag, InMouseEvent);
	return FReply::Handled();
}