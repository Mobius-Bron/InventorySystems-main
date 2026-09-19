#include "Widgets/Inventory/GridSlots/MIS_GridSlot.h"
#include "Items/MIS_InventoryItem.h"
#include "MIS_MessageKeys.h"
#include "Widgets/ItemPopUp/MIS_ItemPopUp.h"

#include "Components/Image.h"

void UMIS_GridSlot::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseEnter(MyGeometry, MouseEvent);

	// [解耦重构] 广播消息而非委托: 本控件不需要知道父级是谁、也不需要 MouseEvent 之外的上下文。
	// SigSource 为自身, 接收方据此区分是哪一个格子。
	MIS::Emit(MSGKEY(MIS_UI_GRID_SLOT_HOVERED), GMP::FSigSource(this), TileIndex);
}

void UMIS_GridSlot::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	MIS::Emit(MSGKEY(MIS_UI_GRID_SLOT_UNHOVERED), GMP::FSigSource(this), TileIndex);
}

FReply UMIS_GridSlot::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// 点击需携带鼠标键: 父级会把它转发给物品点击逻辑, 用于区分左键拾取 / 右键菜单
	MIS::Emit(MSGKEY(MIS_UI_GRID_SLOT_CLICKED), GMP::FSigSource(this),
		TileIndex, MIS::MouseButtonFromEvent(MouseEvent));
	return FReply::Handled();
}

void UMIS_GridSlot::SetInventoryItem(UMIS_InventoryItem* Item)
{
	InventoryItem = Item;
}

void UMIS_GridSlot::SetItemPopUp(UMIS_ItemPopUp* PopUp)
{
	ItemPopUp = PopUp;
	ItemPopUp->SetGridIndex(GetIndex());
	ItemPopUp->OnNativeDestruct.AddUObject(this, &ThisClass::OnItemPopUpDestruct);
}

UMIS_ItemPopUp* UMIS_GridSlot::GetItemPopUp() const
{
	return ItemPopUp.Get();
}

void UMIS_GridSlot::SetOccupiedTexture()
{
	GridSlotState = EMIS_GridSlotState::Occupied;
	Image_GridSlot->SetBrush(Brush_Occupied);
}

void UMIS_GridSlot::SetUnoccupiedTexture()
{
	GridSlotState = EMIS_GridSlotState::Unoccupied;
	Image_GridSlot->SetBrush(Brush_Unoccupied);
}

void UMIS_GridSlot::SetSelectedTexture()
{
	GridSlotState = EMIS_GridSlotState::Selected;
	Image_GridSlot->SetBrush(Brush_Selected);
}

void UMIS_GridSlot::SetGrayedOutTexture()
{
	GridSlotState = EMIS_GridSlotState::GrayedOut;
	Image_GridSlot->SetBrush(Brush_GrayedOut);
}

void UMIS_GridSlot::OnItemPopUpDestruct(UUserWidget* Menu)
{
	ItemPopUp.Reset();
}
