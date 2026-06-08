#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "DH_DebugFunctionLibrary.h"

#include "MultiplayerInventory.h"
#include "Interaction/MIS_Highlightable.h"
#include "Items/Components/MIS_ItemComponent.h"
#include "Items/MIS_InventoryItem.h"
#include "Items/Fragments/MIS_ItemFragment.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/HUD/MIS_HUDWidget.h"
#include "Widgets/Inventory/InventoryBase/MIS_InventoryWidget.h"

UMIS_InventoryComponent::UMIS_InventoryComponent() : InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UMIS_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, InventoryList);
}

void UMIS_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		OwningController = Pawn->GetController<APlayerController>();
	}
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
		if (IsValid(HUDWidget)) HUDWidget->HidePickupMessage();
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
			if (IsValid(HUDWidget)) HUDWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
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
	DH_SCREEN(2.f, DHColors::Cyan, "[背包组件] 拾取交互 | ThisActor=%s",
		ThisActor.IsValid() ? *ThisActor->GetName() : TEXT("空"));

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

	DH_SCREEN(2.f, DHColors::Cyan, "[背包组件] 找到物品组件: %s, 准备拾取", *ItemComp->GetPickupMessage());

	TryAddItem(ItemComp);
}

void UMIS_InventoryComponent::ToggleInventory()
{
	if (!IsValid(InventoryWidget)) return;
	if (!OwningController.IsValid()) return;

	if (bInventoryOpen)
	{
		bInventoryOpen = false;
		InventoryWidget->CloseInventory();
		if (IsValid(HUDWidget)) HUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

		OwningController->SetInputMode(FInputModeGameOnly());
		OwningController->SetShowMouseCursor(false);
	}
	else
	{
		bInventoryOpen = true;
		if (IsValid(HUDWidget)) HUDWidget->SetVisibility(ESlateVisibility::Hidden);
		InventoryWidget->OpenInventory();

		OwningController->SetInputMode(FInputModeGameAndUI());
		OwningController->SetShowMouseCursor(true);
	}
}

void UMIS_InventoryComponent::TryAddItem(UMIS_ItemComponent* ItemComponent)
{
	DH_SCREEN(3.f, DHColors::Cyan, "[背包组件] >>> 进入 TryAddItem | ItemComp=%s | Widget=%s",
		IsValid(ItemComponent) ? TEXT("有效") : TEXT("空"),
		IsValid(InventoryWidget) ? TEXT("有效") : TEXT("空"));

	if (!IsValid(ItemComponent)) return;
	if (!IsValid(InventoryWidget))
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] TryAddItem 失败: InventoryWidget 为空!");
		return;
	}

	FMIS_SlotAvailabilityResult Result = InventoryWidget->HasRoomForItem(ItemComponent);

	DH_SCREEN(3.f, DHColors::Cyan, "[背包组件] HasRoomForItem | 总空间=%d | 槽位数=%d", Result.TotalRoomToFill, Result.SlotAvailabilities.Num());

	UMIS_InventoryItem* FoundItem = InventoryList.FindFirstItemByType(ItemComponent->GetItemManifest().GetItemType());
	Result.Item = FoundItem;

	DH_LOG("[背包组件] HasRoomForItem 结果 | 总空间=%d | 槽位数=%d | 可堆叠=%d | 剩余=%d",
		Result.TotalRoomToFill, Result.SlotAvailabilities.Num(), Result.bStackable, Result.Remainder);

	if (Result.TotalRoomToFill == 0)
	{
		DH_LOG_WARN("[背包组件] -> 库存已满!");
		NoRoomInInventory.Broadcast();
		return;
	}

	if (Result.Item.IsValid() && Result.bStackable)
	{
		DH_LOG("[背包组件] -> 堆叠已有物品 | 填充=%d | 剩余=%d", Result.TotalRoomToFill, Result.Remainder);
		OnStackChange.Broadcast(Result);
		Server_AddStacksToItem(ItemComponent->GetOwner(), Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		DH_LOG("[背包组件] -> 新建物品条目 | 堆叠数=%d | 剩余=%d",
			Result.bStackable ? Result.TotalRoomToFill : 0, Result.Remainder);
		Server_AddNewItem(ItemComponent->GetOwner(), Result.bStackable ? Result.TotalRoomToFill : 0, Result.Remainder);
	}
}

void UMIS_InventoryComponent::Server_AddNewItem_Implementation(AActor* ItemActor, int32 StackCount, int32 Remainder)
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

	DH_SCREEN(3.f, DHColors::Green, "[背包组件] Server_AddNewItem 收到 | Actor=%s | 堆叠=%d | 剩余=%d",
		*ItemActor->GetName(), StackCount, Remainder);

	UMIS_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent);
	NewItem->SetTotalStackCount(StackCount);

	DH_PRINT(EDH_Output::Both, 4.f, FLinearColor::Green,
		"[背包组件] 新物品创建完成 | Item=%s", *NewItem->GetName());

	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		OnItemAdded.Broadcast(NewItem);
	}

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Green, "[背包组件] 物品全部拾取,世界Actor已销毁");
	}
	else if (FMIS_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FMIS_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Green, "[背包组件] 部分拾取,剩余=%d", Remainder);
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

	DH_SCREEN(3.f, DHColors::Green, "[背包组件] Server_AddStacks 收到 | Actor=%s | 堆叠=%d", *ItemActor->GetName(), StackCount);

	const FGameplayTag& ItemType = ItemComponent->GetItemManifest().GetItemType();
	UMIS_InventoryItem* Item = InventoryList.FindFirstItemByType(ItemType);
	if (!IsValid(Item)) return;

	Item->SetTotalStackCount(Item->GetTotalStackCount() + StackCount);

	DH_PRINT(EDH_Output::Both, 4.f, FLinearColor::Green,
		"[背包组件] Server_AddStacks | 新增=%d | 现在总量=%d | 剩余=%d",
		StackCount, Item->GetTotalStackCount(), Remainder);

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Green, "[背包组件] 堆叠完成,世界Actor已销毁");
	}
	else if (FMIS_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FMIS_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Green, "[背包组件] 部分堆叠,世界剩余=%d", Remainder);
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
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			OwningController = OwnerPawn->GetController<APlayerController>();
		}
	}
	if (!OwningController.IsValid())
	{
		DH_SCREEN(5.f, DHColors::Red, "[背包组件] SpawnDroppedItem 失败: OwningController 为空");
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
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-InvComp] >>> Server_EquipSlotClicked (Server RPC) | Equip=%s | Unequip=%s | Role=%d",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"),
		(int32)GetOwnerRole());
	Multicast_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void UMIS_InventoryComponent::Multicast_EquipSlotClicked_Implementation(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip)
{
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-InvComp] >>> Multicast_EquipSlotClicked | Equip=%s | Unequip=%s | bHasEquipListener=%d",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"),
		OnItemEquipped.IsBound());

	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);

	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-InvComp] Multicast_EquipSlotClicked 广播完成");
}

void UMIS_InventoryComponent::RequestEquipSlotClicked(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip)
{
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-InvComp] >>> RequestEquipSlotClicked (Client→Server) | Equip=%s | Unequip=%s",
		IsValid(ItemToEquip) ? *ItemToEquip->GetName() : TEXT("空"),
		IsValid(ItemToUnequip) ? *ItemToUnequip->GetName() : TEXT("空"));
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
