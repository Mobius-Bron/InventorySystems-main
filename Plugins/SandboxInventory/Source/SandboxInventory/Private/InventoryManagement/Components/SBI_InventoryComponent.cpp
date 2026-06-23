// Copyright AmberAeolian. All Rights Reserved.

#include "InventoryManagement/Components/SBI_InventoryComponent.h"
#include "DS_DebugFunctionLibrary.h"

#include "SandboxInventory.h"
#include "Items/IC_InventoryItem.h"
#include "Items/IC_ItemFragment.h"
#include "Components/IC_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Inventory/InventoryBase/SBI_InventoryWidget.h"

namespace
{
	/** 获取物品的最大堆叠数 */
	int32 GetMaxStackSize(const UIC_InventoryItem* Item)
	{
		if (!IsValid(Item)) return 1;
		const FIC_StackableFragment* Stackable = GetFragment<FIC_StackableFragment>(Item, FGameplayTag::EmptyTag);
		return Stackable ? Stackable->GetMaxStackSize() : 1;
	}

	/** 获取物品的堆叠数 */
	int32 GetStackCount(const UIC_InventoryItem* Item)
	{
		if (!IsValid(Item)) return 0;
		const FIC_StackableFragment* Stackable = GetFragment<FIC_StackableFragment>(Item, FGameplayTag::EmptyTag);
		return Stackable ? Stackable->GetStackCount() : 1;
	}
}

USBI_InventoryComponent::USBI_InventoryComponent() : InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void USBI_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, InventoryList);
	DOREPLIFETIME(ThisClass, ExternalInventoryComponent);
}

// ===== 初始化 =====

void USBI_InventoryComponent::Init(APlayerController* InPC)
{
	DS_PRINT(Inventory, 4.f, DSColors::Cyan,
		"[沙盒背包] >>> Init | PC=%s",
		IsValid(InPC) ? *InPC->GetName() : TEXT("空"));

	if (IsValid(InPC))
	{
		OwningController = InPC;
		DS_PRINT(Inventory, 4.f, FLinearColor::Green,
			"[沙盒背包] Init 完成 | 网格=%dx%d | 总槽位=%d",
			Columns, Rows, Columns * Rows);
	}
	else
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒背包] Init 警告: PlayerController 为空!");
	}
}

// ===== 拾取流程 =====

void USBI_InventoryComponent::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;

	FVector TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(OwningController.Get(), ViewportCenter, TraceStart, Forward)) return;

	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility);

	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();

	if (ThisActor != LastActor) return;
}

void USBI_InventoryComponent::PrimaryInteract()
{
	DS_PRINT(Inventory, 2.f, DSColors::Cyan,
		"[沙盒背包] 拾取交互 | ThisActor=%s",
		ThisActor.IsValid() ? *ThisActor->GetName() : TEXT("空"));

	if (!ThisActor.IsValid())
	{
		DS_LOG_WARN(Inventory, "拾取失败: 没有瞄准任何物体");
		return;
	}

	UIC_ItemComponent* ItemComp = ThisActor->FindComponentByClass<UIC_ItemComponent>();
	if (!IsValid(ItemComp))
	{
		DS_LOG_ERR(Inventory, "拾取失败: 目标物体没有 ItemComponent");
		return;
	}

	DS_PRINT(Inventory, 2.f, DSColors::Cyan,
		"[沙盒背包] 找到物品组件: %s, 准备拾取", *ItemComp->GetName());

	TryAddItem(ItemComp);
}

void USBI_InventoryComponent::ToggleInventory()
{
	DS_PRINT(Inventory, 3.f, DSColors::Cyan,
		"[沙盒背包] ToggleInventory | 当前状态=%s",
		bInventoryOpen ? TEXT("打开") : TEXT("关闭"));

	if (!IsValid(InventoryWidget))
	{
		DS_LOG_WARN(Inventory, "ToggleInventory 失败: InventoryWidget 为空");
		return;
	}
	if (!OwningController.IsValid())
	{
		DS_LOG_WARN(Inventory, "ToggleInventory 失败: OwningController 为空");
		return;
	}

	if (bInventoryOpen)
	{
		bInventoryOpen = false;
		InventoryWidget->CloseInventory();
		OwningController->SetInputMode(FInputModeGameOnly());
		OwningController->SetShowMouseCursor(false);
		DS_LOG(Inventory, "背包已关闭");
	}
	else
	{
		bInventoryOpen = true;
		InventoryWidget->OpenInventory();
		OwningController->SetInputMode(FInputModeGameAndUI());
		OwningController->SetShowMouseCursor(true);
		DS_LOG(Inventory, "背包已打开");
	}
}

// ===== 物品添加 =====

void USBI_InventoryComponent::TryAddItem(UIC_ItemComponent* ItemComponent)
{
	DS_PRINT(Inventory, 3.f, DSColors::Cyan,
		"[沙盒背包] >>> TryAddItem | ItemComp=%s | Widget=%s",
		IsValid(ItemComponent) ? TEXT("有效") : TEXT("空"),
		IsValid(InventoryWidget) ? TEXT("有效") : TEXT("空"));

	if (!IsValid(ItemComponent)) return;
	if (!IsValid(InventoryWidget))
	{
		DS_PRINT(Inventory, 5.f, FLinearColor::Red,
			"[沙盒背包] TryAddItem 失败: InventoryWidget 为空!");
		return;
	}

	FSBI_SlotAvailabilityResult Result = HasRoomForItem(ItemComponent);

	DS_PRINT(Inventory, 3.f, DSColors::Cyan,
		"[沙盒背包] HasRoomForItem | 总空间=%d | 槽位数=%d",
		Result.TotalRoomToFill, Result.SlotAvailabilities.Num());

	DS_LOG(Inventory, "HasRoomForItem 结果 | 总空间=%d | 可堆叠=%d | 剩余=%d",
		Result.TotalRoomToFill, Result.bStackable, Result.Remainder);

	if (Result.TotalRoomToFill == 0)
	{
		DS_LOG_WARN(Inventory, "库存已满!");
		NoRoomInInventory.Broadcast();
		return;
	}

	if (Result.Item.IsValid() && Result.bStackable)
	{
		DS_LOG(Inventory, "堆叠已有物品 | 填充=%d | 剩余=%d", Result.TotalRoomToFill, Result.Remainder);
		OnStackChange.Broadcast(Result);
		Server_AddStacksToItem(ItemComponent->GetOwner(), Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		DS_LOG(Inventory, "新建物品条目 | 堆叠数=%d | 剩余=%d",
			Result.bStackable ? Result.TotalRoomToFill : 0, Result.Remainder);
		Server_AddNewItem(ItemComponent->GetOwner(), Result.bStackable ? Result.TotalRoomToFill : 0, Result.Remainder);
	}
}

void USBI_InventoryComponent::Server_AddNewItem_Implementation(AActor* ItemActor, int32 StackCount, int32 Remainder)
{
	if (!IsValid(ItemActor))
	{
		DS_PRINT(Inventory, 5.f, FLinearColor::Red,
			"[沙盒背包] Server_AddNewItem 失败: ItemActor 为空");
		return;
	}

	UIC_ItemComponent* ItemComponent = ItemActor->FindComponentByClass<UIC_ItemComponent>();
	if (!IsValid(ItemComponent))
	{
		DS_PRINT(Inventory, 5.f, FLinearColor::Red,
			"[沙盒背包] Server_AddNewItem 失败: ItemActor(%s) 上没有 ItemComponent", *ItemActor->GetName());
		return;
	}

	DS_PRINT(Inventory, 3.f, FLinearColor::Green,
		"[沙盒背包] Server_AddNewItem | Actor=%s | 堆叠=%d | 剩余=%d",
		*ItemActor->GetName(), StackCount, Remainder);

	UIC_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent);
	if (!NewItem) return;

	NewItem->SetTotalStackCount(StackCount);

	// 放入第一个空槽位
	int32 EmptySlot = FindFirstEmptySlot();
	if (EmptySlot != INDEX_NONE)
	{
		SlotToItem.Add(EmptySlot, NewItem);
		DS_LOG(Inventory, "物品放入槽位 [%d]: %s", EmptySlot, *NewItem->GetName());
	}

	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒背包] 新物品创建完成 | Item=%s | 槽位=%d",
		*NewItem->GetName(), EmptySlot);

	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		OnItemAdded.Broadcast(NewItem);
	}

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
		DS_PRINT(Inventory, 2.f, FLinearColor::Green, "[沙盒背包] 物品全部拾取,世界Actor已销毁");
	}
	else if (FIC_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FIC_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
		DS_PRINT(Inventory, 2.f, FLinearColor::Green, "[沙盒背包] 部分拾取,剩余=%d", Remainder);
	}
}

void USBI_InventoryComponent::Server_AddStacksToItem_Implementation(AActor* ItemActor, int32 StackCount, int32 Remainder)
{
	if (!IsValid(ItemActor))
	{
		DS_PRINT(Inventory, 5.f, FLinearColor::Red,
			"[沙盒背包] Server_AddStacks 失败: ItemActor 为空");
		return;
	}

	UIC_ItemComponent* ItemComponent = ItemActor->FindComponentByClass<UIC_ItemComponent>();
	if (!IsValid(ItemComponent))
	{
		DS_PRINT(Inventory, 5.f, FLinearColor::Red,
			"[沙盒背包] Server_AddStacks 失败: ItemActor(%s) 上没有 ItemComponent", *ItemActor->GetName());
		return;
	}

	DS_PRINT(Inventory, 3.f, FLinearColor::Green,
		"[沙盒背包] Server_AddStacks | Actor=%s | 堆叠=%d", *ItemActor->GetName(), StackCount);

	const FGameplayTag& ItemType = ItemComponent->GetItemManifest().GetItemType();
	UIC_InventoryItem* Item = InventoryList.FindFirstItemByType(ItemType);
	if (!IsValid(Item)) return;

	Item->SetTotalStackCount(Item->GetTotalStackCount() + StackCount);

	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒背包] Server_AddStacks | 新增=%d | 现在总量=%d | 剩余=%d",
		StackCount, Item->GetTotalStackCount(), Remainder);

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
		DS_PRINT(Inventory, 2.f, FLinearColor::Green, "[沙盒背包] 堆叠完成,世界Actor已销毁");
	}
	else if (FIC_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FIC_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
		DS_PRINT(Inventory, 2.f, FLinearColor::Green, "[沙盒背包] 部分堆叠,世界剩余=%d", Remainder);
	}
}

// ===== 物品丢弃 =====

void USBI_InventoryComponent::RequestDropItem(UIC_InventoryItem* ItemToDrop, int32 StackCount)
{
	DS_PRINT(Inventory, 3.f, DSColors::Cyan,
		"[沙盒背包] RequestDropItem | Item=%s | 数量=%d",
		IsValid(ItemToDrop) ? *ItemToDrop->GetName() : TEXT("空"), StackCount);

	if (!IsValid(ItemToDrop))
	{
		DS_LOG_WARN(Inventory, "RequestDropItem 失败: Item 为空");
		return;
	}

	Server_DropItem(ItemToDrop, StackCount);
}

void USBI_InventoryComponent::Server_DropItem_Implementation(UIC_InventoryItem* Item, int32 StackCount)
{
	if (!IsValid(Item))
	{
		DS_LOG_ERR(Inventory, "Server_DropItem 失败: Item 为空");
		return;
	}

	DS_LOG(Inventory, "丢弃物品: %s | 丢弃数量=%d | 当前总量=%d",
		*Item->GetName(), StackCount, Item->GetTotalStackCount());

	const int32 NewStackCount = Item->GetTotalStackCount() - StackCount;
	if (NewStackCount <= 0)
	{
		// 移除对应的槽位映射
		for (auto& Pair : SlotToItem)
		{
			if (Pair.Value == Item)
			{
				SlotToItem.Remove(Pair.Key);
				DS_LOG(Inventory, "移除槽位映射 [%d]", Pair.Key);
				break;
			}
		}
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	SpawnDroppedItem(Item, StackCount);
}

void USBI_InventoryComponent::SpawnDroppedItem(UIC_InventoryItem* Item, int32 StackCount)
{
	if (!OwningController.IsValid())
	{
		DS_PRINT(Inventory, 5.f, FLinearColor::Red,
			"[沙盒背包] SpawnDroppedItem 失败: OwningController 为空");
		return;
	}

	const APawn* OwningPawn = OwningController->GetPawn();
	if (!IsValid(OwningPawn)) return;

	FVector RotatedForward = OwningPawn->GetActorForwardVector();
	RotatedForward = RotatedForward.RotateAngleAxis(FMath::FRandRange(DropSpawnAngleMin, DropSpawnAngleMax), FVector::UpVector);

	const FVector SpawnLocation = OwningPawn->GetActorLocation() + RotatedForward * 200.f;
	const FRotator SpawnRotation = OwningPawn->GetActorRotation();

	Item->GetItemManifestMutable().SpawnPickupActor(GetWorld(), SpawnLocation, SpawnRotation);

	DS_LOG(Inventory, "物品丢弃到世界: %s | 数量=%d | 位置=%s",
		*Item->GetName(), StackCount, *SpawnLocation.ToString());
}

// ===== 外部容器交互 =====

void USBI_InventoryComponent::OpenExternalContainer(USBI_InventoryComponent* ExternalInventory)
{
	DS_PRINT(Inventory, 4.f, DSColors::Orange,
		"[沙盒背包] >>> OpenExternalContainer | External=%s",
		IsValid(ExternalInventory) ? *ExternalInventory->GetName() : TEXT("空"));

	if (!IsValid(ExternalInventory)) return;

	ExternalInventoryComponent = ExternalInventory;
	DS_LOG(Inventory, "已打开外部容器: %s", *ExternalInventory->GetName());
}

void USBI_InventoryComponent::CloseExternalContainer()
{
	DS_PRINT(Inventory, 4.f, DSColors::Orange,
		"[沙盒背包] CloseExternalContainer | 当前=%s",
		IsValid(ExternalInventoryComponent) ? *ExternalInventoryComponent->GetName() : TEXT("空"));

	ExternalInventoryComponent = nullptr;
	DS_LOG(Inventory, "已关闭外部容器");
}

void USBI_InventoryComponent::TransferItemToExternal(UIC_InventoryItem* Item, int32 StackCount)
{
	DS_PRINT(Inventory, 4.f, DSColors::Cyan,
		"[沙盒背包] >>> TransferItemToExternal | Item=%s | 数量=%d | External=%s",
		IsValid(Item) ? *Item->GetName() : TEXT("空"),
		StackCount,
		IsValid(ExternalInventoryComponent) ? *ExternalInventoryComponent->GetName() : TEXT("空"));

	if (!IsValid(Item) || !IsValid(ExternalInventoryComponent))
	{
		DS_LOG_WARN(Inventory, "TransferItemToExternal 失败: Item=%d | External=%d",
			IsValid(Item), IsValid(ExternalInventoryComponent));
		return;
	}

	// 检查外部容器空间
	FSBI_SlotAvailabilityResult Result = ExternalInventoryComponent->HasRoomForItem(Item, StackCount);
	if (Result.TotalRoomToFill == 0)
	{
		DS_LOG_WARN(Inventory, "外部容器已满");
		ExternalInventoryComponent->NoRoomInInventory.Broadcast();
		return;
	}

	// 从自身移除
	Server_DropItem(Item, StackCount);

	// 委托外部容器添加 - 实际需要网络同步,这里简化处理
	DS_LOG(Inventory, "物品转移至外部容器: %s (%d)", *Item->GetName(), StackCount);
}

void USBI_InventoryComponent::TransferItemFromExternal(UIC_InventoryItem* Item, int32 StackCount)
{
	DS_PRINT(Inventory, 4.f, DSColors::Cyan,
		"[沙盒背包] >>> TransferItemFromExternal | Item=%s | 数量=%d | External=%s",
		IsValid(Item) ? *Item->GetName() : TEXT("空"),
		StackCount,
		IsValid(ExternalInventoryComponent) ? *ExternalInventoryComponent->GetName() : TEXT("空"));

	if (!IsValid(Item) || !IsValid(ExternalInventoryComponent))
	{
		DS_LOG_WARN(Inventory, "TransferItemFromExternal 失败: Item=%d | External=%d",
			IsValid(Item), IsValid(ExternalInventoryComponent));
		return;
	}

	// 检查自身空间
	FSBI_SlotAvailabilityResult Result = HasRoomForItem(Item, StackCount);
	if (Result.TotalRoomToFill == 0)
	{
		DS_LOG_WARN(Inventory, "自身背包已满");
		NoRoomInInventory.Broadcast();
		return;
	}

	// 从外部容器移除
	ExternalInventoryComponent->Server_DropItem(Item, StackCount);

	DS_LOG(Inventory, "物品从外部容器转移: %s (%d)", *Item->GetName(), StackCount);
}

void USBI_InventoryComponent::RequestConsumeItem(UIC_InventoryItem* ItemToConsume)
{
	DS_PRINT(Inventory, 3.f, DSColors::Cyan,
		"[沙盒背包] RequestConsumeItem | Item=%s",
		IsValid(ItemToConsume) ? *ItemToConsume->GetName() : TEXT("空"));

	if (!IsValid(ItemToConsume))
	{
		DS_LOG_WARN(Inventory, "RequestConsumeItem 失败: Item 为空");
		return;
	}

	Server_ConsumeItem(ItemToConsume);
}

// ===== 装备交互 =====

void USBI_InventoryComponent::RequestEquipSlotClicked(UIC_InventoryItem* ItemToEquip, UIC_InventoryItem* ItemToUnequip)
{
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] >>> RequestEquipSlotClicked (Client→Server) | Equip=%s | Unequip=%s",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"));

	Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void USBI_InventoryComponent::Server_EquipSlotClicked_Implementation(UIC_InventoryItem* ItemToEquip, UIC_InventoryItem* ItemToUnequip)
{
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] >>> Server_EquipSlotClicked (Server RPC) | Equip=%s | Unequip=%s | Role=%d",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"),
		(int32)GetOwnerRole());

	Multicast_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void USBI_InventoryComponent::Multicast_EquipSlotClicked_Implementation(UIC_InventoryItem* ItemToEquip, UIC_InventoryItem* ItemToUnequip)
{
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] >>> Multicast_EquipSlotClicked | Equip=%s | Unequip=%s | bHasEquipListener=%d",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"),
		OnItemEquipped.IsBound());

	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);

	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] Multicast_EquipSlotClicked 广播完成");
}

void USBI_InventoryComponent::Server_ConsumeItem_Implementation(UIC_InventoryItem* Item)
{
	if (!IsValid(Item))
	{
		DS_LOG_ERR(Inventory, "Server_ConsumeItem 失败: Item 为空");
		return;
	}

	DS_LOG(Inventory, "消耗物品: %s | 当前总量=%d", *Item->GetName(), Item->GetTotalStackCount());

	const int32 NewStackCount = Item->GetTotalStackCount() - 1;
	if (NewStackCount <= 0)
	{
		for (auto& Pair : SlotToItem)
		{
			if (Pair.Value == Item)
			{
				SlotToItem.Remove(Pair.Key);
				DS_LOG(Inventory, "移除槽位映射 [%d]", Pair.Key);
				break;
			}
		}
		InventoryList.RemoveEntry(Item);
		DS_PRINT(Inventory, 2.f, FLinearColor::Green, "[沙盒背包] 物品已消耗并移除");
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
		DS_PRINT(Inventory, 2.f, FLinearColor::Green, "[沙盒背包] 物品消耗1个,剩余=%d", NewStackCount);
	}
}

// ===== 查询 =====

TArray<UIC_InventoryItem*> USBI_InventoryComponent::GetAllItems()
{
	return InventoryList.GetAllItems();
}

UIC_InventoryItem* USBI_InventoryComponent::FindFirstItemByType(const FGameplayTag& ItemType)
{
	return InventoryList.FindFirstItemByType(ItemType);
}

// ===== 空间查询 =====

FSBI_SlotAvailabilityResult USBI_InventoryComponent::HasRoomForItem(UIC_ItemComponent* ItemComponent) const
{
	if (!IsValid(ItemComponent)) return FSBI_SlotAvailabilityResult();

	const FIC_ItemManifest& Manifest = ItemComponent->GetItemManifest();
	const FGameplayTag ItemType = Manifest.GetItemType();

	// 先查找同类物品用于堆叠
	UIC_InventoryItem* ExistingItem = const_cast<USBI_InventoryComponent*>(this)->InventoryList.FindFirstItemByType(ItemType);

	FSBI_SlotAvailabilityResult Result;
	Result.Item = ExistingItem;

	const FIC_StackableFragment* Stackable = Manifest.GetFragmentOfType<FIC_StackableFragment>();
	const bool bStackable = Stackable != nullptr;
	Result.bStackable = bStackable;

	int32 TotalStackCount = 1;
	if (bStackable)
	{
		TotalStackCount = Stackable->GetStackCount();
	}

	if (ExistingItem && bStackable)
	{
		const int32 MaxStack = GetMaxStackSize(ExistingItem);
		const int32 CurrentStack = ExistingItem->GetTotalStackCount();
		Result.TotalRoomToFill = FMath::Min(TotalStackCount, MaxStack - CurrentStack);
		Result.Remainder = TotalStackCount - Result.TotalRoomToFill;
	}
	else
	{
		// 检查空槽位
		const int32 TotalSlots = Columns * Rows;
		int32 OccupiedCount = SlotToItem.Num();
		int32 EmptySlots = TotalSlots - OccupiedCount;

		if (bStackable && EmptySlots > 0)
		{
			const int32 MaxStack = Stackable->GetMaxStackSize();
			const int32 MaxPossible = EmptySlots * MaxStack;
			Result.TotalRoomToFill = FMath::Min(TotalStackCount, MaxPossible);
			Result.Remainder = TotalStackCount - Result.TotalRoomToFill;
		}
		else if (EmptySlots > 0)
		{
			Result.TotalRoomToFill = FMath::Min(TotalStackCount, EmptySlots);
			Result.Remainder = TotalStackCount - Result.TotalRoomToFill;
		}
		else
		{
			Result.TotalRoomToFill = 0;
			Result.Remainder = TotalStackCount;
		}
	}

	DS_LOG(Inventory, "空间查询: 类型=%s | 可堆叠=%d | 总空间=%d | 剩余=%d | 空槽位=%d",
		*ItemType.ToString(), bStackable, Result.TotalRoomToFill, Result.Remainder,
		Columns * Rows - SlotToItem.Num());

	return Result;
}

FSBI_SlotAvailabilityResult USBI_InventoryComponent::HasRoomForItem(UIC_InventoryItem* Item, int32 StackAmountOverride) const
{
	if (!IsValid(Item))
	{
		DS_LOG_WARN(Inventory, "HasRoomForItem(Item) 失败: Item 为空");
		return FSBI_SlotAvailabilityResult();
	}

	const FGameplayTag ItemType = Item->GetItemType();
	const bool bStackable = Item->IsStackable();
	const int32 StackCount = (StackAmountOverride >= 0) ? StackAmountOverride : Item->GetTotalStackCount();

	FSBI_SlotAvailabilityResult Result;
	Result.bStackable = bStackable;

	const int32 TotalSlots = Columns * Rows;
	const int32 EmptySlots = TotalSlots - SlotToItem.Num();

	if (bStackable)
	{
		UIC_InventoryItem* ExistingItem = const_cast<USBI_InventoryComponent*>(this)->InventoryList.FindFirstItemByType(ItemType);
		Result.Item = ExistingItem;

		if (ExistingItem)
		{
			const int32 MaxStack = GetMaxStackSize(ExistingItem);
			const int32 CurrentStack = ExistingItem->GetTotalStackCount();
			Result.TotalRoomToFill = FMath::Min(StackCount, MaxStack - CurrentStack);
			Result.Remainder = StackCount - Result.TotalRoomToFill;
		}
		else
		{
			const int32 MaxStack = GetMaxStackSize(Item);
			Result.TotalRoomToFill = FMath::Min(StackCount, EmptySlots * MaxStack);
			Result.Remainder = StackCount - Result.TotalRoomToFill;
		}
	}
	else
	{
		Result.TotalRoomToFill = FMath::Min(StackCount, EmptySlots);
		Result.Remainder = StackCount - Result.TotalRoomToFill;
	}

	DS_LOG(Inventory, "空间查询(Item): 类型=%s | 可堆叠=%d | 总空间=%d | 剩余=%d | 空槽位=%d/%d",
		*ItemType.ToString(), bStackable, Result.TotalRoomToFill, Result.Remainder,
		EmptySlots, TotalSlots);

	return Result;
}

int32 USBI_InventoryComponent::FindFirstEmptySlot() const
{
	const int32 TotalSlots = Columns * Rows;
	for (int32 i = 0; i < TotalSlots; ++i)
	{
		if (!SlotToItem.Contains(i))
		{
			DS_LOG(Inventory, "FindFirstEmptySlot: 找到空槽位 [%d] | 已用=%d/%d",
				i, SlotToItem.Num(), TotalSlots);
			return i;
		}
	}
	DS_LOG_WARN(Inventory, "FindFirstEmptySlot: 没有空槽位! 已用=%d/%d", SlotToItem.Num(), TotalSlots);
	return INDEX_NONE;
}