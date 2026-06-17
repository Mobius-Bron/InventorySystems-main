// Copyright AmberAeolian. All Rights Reserved.

#include "Items/IC_ItemFragment.h"

void FIC_LabeledNumberFragment::Manifest()
{
	FIC_InventoryItemFragment::Manifest();

	if (bRandomizeOnManifest)
	{
		Value = FMath::FRandRange(Min, Max);
	}
	bRandomizeOnManifest = false;
}