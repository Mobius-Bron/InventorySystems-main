#include "Widgets/Inventory/InventoryBase/MIS_InventoryWidget.h"

#include "Components/CanvasPanel.h"
#include "GameFramework/PlayerController.h"
#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "MIS_MessageKeys.h"
#include "Widgets/Inventory/Spatial/MIS_InventoryGrid.h"

void UMIS_InventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	check(InventoryGrid);
	check(CanvasPanel);
}

void UMIS_InventoryWidget::InitFromComponent(UMIS_InventoryComponent* InInventoryComponent)
{
	InventoryGrid->InitFromComponent(InInventoryComponent, CanvasPanel);

	// [解耦重构] 监听数据层广播的开关消息, 由界面自己决定显隐与输入模式。
	// 原先这些动作由 InventoryComponent 反向调用 UI 完成, 现在数据层只负责发消息。
	if (IsValid(InInventoryComponent) && !bListeningInventoryMessages)
	{
		bListeningInventoryMessages = true;

		const GMP::FSigSource InventorySource(InInventoryComponent);

		MIS::Listen(MSGKEY(MIS_MSG_MENU_TOGGLED), InventorySource, this,
			[this](bool bOpen)
			{
				if (bOpen)
				{
					OpenInventory();
				}
				else
				{
					CloseInventory();
				}
			});
	}
}

void UMIS_InventoryWidget::OpenInventory()
{
	bIsOpen = true;
	SetVisibility(ESlateVisibility::Visible);
	InventoryGrid->ShowCursor();

	// [解耦重构] 输入模式由界面自己负责切换, 数据层不再越权操作 PlayerController
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameAndUI());
		PC->SetShowMouseCursor(true);
	}
}

void UMIS_InventoryWidget::CloseInventory()
{
	bIsOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	InventoryGrid->OnHide();
	InventoryGrid->HideCursor();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
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
