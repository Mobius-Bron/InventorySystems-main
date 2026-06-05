#include "Items/Fragments/MIS_FragmentTags.h"

namespace MIS_FragmentTags
{
	UE_DEFINE_GAMEPLAY_TAG(GridFragment, "FragmentTags.Default.GridFragment")
	UE_DEFINE_GAMEPLAY_TAG(IconFragment, "FragmentTags.Default.IconFragment")
	UE_DEFINE_GAMEPLAY_TAG(StackableFragment, "FragmentTags.Default.StackableFragment")
	
	UE_DEFINE_GAMEPLAY_TAG(ConsumableFragment, "FragmentTags.Consumable.ConsumableFragment")
	
	UE_DEFINE_GAMEPLAY_TAG(EquipmentFragment, "FragmentTags.Equip.EquipmentFragment")

	UE_DEFINE_GAMEPLAY_TAG(ItemNameFragment, "FragmentTags.Description.ItemNameFragment")
	UE_DEFINE_GAMEPLAY_TAG(PrimaryStatFragment, "FragmentTags.Description.PrimaryStatFragment")

	UE_DEFINE_GAMEPLAY_TAG(ItemTypeFragment, "FragmentTags.Description.ItemTypeFragment")
	UE_DEFINE_GAMEPLAY_TAG(FlavorTextFragment, "FragmentTags.Description.FlavorTextFragment")
	UE_DEFINE_GAMEPLAY_TAG(SellValueFragment, "FragmentTags.Description.SellValueFragment")
	UE_DEFINE_GAMEPLAY_TAG(RequiredLevelFragment, "FragmentTags.Description.RequiredLevelFragment")

	namespace StatMod
	{
		UE_DEFINE_GAMEPLAY_TAG(StatMod_1, "FragmentTags.Description.StatMod.1")
		UE_DEFINE_GAMEPLAY_TAG(StatMod_2, "FragmentTags.Description.StatMod.2")
		UE_DEFINE_GAMEPLAY_TAG(StatMod_3, "FragmentTags.Description.StatMod.3")
	}
}
