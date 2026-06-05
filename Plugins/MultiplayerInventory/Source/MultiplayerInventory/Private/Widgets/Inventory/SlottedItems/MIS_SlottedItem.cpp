#include "Widgets/Inventory/SlottedItems/MIS_SlottedItem.h"

#include "DH_DebugFunctionLibrary.h"
#include "Items/MIS_InventoryItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

FReply UMIS_SlottedItem::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnSlottedItemClicked.Broadcast(GridIndex, MouseEvent);
	return FReply::Handled();
}

void UMIS_SlottedItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DH_SCREEN(1.5f, DHColors::Cyan, "[SlottedItem] 鼠标进入 | GridIndex=%d", GridIndex);
	OnSlottedItemHovered.Broadcast(GridIndex);
}

void UMIS_SlottedItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	DH_SCREEN(1.5f, DHColors::Cyan, "[SlottedItem] 鼠标离开 | GridIndex=%d", GridIndex);
	OnSlottedItemUnhovered.Broadcast(GridIndex);
}

void UMIS_SlottedItem::SetInventoryItem(UMIS_InventoryItem* Item)
{
	InventoryItem = TWeakObjectPtr<UMIS_InventoryItem>(Item);
}

UMIS_InventoryItem* UMIS_SlottedItem::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void UMIS_SlottedItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void UMIS_SlottedItem::UpdateStackCount(int32 StackCount)
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
