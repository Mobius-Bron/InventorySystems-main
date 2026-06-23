// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Inventory/InventoryBase/SBI_InventoryWidget.h"

#include "Components/CanvasPanel.h"
#include "InventoryManagement/Components/SBI_InventoryComponent.h"
#include "Widgets/Inventory/Spatial/SBI_InventoryGrid.h"

void USBI_InventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	check(InventoryGrid);
	check(CanvasPanel);
}

void USBI_InventoryWidget::InitFromComponent(USBI_InventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
	InventoryGrid->InitFromComponent(InInventoryComponent, CanvasPanel);
}

void USBI_InventoryWidget::OpenInventory()
{
	SetVisibility(ESlateVisibility::Visible);
	InventoryGrid->ShowCursor();
}

void USBI_InventoryWidget::CloseInventory()
{
	SetVisibility(ESlateVisibility::Collapsed);
	InventoryGrid->OnHide();
	InventoryGrid->HideCursor();
}

void USBI_InventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

FReply USBI_InventoryWidget::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InventoryGrid->DropItem();
	return FReply::Handled();
}

FSBI_SlotAvailabilityResult USBI_InventoryWidget::HasRoomForItem(UIC_ItemComponent* ItemComponent) const
{
	return InventoryGrid->HasRoomForItem(ItemComponent);
}