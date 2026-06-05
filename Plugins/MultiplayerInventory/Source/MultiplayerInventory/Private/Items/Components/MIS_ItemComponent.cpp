#include "Items/Components/MIS_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Items/Manifest/MIS_ItemManifest.h"

UMIS_ItemComponent::UMIS_ItemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UMIS_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemManifest);
}

void UMIS_ItemComponent::InitItemManifest(FMIS_ItemManifest CopyOfManifest)
{
	ItemManifest = FInstancedStruct::Make<FMIS_ItemManifest>(MoveTemp(CopyOfManifest));
}

const FMIS_ItemManifest& UMIS_ItemComponent::GetItemManifest() const
{
	return ItemManifest.Get<FMIS_ItemManifest>();
}

FMIS_ItemManifest& UMIS_ItemComponent::GetItemManifestMutable()
{
	return ItemManifest.GetMutable<FMIS_ItemManifest>();
}

void UMIS_ItemComponent::PickedUp()
{
	OnPickedUp();
	GetOwner()->Destroy();
}
