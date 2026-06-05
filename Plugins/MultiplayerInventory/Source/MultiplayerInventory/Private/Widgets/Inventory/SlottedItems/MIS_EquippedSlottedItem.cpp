#include "Widgets/Inventory/SlottedItems/MIS_EquippedSlottedItem.h"

FReply UMIS_EquippedSlottedItem::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnEquippedSlottedItemClicked.Broadcast(this);
	return FReply::Handled();
}

void UMIS_EquippedSlottedItem::SetImage(UTexture2D* Icon) const
{
	
}
