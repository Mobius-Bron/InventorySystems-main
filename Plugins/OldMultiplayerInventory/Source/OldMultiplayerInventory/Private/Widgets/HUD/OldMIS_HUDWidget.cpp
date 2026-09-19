#include "Widgets/HUD/OldMIS_HUDWidget.h"

#include "BlueprintFunctionLibraries/OldMIS_InventoryFunctionLibrary.h"
#include "InventoryManagement/Components/OldMIS_InventoryComponent.h"
#include "Widgets/HUD/OldMIS_InfoMessage.h"

void UOldMIS_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UOldMIS_InventoryComponent* InventoryComponent = UOldMIS_InventoryFunctionLibrary::GetInventoryComponent(GetOwningPlayer()))
	{
		InventoryComponent->NoRoomInInventory.AddDynamic(this, &ThisClass::OnNoRoom);
	}
}

void UOldMIS_HUDWidget::OnNoRoom()
{
	if (!IsValid(InfoMessage)) return;
	InfoMessage->SetMessage(FText::FromString("No Room In Inventory."));
}
