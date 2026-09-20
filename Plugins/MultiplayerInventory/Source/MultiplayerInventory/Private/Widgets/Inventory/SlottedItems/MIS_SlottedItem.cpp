#include "Widgets/Inventory/SlottedItems/MIS_SlottedItem.h"

#include "MIS_MessageKeys.h"
#include "Items/MIS_InventoryItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

FReply UMIS_SlottedItem::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// [解耦重构] 广播消息; 只传下游真正需要的"哪个格子 + 哪个键", 不传 Slate 事件对象
	MIS::Emit(MSGKEY(MIS_UI_SLOTTED_ITEM_CLICKED), GMP::FSigSource(this),
		GridIndex, MIS::MouseButtonFromEvent(MouseEvent));
	return FReply::Handled();
}

void UMIS_SlottedItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	MIS::Emit(MSGKEY(MIS_UI_SLOTTED_ITEM_HOVERED), GMP::FSigSource(this), GridIndex);
}

void UMIS_SlottedItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	MIS::Emit(MSGKEY(MIS_UI_SLOTTED_ITEM_UNHOVERED), GMP::FSigSource(this), GridIndex);
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
