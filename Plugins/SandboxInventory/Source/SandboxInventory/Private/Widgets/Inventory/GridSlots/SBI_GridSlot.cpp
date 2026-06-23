// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Inventory/GridSlots/SBI_GridSlot.h"
#include "Items/IC_InventoryItem.h"
#include "Widgets/Inventory/ItemPopUp/SBI_ItemPopUp.h"

#include "Components/Image.h"

void USBI_GridSlot::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
	GridSlotHovered.Broadcast(TileIndex, MouseEvent);
}

void USBI_GridSlot::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	GridSlotUnhovered.Broadcast(TileIndex, MouseEvent);
}

FReply USBI_GridSlot::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	GridSlotClicked.Broadcast(TileIndex, MouseEvent);
	return FReply::Handled();
}

void USBI_GridSlot::SetInventoryItem(UIC_InventoryItem* Item)
{
	InventoryItem = Item;
}

UIC_InventoryItem* USBI_GridSlot::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void USBI_GridSlot::SetItemPopUp(USBI_ItemPopUp* PopUp)
{
	ItemPopUp = PopUp;
	PopUp->SetGridIndex(GetTileIndex());
	PopUp->OnNativeDestruct.AddUObject(this, &ThisClass::OnItemPopUpDestruct);
}

USBI_ItemPopUp* USBI_GridSlot::GetItemPopUp() const
{
	return ItemPopUp.Get();
}

void USBI_GridSlot::SetOccupiedTexture()
{
	GridSlotState = ESBI_GridSlotState::Occupied;
	Image_GridSlot->SetBrush(Brush_Occupied);
}

void USBI_GridSlot::SetUnoccupiedTexture()
{
	GridSlotState = ESBI_GridSlotState::Unoccupied;
	Image_GridSlot->SetBrush(Brush_Unoccupied);
}

void USBI_GridSlot::SetSelectedTexture()
{
	GridSlotState = ESBI_GridSlotState::Selected;
	Image_GridSlot->SetBrush(Brush_Selected);
}

void USBI_GridSlot::OnItemPopUpDestruct(UUserWidget* Menu)
{
	ItemPopUp.Reset();
}