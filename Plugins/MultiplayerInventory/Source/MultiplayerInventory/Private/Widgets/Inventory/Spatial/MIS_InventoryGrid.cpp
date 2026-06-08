#include "Widgets/Inventory/Spatial/MIS_InventoryGrid.h"

#include "DH_DebugFunctionLibrary.h"
#include "MultiplayerInventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "BlueprintFunctionLibraries/MIS_InventoryFunctionLibrary.h"
#include "BlueprintFunctionLibraries/MIS_WidgetFunctionLibrary.h"
#include "Items/MIS_InventoryItem.h"
#include "Items/Components/MIS_ItemComponent.h"
#include "Items/Fragments/MIS_FragmentTags.h"
#include "Items/Fragments/MIS_ItemFragment.h"
#include "Widgets/Inventory/GridSlots/MIS_GridSlot.h"
#include "Items/Manifest/MIS_ItemManifest.h"
#include "Widgets/Inventory/HoverItem/MIS_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/MIS_SlottedItem.h"
#include "Widgets/ItemPopUp/MIS_ItemPopUp.h"
#include "Widgets/ItemDescription/MIS_ItemDescription.h"
#include "Widgets/Inventory/GridSlots/MIS_EquippedGridSlot.h"
#include "Widgets/Inventory/SlottedItems/MIS_EquippedSlottedItem.h"

void UMIS_InventoryGrid::NativePreConstruct()
{
	Super::NativePreConstruct();

	ClearGrid();
	ConstructGrid();
}

void UMIS_InventoryGrid::InitFromComponent(UMIS_InventoryComponent* InInventoryComponent, UCanvasPanel* InCanvasPanel)
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

void UMIS_InventoryGrid::ConstructGrid()
{
	if (!GridSlotClass || Columns <= 0 || Rows <= 0) return;
	if (!CanvasPanel) return;

	GridSlots.Reserve(Rows * Columns);

	for (int32 j = 0; j < Rows; ++j)
	{
		for (int32 i = 0; i < Columns; ++i)
		{
			UMIS_GridSlot* GridSlot = CreateWidget<UMIS_GridSlot>(this, GridSlotClass);
			CanvasPanel->AddChild(GridSlot);

			const FIntPoint TilePosition(i, j);
			GridSlot->SetTileIndex(UMIS_WidgetFunctionLibrary::GetIndexFromPosition(TilePosition, Columns));

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

void UMIS_InventoryGrid::ClearGrid()
{
	for (UMIS_GridSlot* GridSlot : GridSlots)
	{
		if (IsValid(GridSlot))
		{
			GridSlot->RemoveFromParent();
		}
	}
	GridSlots.Empty();
}

void UMIS_InventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

	if (CursorExitedCanvas(CanvasPosition, UMIS_WidgetFunctionLibrary::GetWidgetSize(CanvasPanel), MousePosition))
	{
		return;
	}

	UpdateTileParameters(CanvasPosition, MousePosition);
}

void UMIS_InventoryGrid::UpdateTileParameters(const FVector2D& CanvasPosition, const FVector2D& MousePosition)
{
	if (!bMouseWithinCanvas) return;

	const FIntPoint HoveredTileCoordinates = CalculateHoveredCoordinates(CanvasPosition, MousePosition);

	LastTileParameters = TileParameters;
	TileParameters.TileCoordinats = HoveredTileCoordinates;
	TileParameters.TileIndex = UMIS_WidgetFunctionLibrary::GetIndexFromPosition(HoveredTileCoordinates, Columns);
	TileParameters.TileQuadrant = CalculateTileQuadrant(CanvasPosition, MousePosition);

	OnTileParametersUpdated(TileParameters);
}

void UMIS_InventoryGrid::OnTileParametersUpdated(const FMIS_TileParameters& Parameters)
{
	if (!IsValid(HoverItem)) return;

	const FIntPoint Dimensions = HoverItem->GetGridDimensions();
	const FIntPoint StartingCoordinate = CalculateStartingCoordinate(Parameters.TileCoordinats, Dimensions, Parameters.TileQuadrant);
	ItemDropIndex = UMIS_WidgetFunctionLibrary::GetIndexFromPosition(StartingCoordinate, Columns);

	CurrentQueryResult = CheckHoverPosition(StartingCoordinate, Dimensions);

	if (CurrentQueryResult.bHasSpace)
	{
		HighlightSlots(ItemDropIndex, Dimensions);
		return;
	}
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);

	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(CurrentQueryResult.ValidItem.Get(), MIS_FragmentTags::GridFragment);
		if (!GridFragment) return;

		ChangeHoverType(CurrentQueryResult.UpperLeftIndex, GridFragment->GetGridSize(), EMIS_GridSlotState::GrayedOut);
	}
}

FMIS_SpaceQueryResult UMIS_InventoryGrid::CheckHoverPosition(const FIntPoint& Position, const FIntPoint& Dimensions)
{
	FMIS_SpaceQueryResult Result;

	if (!IsInGridBounds(UMIS_WidgetFunctionLibrary::GetIndexFromPosition(Position, Columns), Dimensions)) return Result;

	Result.bHasSpace = true;

	TSet<int32> OccupiedUpperLeftIndices;
	UMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, UMIS_WidgetFunctionLibrary::GetIndexFromPosition(Position, Columns), Dimensions, Columns, [&](const UMIS_GridSlot* GridSlot)
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

bool UMIS_InventoryGrid::CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location)
{
	bLastMouseWithinCanvas = bMouseWithinCanvas;
	bMouseWithinCanvas = UMIS_WidgetFunctionLibrary::IsWithinBounds(BoundaryPos, BoundarySize, Location);
	if (!bMouseWithinCanvas && bLastMouseWithinCanvas)
	{
		UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
		return true;
	}
	return false;
}

void UMIS_InventoryGrid::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	if (!bMouseWithinCanvas) return;
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UMIS_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();
	});
	LastHighlightedDimensions = Dimensions;
	LastHighlightedIndex = Index;
}

void UMIS_InventoryGrid::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UMIS_GridSlot* GridSlot)
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

void UMIS_InventoryGrid::ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EMIS_GridSlotState GridSlotState)
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [State = GridSlotState](UMIS_GridSlot* GridSlot)
	{
		switch (State)
		{
		case EMIS_GridSlotState::Occupied:
			GridSlot->SetOccupiedTexture();
			break;
		case EMIS_GridSlotState::Unoccupied:
			GridSlot->SetUnoccupiedTexture();
			break;
		case EMIS_GridSlotState::GrayedOut:
			GridSlot->SetGrayedOutTexture();
			break;
		case EMIS_GridSlotState::Selected:
			GridSlot->SetSelectedTexture();
			break;
		}
	});

	LastHighlightedIndex = Index;
	LastHighlightedDimensions = Dimensions;
}

FIntPoint UMIS_InventoryGrid::CalculateStartingCoordinate(const FIntPoint& Coordinate, const FIntPoint& Dimensions, const EMIS_TileQuadrant Quadrant) const
{
	const int32 HasEvenWidth = Dimensions.X % 2 == 0 ? 1 : 0;
	const int32 HasEvenHeight = Dimensions.Y % 2 == 0 ? 1 : 0;

	FIntPoint StartingCoord;
	switch (Quadrant)
	{
		case EMIS_TileQuadrant::TopLeft:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
			break;
		case EMIS_TileQuadrant::TopRight:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEvenWidth;
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
			break;
		case EMIS_TileQuadrant::BottomLeft:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEvenHeight;
			break;
		case EMIS_TileQuadrant::BottomRight:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEvenWidth;
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEvenHeight;
			break;
	default:
		DH_LOG_ERR("Invalid Quadrant.");
		return FIntPoint(-1, -1);
	}
	return StartingCoord;
}

FIntPoint UMIS_InventoryGrid::CalculateHoveredCoordinates(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const
{
	return FIntPoint{
		static_cast<int32>(FMath::FloorToInt((MousePosition.X - CanvasPosition.X) / TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePosition.Y - CanvasPosition.Y) / TileSize))
	};
}

EMIS_TileQuadrant UMIS_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const
{
	const float TileLocalX = FMath::Fmod(MousePosition.X - CanvasPosition.X, TileSize);
	const float TileLocalY = FMath::Fmod(MousePosition.Y - CanvasPosition.Y, TileSize);

	const bool bIsTop = TileLocalY < TileSize / 2.f;
	const bool bIsLeft = TileLocalX < TileSize / 2.f;

	EMIS_TileQuadrant HoveredTileQuadrant{EMIS_TileQuadrant::None};
	if (bIsTop && bIsLeft) HoveredTileQuadrant = EMIS_TileQuadrant::TopLeft;
	else if (bIsTop && !bIsLeft) HoveredTileQuadrant = EMIS_TileQuadrant::TopRight;
	else if (!bIsTop && bIsLeft) HoveredTileQuadrant = EMIS_TileQuadrant::BottomLeft;
	else if (!bIsTop && !bIsLeft) HoveredTileQuadrant = EMIS_TileQuadrant::BottomRight;

	return HoveredTileQuadrant;
}

FMIS_SlotAvailabilityResult UMIS_InventoryGrid::HasRoomForItem(const UMIS_ItemComponent* ItemComponent)
{
	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FMIS_SlotAvailabilityResult UMIS_InventoryGrid::HasRoomForItem(const UMIS_InventoryItem* Item, const int32 StackAmountOverride)
{
	return HasRoomForItem(Item->GetItemManifest(), StackAmountOverride);
}

FMIS_SlotAvailabilityResult UMIS_InventoryGrid::HasRoomForItem(const FMIS_ItemManifest& Manifest, const int32 StackAmountOverride)
{
	// ---- 步骤1: 获取物品的堆叠信�?----
	FMIS_SlotAvailabilityResult Result;

	const FMIS_StackableFragment* StackableFragment = Manifest.GetFragmentOfType<FMIS_StackableFragment>();
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
			FMIS_SlotAvailability{
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

bool UMIS_InventoryGrid::HasRoomAtIndex(const UMIS_GridSlot* GridSlot, const FIntPoint& Dimensions, const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize)
{
	bool bHasRoomAtIndex = true;
	UMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, GridSlot->GetIndex(), Dimensions, Columns, [&](const UMIS_GridSlot* SubGridSlot)
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

bool UMIS_InventoryGrid::CheckSlotConstraints(const UMIS_GridSlot* GridSlot, const UMIS_GridSlot* SubGridSlot, const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize) const
{
	if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetIndex())) return false;

	if (!HasValidItem(SubGridSlot))
	{
		OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		return true;
	}

	if (!IsUpperLeftSlot(GridSlot, SubGridSlot)) return false;

	const UMIS_InventoryItem* SubItem = SubGridSlot->GetInventoryItem().Get();
	if (!SubItem->IsStackable()) return false;

	if (!DoesItemTypeMatch(SubItem, ItemType)) return false;

	if (GridSlot->GetStackCount() >= MaxStackSize) return false;

	return true;
}

FIntPoint UMIS_InventoryGrid::GetItemDimensions(const FMIS_ItemManifest& Manifest) const
{
	const FMIS_GridFragment* GridFragment = Manifest.GetFragmentOfType<FMIS_GridFragment>();
	return GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
}

bool UMIS_InventoryGrid::HasValidItem(const UMIS_GridSlot* GridSlot) const
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool UMIS_InventoryGrid::IsUpperLeftSlot(const UMIS_GridSlot* GridSlot, const UMIS_GridSlot* SubGridSlot) const
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetIndex();
}

bool UMIS_InventoryGrid::DoesItemTypeMatch(const UMIS_InventoryItem* SubItem, const FGameplayTag& ItemType) const
{
	return SubItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
}

bool UMIS_InventoryGrid::IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const
{
	if (StartIndex < 0 || StartIndex >= GridSlots.Num()) return false;
	const int32 EndColumn = (StartIndex % Columns) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / Columns) + ItemDimensions.Y;
	return EndColumn <= Columns && EndRow <= Rows;
}

int32 UMIS_InventoryGrid::DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize, const int32 AmountToFill, const UMIS_GridSlot* GridSlot) const
{
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(GridSlot);
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
}

int32 UMIS_InventoryGrid::GetStackAmount(const UMIS_GridSlot* GridSlot) const
{
	int32 CurrentSlotStackCount = GridSlot->GetStackCount();
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		UMIS_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
		CurrentSlotStackCount = UpperLeftGridSlot->GetStackCount();
	}
	return CurrentSlotStackCount;
}

bool UMIS_InventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UMIS_InventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

void UMIS_InventoryGrid::PickUp(UMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] PickUp | idx=%d", GridIndex);
	AssignHoverItem(ClickedInventoryItem, GridIndex, GridIndex);
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
}

void UMIS_InventoryGrid::AssignHoverItem(UMIS_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex)
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] AssignHoverItem | idx=%d | prev=%d", GridIndex, PreviousGridIndex);

	AssignHoverItem(InventoryItem);

	HoverItem->SetPreviousGridIndex(PreviousGridIndex);
	HoverItem->UpdateStackCount(InventoryItem->IsStackable() ? GridSlots[GridIndex]->GetStackCount() : 0);
}

void UMIS_InventoryGrid::RemoveItemFromGrid(UMIS_InventoryItem* InventoryItem, const int32 GridIndex)
{
	const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(InventoryItem, MIS_FragmentTags::GridFragment);

	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] RemoveItemFromGrid | idx=%d | GridFrag=%s",
		GridIndex, GridFragment ? TEXT("有效") : TEXT("空"));

	if (!GridFragment) return;

	UMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, GridIndex, GridFragment->GetGridSize(), Columns, [&](UMIS_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(nullptr);
		GridSlot->SetUpperLeftIndex(INDEX_NONE);
		GridSlot->SetUnoccupiedTexture();
		GridSlot->SetAvailable(true);
		GridSlot->SetStackCount(0);
	});

	if (SlottedItems.Contains(GridIndex))
	{
		TObjectPtr<UMIS_SlottedItem> FoundSlottedItem;
		SlottedItems.RemoveAndCopyValue(GridIndex, FoundSlottedItem);
		FoundSlottedItem->RemoveFromParent();
	}
}

void UMIS_InventoryGrid::OnExternalItemRemoved(UMIS_InventoryItem* Item)
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

void UMIS_InventoryGrid::AssignHoverItem(UMIS_InventoryItem* InventoryItem)
{
	if (!IsValid(HoverItem))
	{
		HoverItem = CreateWidget<UMIS_HoverItem>(GetOwningPlayer(), HoverItemClass);
	}

	const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(InventoryItem, MIS_FragmentTags::GridFragment);
	const FMIS_ImageFragment* ImageFragment = GetFragment<FMIS_ImageFragment>(InventoryItem, MIS_FragmentTags::IconFragment);
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

void UMIS_InventoryGrid::OnHide()
{
	PutHoverItemBack();
}

void UMIS_InventoryGrid::AddStacks(const FMIS_SlotAvailabilityResult& Result)
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

void UMIS_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	check(GridSlots.IsValidIndex(GridIndex));
	UMIS_InventoryItem* ClickedInventoryItem = GridSlots[GridIndex]->GetInventoryItem().Get();

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
		const FMIS_StackableFragment* StackableFragment = ClickedInventoryItem->GetItemManifest().GetFragmentOfType<FMIS_StackableFragment>();
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

void UMIS_InventoryGrid::CreateItemPopUp(const int32 GridIndex)
{
	OnItemUnhovered();

	UMIS_InventoryItem* RightClickedItem = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (IsValid(GridSlots[GridIndex]->GetItemPopUp())) return;

	ItemPopUp = CreateWidget<UMIS_ItemPopUp>(this, ItemPopUpClass);
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

void UMIS_InventoryGrid::PutHoverItemBack()
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] PutHoverItemBack | 放置悬停物品回原位");

	if (!IsValid(HoverItem)) return;

	FMIS_SlotAvailabilityResult Result = HasRoomForItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());
	Result.Item = HoverItem->GetInventoryItem();

	AddStacks(Result);
	ClearHoverItem();
}

void UMIS_InventoryGrid::DropItem()
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

bool UMIS_InventoryGrid::HasHoverItem() const
{
	return IsValid(HoverItem);
}

UMIS_HoverItem* UMIS_InventoryGrid::GetHoverItem() const
{
	return HoverItem;
}

void UMIS_InventoryGrid::AddItem(UMIS_InventoryItem* Item)
{
	DH_SCREEN(3.f, DHColors::Orange, "[背包网格] AddItem 触发 | Item=%s",
		IsValid(Item) ? *Item->GetName() : TEXT("空"));

	FMIS_SlotAvailabilityResult Result = HasRoomForItem(Item);

	DH_SCREEN(3.f, DHColors::Orange, "[背包网格] HasRoomForItem 结果 | 总空间=%d | 槽位数=%d",
		Result.TotalRoomToFill, Result.SlotAvailabilities.Num());

	AddItemToIndices(Result, Item);
}

void UMIS_InventoryGrid::AddItemToIndices(const FMIS_SlotAvailabilityResult& Result, UMIS_InventoryItem* NewItem)
{
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		AddItemAtIndex(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
		UpdateGridSlots(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
	}
}

void UMIS_InventoryGrid::AddItemAtIndex(UMIS_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount)
{
	const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(Item, MIS_FragmentTags::GridFragment);
	const FMIS_ImageFragment* ImageFragment = GetFragment<FMIS_ImageFragment>(Item, MIS_FragmentTags::IconFragment);

	DH_SCREEN(3.f, DHColors::Orange,
		"[背包网格] AddItemAtIndex | idx=%d | GridFrag=%s | ImageFrag=%s | Stack=%d",
		Index, GridFragment ? TEXT("有效") : TEXT("空"),
		ImageFragment ? TEXT("有效") : TEXT("空"), StackAmount);

	if (!GridFragment || !ImageFragment) return;

	UMIS_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	AddSlottedItemToCanvas(Index, GridFragment, SlottedItem);

	SlottedItems.Add(Index, SlottedItem);

	DH_SCREEN(2.f, DHColors::Orange,
		"[背包网格] 物品已添加到格子 | idx=%d | 格子尺寸=%dx%d",
		Index, GridFragment->GetGridSize().X, GridFragment->GetGridSize().Y);
}

UMIS_SlottedItem* UMIS_InventoryGrid::CreateSlottedItem(UMIS_InventoryItem* Item, const bool bStackable, const int32 StackAmount, const FMIS_GridFragment* GridFragment, const FMIS_ImageFragment* ImageFragment, const int32 Index)
{
	UMIS_SlottedItem* SlottedItem = CreateWidget<UMIS_SlottedItem>(GetOwningPlayer(), SlottedItemClass);
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

void UMIS_InventoryGrid::SetSlottedItemImage(const UMIS_SlottedItem* SlottedItem, const FMIS_GridFragment* GridFragment, const FMIS_ImageFragment* ImageFragment) const
{
	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = GetDrawSize(GridFragment);
	SlottedItem->SetImageBrush(IconBrush);
}

FVector2D UMIS_InventoryGrid::GetDrawSize(const FMIS_GridFragment* GridFragment) const
{
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	return GridFragment->GetGridSize() * IconTileWidth;
}

void UMIS_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FMIS_GridFragment* GridFragment, UMIS_SlottedItem* SlottedItem) const
{
	CanvasPanel->AddChild(SlottedItem);
	const FVector2D DrawPos = UMIS_WidgetFunctionLibrary::GetPositionFromIndex(Index, Columns) * TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());

	UCanvasPanelSlot* CPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	CPS->SetSize(GetDrawSize(GridFragment));
	CPS->SetPosition(DrawPosWithPadding);
}

void UMIS_InventoryGrid::UpdateGridSlots(UMIS_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount)
{
	const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(NewItem, MIS_FragmentTags::GridFragment);
	if (!GridFragment) return;

	if (bStackableItem)
	{
		GridSlots[Index]->SetStackCount(StackAmount);
	}

	const FIntPoint Dimensions = GridFragment->GetGridSize();

	UMIS_InventoryFunctionLibrary::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UMIS_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(NewItem);
		GridSlot->SetUpperLeftIndex(Index);
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailable(false);
	});
}

bool UMIS_InventoryGrid::IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index) const
{
	return CheckedIndices.Contains(Index);
}

bool UMIS_InventoryGrid::IsSameStackable(const UMIS_InventoryItem* Item) const
{
	if (!IsValid(Item) || !IsValid(HoverItem)) return false;
	if (!HoverItem->IsStackable() || !Item->IsStackable()) return false;

	const FGameplayTag HoverItemType = HoverItem->GetItemType();
	const FGameplayTag ClickedItemType = Item->GetItemManifest().GetItemType();

	return HoverItemType.MatchesTagExact(ClickedItemType);
}

bool UMIS_InventoryGrid::ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount, const int32 MaxStackSize) const
{
	return RoomInClickedSlot == 0 && HoveredStackCount < MaxStackSize;
}

void UMIS_InventoryGrid::SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 GridIndex)
{
	HoverItem->UpdateStackCount(ClickedStackCount);
	const auto& SlottedItem = SlottedItems.FindChecked(GridIndex);
	SlottedItem->UpdateStackCount(HoveredStackCount);

	GridSlots[GridIndex]->SetStackCount(HoveredStackCount);

	if (UMIS_GridSlot* HoverOriginSlot = GridSlots[HoverItem->GetPreviousGridIndex()]; IsValid(HoverOriginSlot))
	{
		HoverOriginSlot->SetStackCount(HoveredStackCount);
		HoverItem->SetPreviousGridIndex(GridIndex);
	}
}

bool UMIS_InventoryGrid::ShouldConsumeHoverItemStacks(const int32 HoveredStackCount, const int32 RoomInClickedSlot) const
{
	return HoveredStackCount <= RoomInClickedSlot;
}

void UMIS_InventoryGrid::ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 GridIndex)
{
	const int32 AmountToTransfer = HoveredStackCount;
	const int32 NewClickedStackCount = ClickedStackCount + AmountToTransfer;

	GridSlots[GridIndex]->SetStackCount(NewClickedStackCount);
	SlottedItems.FindChecked(GridIndex)->UpdateStackCount(NewClickedStackCount);

	RemoveItemFromGrid(HoverItem->GetInventoryItem(), HoverItem->GetPreviousGridIndex());
	ClearHoverItem();
	ShowCursor();

	const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(GridSlots[GridIndex]->GetInventoryItem().Get(), MIS_FragmentTags::GridFragment);
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	HighlightSlots(GridIndex, Dimensions);
}

bool UMIS_InventoryGrid::ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount) const
{
	return RoomInClickedSlot < HoveredStackCount;
}

void UMIS_InventoryGrid::FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	UMIS_GridSlot* GridSlot = GridSlots[Index];
	const int32 NewStackCount = GridSlot->GetStackCount() + FillAmount;

	GridSlot->SetStackCount(NewStackCount);

	UMIS_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(NewStackCount);

	HoverItem->UpdateStackCount(Remainder);
}

void UMIS_InventoryGrid::SwapWithHoverItem(UMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;

	UMIS_InventoryItem* TempInventoryItem = HoverItem->GetInventoryItem();
	const int32 TempStackCount = HoverItem->GetStackCount();
	const bool bTempIsStackable = HoverItem->IsStackable();

	AssignHoverItem(ClickedInventoryItem, GridIndex, HoverItem->GetPreviousGridIndex());
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
	AddItemAtIndex(TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
	UpdateGridSlots(TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
}

void UMIS_InventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UMIS_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UMIS_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;

	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);

	ItemPopUp->RemoveFromParent();

	AssignHoverItem(RightClickedItem, UpperLeftIndex, UpperLeftIndex);
	HoverItem->UpdateStackCount(SplitAmount);
}

void UMIS_InventoryGrid::OnPopUpMenuDrop(int32 Index)
{
	UMIS_InventoryItem* Item = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(Item)) return;

	ItemPopUp->RemoveFromParent();

	PickUp(Item, Index);
	DropItem();
}

void UMIS_InventoryGrid::OnPopUpMenuConsume(int32 Index)
{
	if (!InventoryComponent.IsValid()) return;
	UMIS_InventoryItem* Item = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(Item)) return;

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UMIS_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
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

void UMIS_InventoryGrid::OnSlottedItemHovered(int32 GridIndex)
{
	UMIS_InventoryItem* Item = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(Item)) return;

	DH_SCREEN(2.f, DHColors::Orange, "[InventoryGrid] 物品悬停 | Item=%s | 开始%.1fs计时器",
		*Item->GetName(), DescriptionTimerDelay);

	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);

	const auto& Manifest = Item->GetItemManifest();

	FTimerDelegate DescriptionTimerDelegate;
	DescriptionTimerDelegate.BindLambda([this, Item, &Manifest]()
	{
		UMIS_ItemDescription* DescWidget = GetItemDescription();
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

void UMIS_InventoryGrid::OnSlottedItemUnhovered(int32 GridIndex)
{
	OnItemUnhovered();
	OnGridItemUnhovered.Broadcast();
}

void UMIS_InventoryGrid::OnItemUnhovered()
{
	DH_SCREEN(1.5f, DHColors::Orange, "[InventoryGrid] 物品离开 | 清除计时器");

	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);

	ClearEquippedItemDescription();
	GetWorld()->GetTimerManager().ClearTimer(EquippedDescriptionTimer);
}

void UMIS_InventoryGrid::OnInventoryMenuToggled(bool bOpen)
{
	if (!bOpen)
	{
		OnHide();
	}
}

void UMIS_InventoryGrid::ShowCursor()
{
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, nullptr);
	GetOwningPlayer()->SetShowMouseCursor(true);
}

void UMIS_InventoryGrid::HideCursor()
{
	GetOwningPlayer()->SetShowMouseCursor(false);
}

void UMIS_InventoryGrid::ClearHoverItem()
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

void UMIS_InventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

void UMIS_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	DH_SCREEN(2.f, DHColors::Magenta, "[背包网格] PutDownOnIndex | idx=%d", Index);

	AddItemAtIndex(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	UpdateGridSlots(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	ClearHoverItem();
}

UUserWidget* UMIS_InventoryGrid::GetVisibleCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(VisibleCursorWidget))
	{
		VisibleCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), VisibleCursorWidgetClass);
	}
	return VisibleCursorWidget;
}

UUserWidget* UMIS_InventoryGrid::GetHiddenCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(HiddenCursorWidget))
	{
		HiddenCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), HiddenCursorWidgetClass);
	}
	return HiddenCursorWidget;
}

void UMIS_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
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

void UMIS_InventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;

	UMIS_GridSlot* GridSlot = GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetOccupiedTexture();
	}
}

void UMIS_InventoryGrid::OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;

	UMIS_GridSlot* GridSlot = GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetUnoccupiedTexture();
	}
}

UMIS_ItemDescription* UMIS_InventoryGrid::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UMIS_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
		OwningCanvasPanel->AddChild(ItemDescription);
	}
	return ItemDescription;
}

void UMIS_InventoryGrid::SetItemDescriptionSizeAndPosition(UMIS_ItemDescription* Description, UCanvasPanel* Canvas) const
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
	UMIS_WidgetFunctionLibrary::GetClampedWidgetPosition(Boundary, ItemDescriptionSize, MouseLocal);

	// 最终位置直接用鼠标视口坐标
	ItemDescriptionCPS->SetPosition(MouseViewport);
}

UMIS_ItemDescription* UMIS_InventoryGrid::GetEquippedItemDescription()
{
	if (!IsValid(EquippedItemDescription))
	{
		EquippedItemDescription = CreateWidget<UMIS_ItemDescription>(GetOwningPlayer(), EquippedItemDescriptionClass);
		OwningCanvasPanel->AddChild(EquippedItemDescription);
	}
	return EquippedItemDescription;
}

void UMIS_InventoryGrid::SetEquippedItemDescriptionSizeAndPosition(UMIS_ItemDescription* Description, UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* EquippedCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	if (!IsValid(EquippedCPS)) return;

	const FVector2D EquippedSize = EquippedItemDescription->GetBoxSize();
	EquippedCPS->SetSize(EquippedSize);

	FVector2D ClampedPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	ClampedPosition.X -= EquippedSize.X;

	EquippedCPS->SetPosition(ClampedPosition);
}

void UMIS_InventoryGrid::ShowEquippedItemDescription(UMIS_InventoryItem* Item)
{
	if (!IsValid(Item)) return;

	const auto& Manifest = Item->GetItemManifest();
	const FMIS_EquipmentFragment* EquipmentFragment = Manifest.GetFragmentOfType<FMIS_EquipmentFragment>();
	if (!EquipmentFragment) return;

	const FGameplayTag HoveredEquipmentType = EquipmentFragment->GetEquipmentType();

	auto AlreadyEquippedSlot = EquippedGridSlots.FindByPredicate([Item](const UMIS_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == Item;
	});
	if (AlreadyEquippedSlot != nullptr) return;

	auto FoundEquippedSlot = EquippedGridSlots.FindByPredicate([HoveredEquipmentType](const UMIS_EquippedGridSlot* GridSlot)
	{
		UMIS_InventoryItem* InvItem = GridSlot->GetInventoryItem().Get();
		if (!IsValid(InvItem)) return false;
		const auto* EquipFrag = InvItem->GetItemManifest().GetFragmentOfType<FMIS_EquipmentFragment>();
		return EquipFrag ? EquipFrag->GetEquipmentType() == HoveredEquipmentType : false;
	});
	UMIS_EquippedGridSlot* EquippedSlot = FoundEquippedSlot ? *FoundEquippedSlot : nullptr;
	if (!IsValid(EquippedSlot)) return;

	UMIS_InventoryItem* EquippedItem = EquippedSlot->GetInventoryItem().Get();
	if (!IsValid(EquippedItem)) return;

	const auto& EquippedItemManifest = EquippedItem->GetItemManifest();
	UMIS_ItemDescription* EquippedDesc = GetEquippedItemDescription();

	EquippedDesc->Collapse();
	EquippedDesc->SetVisibility(ESlateVisibility::HitTestInvisible);
	EquippedItemManifest.AssimilateInventoryFragments(EquippedDesc);
}

void UMIS_InventoryGrid::ClearEquippedItemDescription()
{
	if (IsValid(EquippedItemDescription))
	{
		EquippedItemDescription->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMIS_InventoryGrid::BindEquippedGridSlotDelegates()
{
	for (auto& GridSlot : EquippedGridSlots)
	{
		if (IsValid(GridSlot))
		{
			GridSlot->EquippedGridSlotClicked.AddDynamic(this, &ThisClass::EquippedGridSlotClicked);
		}
	}
}

void UMIS_InventoryGrid::EquippedGridSlotClicked(UMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag)
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

	UMIS_InventoryItem* ItemToEquip = HoverItem->GetInventoryItem();
	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
		"[装备链路-UI] 准备装备 | Item=%s | Tag=%s",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		*EquipmentTypeTag.ToString());

	UMIS_EquippedSlottedItem* EquippedSlottedItem = EquippedGridSlot->OnItemEquipped(
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

bool UMIS_InventoryGrid::CanEquipHoverItem(UMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot) || EquippedGridSlot->GetInventoryItem().IsValid()) return false;

	if (!IsValid(HoverItem)) return false;

	if (HoverItem->IsStackable()) return false;

	UMIS_InventoryItem* HeldItem = HoverItem->GetInventoryItem();
	if (!IsValid(HeldItem)) return false;

	const FMIS_EquipmentFragment* EquipmentFragment = HeldItem->GetItemManifest().GetFragmentOfType<FMIS_EquipmentFragment>();
	if (!EquipmentFragment) return false;

	return EquipmentFragment->GetEquipmentType().MatchesTag(EquipmentTypeTag);
}

void UMIS_InventoryGrid::EquippedSlottedItemClicked(UMIS_EquippedSlottedItem* EquippedSlottedItem)
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

	UMIS_InventoryItem* ItemToEquip = IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr;
	UMIS_InventoryItem* ItemToUnequip = EquippedSlottedItem->GetInventoryItem();

	DH_PRINT(EDH_Output::Both, 3.f, DHColors::Green,
		"[装备链路-UI] 交换装备 | EquipTo=%s | UnequipTo=%s",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"));

	UMIS_EquippedGridSlot* EquippedGridSlot = FindSlotWithEquippedItem(ItemToUnequip);
	ClearSlotOfItem(EquippedGridSlot);

	AssignHoverItem(ItemToUnequip);
	RemoveEquippedSlottedItem(EquippedSlottedItem);
	MakeEquippedSlottedItem(EquippedSlottedItem, EquippedGridSlot, ItemToEquip);
	BroadcastSlotClickedDelegates(ItemToEquip, ItemToUnequip);
}

UMIS_EquippedGridSlot* UMIS_InventoryGrid::FindSlotWithEquippedItem(UMIS_InventoryItem* EquippedItem) const
{
	auto* FoundEquippedGridSlot = EquippedGridSlots.FindByPredicate([EquippedItem](const UMIS_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == EquippedItem;
	});
	return FoundEquippedGridSlot ? *FoundEquippedGridSlot : nullptr;
}

void UMIS_InventoryGrid::ClearSlotOfItem(UMIS_EquippedGridSlot* EquippedGridSlot)
{
	if (IsValid(EquippedGridSlot))
	{
		EquippedGridSlot->SetEquippedSlottedItem(nullptr);
		EquippedGridSlot->SetInventoryItem(nullptr);
		EquippedGridSlot->ClearEquippedState();
	}
}

void UMIS_InventoryGrid::RemoveEquippedSlottedItem(UMIS_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;

	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this, &ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void UMIS_InventoryGrid::MakeEquippedSlottedItem(UMIS_EquippedSlottedItem* OldSlottedItem, UMIS_EquippedGridSlot* EquippedGridSlot, UMIS_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot) || !IsValid(ItemToEquip)) return;

	UMIS_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(
		ItemToEquip,
		OldSlottedItem->GetEquipmentTypeTag(),
		GetTileSize());
	if (IsValid(SlottedItem)) SlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);

	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UMIS_InventoryGrid::BroadcastSlotClickedDelegates(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip) const
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
