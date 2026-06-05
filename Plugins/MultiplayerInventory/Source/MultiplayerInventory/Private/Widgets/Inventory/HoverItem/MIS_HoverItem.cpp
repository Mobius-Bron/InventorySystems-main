#include "Widgets/Inventory/HoverItem/MIS_HoverItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/MIS_InventoryItem.h"

void UMIS_HoverItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void UMIS_HoverItem::UpdateStackCount(const int32 Count)
{
	StackCount = Count;
	// 修改为大于1才显示
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

FGameplayTag UMIS_HoverItem::GetItemType() const
{
	if (InventoryItem.IsValid())
	{
		return InventoryItem->GetItemManifest().GetItemType();
	}
	return FGameplayTag();
}

void UMIS_HoverItem::SetIsStackable(bool bStacks)
{
	bIsStackable = bStacks;
	if (!bStacks)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UMIS_InventoryItem* UMIS_HoverItem::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void UMIS_HoverItem::SetInventoryItem(UMIS_InventoryItem* Item)
{
	InventoryItem = Item;
}
