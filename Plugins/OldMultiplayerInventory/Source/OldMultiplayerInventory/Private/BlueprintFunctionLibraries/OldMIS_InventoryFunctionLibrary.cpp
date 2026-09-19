#include "BlueprintFunctionLibraries/OldMIS_InventoryFunctionLibrary.h"

#include "InventoryManagement/Components/OldMIS_InventoryComponent.h"
#include "Items/OldMIS_InventoryItem.h"

UOldMIS_InventoryComponent* UOldMIS_InventoryFunctionLibrary::GetInventoryComponent(const APlayerController* PlayerController)
{
	if (!IsValid(PlayerController)) return nullptr;
	return PlayerController->FindComponentByClass<UOldMIS_InventoryComponent>();
}

UOldMIS_InventoryItem* UOldMIS_InventoryFunctionLibrary::FindFirstItemByType(APlayerController* PC, FGameplayTag ItemType)
{
	UOldMIS_InventoryComponent* IC = GetInventoryComponent(PC);
	if (!IsValid(IC)) return nullptr;

	return IC->FindFirstItemByType(ItemType);
}
