#include "Items/Manifest/MIS_ItemManifest.h"

#include "DH_DebugFunctionLibrary.h"
#include "Items/Components/MIS_ItemComponent.h"
#include "Items/MIS_InventoryItem.h"
#include "Items/Fragments/MIS_ItemFragment.h"
#include "Widgets/Composite/MIS_CompositeBase.h"


UMIS_InventoryItem* FMIS_ItemManifest::Manifest(UObject* NewOuter)
{
	UMIS_InventoryItem* NewItem = NewObject<UMIS_InventoryItem>(NewOuter);
	NewItem->SetItemManifest(*this);

	for (auto& Fragment : NewItem->GetItemManifestMutable().GetFragmentsMutable())
	{
		Fragment.GetMutable().Manifest();
	}

	ClearFragments();

	return NewItem;
}

void FMIS_ItemManifest::AssimilateInventoryFragments(UMIS_CompositeBase* Composite) const
{
	DH_SCREEN(2.f, DHColors::Orange, "填充物品描述框");
	const auto& InventoryItemFragments = GetAllFragmentsOfType<FMIS_InventoryItemFragment>();
	for (const auto* Fragment : InventoryItemFragments)
	{
		DH_SCREEN(2.f, DHColors::Red, "填充数值");
		Composite->ApplyFunction([Fragment](UMIS_CompositeBase* Widget)
		{
			Fragment->Assimilate(Widget);
		});
	}
}

void FMIS_ItemManifest::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!IsValid(PickupActorClass)) return;

	AActor* PickupActor = WorldContextObject->GetWorld()->SpawnActor(PickupActorClass, &SpawnLocation, &SpawnRotation);
	if (!IsValid(PickupActor)) return;

	if (UMIS_ItemComponent* ItemComp = PickupActor->FindComponentByClass<UMIS_ItemComponent>())
	{
		ItemComp->InitItemManifest(*this);
	}
}

void FMIS_ItemManifest::ClearFragments()
{
	Fragments.Empty();
}
