// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Inventory/Spatial/SBI_InventoryGrid.h"

#include "DS_DebugFunctionLibrary.h"
#include "SandboxInventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/SBI_InventoryComponent.h"
#include "Items/IC_InventoryItem.h"
#include "Items/IC_ItemFragment.h"
#include "Widgets/Inventory/GridSlots/SBI_GridSlot.h"
#include "Widgets/Inventory/HoverItem/SBI_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/SBI_SlottedItem.h"
#include "Widgets/Inventory/ItemPopUp/SBI_ItemPopUp.h"

void USBI_InventoryGrid::NativePreConstruct()
{
	Super::NativePreConstruct();
	ClearGrid();
	ConstructGrid();
}

void USBI_InventoryGrid::InitFromComponent(USBI_InventoryComponent* InInventoryComponent, UCanvasPanel* InCanvasPanel)
{
	InventoryComponent = InInventoryComponent;
	OwningCanvasPanel = InCanvasPanel;

	DS_PRINT(Inventory, 5.f, DSColors::Orange,
		"[沙盒网格] InitFromComponent | InvComp=%s | Canvas=%s | GridSlots=%d",
		IsValid(InInventoryComponent) ? TEXT("有效") : TEXT("空"),
		IsValid(InCanvasPanel) ? TEXT("有效") : TEXT("空"),
		GridSlots.Num());

	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
		InventoryComponent->OnItemRemoved.AddDynamic(this, &ThisClass::OnExternalItemRemoved);
		DS_PRINT(Inventory, 3.f, DSColors::Orange,
			"[沙盒网格] 已绑定 OnItemAdded + OnItemRemoved 委托");
	}
}

// ===== 网格构建 =====

void USBI_InventoryGrid::ConstructGrid()
{
	if (!GridSlotClass || Columns <= 0 || Rows <= 0) return;
	if (!OwningCanvasPanel.IsValid()) return;

	GridSlots.Reserve(Rows * Columns);

	for (int32 j = 0; j < Rows; ++j)
	{
		for (int32 i = 0; i < Columns; ++i)
		{
			USBI_GridSlot* GridSlot = CreateWidget<USBI_GridSlot>(this, GridSlotClass);
			OwningCanvasPanel->AddChild(GridSlot);

			const int32 Index = j * Columns + i;
			GridSlot->SetTileIndex(Index);

			UCanvasPanelSlot* GridCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			GridCPS->SetSize(FVector2D(TileSize));
			GridCPS->SetPosition(FVector2D(i * TileSize, j * TileSize));

			GridSlots.Add(GridSlot);
			GridSlot->SetUnoccupiedTexture();
			GridSlot->GridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
			GridSlot->GridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
			GridSlot->GridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnhovered);
		}
	}
}

void USBI_InventoryGrid::ClearGrid()
{
	for (USBI_GridSlot* GridSlot : GridSlots)
	{
		if (IsValid(GridSlot))
		{
			GridSlot->RemoveFromParent();
		}
	}
	GridSlots.Empty();

	for (auto& Pair : SlottedItems)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->RemoveFromParent();
		}
	}
	SlottedItems.Empty();
}

void USBI_InventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!OwningCanvasPanel.IsValid()) return;

	const FGeometry& CanvasGeo = OwningCanvasPanel->GetCachedGeometry();
	FVector2D _, CanvasPosition;
	USlateBlueprintLibrary::LocalToViewport(OwningCanvasPanel.Get(), CanvasGeo,
		USlateBlueprintLibrary::GetLocalTopLeft(CanvasGeo), _, CanvasPosition);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	if (IsValid(HoverItem))
	{
		HoverItem->SetVisibility(ESlateVisibility::Visible);
		UCanvasPanelSlot* HoverCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(HoverItem);
		HoverCPS->SetPosition(MousePosition - CanvasPosition - FVector2D(TileSize * 0.5f, TileSize * 0.5f));
	}
}

// ===== 物品添加 =====

void USBI_InventoryGrid::AddItem(UIC_InventoryItem* Item)
{
	if (!IsValid(Item)) return;

	DS_LOG(Inventory, "网格 AddItem: %s (Type: %s, Stack: %d)",
		*Item->GetName(), *Item->GetItemType().ToString(), Item->GetTotalStackCount());

	// 查找该物品所在的槽位
	const bool bStackable = Item->IsStackable();
	const int32 StackCount = Item->GetTotalStackCount();

	// 遍历所有 GridSlot，找到匹配的物品
	for (int32 i = 0; i < GridSlots.Num(); ++i)
	{
		if (SlottedItems.Contains(i)) continue; // 已经有物品

		if (IsValid(GridSlots[i]->GetInventoryItem()) && GridSlots[i]->GetInventoryItem() == Item)
		{
			AddItemAtIndex(Item, i, bStackable, StackCount);
			return;
		}
	}

	// 没找到对应的槽位，找第一个空槽位
	for (int32 i = 0; i < GridSlots.Num(); ++i)
	{
		if (!SlottedItems.Contains(i))
		{
			AddItemAtIndex(Item, i, bStackable, StackCount);
			return;
		}
	}
}

void USBI_InventoryGrid::AddItemAtIndex(UIC_InventoryItem* Item, int32 Index, bool bStackable, int32 StackAmount)
{
	if (!GridSlots.IsValidIndex(Index)) return;

	// 如果已有物品在此槽位，更新堆叠
	if (TObjectPtr<USBI_SlottedItem>* Existing = SlottedItems.Find(Index))
	{
		(*Existing)->UpdateStackCount(StackAmount);
		(*Existing)->SetIsStackable(bStackable);
		GridSlots[Index]->SetStackCount(StackAmount);
		GridSlots[Index]->SetInventoryItem(Item);
		GridSlots[Index]->SetOccupiedTexture();
		return;
	}

	USBI_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, Index);
	SlottedItems.Add(Index, SlottedItem);
	GridSlots[Index]->SetStackCount(StackAmount);
	GridSlots[Index]->SetInventoryItem(Item);
	GridSlots[Index]->SetOccupiedTexture();
}

USBI_SlottedItem* USBI_InventoryGrid::CreateSlottedItem(UIC_InventoryItem* Item, bool bStackable, int32 StackAmount, int32 Index)
{
	if (!SlottedItemClass) return nullptr;

	USBI_SlottedItem* SlottedItem = CreateWidget<USBI_SlottedItem>(this, SlottedItemClass);
	OwningCanvasPanel->AddChild(SlottedItem);

	const FIC_ImageFragment* ImageFragment = GetFragment<FIC_ImageFragment>(Item, FGameplayTag::EmptyTag);
	SetSlottedItemImage(SlottedItem, ImageFragment);

	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetInventoryItem(Item);
	SlottedItem->SetIsStackable(bStackable);
	SlottedItem->UpdateStackCount(StackAmount);

	UCanvasPanelSlot* SlotCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	SlotCPS->SetSize(FVector2D(TileSize));

	int32 Row, Col;
	Row = Index / Columns;
	Col = Index % Columns;
	SlotCPS->SetPosition(FVector2D(Col * TileSize, Row * TileSize));

	SlottedItem->OnSlottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
	SlottedItem->OnSlottedItemHovered.AddDynamic(this, &ThisClass::OnSlottedItemHovered);
	SlottedItem->OnSlottedItemUnhovered.AddDynamic(this, &ThisClass::OnSlottedItemUnhovered);

	return SlottedItem;
}

void USBI_InventoryGrid::SetSlottedItemImage(const USBI_SlottedItem* SlottedItem, const FIC_ImageFragment* ImageFragment) const
{
	if (!SlottedItem || !ImageFragment) return;

	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.ImageSize = ImageFragment->GetIconDimensions();
	SlottedItem->SetImageBrush(Brush);
}

void USBI_InventoryGrid::OnExternalItemRemoved(UIC_InventoryItem* Item)
{
	if (!IsValid(Item)) return;

	for (auto It = SlottedItems.CreateIterator(); It; ++It)
	{
		if (It.Value()->GetInventoryItem() == Item)
		{
			DS_LOG(Inventory, "网格移除物品: %s | 槽位=%d", *Item->GetName(), It.Key());

			It.Value()->RemoveFromParent();
			GridSlots[It.Key()]->SetInventoryItem(nullptr);
			GridSlots[It.Key()]->SetStackCount(0);
			GridSlots[It.Key()]->SetUnoccupiedTexture();
			It.RemoveCurrent();
			break;
		}
	}
}

// ===== 空间查询 =====

FSBI_SlotAvailabilityResult USBI_InventoryGrid::HasRoomForItem(const UIC_ItemComponent* ItemComponent)
{
	if (InventoryComponent.IsValid())
	{
		return const_cast<USBI_InventoryComponent*>(InventoryComponent.Get())->HasRoomForItem(const_cast<UIC_ItemComponent*>(ItemComponent));
	}
	return FSBI_SlotAvailabilityResult();
}

// ===== 拖拽 =====

void USBI_InventoryGrid::AssignHoverItem(UIC_InventoryItem* InventoryItem)
{
	if (!IsValid(InventoryItem)) return;

	DS_LOG(Inventory, "AssignHoverItem: %s", *InventoryItem->GetName());

	if (!HoverItem && HoverItemClass)
	{
		HoverItem = CreateWidget<USBI_HoverItem>(this, HoverItemClass);
		OwningCanvasPanel->AddChild(HoverItem);
	}

	if (HoverItem)
	{
		const FIC_ImageFragment* ImageFragment = GetFragment<FIC_ImageFragment>(InventoryItem, FGameplayTag::EmptyTag);
		if (ImageFragment)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(ImageFragment->GetIcon());
			Brush.ImageSize = ImageFragment->GetIconDimensions();
			HoverItem->SetImageBrush(Brush);
		}

		HoverItem->SetInventoryItem(InventoryItem);
		HoverItem->SetIsStackable(InventoryItem->IsStackable());
		HoverItem->UpdateStackCount(InventoryItem->GetTotalStackCount());

		UCanvasPanelSlot* HoverCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(HoverItem);
		HoverCPS->SetSize(FVector2D(TileSize));
		HoverCPS->SetZOrder(100);
	}
}

void USBI_InventoryGrid::ClearHoverItem()
{
	if (HoverItem)
	{
		HoverItem->RemoveFromParent();
		HoverItem = nullptr;
	}
}

bool USBI_InventoryGrid::HasHoverItem() const
{
	return IsValid(HoverItem);
}

USBI_HoverItem* USBI_InventoryGrid::GetHoverItem() const
{
	return HoverItem;
}

void USBI_InventoryGrid::DropItem()
{
	if (!IsValid(HoverItem)) return;

	UIC_InventoryItem* Item = HoverItem->GetInventoryItem();
	if (!IsValid(Item)) return;

	DS_LOG(Inventory, "DropItem: %s -> 目标槽位=%d", *Item->GetName(), ItemDropIndex);

	// 如果目标槽位已有物品，交换
	if (TObjectPtr<USBI_SlottedItem>* TargetItem = SlottedItems.Find(ItemDropIndex))
	{
		UIC_InventoryItem* TargetInventoryItem = (*TargetItem)->GetInventoryItem();
		int32 PrevIndex = HoverItem->GetPreviousGridIndex();

		// 交换
		SlottedItems.Remove(ItemDropIndex);
		SlottedItems.Remove(PrevIndex);

		if (PrevIndex >= 0)
		{
			SlottedItems.Add(PrevIndex, *TargetItem);
			(*TargetItem)->SetGridIndex(PrevIndex);
			UCanvasPanelSlot* CPSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(*TargetItem);
			int32 Row, Col;
			Row = PrevIndex / Columns;
			Col = PrevIndex % Columns;
			CPSlot->SetPosition(FVector2D(Col * TileSize, Row * TileSize));
			GridSlots[PrevIndex]->SetInventoryItem(TargetInventoryItem);
			GridSlots[PrevIndex]->SetOccupiedTexture();
		}

		// 获取原来 HoverItem 的 slotted item
		TObjectPtr<USBI_SlottedItem>* MovedItem = SlottedItems.Find(PrevIndex);
		// 移动原来的物品到目标槽位
		if (TObjectPtr<USBI_SlottedItem>* OrigItem = SlottedItems.Find(PrevIndex))
		{
			SlottedItems.Remove(PrevIndex);
		}

		// 重新创建目标位置的 slotted item
		USBI_SlottedItem* NewSlotted = CreateSlottedItem(Item, Item->IsStackable(), Item->GetTotalStackCount(), ItemDropIndex);
		SlottedItems.Add(ItemDropIndex, NewSlotted);
		GridSlots[ItemDropIndex]->SetInventoryItem(Item);
		GridSlots[ItemDropIndex]->SetStackCount(Item->GetTotalStackCount());
		GridSlots[ItemDropIndex]->SetOccupiedTexture();
	}
	else
	{
		// 目标为空，移动
		int32 PrevIndex = HoverItem->GetPreviousGridIndex();

		if (TObjectPtr<USBI_SlottedItem>* OrigItem = SlottedItems.Find(PrevIndex))
		{
			SlottedItems.Remove(PrevIndex);
			(*OrigItem)->SetGridIndex(ItemDropIndex);
			SlottedItems.Add(ItemDropIndex, *OrigItem);

			UCanvasPanelSlot* CPSlot2 = UWidgetLayoutLibrary::SlotAsCanvasSlot(*OrigItem);
			int32 Row, Col;
			Row = ItemDropIndex / Columns;
			Col = ItemDropIndex % Columns;
			CPSlot2->SetPosition(FVector2D(Col * TileSize, Row * TileSize));

			GridSlots[PrevIndex]->SetInventoryItem(nullptr);
			GridSlots[PrevIndex]->SetStackCount(0);
			GridSlots[PrevIndex]->SetUnoccupiedTexture();

			GridSlots[ItemDropIndex]->SetInventoryItem(Item);
			GridSlots[ItemDropIndex]->SetStackCount(Item->GetTotalStackCount());
			GridSlots[ItemDropIndex]->SetOccupiedTexture();
		}
	}

	ClearHoverItem();
}

// ===== 显示 =====

void USBI_InventoryGrid::ShowCursor()
{
	SetVisibility(ESlateVisibility::Visible);
}

void USBI_InventoryGrid::HideCursor()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void USBI_InventoryGrid::OnHide()
{
	ClearHoverItem();
}

void USBI_InventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

// ===== GridSlot 事件 =====

void USBI_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	DS_LOG(Inventory, "GridSlot 点击: %d | 按键=%s", GridIndex, *MouseEvent.GetEffectingButton().ToString());

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// 右键弹出菜单
		if (ItemPopUpClass && GridSlots.IsValidIndex(GridIndex))
		{
			USBI_GridSlot* GridSlot = GridSlots[GridIndex];
			if (IsValid(GridSlot->GetInventoryItem()))
			{
				USBI_ItemPopUp* PopUp = CreateWidget<USBI_ItemPopUp>(this, ItemPopUpClass);
				OwningCanvasPanel->AddChild(PopUp);

				FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
				const FGeometry& CanvasGeo = OwningCanvasPanel->GetCachedGeometry();
				FVector2D _, CanvasPos;
				USlateBlueprintLibrary::LocalToViewport(OwningCanvasPanel.Get(), CanvasGeo,
					USlateBlueprintLibrary::GetLocalTopLeft(CanvasGeo), _, CanvasPos);

				UCanvasPanelSlot* PopUpCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(PopUp);
				PopUpCPS->SetPosition(MousePos - CanvasPos);

				GridSlot->SetItemPopUp(PopUp);

				UIC_InventoryItem* Item = GridSlot->GetInventoryItem();
				if (Item->IsStackable())
				{
					PopUp->SetSliderParams(Item->GetTotalStackCount(), 1);
				}
				else
				{
					PopUp->CollapseSplitButton();
				}

				PopUp->OnSplit.BindDynamic(this, &ThisClass::OnItemSplit);
				PopUp->OnDrop.BindDynamic(this, &ThisClass::OnItemDrop);
				PopUp->OnConsume.BindDynamic(this, &ThisClass::OnItemConsume);

				DS_LOG(Inventory, "打开 ItemPopUp: 槽位=%d | Item=%s", GridIndex, *Item->GetName());
			}
		}
	}
}

void USBI_InventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	ItemDropIndex = GridIndex;
}

void USBI_InventoryGrid::OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
}

// ===== SlottedItem 事件 =====

void USBI_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (TObjectPtr<USBI_SlottedItem>* SlottedItem = SlottedItems.Find(GridIndex))
		{
			UIC_InventoryItem* Item = (*SlottedItem)->GetInventoryItem();
			if (IsValid(Item))
			{
				DS_LOG(Inventory, "开始拖拽物品: 槽位=%d | Item=%s", GridIndex, *Item->GetName());
				AssignHoverItem(Item);
				HoverItem->SetPreviousGridIndex(GridIndex);
			}
		}
	}
}

void USBI_InventoryGrid::OnSlottedItemHovered(int32 GridIndex)
{
	if (TObjectPtr<USBI_SlottedItem>* SlottedItem = SlottedItems.Find(GridIndex))
	{
		UIC_InventoryItem* Item = (*SlottedItem)->GetInventoryItem();
		if (IsValid(Item))
		{
			OnGridItemHovered.Broadcast(Item);
		}
	}
}

void USBI_InventoryGrid::OnSlottedItemUnhovered(int32 GridIndex)
{
	OnGridItemUnhovered.Broadcast();
}

// ===== PopUp 回调 =====

void USBI_InventoryGrid::OnItemSplit(int32 SplitAmount, int32 Index)
{
	DS_LOG(Inventory, "拆分物品: 槽位=%d | 数量=%d", Index, SplitAmount);

	if (TObjectPtr<USBI_SlottedItem>* SlottedItem = SlottedItems.Find(Index))
	{
		UIC_InventoryItem* Item = (*SlottedItem)->GetInventoryItem();
		if (IsValid(Item) && InventoryComponent.IsValid())
		{
			InventoryComponent->RequestDropItem(Item, SplitAmount);
		}
	}
}

void USBI_InventoryGrid::OnItemDrop(int32 Index)
{
	DS_LOG(Inventory, "丢弃物品: 槽位=%d", Index);

	if (TObjectPtr<USBI_SlottedItem>* SlottedItem = SlottedItems.Find(Index))
	{
		UIC_InventoryItem* Item = (*SlottedItem)->GetInventoryItem();
		if (IsValid(Item) && InventoryComponent.IsValid())
		{
			InventoryComponent->RequestDropItem(Item, Item->GetTotalStackCount());
		}
	}
}

void USBI_InventoryGrid::OnItemConsume(int32 Index)
{
	DS_LOG(Inventory, "消耗物品: 槽位=%d", Index);

	if (TObjectPtr<USBI_SlottedItem>* SlottedItem = SlottedItems.Find(Index))
	{
		UIC_InventoryItem* Item = (*SlottedItem)->GetInventoryItem();
		if (IsValid(Item) && InventoryComponent.IsValid())
		{
			InventoryComponent->RequestConsumeItem(Item);
		}
	}
}