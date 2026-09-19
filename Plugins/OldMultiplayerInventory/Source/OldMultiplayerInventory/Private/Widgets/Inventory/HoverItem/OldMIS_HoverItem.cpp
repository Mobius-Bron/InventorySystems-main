#include "Widgets/Inventory/HoverItem/OldMIS_HoverItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/OldMIS_InventoryItem.h"

void UOldMIS_HoverItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void UOldMIS_HoverItem::UpdateStackCount(const int32 Count)
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

FGameplayTag UOldMIS_HoverItem::GetItemType() const
{
	if (InventoryItem.IsValid())
	{
		return InventoryItem->GetItemManifest().GetItemType();
	}
	return FGameplayTag();
}

void UOldMIS_HoverItem::SetIsStackable(bool bStacks)
{
	bIsStackable = bStacks;
	if (!bStacks)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UOldMIS_InventoryItem* UOldMIS_HoverItem::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void UOldMIS_HoverItem::SetInventoryItem(UOldMIS_InventoryItem* Item)
{
	InventoryItem = Item;
}
