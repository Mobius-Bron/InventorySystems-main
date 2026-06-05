#include "BlueprintFunctionLibraries/MIS_InventoryFunctionLibrary.h"

#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "Items/MIS_InventoryItem.h"

UMIS_InventoryComponent* UMIS_InventoryFunctionLibrary::GetInventoryComponent(const APlayerController* PlayerController)
{
	if (!IsValid(PlayerController)) return nullptr;
	return PlayerController->FindComponentByClass<UMIS_InventoryComponent>();
}

UMIS_InventoryItem* UMIS_InventoryFunctionLibrary::FindFirstItemByType(APlayerController* PC, FGameplayTag ItemType)
{
	UMIS_InventoryComponent* IC = GetInventoryComponent(PC);
	if (!IsValid(IC)) return nullptr;

	return IC->FindFirstItemByType(ItemType);
}
