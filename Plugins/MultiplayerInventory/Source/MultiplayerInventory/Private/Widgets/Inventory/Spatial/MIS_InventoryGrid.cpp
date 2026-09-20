#include "Widgets/Inventory/Spatial/MIS_InventoryGrid.h"

#include "DH_DebugFunctionLibrary.h"
#include "MIS_MessageKeys.h"
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

	if (InventoryComponent.IsValid())
	{
		// [服务端权威] 把本网格的实际布局上报给服务端, 使其能独立校验落点合法性。
		// 专用服务器没有 UI, 需改为在 InventoryComponent 的蓝图属性里配置 GridColumns/GridRows。
		InventoryComponent->Server_SetGridLayout(Columns, Rows);

		// [解耦重构] 不再绑定数据层委托, 改为监听数据层广播的消息。
		// SigSource 使用库存组件本身, 与数据层发送侧保持一致 (定向投递, 多实例不串台)。
		const GMP::FSigSource InventorySource(InventoryComponent.Get());

		MIS::Listen(MSGKEY(MIS_MSG_ITEM_ADDED), InventorySource, this,
			[this](UMIS_InventoryItem* Item, int32 UpperLeftIndex)
			{
				AddItem(Item, UpperLeftIndex);
			});

		MIS::Listen(MSGKEY(MIS_MSG_ITEM_REMOVED), InventorySource, this,
			[this](UMIS_InventoryItem* Item)
			{
				OnExternalItemRemoved(Item);
			});

		MIS::Listen(MSGKEY(MIS_MSG_STACK_CHANGED), InventorySource, this,
			[this](FMIS_SlotAvailabilityResult Result)
			{
				AddStacks(Result);
			});
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

			// [解耦重构] 监听格子广播的消息 (SigSource 为该格子本身, 据此区分是哪一个)
			const GMP::FSigSource SlotSource(GridSlot);
			MIS::Listen(MSGKEY(MIS_UI_GRID_SLOT_CLICKED), SlotSource, this,
				[this](int32 InIndex, uint8 InButton) { OnGridSlotClicked(InIndex, InButton); });
			MIS::Listen(MSGKEY(MIS_UI_GRID_SLOT_HOVERED), SlotSource, this,
				[this](int32 InIndex) { OnGridSlotHovered(InIndex); });
			MIS::Listen(MSGKEY(MIS_UI_GRID_SLOT_UNHOVERED), SlotSource, this,
				[this](int32 InIndex) { OnGridSlotUnhovered(InIndex); });
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

void UMIS_InventoryGrid::PickUp(UMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	AssignHoverItem(ClickedInventoryItem, GridIndex, GridIndex);
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
}

void UMIS_InventoryGrid::AssignHoverItem(UMIS_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex)
{
	AssignHoverItem(InventoryItem);

	HoverItem->SetPreviousGridIndex(PreviousGridIndex);
	HoverItem->UpdateStackCount(InventoryItem->IsStackable() ? GridSlots[GridIndex]->GetStackCount() : 0);
}

void UMIS_InventoryGrid::RemoveItemFromGrid(UMIS_InventoryItem* InventoryItem, const int32 GridIndex)
{
	const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(InventoryItem, MIS_FragmentTags::GridFragment);

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
	if (Result.SlotAvailabilities.Num() == 0 && Result.Item.IsValid())
	{
		for (auto& Pair : SlottedItems)
		{
			if (Pair.Value && Pair.Value->GetInventoryItem() == Result.Item.Get())
			{
				const int32 NewStack = GridSlots[Pair.Key]->GetStackCount() + Result.TotalRoomToFill;
				Pair.Value->UpdateStackCount(NewStack);
				GridSlots[Pair.Key]->SetStackCount(NewStack);
				return;
			}
		}
		DH_PRINT(EDH_Output::Both, 3.f, FLinearColor::Red, "[背包网格] AddStacks 失败: 找不到物品对应的SlottedItem");
		return;
	}

	for (const auto& Availability : Result.SlotAvailabilities)
	{
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

void UMIS_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, uint8 MouseButton)
{
	check(GridSlots.IsValidIndex(GridIndex));
	UMIS_InventoryItem* ClickedInventoryItem = GridSlots[GridIndex]->GetInventoryItem().Get();

	// [解耦重构] 鼠标键由发送方以 uint8 传递, 不再传 Slate 事件对象
	const bool bLeftClick = (MouseButton == MIS::MouseButton_Left);
	const bool bRightClick = (MouseButton == MIS::MouseButton_Right);

	if (!IsValid(HoverItem) && bLeftClick)
	{
		OnItemUnhovered();
		PickUp(ClickedInventoryItem, GridIndex);
		return;
	}

	// ---- 情况2: 右键显示弹出菜单 ----
	if (bRightClick)
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

	// [解耦重构] 监听弹窗广播的消息 (SigSource 为该弹窗本身)
	const GMP::FSigSource PopUpSource(ItemPopUp);

	const int32 SliderMax = GridSlots[GridIndex]->GetStackCount() - 1;
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		MIS::Listen(MSGKEY(MIS_UI_POPUP_SPLIT), PopUpSource, this,
			[this](int32 InIndex, int32 InAmount) { OnPopUpMenuSplit(InAmount, InIndex); });
		ItemPopUp->SetSliderParams(SliderMax, FMath::Max(1, GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		ItemPopUp->CollapseSplitButton();
	}

	MIS::Listen(MSGKEY(MIS_UI_POPUP_DROP), PopUpSource, this,
		[this](int32 InIndex) { OnPopUpMenuDrop(InIndex); });

	if (RightClickedItem->IsConsumable())
	{
		MIS::Listen(MSGKEY(MIS_UI_POPUP_CONSUME), PopUpSource, this,
			[this](int32 InIndex) { OnPopUpMenuConsume(InIndex); });
	}
	else
	{
		ItemPopUp->CollapseConsumeButton();
	}
}

void UMIS_InventoryGrid::PutHoverItemBack()
{
	if (!IsValid(HoverItem)) return;

	FMIS_SlotAvailabilityResult Result = HasRoomForItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());
	Result.Item = HoverItem->GetInventoryItem();

	AddStacks(Result);
	ClearHoverItem();
}

void UMIS_InventoryGrid::DropItem()
{
	OnItemUnhovered();

	if (!IsValid(HoverItem)) return;
	if (!IsValid(HoverItem->GetInventoryItem())) return;
	if (!InventoryComponent.IsValid()) return;

	// [解耦重构] UI 只发意图命令, 由数据层监听 MIS.Cmd.DropItem 后自行决定如何处理
	MIS::Emit(MSGKEY(MIS_CMD_DROP_ITEM), GMP::FSigSource(InventoryComponent.Get()),
		HoverItem->GetInventoryItem(), HoverItem->GetStackCount());

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

void UMIS_InventoryGrid::AddItem(UMIS_InventoryItem* Item, int32 UpperLeftIndex)
{
	if (!IsValid(Item)) return;

	// [服务端权威] 优先按服务端分配的落点渲染, UI 不再自己找位置。
	// 这样做同时保证: 各端(含监听服上的本地玩家)看到的布局与服务端一致。
	if (GridSlots.IsValidIndex(UpperLeftIndex))
	{
		const FMIS_GridFragment* GridFragment = GetFragment<FMIS_GridFragment>(Item, MIS_FragmentTags::GridFragment);
		const FMIS_ImageFragment* ImageFragment = GetFragment<FMIS_ImageFragment>(Item, MIS_FragmentTags::IconFragment);
		if (!GridFragment || !ImageFragment)
		{
			DH_LOG_WARN("[背包网格] AddItem 缺少 Grid/Image Fragment, 无法渲染 | Item=%s",
				*Item->GetName());
			return;
		}

		const bool bStackable = Item->IsStackable();
		const int32 StackAmount = Item->GetTotalStackCount();

		UMIS_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, UpperLeftIndex);
		AddSlottedItemToCanvas(UpperLeftIndex, GridFragment, SlottedItem);
		SlottedItems.Add(UpperLeftIndex, SlottedItem);
		UpdateGridSlots(Item, UpperLeftIndex, bStackable, StackAmount);

		return;
	}

	// 回退路径: 落点缺失 (服务端未启用权威分配或数据异常) 时沿用本地空间计算。
	// 正常流程不该走到这里, 因此按告警输出, 便于发现。
	DH_LOG_WARN("[背包网格] 落点缺失, 回退到本地空间计算 | Item=%s", *Item->GetName());

	FMIS_SlotAvailabilityResult Result = HasRoomForItem(Item);

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

	if (!GridFragment || !ImageFragment) return;

	UMIS_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	AddSlottedItemToCanvas(Index, GridFragment, SlottedItem);

	SlottedItems.Add(Index, SlottedItem);
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
	// [解耦重构] 监听该物品图标广播的消息
	const GMP::FSigSource ItemSource(SlottedItem);
	MIS::Listen(MSGKEY(MIS_UI_SLOTTED_ITEM_CLICKED), ItemSource, this,
		[this](int32 InGridIndex, uint8 InMouseButton) { OnSlottedItemClicked(InGridIndex, InMouseButton); });
	MIS::Listen(MSGKEY(MIS_UI_SLOTTED_ITEM_HOVERED), ItemSource, this,
		[this](int32 InGridIndex) { OnSlottedItemHovered(InGridIndex); });
	MIS::Listen(MSGKEY(MIS_UI_SLOTTED_ITEM_UNHOVERED), ItemSource, this,
		[this](int32 InGridIndex) { OnSlottedItemUnhovered(InGridIndex); });

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

	// [解耦重构] UI 只发意图命令
	MIS::Emit(MSGKEY(MIS_CMD_CONSUME_ITEM), GMP::FSigSource(InventoryComponent.Get()), Item);

	if (NewStackCount <= 0)
	{
		RemoveItemFromGrid(Item, UpperLeftIndex);
	}
}

void UMIS_InventoryGrid::OnSlottedItemHovered(int32 GridIndex)
{
	UMIS_InventoryItem* Item = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(Item)) return;

	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);

	const auto& Manifest = Item->GetItemManifest();

	FTimerDelegate DescriptionTimerDelegate;
	DescriptionTimerDelegate.BindLambda([this, Item, &Manifest]()
	{
		UMIS_ItemDescription* DescWidget = GetItemDescription();
		if (!IsValid(DescWidget)) return;

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

	// [解耦重构] 改为广播消息, 外部(如 HUD/描述面板)按需监听
	MIS::Emit(MSGKEY(MIS_UI_GRID_ITEM_HOVER_CHANGED), GMP::FSigSource(this), Item, true);
}

void UMIS_InventoryGrid::OnSlottedItemUnhovered(int32 GridIndex)
{
	OnItemUnhovered();
	MIS::Emit(MSGKEY(MIS_UI_GRID_ITEM_HOVER_CHANGED), GMP::FSigSource(this),
		static_cast<UMIS_InventoryItem*>(nullptr), false);
}

void UMIS_InventoryGrid::OnItemUnhovered()
{
	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);

	ClearEquippedItemDescription();
	GetWorld()->GetTimerManager().ClearTimer(EquippedDescriptionTimer);
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

void UMIS_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	AddItemAtIndex(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	UpdateGridSlots(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	ClearHoverItem();
}

void UMIS_InventoryGrid::OnGridSlotClicked(int32 GridIndex, uint8 MouseButton)
{
	if (!IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(ItemDropIndex)) return;

	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		// 转发给物品点击逻辑, 鼠标键原样透传 (保持"左键拾取 / 右键菜单"语义)
		OnSlottedItemClicked(CurrentQueryResult.UpperLeftIndex, MouseButton);
		return;
	}

	if (!IsInGridBounds(ItemDropIndex, HoverItem->GetGridDimensions())) return;
	auto GridSlot = GridSlots[ItemDropIndex];
	if (!GridSlot->GetInventoryItem().IsValid())
	{
		PutDownOnIndex(ItemDropIndex);
	}
}

void UMIS_InventoryGrid::OnGridSlotHovered(int32 GridIndex)
{
	if (IsValid(HoverItem)) return;

	UMIS_GridSlot* GridSlot = GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetOccupiedTexture();
	}
}

void UMIS_InventoryGrid::OnGridSlotUnhovered(int32 GridIndex)
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
			// [解耦重构] 监听装备槽广播的消息
			MIS::Listen(MSGKEY(MIS_UI_EQUIPPED_GRID_SLOT_CLICKED), GMP::FSigSource(GridSlot), this,
				[this](UMIS_EquippedGridSlot* InSlot, FGameplayTag InTag) { EquippedGridSlotClicked(InSlot, InTag); });
		}
	}
}

void UMIS_InventoryGrid::EquippedGridSlotClicked(UMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag)
{
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

	UMIS_EquippedSlottedItem* EquippedSlottedItem = EquippedGridSlot->OnItemEquipped(
		HoverItem->GetInventoryItem(),
		EquipmentTypeTag,
		TileSize
	);
	// [解耦重构] 监听新装备图标的消息
	MIS::Listen(MSGKEY(MIS_UI_EQUIPPED_SLOTTED_ITEM_CLICKED), GMP::FSigSource(EquippedSlottedItem), this,
		[this](UMIS_EquippedSlottedItem* InItem) { EquippedSlottedItemClicked(InItem); });

	if (InventoryComponent.IsValid())
	{
		MIS::Emit(MSGKEY(MIS_CMD_EQUIP_SLOT), GMP::FSigSource(InventoryComponent.Get()),
			ItemToEquip, static_cast<UMIS_InventoryItem*>(nullptr));
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

	// [解耦重构] 解绑该图标的消息监听 (精确到具体发送方, 不影响其它图标)
	MIS::Unbind(MSGKEY(MIS_UI_EQUIPPED_SLOTTED_ITEM_CLICKED), this, GMP::FSigSource(EquippedSlottedItem));
	EquippedSlottedItem->RemoveFromParent();
}

void UMIS_InventoryGrid::MakeEquippedSlottedItem(UMIS_EquippedSlottedItem* OldSlottedItem, UMIS_EquippedGridSlot* EquippedGridSlot, UMIS_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot) || !IsValid(ItemToEquip)) return;

	UMIS_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(
		ItemToEquip,
		OldSlottedItem->GetEquipmentTypeTag(),
		GetTileSize());
	if (IsValid(SlottedItem))
	{
		MIS::Listen(MSGKEY(MIS_UI_EQUIPPED_SLOTTED_ITEM_CLICKED), GMP::FSigSource(SlottedItem), this,
			[this](UMIS_EquippedSlottedItem* InItem) { EquippedSlottedItemClicked(InItem); });
	}

	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UMIS_InventoryGrid::BroadcastSlotClickedDelegates(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip) const
{
	if (InventoryComponent.IsValid())
	{
		// [解耦重构] UI 只发意图命令
		MIS::Emit(MSGKEY(MIS_CMD_EQUIP_SLOT), GMP::FSigSource(InventoryComponent.Get()),
			ItemToEquip, ItemToUnequip);
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-UI] BroadcastSlotClickedDelegates失败: InventoryComponent无效");
	}
}
