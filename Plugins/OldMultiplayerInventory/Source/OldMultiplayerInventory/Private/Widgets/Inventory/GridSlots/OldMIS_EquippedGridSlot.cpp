#include "Widgets/Inventory/GridSlots/OldMIS_EquippedGridSlot.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Items/OldMIS_InventoryItem.h"
#include "Items/Fragments/OldMIS_FragmentTags.h"
#include "Items/Fragments/OldMIS_ItemFragment.h"
#include "Widgets/Inventory/SlottedItems/OldMIS_EquippedSlottedItem.h"

void UOldMIS_EquippedGridSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
}

void UOldMIS_EquippedGridSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
}

FReply UOldMIS_EquippedGridSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	EquippedGridSlotClicked.Broadcast(this, EquipmentTypeTag);
	return FReply::Handled();
}

UOldMIS_EquippedSlottedItem* UOldMIS_EquippedGridSlot::OnItemEquipped(UOldMIS_InventoryItem* Item, const FGameplayTag& EquipmentTag, float TileSize)
{
	if (!EquipmentTag.MatchesTagExact(EquipmentTypeTag)) return nullptr;

	const FOldMIS_GridFragment* GridFragment = GetFragment<FOldMIS_GridFragment>(Item, OldMIS_FragmentTags::GridFragment);
	if (!GridFragment) return nullptr;
	const FIntPoint GridDimensions = GridFragment->GetGridSize();

	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	const FVector2D DrawSize = GridDimensions * IconTileWidth;

	EquippedSlottedItem = CreateWidget<UOldMIS_EquippedSlottedItem>(GetOwningPlayer(), EquippedSlottedItemClass);

	EquippedSlottedItem->SetInventoryItem(Item);
	EquippedSlottedItem->SetEquipmentTypeTag(EquipmentTag);
	EquippedSlottedItem->UpdateStackCount(0);

	SetInventoryItem(Item);

	const FOldMIS_ImageFragment* ImageFragment = GetFragment<FOldMIS_ImageFragment>(Item, OldMIS_FragmentTags::IconFragment);
	if (!ImageFragment) return nullptr;

	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = DrawSize;

	EquippedSlottedItem->SetImageBrush(Brush);

	Overlay_Root->AddChildToOverlay(EquippedSlottedItem);
	FGeometry OverlayGeometry = Overlay_Root->GetCachedGeometry();
	auto OverlaySize = OverlayGeometry.Size;

	const float LeftPadding = OverlaySize.X / 2.f - DrawSize.X / 2.f;
	const float TopPadding = OverlaySize.Y / 2.f - DrawSize.Y / 2.f;

	UOverlaySlot* OverlaySlot = UWidgetLayoutLibrary::SlotAsOverlaySlot(EquippedSlottedItem);
	OverlaySlot->SetPadding(FMargin(LeftPadding, TopPadding));

	Image_GrayedOutIcon->SetVisibility(ESlateVisibility::Collapsed);
	SetOccupiedTexture();
	SetAvailable(false);

	return EquippedSlottedItem;
}

void UOldMIS_EquippedGridSlot::ClearEquippedState()
{
	Image_GrayedOutIcon->SetVisibility(ESlateVisibility::Visible);
	SetUnoccupiedTexture();
	SetAvailable(true);
}
