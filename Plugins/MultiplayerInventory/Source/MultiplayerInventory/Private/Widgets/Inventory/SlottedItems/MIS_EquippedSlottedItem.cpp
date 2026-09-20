#include "Widgets/Inventory/SlottedItems/MIS_EquippedSlottedItem.h"

#include "MIS_MessageKeys.h"

FReply UMIS_EquippedSlottedItem::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// [解耦重构] 广播消息: SigSource 与载荷都是自身, 接收方无需预先绑定
	MIS::Emit(MSGKEY(MIS_UI_EQUIPPED_SLOTTED_ITEM_CLICKED), GMP::FSigSource(this), this);
	return FReply::Handled();
}
