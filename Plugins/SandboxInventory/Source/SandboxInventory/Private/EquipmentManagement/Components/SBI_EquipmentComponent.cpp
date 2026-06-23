// Copyright AmberAeolian. All Rights Reserved.

#include "EquipmentManagement/Components/SBI_EquipmentComponent.h"

#include "DS_DebugFunctionLibrary.h"
#include "SandboxInventory.h"
#include "EquipmentManagement/EquipActor/SBI_EquipActor.h"
#include "GameFramework/PlayerController.h"
#include "InventoryManagement/Components/SBI_InventoryComponent.h"
#include "Items/IC_InventoryItem.h"
#include "Items/IC_ItemFragment.h"
#include "Items/Fragments/SBI_ItemFragment.h"

USBI_EquipmentComponent::USBI_EquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USBI_EquipmentComponent::SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh)
{
	DS_PRINT(Inventory, 4.f, DSColors::Cyan,
		"[沙盒装备] SetOwningSkeletalMesh | Mesh=%s | bIsProxy=%d",
		IsValid(OwningMesh) ? *OwningMesh->GetName() : TEXT("空"), bIsProxy);
	OwningSkeletalMesh = OwningMesh;
}

void USBI_EquipmentComponent::Init(APlayerController* InPC, USBI_InventoryComponent* InInvComp, USkeletalMeshComponent* InMesh)
{
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] ========== Init 开始 ==========");
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] PC=%s | InvComp=%s | Mesh=%s | bIsProxy=%d",
		IsValid(InPC) ? *InPC->GetName() : TEXT("空"),
		IsValid(InInvComp) ? *InInvComp->GetName() : TEXT("空"),
		IsValid(InMesh) ? *InMesh->GetName() : TEXT("空"),
		bIsProxy);

	if (IsValid(InPC))
	{
		OwningPlayerController = InPC;
	}
	else
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] Init 警告: PlayerController 为空!");
	}

	if (IsValid(InInvComp))
	{
		InventoryComponent = InInvComp;

		if (!InventoryComponent->OnItemEquipped.IsAlreadyBound(this, &ThisClass::OnItemEquipped))
		{
			InventoryComponent->OnItemEquipped.AddDynamic(this, &ThisClass::OnItemEquipped);
		}

		if (!InventoryComponent->OnItemUnequipped.IsAlreadyBound(this, &ThisClass::OnItemUnequipped))
		{
			InventoryComponent->OnItemUnequipped.AddDynamic(this, &ThisClass::OnItemUnequipped);
		}

		DS_LOG(Inventory, "沙盒装备: Init 完成, 已绑定 OnItemEquipped + OnItemUnequipped");
	}
	else
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] Init 警告: InventoryComponent 为空!");
	}

	if (IsValid(InMesh))
	{
		OwningSkeletalMesh = InMesh;
		DS_LOG(Inventory, "沙盒装备: 已持有骨骼Mesh=%s", *InMesh->GetName());
	}

	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] ========== Init 完成 | PC=%d | InvComp=%d | Mesh=%d ==========",
		OwningPlayerController.IsValid(), InventoryComponent.IsValid(), OwningSkeletalMesh.IsValid());
}

// ===== 装备 Actor 管理 =====

ASBI_EquipActor* USBI_EquipmentComponent::SpawnEquippedActor(FSBI_EquipmentFragment* EquipmentFragment, const FIC_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh)
{
	DS_PRINT(Inventory, 4.f, DSColors::Cyan,
		"[沙盒装备] >>> SpawnEquippedActor | AttachMesh=%s | TypeTag=%s",
		IsValid(AttachMesh) ? *AttachMesh->GetName() : TEXT("空"),
		*EquipmentFragment->GetEquipmentType().ToString());

	ASBI_EquipActor* SpawnedEquipActor = EquipmentFragment->SpawnAttachedActor(AttachMesh);

	if (IsValid(SpawnedEquipActor))
	{
		SpawnedEquipActor->SetEquipmentType(EquipmentFragment->GetEquipmentType());
		SpawnedEquipActor->SetOwner(GetOwner());
		EquipmentFragment->SetEquippedActor(SpawnedEquipActor);
		DS_PRINT(Inventory, 4.f, FLinearColor::Green,
			"[沙盒装备] SpawnEquippedActor: 生成成功! | Actor=%s",
			*SpawnedEquipActor->GetName());
	}
	else
	{
		DS_PRINT(Inventory, 3.f, FLinearColor::Red,
			"[沙盒装备] SpawnEquippedActor: 生成失败! (EquipActorClass 可能为空)");
	}

	return SpawnedEquipActor;
}

ASBI_EquipActor* USBI_EquipmentComponent::FindEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	auto FoundActor = EquippedActors.FindByPredicate([&EquipmentTypeTag](const ASBI_EquipActor* EquippedActor)
	{
		return EquippedActor->GetEquipmentType().MatchesTagExact(EquipmentTypeTag);
	});
	return FoundActor ? *FoundActor : nullptr;
}

void USBI_EquipmentComponent::RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	if (ASBI_EquipActor* EquippedActor = FindEquippedActor(EquipmentTypeTag); IsValid(EquippedActor))
	{
		DS_LOG(Inventory, "沙盒装备: 移除装备Actor | Type=%s", *EquipmentTypeTag.ToString());
		EquippedActors.Remove(EquippedActor);
		EquippedActor->Destroy();
	}
}

// ===== 装备/卸下事件回调 =====

void USBI_EquipmentComponent::OnItemEquipped(UIC_InventoryItem* EquippedItem)
{
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] ========== OnItemEquipped 触发! ==========");
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] Item=%s | bIsProxy=%d | HasAuth=%d",
		IsValid(EquippedItem) ? *EquippedItem->GetName() : TEXT("空"),
		bIsProxy,
		OwningPlayerController.IsValid() ? OwningPlayerController->HasAuthority() : -1);

	if (!IsValid(EquippedItem))
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] OnItemEquipped 退出: Item 为空");
		return;
	}

	if (!OwningPlayerController.IsValid())
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] OnItemEquipped 退出: PlayerController 为空 (Init 未调用?)");
		return;
	}

	if (!OwningPlayerController->HasAuthority())
	{
		DS_PRINT(Inventory, 3.f, FLinearColor::Yellow,
			"[沙盒装备] OnItemEquipped 退出: 无 Authority (非服务端), 跳过 Actor 生成");
		return;
	}

	FIC_ItemManifest& ItemManifest = EquippedItem->GetItemManifestMutable();
	FSBI_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FSBI_EquipmentFragment>();

	if (!EquipmentFragment)
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] OnItemEquipped 退出: 物品没有 EquipmentFragment (物品=%s)",
			*EquippedItem->GetName());
		return;
	}

	if (!OwningSkeletalMesh.IsValid())
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] OnItemEquipped 退出: 骨骼Mesh 为空 (SetOwningSkeletalMesh 未调用?)");
		return;
	}

	// 如果已有同类型装备, 先卸下
	RemoveEquippedActor(EquipmentFragment->GetEquipmentType());

	// 生成装备 Actor
	ASBI_EquipActor* EquipActor = SpawnEquippedActor(EquipmentFragment, ItemManifest, OwningSkeletalMesh.Get());
	if (IsValid(EquipActor))
	{
		EquippedActors.Add(EquipActor);
		EquipmentFragment->bEquipped = true;
	}
}

void USBI_EquipmentComponent::OnItemUnequipped(UIC_InventoryItem* UnequippedItem)
{
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] ========== OnItemUnequipped 触发! ==========");
	DS_PRINT(Inventory, 4.f, FLinearColor::Green,
		"[沙盒装备] Item=%s | bIsProxy=%d",
		IsValid(UnequippedItem) ? *UnequippedItem->GetName() : TEXT("空"), bIsProxy);

	if (!IsValid(UnequippedItem))
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] OnItemUnequipped 退出: Item 为空");
		return;
	}

	FIC_ItemManifest& ItemManifest = UnequippedItem->GetItemManifestMutable();
	FSBI_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FSBI_EquipmentFragment>();

	if (!EquipmentFragment)
	{
		DS_PRINT(Inventory, 2.f, FLinearColor::Red,
			"[沙盒装备] OnItemUnequipped 退出: 物品没有 EquipmentFragment");
		return;
	}

	RemoveEquippedActor(EquipmentFragment->GetEquipmentType());
	EquipmentFragment->DestroyAttachedActor();
	EquipmentFragment->bEquipped = false;
}