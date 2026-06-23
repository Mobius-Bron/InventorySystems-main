// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Inventory/HoverItem/SBI_HoverItem.h"
#include "Items/IC_InventoryItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void USBI_HoverItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void USBI_HoverItem::UpdateStackCount(const int32 Count)
{
	StackCount = Count;
	if (Count > 1)
	{
		Text_StackCount->SetText(FText::AsNumber(Count));
		Text_StackCount->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FGameplayTag USBI_HoverItem::GetItemType() const
{
	if (InventoryItem.IsValid())
	{
		return InventoryItem->GetItemManifest().GetItemType();
	}
	return FGameplayTag();
}

void USBI_HoverItem::SetIsStackable(bool bStacks)
{
	bIsStackable = bStacks;
	if (!bStacks)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UIC_InventoryItem* USBI_HoverItem::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void USBI_HoverItem::SetInventoryItem(UIC_InventoryItem* Item)
{
	InventoryItem = Item;
}