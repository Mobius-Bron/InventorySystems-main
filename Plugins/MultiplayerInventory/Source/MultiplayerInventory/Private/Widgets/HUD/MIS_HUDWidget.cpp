#include "Widgets/HUD/MIS_HUDWidget.h"

#include "BlueprintFunctionLibraries/MIS_InventoryFunctionLibrary.h"
#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "Widgets/HUD/MIS_InfoMessage.h"

void UMIS_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UMIS_InventoryComponent* InventoryComponent = UMIS_InventoryFunctionLibrary::GetInventoryComponent(GetOwningPlayer()))
	{
		InventoryComponent->NoRoomInInventory.AddDynamic(this, &ThisClass::OnNoRoom);
	}
}

void UMIS_HUDWidget::OnNoRoom()
{
	if (!IsValid(InfoMessage)) return;
	InfoMessage->SetMessage(FText::FromString("No Room In Inventory."));
}
