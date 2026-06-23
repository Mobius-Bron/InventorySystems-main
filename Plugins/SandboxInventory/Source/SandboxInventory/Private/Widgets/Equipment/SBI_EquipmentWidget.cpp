// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Equipment/SBI_EquipmentWidget.h"

#include "DS_DebugFunctionLibrary.h"
#include "SandboxInventory.h"
#include "Components/CanvasPanel.h"
#include "InventoryManagement/Components/SBI_InventoryComponent.h"
#include "Items/IC_InventoryItem.h"
#include "Items/IC_ItemFragment.h"
#include "Items/Fragments/SBI_ItemFragment.h"
#include "Widgets/Equipment/SBI_EquipmentSlot.h"
#include "Widgets/Inventory/HoverItem/SBI_HoverItem.h"
#include "Widgets/Inventory/Spatial/SBI_InventoryGrid.h"

void USBI_EquipmentWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 绑定代码创建的槽位事件
	for (USBI_EquipmentSlot* EquipSlot : EquipmentSlots)
	{
		if (IsValid(EquipSlot))
		{
			EquipSlot->EquipmentSlotClicked.AddDynamic(this, &ThisClass::OnEquipmentSlotClicked);
		}
	}
}

void USBI_EquipmentWidget::InitFromComponent(USBI_InventoryComponent* InInventoryComponent, USBI_InventoryGrid* InInventoryGrid)
{
	InventoryComponent = InInventoryComponent;
	InventoryGrid = InInventoryGrid;

	DS_PRINT(Inventory, 4.f, DSColors::Orange,
		"[沙盒装备UI] InitFromComponent | InvComp=%s | Grid=%s",
		IsValid(InInventoryComponent) ? TEXT("有效") : TEXT("空"),
		IsValid(InInventoryGrid) ? TEXT("有效") : TEXT("空"));

	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemEquipped.AddDynamic(this, &ThisClass::OnItemEquipped);
		InventoryComponent->OnItemUnequipped.AddDynamic(this, &ThisClass::OnItemUnequipped);
		DS_LOG(Inventory, "沙盒装备UI: 已绑定 OnItemEquipped + OnItemUnequipped");
	}
}

void USBI_EquipmentWidget::OpenEquipment()
{
	SetVisibility(ESlateVisibility::Visible);
	RefreshEquipmentSlots();
	DS_LOG(Inventory, "装备界面已打开 | 槽位数=%d", EquipmentSlots.Num());
}

void USBI_EquipmentWidget::CloseEquipment()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void USBI_EquipmentWidget::RefreshEquipmentSlots()
{
	if (!InventoryComponent.IsValid()) return;

	for (USBI_EquipmentSlot* EquipSlot : EquipmentSlots)
	{
		if (!IsValid(EquipSlot)) continue;

		// 查找匹配的物品
		const FGameplayTag& SlotType = EquipSlot->GetEquipmentTypeTag();
		UIC_InventoryItem* FoundItem = nullptr;

		for (UIC_InventoryItem* Item : InventoryComponent->GetAllItems())
		{
			if (!IsValid(Item)) continue;

			FSBI_EquipmentFragment* EquipFrag = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FSBI_EquipmentFragment>();
			if (EquipFrag && EquipFrag->bEquipped && EquipFrag->GetEquipmentType().MatchesTagExact(SlotType))
			{
				FoundItem = Item;
				break;
			}
		}

		EquipSlot->SetInventoryItem(FoundItem);
	}
}

void USBI_EquipmentWidget::OnEquipmentSlotClicked(const FGameplayTag& EquipmentTypeTag, const FPointerEvent& MouseEvent)
{
	DS_LOG(Inventory, "装备槽位点击: %s", *EquipmentTypeTag.ToString());

	if (!InventoryComponent.IsValid()) return;

	// 查找当前已装备到该槽位的物品
	UIC_InventoryItem* CurrentlyEquipped = nullptr;
	for (UIC_InventoryItem* Item : InventoryComponent->GetAllItems())
	{
		if (!IsValid(Item)) continue;
		FSBI_EquipmentFragment* EquipFrag = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FSBI_EquipmentFragment>();
		if (EquipFrag && EquipFrag->bEquipped && EquipFrag->GetEquipmentType().MatchesTagExact(EquipmentTypeTag))
		{
			CurrentlyEquipped = Item;
			break;
		}
	}

	if (CurrentlyEquipped)
	{
		// 卸下
		DS_LOG(Inventory, "卸下装备: %s", *CurrentlyEquipped->GetName());
		InventoryComponent->RequestEquipSlotClicked(nullptr, CurrentlyEquipped);
	}
	else if (InventoryGrid.IsValid() && InventoryGrid->HasHoverItem())
	{
		// 尝试装备拖拽中的物品
		UIC_InventoryItem* HoverItem = InventoryGrid->GetHoverItem()->GetInventoryItem();
		if (IsValid(HoverItem))
		{
			TryEquipItem(HoverItem, EquipmentTypeTag);
		}
	}
}

void USBI_EquipmentWidget::TryEquipItem(UIC_InventoryItem* Item, const FGameplayTag& EquipmentTypeTag)
{
	if (!IsValid(Item) || !InventoryComponent.IsValid()) return;

	FSBI_EquipmentFragment* EquipFrag = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FSBI_EquipmentFragment>();
	if (!EquipFrag)
	{
		DS_LOG_WARN(Inventory, "装备失败: 物品 %s 没有 EquipmentFragment", *Item->GetName());
		return;
	}

	if (!EquipFrag->GetEquipmentType().MatchesTag(EquipmentTypeTag))
	{
		DS_LOG_WARN(Inventory, "装备失败: 类型不匹配 | 物品=%s | 需要=%s",
			*EquipFrag->GetEquipmentType().ToString(), *EquipmentTypeTag.ToString());
		return;
	}

	// 查找该槽位当前已装备的物品
	UIC_InventoryItem* CurrentlyEquipped = nullptr;
	for (UIC_InventoryItem* EquippedItem : InventoryComponent->GetAllItems())
	{
		if (!IsValid(EquippedItem) || EquippedItem == Item) continue;
		FSBI_EquipmentFragment* Frag = EquippedItem->GetItemManifestMutable().GetFragmentOfTypeMutable<FSBI_EquipmentFragment>();
		if (Frag && Frag->bEquipped && Frag->GetEquipmentType().MatchesTagExact(EquipmentTypeTag))
		{
			CurrentlyEquipped = EquippedItem;
			break;
		}
	}

	DS_LOG(Inventory, "装备物品: %s -> 槽位=%s | 卸下=%s",
		*Item->GetName(), *EquipmentTypeTag.ToString(),
		CurrentlyEquipped ? *CurrentlyEquipped->GetName() : TEXT("空"));

	InventoryComponent->RequestEquipSlotClicked(Item, CurrentlyEquipped);
}

void USBI_EquipmentWidget::OnItemEquipped(UIC_InventoryItem* Item)
{
	DS_LOG(Inventory, "装备UI: 收到装备事件 | Item=%s", IsValid(Item) ? *Item->GetName() : TEXT("空"));
	RefreshEquipmentSlots();
}

void USBI_EquipmentWidget::OnItemUnequipped(UIC_InventoryItem* Item)
{
	DS_LOG(Inventory, "装备UI: 收到卸下事件 | Item=%s", IsValid(Item) ? *Item->GetName() : TEXT("空"));
	RefreshEquipmentSlots();
}