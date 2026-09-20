#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "DH_DebugFunctionLibrary.h"

#include "GMPCore.h"
#include "MIS_MessageKeys.h"

#include "MultiplayerInventory.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/MIS_Highlightable.h"
#include "Items/Components/MIS_ItemComponent.h"
#include "Items/MIS_InventoryItem.h"
#include "Items/Fragments/MIS_ItemFragment.h"
#include "Items/Manifest/MIS_ItemManifest.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Inventory/InventoryBase/MIS_InventoryWidget.h"

using GMP::FSigSource;  // 本编译单元内简化书写

namespace
{
	/**
	 * [服务端权威] 落点校验的严格程度。
	 *   0 = 仅告警 (默认): 校验失败只记日志, 仍然接受 —— 便于先在实机多人环境下收集证据,
	 *       确认校验逻辑无误 (不会误伤正常操作) 之后再切换。
	 *   1 = 严格拒绝: 校验失败则丢弃该次拾取, 并回告客户端提示背包已满。
	 */
	static TAutoConsoleVariable<int32> CVarMISServerAuthStrict(
		TEXT("MIS.ServerAuthStrict"),
		0,
		TEXT("库存服务端落点校验: 0=仅告警, 1=拒绝非法放置"),
		ECVF_Default);
}  // namespace

UMIS_InventoryComponent::UMIS_InventoryComponent() : InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UMIS_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// [联机修复] 背包内容只复制给拥有者。
	// 原先走默认条件, 会把每个玩家的背包完整复制给所有客户端: 既浪费带宽,
	// 也等于把对手的背包直接暴露给客户端作弊工具 (掳掠/撤离类玩法尤其敏感)。
	DOREPLIFETIME_CONDITION(ThisClass, InventoryList, COND_OwnerOnly);
}

void UMIS_InventoryComponent::Init(APlayerController* InPC)
{
	if (IsValid(InPC))
	{
		OwningController = InPC;
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[背包组件] Init 警告: PlayerController 为空!");
	}
}

void UMIS_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	const FSigSource InventorySource(this);

	// [解耦重构] UI -> 数据层: 监听意图命令, 收到后转换成对应的 Server RPC 请求。
	// 监听者传入 this, GMP 内部保存弱引用, 组件销毁时会自动解绑。
	MIS::Listen(MSGKEY(MIS_CMD_DROP_ITEM), InventorySource, this,
		[this](UMIS_InventoryItem* Item, int32 StackCount)
		{
			RequestDropItem(Item, StackCount);
		});

	MIS::Listen(MSGKEY(MIS_CMD_CONSUME_ITEM), InventorySource, this,
		[this](UMIS_InventoryItem* Item)
		{
			RequestConsumeItem(Item);
		});

	MIS::Listen(MSGKEY(MIS_CMD_EQUIP_SLOT), InventorySource, this,
		[this](UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip)
		{
			RequestEquipSlotClicked(ItemToEquip, ItemToUnequip);
		});
}

void UMIS_InventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MIS::Unbind(MSGKEY(MIS_CMD_DROP_ITEM), this);
	MIS::Unbind(MSGKEY(MIS_CMD_CONSUME_ITEM), this);
	MIS::Unbind(MSGKEY(MIS_CMD_EQUIP_SLOT), this);

	Super::EndPlay(EndPlayReason);
}

void UMIS_InventoryComponent::TraceForItem()
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
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);

	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();
	
	if (!ThisActor.IsValid())
	{
		// 消息化: 射线没有命中任何物体, 通知 UI 收起拾取提示
		MIS::Emit(MSGKEY(MIS_MSG_PICKUP_PROMPT), FSigSource(this), FString(), false);
	}

	if (ThisActor == LastActor) return;

	if (ThisActor.IsValid())
	{
		if (UActorComponent* Highlightable = ThisActor->FindComponentByInterface(UMIS_Highlightable::StaticClass()); IsValid(Highlightable))
		{
			IMIS_Highlightable::Execute_Highlight(Highlightable);
		}

		if (UMIS_ItemComponent* ItemComponent = ThisActor->FindComponentByClass<UMIS_ItemComponent>())
		{
			// 消息化: 命中可拾取物, 把提示文本交给 UI 自行决定如何呈现
			MIS::Emit(MSGKEY(MIS_MSG_PICKUP_PROMPT), FSigSource(this), ItemComponent->GetPickupMessage(), true);
		}
	}

	if (LastActor.IsValid())
	{
		if (UActorComponent* Highlightable = LastActor->FindComponentByInterface(UMIS_Highlightable::StaticClass()); IsValid(Highlightable))
		{
			IMIS_Highlightable::Execute_UnHighlight(Highlightable);
		}
	}
}

void UMIS_InventoryComponent::PrimaryInteract()
{
	// [健壮性修复] TraceForItem 原本只在 Look(鼠标移动) 时被调用, 于是:
	//   1) 刚进入游戏、还没移动过视角就按拾取键 -> ThisActor 为空, 拾取无声失败;
	//   2) 瞄准后目标被他人拾走/销毁 -> ThisActor 仍指向失效对象。
	// 在拾取前强制刷新一次射线, 保证操作的是"此刻真正瞄准的目标"。
	TraceForItem();

	if (!ThisActor.IsValid())
	{
		DH_LOG_ERR("[背包组件] 拾取失败: 没有瞄准任何物体 (TraceForItem未调用?)");
		return;
	}

	UMIS_ItemComponent* ItemComp = ThisActor->FindComponentByClass<UMIS_ItemComponent>();
	if (!IsValid(ItemComp))
	{
		DH_LOG_ERR("[背包组件] 拾取失败: 目标物体没有 ItemComponent");
		return;
	}

	TryAddItem(ItemComp);
}

void UMIS_InventoryComponent::ToggleInventory()
{
	// [解耦重构] 数据层只翻转状态并广播, 不再直接开关 UI、不再操作输入模式和鼠标光标。
	// 界面显隐 / HUD 显隐 / 输入模式全部由监听 MIS.Inv.MenuToggled 的 UI 侧自行处理。
	// 副作用: 服务端(无 UI)调用本函数也不会再因为拿不到 Widget 而提前返回。
	bInventoryOpen = !bInventoryOpen;

	MIS::Emit(MSGKEY(MIS_MSG_MENU_TOGGLED), FSigSource(this), bInventoryOpen);
}

void UMIS_InventoryComponent::TryAddItem(UMIS_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent)) return;
	if (!IsValid(InventoryWidget))
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] TryAddItem 失败: InventoryWidget 为空!");
		return;
	}

	FMIS_SlotAvailabilityResult Result = InventoryWidget->HasRoomForItem(ItemComponent);

	UMIS_InventoryItem* FoundItem = InventoryList.FindFirstItemByType(ItemComponent->GetItemManifest().GetItemType());
	Result.Item = FoundItem;

	if (Result.TotalRoomToFill == 0)
	{
		DH_LOG_WARN("[背包组件] -> 库存已满!");
		MIS::Emit(MSGKEY(MIS_MSG_NO_ROOM), FSigSource(this));
		return;
	}

	if (Result.Item.IsValid() && Result.bStackable)
	{
		MIS::Emit(MSGKEY(MIS_MSG_STACK_CHANGED), FSigSource(this), Result);
		Server_AddStacksToItem(ItemComponent->GetOwner(), Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		// [服务端权威] 客户端算出的落点: 取第一个可用槽位作为物品左上角, 随请求交给服务端复核。
		const int32 TargetGridIndex = Result.SlotAvailabilities.Num() > 0
			? Result.SlotAvailabilities[0].Index
			: INDEX_NONE;

		Server_AddNewItem(ItemComponent->GetOwner(), Result.bStackable ? Result.TotalRoomToFill : 0,
			Result.Remainder, TargetGridIndex);
	}
}

void UMIS_InventoryComponent::Server_AddNewItem_Implementation(AActor* ItemActor, int32 StackCount, int32 Remainder, int32 TargetGridIndex)
{
	if (!IsValid(ItemActor))
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] Server_AddNewItem 失败: ItemActor 为空 — 拾取Actor未开启bReplicates?");
		return;
	}

	UMIS_ItemComponent* ItemComponent = ItemActor->FindComponentByClass<UMIS_ItemComponent>();
	if (!IsValid(ItemComponent))
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] Server_AddNewItem 失败: ItemActor(%s) 上没有 ItemComponent", *ItemActor->GetName());
		return;
	}

	// ---- [服务端权威] 复核客户端上报的落点 ----
	// 客户端是用本地 UI 的槽位视图算出该位置的; 服务端用自己复制到的位置表独立校验,
	// 避免"拾取是否成功由客户端说了算"(作弊, 或快速丢/捡时本地状态滞后导致的超容)。
	// 布局未知 (GridColumns/GridRows 均为 0) 时 ValidatePlacement 直接放行。
	const FMIS_ItemManifest& NewItemManifest = ItemComponent->GetItemManifest();
	const bool bPlacementValid = ValidatePlacement(NewItemManifest, TargetGridIndex, nullptr);

	if (!bPlacementValid)
	{
		const bool bStrict = CVarMISServerAuthStrict.GetValueOnGameThread() != 0;
		DH_LOG_WARN("[背包组件] [服务端权威] 落点校验失败 | 落点=%d | 严格模式=%d | 网格=%dx%d",
			TargetGridIndex, bStrict ? 1 : 0, GridColumns, GridRows);

		if (bStrict)
		{
			// 严格模式: 拒绝本次拾取, 并回告客户端提示背包已满
			Client_NotifyPlacementRejected();
			return;
		}
		// 宽松模式: 仅告警并继续, 便于先在实机多人环境收集证据
	}

	UMIS_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent, TargetGridIndex);
	NewItem->SetTotalStackCount(StackCount);

	// 本地即时通知(远端由 FastArray 的 PostReplicatedAdd 负责), 避免重复或遗漏
	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		MIS::Emit(MSGKEY(MIS_MSG_ITEM_ADDED), FSigSource(this), NewItem, TargetGridIndex);
	}

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FMIS_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FMIS_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}

void UMIS_InventoryComponent::Server_AddStacksToItem_Implementation(AActor* ItemActor, int32 StackCount, int32 Remainder)
{
	if (!IsValid(ItemActor))
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] Server_AddStacks 失败: ItemActor 为空");
		return;
	}

	UMIS_ItemComponent* ItemComponent = ItemActor->FindComponentByClass<UMIS_ItemComponent>();
	if (!IsValid(ItemComponent))
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] Server_AddStacks 失败: ItemActor(%s) 上没有 ItemComponent", *ItemActor->GetName());
		return;
	}

	const FGameplayTag& ItemType = ItemComponent->GetItemManifest().GetItemType();
	UMIS_InventoryItem* Item = InventoryList.FindFirstItemByType(ItemType);
	if (!IsValid(Item)) return;

	Item->SetTotalStackCount(Item->GetTotalStackCount() + StackCount);

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FMIS_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FMIS_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}

void UMIS_InventoryComponent::Server_DropItem_Implementation(UMIS_InventoryItem* Item, int32 StackCount)
{
	if (!IsValid(Item))
	{
		DH_LOG_ERR("[背包组件] Server_DropItem 失败: Item 为空");
		return;
	}

	const int32 NewStackCount = Item->GetTotalStackCount() - StackCount;
	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	SpawnDroppedItem(Item, StackCount);
}

void UMIS_InventoryComponent::SpawnDroppedItem(UMIS_InventoryItem* Item, int32 StackCount)
{
	if (!OwningController.IsValid())
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] SpawnDroppedItem 失败: OwningController 为空 (Init未调用?)");
		return;
	}

	const APawn* OwningPawn = OwningController->GetPawn();
	if (!IsValid(OwningPawn)) return;

	FVector RotatedForward = OwningPawn->GetActorForwardVector();
	RotatedForward = RotatedForward.RotateAngleAxis(FMath::FRandRange(DropSpawnAngleMin, DropSpawnAngleMax), FVector::UpVector);
	FVector SpawnLocation = OwningPawn->GetActorLocation() + RotatedForward * FMath::FRandRange(DropSpawnDistanceMin, DropSpawnDistanceMax);
	SpawnLocation.Z -= RelativeSpawnElevation;
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FMIS_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
	if (FMIS_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfTypeMutable<FMIS_StackableFragment>())
	{
		StackableFragment->SetStackCount(StackCount);
	}
	ItemManifest.SpawnPickupActor(this, SpawnLocation, SpawnRotation);
}

void UMIS_InventoryComponent::Server_ConsumeItem_Implementation(UMIS_InventoryItem* Item)
{
	const int32 NewStackCount = Item->GetTotalStackCount() - 1;
	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	if (FMIS_ConsumableFragment* ConsumableFragment = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FMIS_ConsumableFragment>())
	{
		ConsumableFragment->OnConsume(OwningController.Get());
	}
}

void UMIS_InventoryComponent::Server_EquipSlotClicked_Implementation(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip)
{
	Multicast_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void UMIS_InventoryComponent::Multicast_EquipSlotClicked_Implementation(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip)
{
	MIS::Emit(MSGKEY(MIS_MSG_ITEM_EQUIPPED), FSigSource(this), ItemToEquip);
	MIS::Emit(MSGKEY(MIS_MSG_ITEM_UNEQUIPPED), FSigSource(this), ItemToUnequip);
}

void UMIS_InventoryComponent::RequestEquipSlotClicked(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip)
{
	Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void UMIS_InventoryComponent::RequestDropItem(UMIS_InventoryItem* Item, int32 StackCount)
{
	if (!IsValid(Item)) return;
	Server_DropItem(Item, StackCount);
}

void UMIS_InventoryComponent::RequestConsumeItem(UMIS_InventoryItem* Item)
{
	if (!IsValid(Item)) return;
	Server_ConsumeItem(Item);
}

void UMIS_InventoryComponent::AddRepSubObj(UObject* SubObj)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
	{
		AddReplicatedSubObject(SubObj);
	}
}

TArray<UMIS_InventoryItem*> UMIS_InventoryComponent::GetAllItems()
{
	return InventoryList.GetAllItems();
}

UMIS_InventoryItem* UMIS_InventoryComponent::FindFirstItemByType(const FGameplayTag& ItemType)
{
	return InventoryList.FindFirstItemByType(ItemType);
}

bool UMIS_InventoryComponent::IsInventoryOpen() const
{
	return bInventoryOpen;
}

// ======================================================================
// [服务端权威] 网格布局与位置校验
// ======================================================================

void UMIS_InventoryComponent::Server_SetGridLayout_Implementation(int32 InColumns, int32 InRows)
{
	GridColumns = FMath::Max(0, InColumns);
	GridRows = FMath::Max(0, InRows);
}

void UMIS_InventoryComponent::Client_NotifyPlacementRejected_Implementation()
{
	DH_LOG_WARN("[背包组件] [服务端权威] 落点被服务端拒绝, 回告客户端背包已满");

	// 复用既有的"背包已满"通知: UI 侧监听 MIS.Inv.NoRoom 即可
	MIS::Emit(MSGKEY(MIS_MSG_NO_ROOM), FSigSource(this));
}

FIntPoint UMIS_InventoryComponent::GetManifestGridSize(const FMIS_ItemManifest& Manifest) const
{
	if (const FMIS_GridFragment* GridFragment = Manifest.GetFragmentOfType<FMIS_GridFragment>())
	{
		return GridFragment->GetGridSize();
	}
	return FIntPoint(1, 1);
}

bool UMIS_InventoryComponent::IsIndexInBounds(int32 Index, const FIntPoint& Dim) const
{
	if (Index < 0 || GridColumns <= 0 || GridRows <= 0) return false;
	if (Dim.X <= 0 || Dim.Y <= 0) return false;

	const int32 StartX = Index % GridColumns;
	const int32 StartY = Index / GridColumns;

	// 必须整块落在网格内; 同时 (StartX + Dim.X) <= GridColumns 防止物品跨行绕到下一行
	return (StartX + Dim.X) <= GridColumns && (StartY + Dim.Y) <= GridRows;
}

void UMIS_InventoryComponent::BuildOccupancyMap(TArray<UMIS_InventoryItem*>& OutSlotItems) const
{
	OutSlotItems.Reset();

	const int32 TotalSlots = GridColumns * GridRows;
	if (TotalSlots <= 0) return;

	OutSlotItems.SetNumZeroed(TotalSlots);

	for (const FMIS_InventoryEntry& Entry : InventoryList.Entries)
	{
		if (!IsValid(Entry.Item)) continue;
		if (Entry.UpperLeftIndex == INDEX_NONE) continue;

		const FIntPoint Dim = GetManifestGridSize(Entry.Item->GetItemManifest());
		for (int32 dy = 0; dy < Dim.Y; ++dy)
		{
			for (int32 dx = 0; dx < Dim.X; ++dx)
			{
				const int32 SlotIndex = Entry.UpperLeftIndex + dx + dy * GridColumns;
				if (OutSlotItems.IsValidIndex(SlotIndex))
				{
					OutSlotItems[SlotIndex] = Entry.Item;
				}
			}
		}
	}
}

bool UMIS_InventoryComponent::ValidatePlacement(const FMIS_ItemManifest& Manifest, int32 UpperLeftIndex, const UMIS_InventoryItem* IgnoreItem) const
{
	// 尚未配置布局 (专用服务器既没收到 UI 上报, 也没在蓝图里配置) 时不做校验, 避免误拒正常操作
	if (GridColumns <= 0 || GridRows <= 0) return true;

	// 客户端未给出落点时不在此判定, 交由后续逻辑处理
	if (UpperLeftIndex == INDEX_NONE) return true;

	const FIntPoint Dim = GetManifestGridSize(Manifest);
	if (!IsIndexInBounds(UpperLeftIndex, Dim)) return false;

	TArray<UMIS_InventoryItem*> SlotItems;
	BuildOccupancyMap(SlotItems);

	for (int32 dy = 0; dy < Dim.Y; ++dy)
	{
		for (int32 dx = 0; dx < Dim.X; ++dx)
		{
			const int32 SlotIndex = UpperLeftIndex + dx + dy * GridColumns;
			if (!SlotItems.IsValidIndex(SlotIndex)) return false;

			const UMIS_InventoryItem* Occupant = SlotItems[SlotIndex];
			if (Occupant && Occupant != IgnoreItem) return false;  // 该格已被其它物品占据
		}
	}

	return true;
}

void UMIS_InventoryComponent::SetItemGridIndex(UMIS_InventoryItem* Item, int32 UpperLeftIndex)
{
	if (!IsValid(Item)) return;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (FMIS_InventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Item == Item)
		{
			if (Entry.UpperLeftIndex != UpperLeftIndex)
			{
				Entry.UpperLeftIndex = UpperLeftIndex;
				InventoryList.MarkItemDirty(Entry);
			}
			return;
		}
	}
}

int32 UMIS_InventoryComponent::GetItemGridIndex(const UMIS_InventoryItem* Item) const
{
	for (const FMIS_InventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Item == Item) return Entry.UpperLeftIndex;
	}
	return INDEX_NONE;
}
