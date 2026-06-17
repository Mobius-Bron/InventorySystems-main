// Copyright AmberAeolian. All Rights Reserved.

#include "Components/IC_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Items/IC_ItemManifest.h"

UIC_ItemComponent::UIC_ItemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UIC_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemManifest);
}

void UIC_ItemComponent::InitItemManifest(FIC_ItemManifest CopyOfManifest)
{
	ItemManifest = FInstancedStruct::Make<FIC_ItemManifest>(MoveTemp(CopyOfManifest));
}

const FIC_ItemManifest& UIC_ItemComponent::GetItemManifest() const
{
	return ItemManifest.Get<FIC_ItemManifest>();
}

FIC_ItemManifest& UIC_ItemComponent::GetItemManifestMutable()
{
	return ItemManifest.GetMutable<FIC_ItemManifest>();
}

void UIC_ItemComponent::PickedUp()
{
	OnPickedUp();
	GetOwner()->Destroy();
}