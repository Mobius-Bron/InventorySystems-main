#include "Widgets/Inventory/GridSlots/OldMIS_GridSlot.h"
#include "Items/OldMIS_InventoryItem.h"
#include "Widgets/ItemPopUp/OldMIS_ItemPopUp.h"

#include "Components/Image.h"

void UOldMIS_GridSlot::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
	GridSlotHovered.Broadcast(TileIndex, MouseEvent);
}

void UOldMIS_GridSlot::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	GridSlotUnhovered.Broadcast(TileIndex, MouseEvent);
}

FReply UOldMIS_GridSlot::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	GridSlotClicked.Broadcast(TileIndex, MouseEvent);
	return FReply::Handled();
}

void UOldMIS_GridSlot::SetInventoryItem(UOldMIS_InventoryItem* Item)
{
	InventoryItem = Item;
}

void UOldMIS_GridSlot::SetItemPopUp(UOldMIS_ItemPopUp* PopUp)
{
	ItemPopUp = PopUp;
	ItemPopUp->SetGridIndex(GetIndex());
	ItemPopUp->OnNativeDestruct.AddUObject(this, &ThisClass::OnItemPopUpDestruct);
}

UOldMIS_ItemPopUp* UOldMIS_GridSlot::GetItemPopUp() const
{
	return ItemPopUp.Get();
}

void UOldMIS_GridSlot::SetOccupiedTexture()
{
	GridSlotState = EOldMIS_GridSlotState::Occupied;
	Image_GridSlot->SetBrush(Brush_Occupied);
}

void UOldMIS_GridSlot::SetUnoccupiedTexture()
{
	GridSlotState = EOldMIS_GridSlotState::Unoccupied;
	Image_GridSlot->SetBrush(Brush_Unoccupied);
}

void UOldMIS_GridSlot::SetSelectedTexture()
{
	GridSlotState = EOldMIS_GridSlotState::Selected;
	Image_GridSlot->SetBrush(Brush_Selected);
}

void UOldMIS_GridSlot::SetGrayedOutTexture()
{
	GridSlotState = EOldMIS_GridSlotState::GrayedOut;
	Image_GridSlot->SetBrush(Brush_GrayedOut);
}

void UOldMIS_GridSlot::OnItemPopUpDestruct(UUserWidget* Menu)
{
	ItemPopUp.Reset();
}
