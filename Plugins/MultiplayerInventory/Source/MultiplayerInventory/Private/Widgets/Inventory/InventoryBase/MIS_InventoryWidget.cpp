#include "Widgets/Inventory/InventoryBase/MIS_InventoryWidget.h"

#include "Components/CanvasPanel.h"
#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "Widgets/Inventory/Spatial/MIS_InventoryGrid.h"

void UMIS_InventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	check(InventoryGrid);
	check(CanvasPanel);
}

void UMIS_InventoryWidget::InitFromComponent(UMIS_InventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
	InventoryGrid->InitFromComponent(InInventoryComponent, CanvasPanel);
}

void UMIS_InventoryWidget::OpenInventory()
{
	SetVisibility(ESlateVisibility::Visible);
	InventoryGrid->ShowCursor();
}

void UMIS_InventoryWidget::CloseInventory()
{
	SetVisibility(ESlateVisibility::Collapsed);
	InventoryGrid->OnHide();
	InventoryGrid->HideCursor();
}

void UMIS_InventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

FReply UMIS_InventoryWidget::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InventoryGrid->DropItem();
	return FReply::Handled();
}

FMIS_SlotAvailabilityResult UMIS_InventoryWidget::HasRoomForItem(UMIS_ItemComponent* ItemComponent) const
{
	return InventoryGrid->HasRoomForItem(ItemComponent);
}
