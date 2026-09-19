#include "Items/Manifest/OldMIS_ItemManifest.h"

#include "DH_DebugFunctionLibrary.h"
#include "Items/Components/OldMIS_ItemComponent.h"
#include "Items/OldMIS_InventoryItem.h"
#include "Items/Fragments/OldMIS_ItemFragment.h"
#include "Widgets/Composite/OldMIS_CompositeBase.h"


UOldMIS_InventoryItem* FOldMIS_ItemManifest::Manifest(UObject* NewOuter)
{
	UOldMIS_InventoryItem* NewItem = NewObject<UOldMIS_InventoryItem>(NewOuter);
	NewItem->SetItemManifest(*this);

	for (auto& Fragment : NewItem->GetItemManifestMutable().GetFragmentsMutable())
	{
		Fragment.GetMutable().Manifest();
	}

	ClearFragments();

	return NewItem;
}

void FOldMIS_ItemManifest::AssimilateInventoryFragments(UOldMIS_CompositeBase* Composite) const
{
	DH_SCREEN(2.f, DHColors::Orange, "填充物品描述框");
	const auto& InventoryItemFragments = GetAllFragmentsOfType<FOldMIS_InventoryItemFragment>();
	for (const auto* Fragment : InventoryItemFragments)
	{
		DH_SCREEN(2.f, DHColors::Red, "填充数值");
		Composite->ApplyFunction([Fragment](UOldMIS_CompositeBase* Widget)
		{
			Fragment->Assimilate(Widget);
		});
	}
}

void FOldMIS_ItemManifest::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!IsValid(PickupActorClass)) return;

	AActor* PickupActor = WorldContextObject->GetWorld()->SpawnActor(PickupActorClass, &SpawnLocation, &SpawnRotation);
	if (!IsValid(PickupActor)) return;

	if (UOldMIS_ItemComponent* ItemComp = PickupActor->FindComponentByClass<UOldMIS_ItemComponent>())
	{
		ItemComp->InitItemManifest(*this);
	}
}

void FOldMIS_ItemManifest::ClearFragments()
{
	Fragments.Empty();
}
