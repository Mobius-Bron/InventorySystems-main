// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Inventory/SlottedItems/SBI_SlottedItem.h"
#include "Items/IC_InventoryItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

FReply USBI_SlottedItem::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnSlottedItemClicked.Broadcast(GridIndex, MouseEvent);
	return FReply::Handled();
}

void USBI_SlottedItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnSlottedItemHovered.Broadcast(GridIndex);
}

void USBI_SlottedItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	OnSlottedItemUnhovered.Broadcast(GridIndex);
}

void USBI_SlottedItem::SetInventoryItem(UIC_InventoryItem* Item)
{
	InventoryItem = TWeakObjectPtr<UIC_InventoryItem>(Item);
}

UIC_InventoryItem* USBI_SlottedItem::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void USBI_SlottedItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void USBI_SlottedItem::UpdateStackCount(int32 StackCount)
{
	if (StackCount > 1)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Visible);
		Text_StackCount->SetText(FText::AsNumber(StackCount));
	}
	else
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}