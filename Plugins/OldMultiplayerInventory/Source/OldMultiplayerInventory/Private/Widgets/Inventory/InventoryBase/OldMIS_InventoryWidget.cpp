#include "Widgets/Inventory/InventoryBase/OldMIS_InventoryWidget.h"

#include "Components/CanvasPanel.h"
#include "InventoryManagement/Components/OldMIS_InventoryComponent.h"
#include "Widgets/Inventory/Spatial/OldMIS_InventoryGrid.h"

void UOldMIS_InventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	check(InventoryGrid);
	check(CanvasPanel);
}

void UOldMIS_InventoryWidget::InitFromComponent(UOldMIS_InventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
	InventoryGrid->InitFromComponent(InInventoryComponent, CanvasPanel);
}

void UOldMIS_InventoryWidget::OpenInventory()
{
	SetVisibility(ESlateVisibility::Visible);
	InventoryGrid->ShowCursor();
}

void UOldMIS_InventoryWidget::CloseInventory()
{
	SetVisibility(ESlateVisibility::Collapsed);
	InventoryGrid->OnHide();
	InventoryGrid->HideCursor();
}

void UOldMIS_InventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

FReply UOldMIS_InventoryWidget::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InventoryGrid->DropItem();
	return FReply::Handled();
}

FOldMIS_SlotAvailabilityResult UOldMIS_InventoryWidget::HasRoomForItem(UOldMIS_ItemComponent* ItemComponent) const
{
	return InventoryGrid->HasRoomForItem(ItemComponent);
}
