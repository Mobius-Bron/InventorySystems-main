#include "Widgets/Inventory/SlottedItems/OldMIS_EquippedSlottedItem.h"

FReply UOldMIS_EquippedSlottedItem::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnEquippedSlottedItemClicked.Broadcast(this);
	return FReply::Handled();
}

void UOldMIS_EquippedSlottedItem::SetImage(UTexture2D* Icon) const
{
	
}
