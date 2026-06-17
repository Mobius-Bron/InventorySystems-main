// Copyright AmberAeolian. All Rights Reserved.

#include "Items/IC_ItemManifest.h"

#include "DS_DebugFunctionLibrary.h"
#include "Items/IC_InventoryItem.h"
#include "Items/IC_ItemFragment.h"
#include "Components/IC_ItemComponent.h"

UIC_InventoryItem* FIC_ItemManifest::Manifest(UObject* NewOuter)
{
	UIC_InventoryItem* NewItem = NewObject<UIC_InventoryItem>(NewOuter);
	NewItem->SetItemManifest(*this);

	for (auto& Fragment : NewItem->GetItemManifestMutable().GetFragmentsMutable())
	{
		Fragment.GetMutable().Manifest();
	}

	ClearFragments();

	DS_LOG(Inventory, "ItemManifest::Manifest | ItemType=%s", *ItemType.ToString());

	return NewItem;
}

void FIC_ItemManifest::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!IsValid(PickupActorClass)) return;

	AActor* PickupActor = WorldContextObject->GetWorld()->SpawnActor(PickupActorClass, &SpawnLocation, &SpawnRotation);
	if (!IsValid(PickupActor)) return;

	if (UIC_ItemComponent* ItemComp = PickupActor->FindComponentByClass<UIC_ItemComponent>())
	{
		ItemComp->InitItemManifest(*this);
	}

	DS_PRINT(Interaction, 2.f, DSColors::Cyan, "SpawnPickupActor | %s at %s", *ItemType.ToString(), *SpawnLocation.ToString());
}

void FIC_ItemManifest::ClearFragments()
{
	Fragments.Empty();
}