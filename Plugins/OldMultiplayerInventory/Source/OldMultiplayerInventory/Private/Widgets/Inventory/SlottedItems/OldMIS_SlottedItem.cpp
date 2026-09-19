#include "Widgets/Inventory/SlottedItems/OldMIS_SlottedItem.h"

#include "DH_DebugFunctionLibrary.h"
#include "Items/OldMIS_InventoryItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

FReply UOldMIS_SlottedItem::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnSlottedItemClicked.Broadcast(GridIndex, MouseEvent);
	return FReply::Handled();
}

void UOldMIS_SlottedItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DH_SCREEN(1.5f, DHColors::Cyan, "[SlottedItem] 鼠标进入 | GridIndex=%d", GridIndex);
	OnSlottedItemHovered.Broadcast(GridIndex);
}

void UOldMIS_SlottedItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	DH_SCREEN(1.5f, DHColors::Cyan, "[SlottedItem] 鼠标离开 | GridIndex=%d", GridIndex);
	OnSlottedItemUnhovered.Broadcast(GridIndex);
}

void UOldMIS_SlottedItem::SetInventoryItem(UOldMIS_InventoryItem* Item)
{
	InventoryItem = TWeakObjectPtr<UOldMIS_InventoryItem>(Item);
}

UOldMIS_InventoryItem* UOldMIS_SlottedItem::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void UOldMIS_SlottedItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void UOldMIS_SlottedItem::UpdateStackCount(int32 StackCount)
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
