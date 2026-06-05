#include "Widgets/Inventory/GridSlots/MIS_GridSlot.h"
#include "Items/MIS_InventoryItem.h"
#include "Widgets/ItemPopUp/MIS_ItemPopUp.h"

#include "Components/Image.h"

void UMIS_GridSlot::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
	GridSlotHovered.Broadcast(TileIndex, MouseEvent);
}

void UMIS_GridSlot::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	GridSlotUnhovered.Broadcast(TileIndex, MouseEvent);
}

FReply UMIS_GridSlot::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	GridSlotClicked.Broadcast(TileIndex, MouseEvent);
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
