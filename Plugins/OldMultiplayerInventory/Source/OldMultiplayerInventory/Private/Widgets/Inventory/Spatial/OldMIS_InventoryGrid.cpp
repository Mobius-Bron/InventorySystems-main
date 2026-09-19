#include "Widgets/Inventory/Spatial/OldMIS_InventoryGrid.h"

#include "DH_DebugFunctionLibrary.h"
#include "OldMultiplayerInventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/OldMIS_InventoryComponent.h"
#include "BlueprintFunctionLibraries/OldMIS_InventoryFunctionLibrary.h"
#include "BlueprintFunctionLibraries/OldMIS_WidgetFunctionLibrary.h"
#include "Items/OldMIS_InventoryItem.h"
#include "Items/Components/OldMIS_ItemComponent.h"
#include "Items/Fragments/OldMIS_FragmentTags.h"
#include "Items/Fragments/OldMIS_ItemFragment.h"
#include "Widgets/Inventory/GridSlots/OldMIS_GridSlot.h"
#include "Items/Manifest/OldMIS_ItemManifest.h"
#include "Widgets/Inventory/HoverItem/OldMIS_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/OldMIS_SlottedItem.h"
#include "Widgets/ItemPopUp/OldMIS_ItemPopUp.h"
#include "Widgets/ItemDescription/OldMIS_ItemDescription.h"
#include "Widgets/Inventory/GridSlots/OldMIS_EquippedGridSlot.h"
#include "Widgets/Inventory/SlottedItems/OldMIS_EquippedSlottedItem.h"

void UOldMIS_InventoryGrid::NativePreConstruct()
{
	Super::NativePreConstruct();

	ClearGrid();
	ConstructGrid();
}

void UOldMIS_InventoryGrid::InitFromComponent(UOldMIS_InventoryComponent* InInventoryComponent, UCanvasPanel* InCanvasPanel)
{
	InventoryComponent = InInventoryComponent;
	OwningCanvasPanel = InCanvasPanel;

	DH_SCREEN(5.f, DHColors::Orange,
		"[背包网格] InitFromComponent | InvComp=%s | Canvas=%s | GridSlots=%d",
		IsValid(InInventoryComponent) ? TEXT("有效") : TEXT("空"),
		IsValid(InCanvasPanel) ? TEXT("有效") : TEXT("空"),
		GridSlots.Num());

	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
		InventoryComponent->OnItemRemoved.AddDynamic(this, &ThisClass::OnExternalItemRemoved);
		InventoryComponent->OnStackChange.AddDynamic(this, &ThisClass::AddStacks);
		DH_SCREEN(3.f, DHColors::Orange, "[背包网格] 已绑定 OnItemAdded + OnItemRemoved + OnStackChange 委托");
	}

	BindEquippedGridSlotDelegates();
}

void UOldMIS_InventoryGrid::ConstructGrid()
{
	if (!GridSlotClass || Columns <= 0 || Rows <= 0) return;
	if (!CanvasPanel) return;

	GridSlots.Reserve(Rows * Columns);

	for (int32 j = 0; j < Rows; ++j)
	{
		for (int32 i = 0; i < Columns; ++i)
		{
			UOldMIS_GridSlot* GridSlot = CreateWidget<UOldMIS_GridSlot>(this, GridSlotClass);
			CanvasPanel->AddChild(GridSlot);

			const FIntPoint TilePosition(i, j);
			GridSlot->SetTileIndex(UOldMIS_WidgetFunctionLibrary::GetIndexFromPosition(TilePosition, Columns));

			UCanvasPanelSlot* GridCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			GridCPS->SetSize(FVector2D(TileSize));
			GridCPS->SetPosition(TilePosition * TileSize);

			GridSlots.Add(GridSlot);
			GridSlot->SetUnoccupiedTexture();
			GridSlot->GridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
			GridSlot->GridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
			GridSlot->GridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnhovered);
		}
	}
}

void UOldMIS_InventoryGrid::ClearGrid()
{
	for (UOldMIS_GridSlot* GridSlot : GridSlots)
	{
		if (IsValid(GridSlot))
		{
			GridSlot->RemoveFromParent();
		}
	}
	GridSlots.Empty();
}

void UOldMIS_InventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CanvasPanel) return;

	if (IsValid(ItemDescription))
	{
		SetItemDescriptionSizeAndPosition(ItemDescription, CanvasPanel);
	}

	if (IsValid(EquippedItemDescription))
	{
		SetEquippedItemDescriptionSizeAndPosition(EquippedItemDescription, CanvasPanel);
	}

	const FGeometry& CanvasGeo = CanvasPanel->GetCachedGeometry();
	FVector2D _, CanvasPosition;
	USlateBlueprintLibrary::LocalToViewport(CanvasPanel, CanvasGeo, USlateBlueprintLibrary::GetLocalTopLeft(CanvasGeo), _, CanvasPosition);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	if (CursorExitedCanvas(CanvasPosition, UOldMIS_WidgetFunctionLibrary::GetWidgetSize(CanvasPanel), MousePosition))
	{
		return;
	}

	UpdateTileParameters(CanvasPosition, MousePosition);
}

void UOldMIS_InventoryGrid::UpdateTileParameters(const FVector2D& CanvasPosition, const FVector2D& MousePosition)
{
	if (!bMouseWithinCanvas) return;

	const FIntPoint HoveredTileCoordinates = CalculateHoveredCoordinates(CanvasPosition, MousePosition);

	LastTileParameters = TileParameters;
	TileParameters.TileCoordinats = HoveredTileCoordinates;
	TileParameters.TileIndex = UOldMIS_WidgetFunctionLibrary::GetIndexFromPosition(HoveredTileCoordinates, Columns);
	TileParameters.TileQuadrant = CalculateTileQuadrant(CanvasPosition, MousePosition);

	OnTileParametersUpdated(TileParameters);
}

void UOldMIS_InventoryGrid::OnTileParametersUpdated(const FOldMIS_TileParameters& Parameters)
{
	if (!IsValid(HoverItem)) return;

	const FIntPoint Dimensions = HoverItem->GetGridDimensions();
	const FIntPoint StartingCoordinate = CalculateStartingCoordinate(Parameters.TileCoordinats, Dimensions, Parameters.TileQuadrant);
	ItemDropIndex = UOldMIS_WidgetFunctionLibrary::GetIndexFromPosition(StartingCoordinate, Columns);

	CurrentQueryResult = CheckHoverPosition(StartingCoordinate, Dimensions);

	if (CurrentQueryResult.bHasSpace)
	{
		HighlightSlots(ItemDropIndex, Dimensions);
		return;
	}
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);

	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		const FOldMIS_GridFragment* GridFragment = GetFragment<FOldMIS_GridFragment>(CurrentQueryResult.ValidItem.Get(), OldMIS_FragmentTags::GridFragment);
		if (!GridFragment) return;

		ChangeHoverType(CurrentQueryResult.UpperLeftIndex, GridFragment->GetGridSize(), EOldMIS_GridSlotState::GrayedOut);
	}
}

FOldMIS_SpaceQueryResult UOldMIS_InventoryGrid::CheckHoverPosition(const FIntPoint& Position, const FIntPoint& Dimensions)
{
	FOldMIS_SpaceQueryResult Result;

	if (!IsInGridBounds(UOldMIS_WidgetFunctionLibrary::GetIndexFromPosition(Position, Columns), Dimensions)) return Result;

	Result.bHasSpace = true;

	TSet<int32> OccupiedUpperLeftIndices;
	UOldMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, UOldMIS_WidgetFunctionLibrary::GetIndexFromPosition(Position, Columns), Dimensions, Columns, [&](const UOldMIS_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
			Result.bHasSpace = false;
		}
	});

	if (OccupiedUpperLeftIndices.Num() == 1)
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateConstIterator();
		Result.ValidItem = GridSlots[Index]->GetInventoryItem();
		Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	}

	return Result;
}

bool UOldMIS_InventoryGrid::CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location)
{
	bLastMouseWithinCanvas = bMouseWithinCanvas;
	bMouseWithinCanvas = UOldMIS_WidgetFunctionLibrary::IsWithinBounds(BoundaryPos, BoundarySize, Location);
	if (!bMouseWithinCanvas && bLastMouseWithinCanvas)
	{
		UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
		return true;
	}
	return false;
}

void UOldMIS_InventoryGrid::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	if (!bMouseWithinCanvas) return;
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UOldMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UOldMIS_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();
	});
	LastHighlightedDimensions = Dimensions;
	LastHighlightedIndex = Index;
}

void UOldMIS_InventoryGrid::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UOldMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UOldMIS_GridSlot* GridSlot)
	{
		if (GridSlot->IsAvailable())
		{
			GridSlot->SetUnoccupiedTexture();
		}
		else
		{
			GridSlot->SetOccupiedTexture();
		}
	});
}

void UOldMIS_InventoryGrid::ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EOldMIS_GridSlotState GridSlotState)
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UOldMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [State = GridSlotState](UOldMIS_GridSlot* GridSlot)
	{
		switch (State)
		{
		case EOldMIS_GridSlotState::Occupied:
			GridSlot->SetOccupiedTexture();
			break;
		case EOldMIS_GridSlotState::Unoccupied:
			GridSlot->SetUnoccupiedTexture();
			break;
		case EOldMIS_GridSlotState::GrayedOut:
			GridSlot->SetGrayedOutTexture();
			break;
		case EOldMIS_GridSlotState::Selected:
			GridSlot->SetSelectedTexture();
			break;
		}
	});

	LastHighlightedIndex = Index;
	LastHighlightedDimensions = Dimensions;
}

FIntPoint UOldMIS_InventoryGrid::CalculateStartingCoordinate(const FIntPoint& Coordinate, const FIntPoint& Dimensions, const EOldMIS_TileQuadrant Quadrant) const
{
	const int32 HasEvenWidth = Dimensions.X % 2 == 0 ? 1 : 0;
	const int32 HasEvenHeight = Dimensions.Y % 2 == 0 ? 1 : 0;

	FIntPoint StartingCoord;
	switch (Quadrant)
	{
		case EOldMIS_TileQuadrant::TopLeft:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
			break;
		case EOldMIS_TileQuadrant::TopRight:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEvenWidth;
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
			break;
		case EOldMIS_TileQuadrant::BottomLeft:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEvenHeight;
			break;
		case EOldMIS_TileQuadrant::BottomRight:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEvenWidth;
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEvenHeight;
			break;
	default:
		DH_LOG_ERR("Invalid Quadrant.");
		return FIntPoint(-1, -1);
	}
	return StartingCoord;
}

FIntPoint UOldMIS_InventoryGrid::CalculateHoveredCoordinates(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const
{
	return FIntPoint{
		static_cast<int32>(FMath::FloorToInt((MousePosition.X - CanvasPosition.X) / TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePosition.Y - CanvasPosition.Y) / TileSize))
	};
}

EOldMIS_TileQuadrant UOldMIS_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const
{
	const float TileLocalX = FMath::Fmod(MousePosition.X - CanvasPosition.X, TileSize);
	const float TileLocalY = FMath::Fmod(MousePosition.Y - CanvasPosition.Y, TileSize);

	const bool bIsTop = TileLocalY < TileSize / 2.f;
	const bool bIsLeft = TileLocalX < TileSize / 2.f;

	EOldMIS_TileQuadrant HoveredTileQuadrant{EOldMIS_TileQuadrant::None};
	if (bIsTop && bIsLeft) HoveredTileQuadrant = EOldMIS_TileQuadrant::TopLeft;
	else if (bIsTop && !bIsLeft) HoveredTileQuadrant = EOldMIS_TileQuadrant::TopRight;
	else if (!bIsTop && bIsLeft) HoveredTileQuadrant = EOldMIS_TileQuadrant::BottomLeft;
	else if (!bIsTop && !bIsLeft) HoveredTileQuadrant = EOldMIS_TileQuadrant::BottomRight;

	return HoveredTileQuadrant;
}

FOldMIS_SlotAvailabilityResult UOldMIS_InventoryGrid::HasRoomForItem(const UOldMIS_ItemComponent* ItemComponent)
{
	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FOldMIS_SlotAvailabilityResult UOldMIS_InventoryGrid::HasRoomForItem(const UOldMIS_InventoryItem* Item, const int32 StackAmountOverride)
{
	return HasRoomForItem(Item->GetItemManifest(), StackAmountOverride);
}

FOldMIS_SlotAvailabilityResult UOldMIS_InventoryGrid::HasRoomForItem(const FOldMIS_ItemManifest& Manifest, const int32 StackAmountOverride)
{
	// ---- 步骤1: 获取物品的堆叠信�?----
	FOldMIS_SlotAvailabilityResult Result;

	const FOldMIS_StackableFragment* StackableFragment = Manifest.GetFragmentOfType<FOldMIS_StackableFragment>();
	Result.bStackable = StackableFragment != nullptr;

	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 1;
	int32 AmountToFill = StackableFragment ? StackableFragment->GetStackCount() : 1;
	if (StackAmountOverride != -1 && Result.bStackable)
	{
		AmountToFill = StackAmountOverride;
	}

	// ---- 步骤2: 逐个槽位检查是否有空间 ----
	TSet<int32> CheckedIndices;
	for (const auto& GridSlot : GridSlots)
	{
		// 全部填充完毕
		if (AmountToFill == 0) break;

		// 跳过已检查的槽位
		if (IsIndexClaimed(CheckedIndices, GridSlot->GetIndex())) continue;

		// 检查物品尺寸是否越界	
		if (!IsInGridBounds(GridSlot->GetIndex(), GetItemDimensions(Manifest))) continue;

		// ---- 步骤3: 检查该槽位区域是否可容下
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(GridSlot, GetItemDimensions(Manifest), CheckedIndices, TentativelyClaimed, Manifest.GetItemType(), MaxStackSize))
		{
			continue; // 该区域不可用,跳过
		}

		// ---- 步骤4: 计算该区域可填充的数�?----
		const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlot);
		if (AmountToFillInSlot == 0) continue;

		// 标记该区域为已占用
		CheckedIndices.Append(TentativelyClaimed);

		// ---- 步骤5: 记录可用性结果----
		Result.TotalRoomToFill += AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
			FOldMIS_SlotAvailability{
				HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetIndex(),
				Result.bStackable ? AmountToFillInSlot : 0,
				HasValidItem(GridSlot)
			}
		);

		AmountToFill -= AmountToFillInSlot;
		Result.Remainder = AmountToFill;

		if (AmountToFill == 0) return Result; // 全部完成
	}

	return Result;
}

bool UOldMIS_InventoryGrid::HasRoomAtIndex(const UOldMIS_GridSlot* GridSlot, const FIntPoint& Dimensions, const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize)
{
	bool bHasRoomAtIndex = true;
	UOldMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, GridSlot->GetIndex(), Dimensions, Columns, [&](const UOldMIS_GridSlot* SubGridSlot)
	{
		if (CheckSlotConstraints(GridSlot, SubGridSlot, CheckedIndices, OutTentativelyClaimed, ItemType, MaxStackSize))
		{
			OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		}
		else
		{
			bHasRoomAtIndex = false;
		}
	});

	return bHasRoomAtIndex;
}

bool UOldMIS_InventoryGrid::CheckSlotConstraints(const UOldMIS_GridSlot* GridSlot, const UOldMIS_GridSlot* SubGridSlot, const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize) const
{
	if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetIndex())) return false;

	if (!HasValidItem(SubGridSlot))
	{
		OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		return true;
	}

	if (!IsUpperLeftSlot(GridSlot, SubGridSlot)) return false;

	const UOldMIS_InventoryItem* SubItem = SubGridSlot->GetInventoryItem().Get();
	if (!SubItem->IsStackable()) return false;

	if (!DoesItemTypeMatch(SubItem, ItemType)) return false;

	if (GridSlot->GetStackCount() >= MaxStackSize) return false;

	return true;
}

FIntPoint UOldMIS_InventoryGrid::GetItemDimensions(const FOldMIS_ItemManifest& Manifest) const
{
	const FOldMIS_GridFragment* GridFragment = Manifest.GetFragmentOfType<FOldMIS_GridFragment>();
	return GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
}

bool UOldMIS_InventoryGrid::HasValidItem(const UOldMIS_GridSlot* GridSlot) const
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool UOldMIS_InventoryGrid::IsUpperLeftSlot(const UOldMIS_GridSlot* GridSlot, const UOldMIS_GridSlot* SubGridSlot) const
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetIndex();
}

bool UOldMIS_InventoryGrid::DoesItemTypeMatch(const UOldMIS_InventoryItem* SubItem, const FGameplayTag& ItemType) const
{
	return SubItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
}

bool UOldMIS_InventoryGrid::IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const
{
	if (StartIndex < 0 || StartIndex >= GridSlots.Num()) return false;
	const int32 EndColumn = (StartIndex % Columns) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / Columns) + ItemDimensions.Y;
	return EndColumn <= Columns && EndRow <= Rows;
}

int32 UOldMIS_InventoryGrid::DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize, const int32 AmountToFill, const UOldMIS_GridSlot* GridSlot) const
{
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(GridSlot);
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
}

int32 UOldMIS_InventoryGrid::GetStackAmount(const UOldMIS_GridSlot* GridSlot) const
{
	int32 CurrentSlotStackCount = GridSlot->GetStackCount();
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		UOldMIS_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
		CurrentSlotStackCount = UpperLeftGridSlot->GetStackCount();
	}
	return CurrentSlotStackCount;
}

bool UOldMIS_InventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UOldMIS_InventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

void UOldMIS_InventoryGrid::PickUp(UOldMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] PickUp | idx=%d", GridIndex);
	AssignHoverItem(ClickedInventoryItem, GridIndex, GridIndex);
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
}

void UOldMIS_InventoryGrid::AssignHoverItem(UOldMIS_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex)
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] AssignHoverItem | idx=%d | prev=%d", GridIndex, PreviousGridIndex);

	AssignHoverItem(InventoryItem);

	HoverItem->SetPreviousGridIndex(PreviousGridIndex);
	HoverItem->UpdateStackCount(InventoryItem->IsStackable() ? GridSlots[GridIndex]->GetStackCount() : 0);
}

void UOldMIS_InventoryGrid::RemoveItemFromGrid(UOldMIS_InventoryItem* InventoryItem, const int32 GridIndex)
{
	const FOldMIS_GridFragment* GridFragment = GetFragment<FOldMIS_GridFragment>(InventoryItem, OldMIS_FragmentTags::GridFragment);

	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] RemoveItemFromGrid | idx=%d | GridFrag=%s",
		GridIndex, GridFragment ? TEXT("有效") : TEXT("空"));

	if (!GridFragment) return;

	UOldMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, GridIndex, GridFragment->GetGridSize(), Columns, [&](UOldMIS_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(nullptr);
		GridSlot->SetUpperLeftIndex(INDEX_NONE);
		GridSlot->SetUnoccupiedTexture();
		GridSlot->SetAvailable(true);
		GridSlot->SetStackCount(0);
	});

	if (SlottedItems.Contains(GridIndex))
	{
		TObjectPtr<UOldMIS_SlottedItem> FoundSlottedItem;
		SlottedItems.RemoveAndCopyValue(GridIndex, FoundSlottedItem);
		FoundSlottedItem->RemoveFromParent();
	}
}

void UOldMIS_InventoryGrid::OnExternalItemRemoved(UOldMIS_InventoryItem* Item)
{
	if (!IsValid(Item)) return;

	for (const auto& Pair : SlottedItems)
	{
		if (Pair.Value && Pair.Value->GetInventoryItem() == Item)
		{
			RemoveItemFromGrid(Item, Pair.Key);
			return;
		}
	}
}

void UOldMIS_InventoryGrid::AssignHoverItem(UOldMIS_InventoryItem* InventoryItem)
{
	if (!IsValid(HoverItem))
	{
		HoverItem = CreateWidget<UOldMIS_HoverItem>(GetOwningPlayer(), HoverItemClass);
	}

	const FOldMIS_GridFragment* GridFragment = GetFragment<FOldMIS_GridFragment>(InventoryItem, OldMIS_FragmentTags::GridFragment);
	const FOldMIS_ImageFragment* ImageFragment = GetFragment<FOldMIS_ImageFragment>(InventoryItem, OldMIS_FragmentTags::IconFragment);
	if (!GridFragment || !ImageFragment) return;

	const FVector2D DrawSize = GetDrawSize(GridFragment);

	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = DrawSize * UWidgetLayoutLibrary::GetViewportScale(this);

	HoverItem->SetImageBrush(IconBrush);
	HoverItem->SetGridDimensions(GridFragment->GetGridSize());
	HoverItem->SetInventoryItem(InventoryItem);
	HoverItem->SetIsStackable(InventoryItem->IsStackable());

	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, HoverItem);
}

void UOldMIS_InventoryGrid::OnHide()
{
	PutHoverItemBack();
}

void UOldMIS_InventoryGrid::AddStacks(const FOldMIS_SlotAvailabilityResult& Result)
{
	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Orange,
		"[背包网格] AddStacks 触发 | 槽位数=%d | 可堆叠=%d",
		Result.SlotAvailabilities.Num(), Result.bStackable);

	if (Result.SlotAvailabilities.Num() == 0 && Result.Item.IsValid())
	{
		for (auto& Pair : SlottedItems)
		{
			if (Pair.Value && Pair.Value->GetInventoryItem() == Result.Item.Get())
			{
				const int32 NewStack = GridSlots[Pair.Key]->GetStackCount() + Result.TotalRoomToFill;
				Pair.Value->UpdateStackCount(NewStack);
				GridSlots[Pair.Key]->SetStackCount(NewStack);
				DH_PRINT(EDH_Output::Both, 3.f, DHColors::Orange,
					"[背包网格] AddStacks 堆叠更新 | idx=%d | 新堆叠=%d", Pair.Key, NewStack);
				return;
			}
		}
		DH_PRINT(EDH_Output::Both, 3.f, FLinearColor::Red, "[背包网格] AddStacks 失败: 找不到物品对应的SlottedItem");
		return;
	}

	for (const auto& Availability : Result.SlotAvailabilities)
	{
		DH_PRINT(EDH_Output::Both, 3.f, DHColors::Orange,
			"[背包网格] AddStacks 槽位 | idx=%d | 已有物品=%d | 填充=%d",
			Availability.Index, Availability.bItemAtIndex, Availability.AmountToFill);

		if (Availability.bItemAtIndex)
		{
			const auto& GridSlot = GridSlots[Availability.Index];
			const auto& SlottedItem = SlottedItems.FindChecked(Availability.Index);
			SlottedItem->UpdateStackCount(GridSlot->GetStackCount() + Availability.AmountToFill);
			GridSlot->SetStackCount(GridSlot->GetStackCount() + Availability.AmountToFill);
		}
		else
		{
			AddItemAtIndex(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
			UpdateGridSlots(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
		}
	}
}

void UOldMIS_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	check(GridSlots.IsValidIndex(GridIndex));
	UOldMIS_InventoryItem* ClickedInventoryItem = GridSlots[GridIndex]->GetInventoryItem().Get();

	DH_SCREEN(2.f, DHColors::Magenta,
		"[背包网格] 槽位点击 | idx=%d | 有HoverItem=%d | 左键=%d | 右键=%d",
		GridIndex, IsValid(HoverItem), IsLeftClick(MouseEvent), IsRightClick(MouseEvent));

	if (!IsValid(HoverItem) && IsLeftClick(MouseEvent))
	{
		DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] >> 情况1: 拾起物品");
		OnItemUnhovered();
		PickUp(ClickedInventoryItem, GridIndex);
		return;
	}

	// ---- 情况2: 右键显示弹出菜单 ----
	if (IsRightClick(MouseEvent))
	{
		CreateItemPopUp(GridIndex);
		return;
	}

	// ---- 情况3: 已有 HoverItem
	if (IsSameStackable(ClickedInventoryItem))
	{
		const int32 ClickedStackCount = GridSlots[GridIndex]->GetStackCount();
		const FOldMIS_StackableFragment* StackableFragment = ClickedInventoryItem->GetItemManifest().GetFragmentOfType<FOldMIS_StackableFragment>();
		const int32 MaxStackSize = StackableFragment->GetMaxStackSize();
		const int32 RoomInClickedSlot = MaxStackSize - ClickedStackCount;
		const int32 HoveredStackCount = HoverItem->GetStackCount();

		if (ShouldSwapStackCounts(RoomInClickedSlot, HoveredStackCount, MaxStackSize))
		{
			SwapStackCounts(ClickedStackCount, HoveredStackCount, GridIndex);
			return;
		}

		if (ShouldConsumeHoverItemStacks(HoveredStackCount, RoomInClickedSlot))
		{
			ConsumeHoverItemStacks(ClickedStackCount, HoveredStackCount, GridIndex);
			return;
		}

		if (ShouldFillInStack(RoomInClickedSlot, HoveredStackCount))
		{
			FillInStack(RoomInClickedSlot, HoveredStackCount - RoomInClickedSlot, GridIndex);
			return;
		}

		if (RoomInClickedSlot == 0)
		{
			return;
		}
	}

	// ---- 情况4: 不同物品,是否可以交换位置 ----
	if (CurrentQueryResult.ValidItem.IsValid())
	{
		SwapWithHoverItem(ClickedInventoryItem, GridIndex);
	}
}

void UOldMIS_InventoryGrid::CreateItemPopUp(const int32 GridIndex)
{
	OnItemUnhovered();

	UOldMIS_InventoryItem* RightClickedItem = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (IsValid(GridSlots[GridIndex]->GetItemPopUp())) return;

	ItemPopUp = CreateWidget<UOldMIS_ItemPopUp>(this, ItemPopUpClass);
	GridSlots[GridIndex]->SetItemPopUp(ItemPopUp);

	OwningCanvasPanel->AddChild(ItemPopUp);
	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemPopUp);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	CanvasSlot->SetPosition(MousePosition - ItemPopUpOffset);
	CanvasSlot->SetSize(ItemPopUp->GetBoxSize());

	const int32 SliderMax = GridSlots[GridIndex]->GetStackCount() - 1;
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		ItemPopUp->OnSplit.BindDynamic(this, &ThisClass::OnPopUpMenuSplit);
		ItemPopUp->SetSliderParams(SliderMax, FMath::Max(1, GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		ItemPopUp->CollapseSplitButton();
	}

	ItemPopUp->OnDrop.BindDynamic(this, &ThisClass::OnPopUpMenuDrop);

	if (RightClickedItem->IsConsumable())
	{
		ItemPopUp->OnConsume.BindDynamic(this, &ThisClass::OnPopUpMenuConsume);
	}
	else
	{
		ItemPopUp->CollapseConsumeButton();
	}
}

void UOldMIS_InventoryGrid::PutHoverItemBack()
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] PutHoverItemBack | 放置悬停物品回原位");

	if (!IsValid(HoverItem)) return;

	FOldMIS_SlotAvailabilityResult Result = HasRoomForItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());
	Result.Item = HoverItem->GetInventoryItem();

	AddStacks(Result);
	ClearHoverItem();
}

void UOldMIS_InventoryGrid::DropItem()
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] DropItem | 丢弃悬停物品");

	OnItemUnhovered();

	if (!IsValid(HoverItem)) return;
	if (!IsValid(HoverItem->GetInventoryItem())) return;
	if (!InventoryComponent.IsValid()) return;

	InventoryComponent->RequestDropItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());

	ClearHoverItem();
	ShowCursor();
}

bool UOldMIS_InventoryGrid::HasHoverItem() const
{
	return IsValid(HoverItem);
}

UOldMIS_HoverItem* UOldMIS_InventoryGrid::GetHoverItem() const
{
	return HoverItem;
}

void UOldMIS_InventoryGrid::AddItem(UOldMIS_InventoryItem* Item)
{
	DH_SCREEN(3.f, DHColors::Orange, "[背包网格] AddItem 触发 | Item=%s",
		IsValid(Item) ? *Item->GetName() : TEXT("空"));

	FOldMIS_SlotAvailabilityResult Result = HasRoomForItem(Item);

	DH_SCREEN(3.f, DHColors::Orange, "[背包网格] HasRoomForItem 结果 | 总空间=%d | 槽位数=%d",
		Result.TotalRoomToFill, Result.SlotAvailabilities.Num());

	AddItemToIndices(Result, Item);
}

void UOldMIS_InventoryGrid::AddItemToIndices(const FOldMIS_SlotAvailabilityResult& Result, UOldMIS_InventoryItem* NewItem)
{
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		AddItemAtIndex(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
		UpdateGridSlots(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
	}
}

void UOldMIS_InventoryGrid::AddItemAtIndex(UOldMIS_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount)
{
	const FOldMIS_GridFragment* GridFragment = GetFragment<FOldMIS_GridFragment>(Item, OldMIS_FragmentTags::GridFragment);
	const FOldMIS_ImageFragment* ImageFragment = GetFragment<FOldMIS_ImageFragment>(Item, OldMIS_FragmentTags::IconFragment);

	DH_SCREEN(3.f, DHColors::Orange,
		"[背包网格] AddItemAtIndex | idx=%d | GridFrag=%s | ImageFrag=%s | Stack=%d",
		Index, GridFragment ? TEXT("有效") : TEXT("空"),
		ImageFragment ? TEXT("有效") : TEXT("空"), StackAmount);

	if (!GridFragment || !ImageFragment) return;

	UOldMIS_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	AddSlottedItemToCanvas(Index, GridFragment, SlottedItem);

	SlottedItems.Add(Index, SlottedItem);

	DH_SCREEN(2.f, DHColors::Orange,
		"[背包网格] 物品已添加到格子 | idx=%d | 格子尺寸=%dx%d",
		Index, GridFragment->GetGridSize().X, GridFragment->GetGridSize().Y);
}

UOldMIS_SlottedItem* UOldMIS_InventoryGrid::CreateSlottedItem(UOldMIS_InventoryItem* Item, const bool bStackable, const int32 StackAmount, const FOldMIS_GridFragment* GridFragment, const FOldMIS_ImageFragment* ImageFragment, const int32 Index)
{
	UOldMIS_SlottedItem* SlottedItem = CreateWidget<UOldMIS_SlottedItem>(GetOwningPlayer(), SlottedItemClass);
	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImage(SlottedItem, GridFragment, ImageFragment);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);
	const int32 StackUpdateAmount = bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);
	SlottedItem->OnSlottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
	SlottedItem->OnSlottedItemHovered.AddDynamic(this, &ThisClass::OnSlottedItemHovered);
	SlottedItem->OnSlottedItemUnhovered.AddDynamic(this, &ThisClass::OnSlottedItemUnhovered);

	return SlottedItem;
}

void UOldMIS_InventoryGrid::SetSlottedItemImage(const UOldMIS_SlottedItem* SlottedItem, const FOldMIS_GridFragment* GridFragment, const FOldMIS_ImageFragment* ImageFragment) const
{
	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = GetDrawSize(GridFragment);
	SlottedItem->SetImageBrush(IconBrush);
}

FVector2D UOldMIS_InventoryGrid::GetDrawSize(const FOldMIS_GridFragment* GridFragment) const
{
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	return GridFragment->GetGridSize() * IconTileWidth;
}

void UOldMIS_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FOldMIS_GridFragment* GridFragment, UOldMIS_SlottedItem* SlottedItem) const
{
	CanvasPanel->AddChild(SlottedItem);
	const FVector2D DrawPos = UOldMIS_WidgetFunctionLibrary::GetPositionFromIndex(Index, Columns) * TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());

	UCanvasPanelSlot* CPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	CPS->SetSize(GetDrawSize(GridFragment));
	CPS->SetPosition(DrawPosWithPadding);
}

void UOldMIS_InventoryGrid::UpdateGridSlots(UOldMIS_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount)
{
	const FOldMIS_GridFragment* GridFragment = GetFragment<FOldMIS_GridFragment>(NewItem, OldMIS_FragmentTags::GridFragment);
	if (!GridFragment) return;

	if (bStackableItem)
	{
		GridSlots[Index]->SetStackCount(StackAmount);
	}

	const FIntPoint Dimensions = GridFragment->GetGridSize();

	UOldMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UOldMIS_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(NewItem);
		GridSlot->SetUpperLeftIndex(Index);
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailable(false);
	});
}

bool UOldMIS_InventoryGrid::IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index) const
{
	return CheckedIndices.Contains(Index);
}

bool UOldMIS_InventoryGrid::IsSameStackable(const UOldMIS_InventoryItem* Item) const
{
	if (!IsValid(Item) || !IsValid(HoverItem)) return false;
	if (!HoverItem->IsStackable() || !Item->IsStackable()) return false;

	const FGameplayTag HoverItemType = HoverItem->GetItemType();
	const FGameplayTag ClickedItemType = Item->GetItemManifest().GetItemType();

	return HoverItemType.MatchesTagExact(ClickedItemType);
}

bool UOldMIS_InventoryGrid::ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount, const int32 MaxStackSize) const
{
	return RoomInClickedSlot == 0 && HoveredStackCount < MaxStackSize;
}

void UOldMIS_InventoryGrid::SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 GridIndex)
{
	HoverItem->UpdateStackCount(ClickedStackCount);
	const auto& SlottedItem = SlottedItems.FindChecked(GridIndex);
	SlottedItem->UpdateStackCount(HoveredStackCount);

	GridSlots[GridIndex]->SetStackCount(HoveredStackCount);

	if (UOldMIS_GridSlot* HoverOriginSlot = GridSlots[HoverItem->GetPreviousGridIndex()]; IsValid(HoverOriginSlot))
	{
		HoverOriginSlot->SetStackCount(HoveredStackCount);
		HoverItem->SetPreviousGridIndex(GridIndex);
	}
}

bool UOldMIS_InventoryGrid::ShouldConsumeHoverItemStacks(const int32 HoveredStackCount, const int32 RoomInClickedSlot) const
{
	return HoveredStackCount <= RoomInClickedSlot;
}

void UOldMIS_InventoryGrid::ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 GridIndex)
{
	const int32 AmountToTransfer = HoveredStackCount;
	const int32 NewClickedStackCount = ClickedStackCount + AmountToTransfer;

	GridSlots[GridIndex]->SetStackCount(NewClickedStackCount);
	SlottedItems.FindChecked(GridIndex)->UpdateStackCount(NewClickedStackCount);

	RemoveItemFromGrid(HoverItem->GetInventoryItem(), HoverItem->GetPreviousGridIndex());
	ClearHoverItem();
	ShowCursor();

	const FOldMIS_GridFragment* GridFragment = GetFragment<FOldMIS_GridFragment>(GridSlots[GridIndex]->GetInventoryItem().Get(), OldMIS_FragmentTags::GridFragment);
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	HighlightSlots(GridIndex, Dimensions);
}

bool UOldMIS_InventoryGrid::ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount) const
{
	return RoomInClickedSlot < HoveredStackCount;
}

void UOldMIS_InventoryGrid::FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	UOldMIS_GridSlot* GridSlot = GridSlots[Index];
	const int32 NewStackCount = GridSlot->GetStackCount() + FillAmount;

	GridSlot->SetStackCount(NewStackCount);

	UOldMIS_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(NewStackCount);

	HoverItem->UpdateStackCount(Remainder);
}

void UOldMIS_InventoryGrid::SwapWithHoverItem(UOldMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;

	UOldMIS_InventoryItem* TempInventoryItem = HoverItem->GetInventoryItem();
	const int32 TempStackCount = HoverItem->GetStackCount();
	const bool bTempIsStackable = HoverItem->IsStackable();

	AssignHoverItem(ClickedInventoryItem, GridIndex, HoverItem->GetPreviousGridIndex());
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
	AddItemAtIndex(TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
	UpdateGridSlots(TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
}

void UOldMIS_InventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UOldMIS_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UOldMIS_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;

	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);

	ItemPopUp->RemoveFromParent();

	AssignHoverItem(RightClickedItem, UpperLeftIndex, UpperLeftIndex);
	HoverItem->UpdateStackCount(SplitAmount);
}

void UOldMIS_InventoryGrid::OnPopUpMenuDrop(int32 Index)
{
	UOldMIS_InventoryItem* Item = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(Item)) return;

	ItemPopUp->RemoveFromParent();

	PickUp(Item, Index);
	DropItem();
}

void UOldMIS_InventoryGrid::OnPopUpMenuConsume(int32 Index)
{
	if (!InventoryComponent.IsValid()) return;
	UOldMIS_InventoryItem* Item = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(Item)) return;

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UOldMIS_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 NewStackCount = UpperLeftGridSlot->GetStackCount() - 1;

	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);

	ItemPopUp->RemoveFromParent();

	InventoryComponent->RequestConsumeItem(Item);

	if (NewStackCount <= 0)
	{
		RemoveItemFromGrid(Item, UpperLeftIndex);
	}
}

void UOldMIS_InventoryGrid::OnSlottedItemHovered(int32 GridIndex)
{
	UOldMIS_InventoryItem* Item = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(Item)) return;

	DH_SCREEN(2.f, DHColors::Orange, "[InventoryGrid] 物品悬停 | Item=%s | 开始%.1fs计时器",
		*Item->GetName(), DescriptionTimerDelay);

	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);

	const auto& Manifest = Item->GetItemManifest();

	FTimerDelegate DescriptionTimerDelegate;
	DescriptionTimerDelegate.BindLambda([this, Item, &Manifest]()
	{
		UOldMIS_ItemDescription* DescWidget = GetItemDescription();
		if (!IsValid(DescWidget)) return;

		DH_SCREEN(2.f, DHColors::Orange, "[InventoryGrid] 计时器触发 | 显示描述 | Item=%s",
			*Item->GetName());

		DescWidget->Collapse();
		Manifest.AssimilateInventoryFragments(DescWidget);
		DescWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	});

	GetWorld()->GetTimerManager().SetTimer(DescriptionTimer, DescriptionTimerDelegate, DescriptionTimerDelay, false);

	// ---- 装备物品描述计时器 ----
	ClearEquippedItemDescription();
	GetWorld()->GetTimerManager().ClearTimer(EquippedDescriptionTimer);

	FTimerDelegate EquippedDescriptionTimerDelegate;
	EquippedDescriptionTimerDelegate.BindUObject(this, &ThisClass::ShowEquippedItemDescription, Item);
	GetWorld()->GetTimerManager().SetTimer(EquippedDescriptionTimer, EquippedDescriptionTimerDelegate, EquippedDescriptionTimerDelay, false);

	OnGridItemHovered.Broadcast(Item);
}

void UOldMIS_InventoryGrid::OnSlottedItemUnhovered(int32 GridIndex)
{
	OnItemUnhovered();
	OnGridItemUnhovered.Broadcast();
}

void UOldMIS_InventoryGrid::OnItemUnhovered()
{
	DH_SCREEN(1.5f, DHColors::Orange, "[InventoryGrid] 物品离开 | 清除计时器");

	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);

	ClearEquippedItemDescription();
	GetWorld()->GetTimerManager().ClearTimer(EquippedDescriptionTimer);
}

void UOldMIS_InventoryGrid::OnInventoryMenuToggled(bool bOpen)
{
	if (!bOpen)
	{
		OnHide();
	}
}

void UOldMIS_InventoryGrid::ShowCursor()
{
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, nullptr);
	GetOwningPlayer()->SetShowMouseCursor(true);
}

void UOldMIS_InventoryGrid::HideCursor()
{
	GetOwningPlayer()->SetShowMouseCursor(false);
}

void UOldMIS_InventoryGrid::ClearHoverItem()
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] ClearHoverItem | 清除悬停物品");

	if (!IsValid(HoverItem)) return;

	HoverItem->SetInventoryItem(nullptr);
	HoverItem->SetIsStackable(false);
	HoverItem->SetPreviousGridIndex(INDEX_NONE);
	HoverItem->UpdateStackCount(0);
	HoverItem->SetImageBrush(FSlateNoResource());

	HoverItem->RemoveFromParent();
	HoverItem = nullptr;

	ShowCursor();
}

void UOldMIS_InventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

void UOldMIS_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] PutDownOnIndex | idx=%d", Index);

	AddItemAtIndex(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	UpdateGridSlots(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	ClearHoverItem();
}

UUserWidget* UOldMIS_InventoryGrid::GetVisibleCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(VisibleCursorWidget))
	{
		VisibleCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), VisibleCursorWidgetClass);
	}
	return VisibleCursorWidget;
}

UUserWidget* UOldMIS_InventoryGrid::GetHiddenCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(HiddenCursorWidget))
	{
		HiddenCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), HiddenCursorWidgetClass);
	}
	return HiddenCursorWidget;
}

void UOldMIS_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	DH_PRINT(EDH_Output::Both, 2.f, DHColors::Magenta,
		"[背包网格] 背景下点击 | ItemDropIndex=%d | 有HoverItem=%d | 有ValidItem=%d",
		ItemDropIndex, IsValid(HoverItem), CurrentQueryResult.ValidItem.IsValid());

	if (!IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(ItemDropIndex)) return;

	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] >> 交换: 点击已有物品");
		OnSlottedItemClicked(CurrentQueryResult.UpperLeftIndex, MouseEvent);
		return;
	}

	if (!IsInGridBounds(ItemDropIndex, HoverItem->GetGridDimensions())) return;
	auto GridSlot = GridSlots[ItemDropIndex];
	if (!GridSlot->GetInventoryItem().IsValid())
	{
		DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] >> 放下: 放到空位");
		PutDownOnIndex(ItemDropIndex);
	}
}

void UOldMIS_InventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;

	UOldMIS_GridSlot* GridSlot = GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetOccupiedTexture();
	}
}

void UOldMIS_InventoryGrid::OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;

	UOldMIS_GridSlot* GridSlot = GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetUnoccupiedTexture();
	}
}

UOldMIS_ItemDescription* UOldMIS_InventoryGrid::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UOldMIS_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
		OwningCanvasPanel->AddChild(ItemDescription);
	}
	return ItemDescription;
}

void UOldMIS_InventoryGrid::SetItemDescriptionSizeAndPosition(UOldMIS_ItemDescription* Description, UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* ItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	if (!IsValid(ItemDescriptionCPS)) return;

	const FVector2D ItemDescriptionSize = Description->GetBoxSize();
	ItemDescriptionCPS->SetSize(ItemDescriptionSize);

	const FVector2D Boundary = Canvas->GetCachedGeometry().GetLocalSize();
	FVector2D _, CanvasViewportOrigin;
	USlateBlueprintLibrary::LocalToViewport(Canvas, Canvas->GetCachedGeometry(),
		USlateBlueprintLibrary::GetLocalTopLeft(Canvas->GetCachedGeometry()), _, CanvasViewportOrigin);
	const FVector2D MouseViewport = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	const FVector2D MouseLocal = MouseViewport - CanvasViewportOrigin;

	// 溢出检测（仅检测，不做偏移）
	UOldMIS_WidgetFunctionLibrary::GetClampedWidgetPosition(Boundary, ItemDescriptionSize, MouseLocal);

	// 最终位置直接用鼠标视口坐标
	ItemDescriptionCPS->SetPosition(MouseViewport);
}

UOldMIS_ItemDescription* UOldMIS_InventoryGrid::GetEquippedItemDescription()
{
	if (!IsValid(EquippedItemDescription))
	{
		EquippedItemDescription = CreateWidget<UOldMIS_ItemDescription>(GetOwningPlayer(), EquippedItemDescriptionClass);
		OwningCanvasPanel->AddChild(EquippedItemDescription);
	}
	return EquippedItemDescription;
}

void UOldMIS_InventoryGrid::SetEquippedItemDescriptionSizeAndPosition(UOldMIS_ItemDescription* Description, UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* EquippedCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	if (!IsValid(EquippedCPS)) return;

	const FVector2D EquippedSize = EquippedItemDescription->GetBoxSize();
	EquippedCPS->SetSize(EquippedSize);

	FVector2D ClampedPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	ClampedPosition.X -= EquippedSize.X;

	EquippedCPS->SetPosition(ClampedPosition);
}

void UOldMIS_InventoryGrid::ShowEquippedItemDescription(UOldMIS_InventoryItem* Item)
{
	if (!IsValid(Item)) return;

	const auto& Manifest = Item->GetItemManifest();
	const FOldMIS_EquipmentFragment* EquipmentFragment = Manifest.GetFragmentOfType<FOldMIS_EquipmentFragment>();
	if (!EquipmentFragment) return;

	const FGameplayTag HoveredEquipmentType = EquipmentFragment->GetEquipmentType();

	auto AlreadyEquippedSlot = EquippedGridSlots.FindByPredicate([Item](const UOldMIS_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == Item;
	});
	if (AlreadyEquippedSlot != nullptr) return;

	auto FoundEquippedSlot = EquippedGridSlots.FindByPredicate([HoveredEquipmentType](const UOldMIS_EquippedGridSlot* GridSlot)
	{
		UOldMIS_InventoryItem* InvItem = GridSlot->GetInventoryItem().Get();
		if (!IsValid(InvItem)) return false;
		const auto* EquipFrag = InvItem->GetItemManifest().GetFragmentOfType<FOldMIS_EquipmentFragment>();
		return EquipFrag ? EquipFrag->GetEquipmentType() == HoveredEquipmentType : false;
	});
	UOldMIS_EquippedGridSlot* EquippedSlot = FoundEquippedSlot ? *FoundEquippedSlot : nullptr;
	if (!IsValid(EquippedSlot)) return;

	UOldMIS_InventoryItem* EquippedItem = EquippedSlot->GetInventoryItem().Get();
	if (!IsValid(EquippedItem)) return;

	const auto& EquippedItemManifest = EquippedItem->GetItemManifest();
	UOldMIS_ItemDescription* EquippedDesc = GetEquippedItemDescription();

	EquippedDesc->Collapse();
	EquippedDesc->SetVisibility(ESlateVisibility::HitTestInvisible);
	EquippedItemManifest.AssimilateInventoryFragments(EquippedDesc);
}

void UOldMIS_InventoryGrid::ClearEquippedItemDescription()
{
	if (IsValid(EquippedItemDescription))
	{
		EquippedItemDescription->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UOldMIS_InventoryGrid::BindEquippedGridSlotDelegates()
{
	for (auto& GridSlot : EquippedGridSlots)
	{
		if (IsValid(GridSlot))
		{
			GridSlot->EquippedGridSlotClicked.AddDynamic(this, &ThisClass::EquippedGridSlotClicked);
		}
	}
}

void UOldMIS_InventoryGrid::EquippedGridSlotClicked(UOldMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag)
{
	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
		"[装备链路-UI] >>> 点击装备槽 | Slot=%s | Tag=%s | 有Hover=%d",
		IsValid(EquippedGridSlot) ? *EquippedGridSlot->GetName() : TEXT("空"),
		*EquipmentTypeTag.ToString(),
		IsValid(HoverItem));

	if (!CanEquipHoverItem(EquippedGridSlot, EquipmentTypeTag))
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-UI] 装备槽点击被CanEquipHoverItem拒绝");
		return;
	}

	if (!IsValid(HoverItem))
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-UI] 装备槽点击失败: HoverItem为空");
		return;
	}

	UOldMIS_InventoryItem* ItemToEquip = HoverItem->GetInventoryItem();
	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
		"[装备链路-UI] 准备装备 | Item=%s | Tag=%s",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		*EquipmentTypeTag.ToString());

	UOldMIS_EquippedSlottedItem* EquippedSlottedItem = EquippedGridSlot->OnItemEquipped(
		HoverItem->GetInventoryItem(),
		EquipmentTypeTag,
		TileSize
	);
	EquippedSlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);

	if (InventoryComponent.IsValid())
	{
		DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
			"[装备链路-UI] >>> 调用 RequestEquipSlotClicked | EquipTo=%s | UnequipTo=nullptr",
			*ItemToEquip->GetName());
		InventoryComponent->RequestEquipSlotClicked(ItemToEquip, nullptr);
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-UI] 装备槽点击失败: InventoryComponent无效");
	}

	ClearHoverItem();
}

bool UOldMIS_InventoryGrid::CanEquipHoverItem(UOldMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot) || EquippedGridSlot->GetInventoryItem().IsValid()) return false;

	if (!IsValid(HoverItem)) return false;

	if (HoverItem->IsStackable()) return false;

	UOldMIS_InventoryItem* HeldItem = HoverItem->GetInventoryItem();
	if (!IsValid(HeldItem)) return false;

	const FOldMIS_EquipmentFragment* EquipmentFragment = HeldItem->GetItemManifest().GetFragmentOfType<FOldMIS_EquipmentFragment>();
	if (!EquipmentFragment) return false;

	return EquipmentFragment->GetEquipmentType().MatchesTag(EquipmentTypeTag);
}

void UOldMIS_InventoryGrid::EquippedSlottedItemClicked(UOldMIS_EquippedSlottedItem* EquippedSlottedItem)
{
	OnItemUnhovered();

	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
		"[装备链路-UI] >>> 点击已装备物品 | SlottedItem=%s",
		IsValid(EquippedSlottedItem) ? *EquippedSlottedItem->GetName() : TEXT("空"));

	if (!IsValid(EquippedSlottedItem))
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-UI] 点击已装备物品失败: EquippedSlottedItem为空");
		return;
	}

	if (IsValid(GetHoverItem()) && GetHoverItem()->IsStackable())
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-UI] 点击已装备物品被拒绝: HoverItem可堆叠");
		return;
	}

	UOldMIS_InventoryItem* ItemToEquip = IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr;
	UOldMIS_InventoryItem* ItemToUnequip = EquippedSlottedItem->GetInventoryItem();

	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
		"[装备链路-UI] 交换装备 | EquipTo=%s | UnequipTo=%s",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"));

	UOldMIS_EquippedGridSlot* EquippedGridSlot = FindSlotWithEquippedItem(ItemToUnequip);
	ClearSlotOfItem(EquippedGridSlot);

	AssignHoverItem(ItemToUnequip);
	RemoveEquippedSlottedItem(EquippedSlottedItem);
	MakeEquippedSlottedItem(EquippedSlottedItem, EquippedGridSlot, ItemToEquip);
	BroadcastSlotClickedDelegates(ItemToEquip, ItemToUnequip);
}

UOldMIS_EquippedGridSlot* UOldMIS_InventoryGrid::FindSlotWithEquippedItem(UOldMIS_InventoryItem* EquippedItem) const
{
	auto* FoundEquippedGridSlot = EquippedGridSlots.FindByPredicate([EquippedItem](const UOldMIS_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == EquippedItem;
	});
	return FoundEquippedGridSlot ? *FoundEquippedGridSlot : nullptr;
}

void UOldMIS_InventoryGrid::ClearSlotOfItem(UOldMIS_EquippedGridSlot* EquippedGridSlot)
{
	if (IsValid(EquippedGridSlot))
	{
		EquippedGridSlot->SetEquippedSlottedItem(nullptr);
		EquippedGridSlot->SetInventoryItem(nullptr);
		EquippedGridSlot->ClearEquippedState();
	}
}

void UOldMIS_InventoryGrid::RemoveEquippedSlottedItem(UOldMIS_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;

	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this, &ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void UOldMIS_InventoryGrid::MakeEquippedSlottedItem(UOldMIS_EquippedSlottedItem* OldSlottedItem, UOldMIS_EquippedGridSlot* EquippedGridSlot, UOldMIS_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot) || !IsValid(ItemToEquip)) return;

	UOldMIS_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(
		ItemToEquip,
		OldSlottedItem->GetEquipmentTypeTag(),
		GetTileSize());
	if (IsValid(SlottedItem)) SlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);

	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UOldMIS_InventoryGrid::BroadcastSlotClickedDelegates(UOldMIS_InventoryItem* ItemToEquip, UOldMIS_InventoryItem* ItemToUnequip) const
{
	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
		"[装备链路-UI] BroadcastSlotClickedDelegates | Equip=%s | Unequip=%s | InvComp有效=%d",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"),
		InventoryComponent.IsValid());

	if (InventoryComponent.IsValid())
	{
		InventoryComponent->RequestEquipSlotClicked(ItemToEquip, ItemToUnequip);
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-UI] BroadcastSlotClickedDelegates失败: InventoryComponent无效");
	}
}
