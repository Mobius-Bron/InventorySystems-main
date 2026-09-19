#include "EquipmentManagement/Components/MIS_EquipmentComponent.h"

#include "DH_DebugFunctionLibrary.h"
#include "EquipmentManagement/EquipActor/MIS_EquipActor.h"
#include "GameFramework/PlayerController.h"
#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "Items/MIS_InventoryItem.h"
#include "Items/Fragments/MIS_ItemFragment.h"
#include "MIS_MessageKeys.h"

UMIS_EquipmentComponent::UMIS_EquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMIS_EquipmentComponent::SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh)
{
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Cyan,
		"[装备链路-EquipComp] SetOwningSkeletalMesh | Mesh=%s | bIsProxy=%d",
		IsValid(OwningMesh) ? *OwningMesh->GetName() : TEXT("空"), bIsProxy);
	OwningSkeletalMesh = OwningMesh;
}

void UMIS_EquipmentComponent::Init(APlayerController* InPC, UMIS_InventoryComponent* InInvComp, USkeletalMeshComponent* InMesh)
{
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-EquipComp] ========== Init (外部初始化) ==========");
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-EquipComp] PC=%s | InvComp=%s | Mesh=%s | bIsProxy=%d",
		IsValid(InPC) ? *InPC->GetName() : TEXT("空"),
		IsValid(InInvComp) ? *InInvComp->GetName() : TEXT("空"),
		IsValid(InMesh) ? *InMesh->GetName() : TEXT("空"),
		bIsProxy);

	// ---- 步骤1: 持有 PlayerController ----
	if (IsValid(InPC))
	{
		OwningPlayerController = InPC;
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-EquipComp] Init 警告: PlayerController 为空!");
	}

	// ---- 步骤2: 持有 InventoryComponent 并监听装备状态消息 ----
	if (IsValid(InInvComp))
	{
		InventoryComponent = InInvComp;

		// [解耦重构] 不再绑定委托, 改为监听库存组件广播的装备状态消息 (MIS.Inv.ItemEquipped/Unequipped)。
		// 监听者传入 this, GMP 内部持弱引用, 组件销毁自动解绑; 标志位防止 Init 被多次调用时重复注册。
		if (!bListeningInventoryMessages)
		{
			bListeningInventoryMessages = true;

			const GMP::FSigSource InventorySource(InInvComp);

			MIS::Listen(MSGKEY(MIS_MSG_ITEM_EQUIPPED), InventorySource, this,
				[this](UMIS_InventoryItem* EquippedItem)
				{
					OnItemEquipped(EquippedItem);
				});

			MIS::Listen(MSGKEY(MIS_MSG_ITEM_UNEQUIPPED), InventorySource, this,
				[this](UMIS_InventoryItem* UnequippedItem)
				{
					OnItemUnequipped(UnequippedItem);
				});

			DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
				"[装备链路-EquipComp] Init: 已监听 MIS.Inv.ItemEquipped / MIS.Inv.ItemUnequipped");
		}
		else
		{
			DH_PRINT(EDH_Output::Both, 4.f, DHColors::Cyan,
				"[装备链路-EquipComp] Init: 装备消息已监听,跳过");
		}
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-EquipComp] Init 警告: InventoryComponent 为空!");
	}

	// ---- 步骤3: 持有 SkeletalMesh (可选的,已通过 SetOwningSkeletalMesh 设置时可传 nullptr) ----
	if (IsValid(InMesh))
	{
		OwningSkeletalMesh = InMesh;
		DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
			"[装备链路-EquipComp] Init: 已持有骨骼Mesh=%s", *InMesh->GetName());
	}

	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-EquipComp] ========== Init 完成 | PC有效=%d | InvComp有效=%d | Mesh有效=%d ==========",
		OwningPlayerController.IsValid(), InventoryComponent.IsValid(), OwningSkeletalMesh.IsValid());
}

// ===================== 装备 Actor 管理 =====================

AMIS_EquipActor* UMIS_EquipmentComponent::SpawnEquippedActor(FMIS_EquipmentFragment* EquipmentFragment, const FMIS_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh)
{
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Cyan,
		"[装备链路-EquipComp] >>> SpawnEquippedActor | AttachMesh=%s | TypeTag=%s",
		IsValid(AttachMesh) ? *AttachMesh->GetName() : TEXT("空"),
		*EquipmentFragment->GetEquipmentType().ToString());

	AMIS_EquipActor* SpawnedEquipActor = EquipmentFragment->SpawnAttachedActor(AttachMesh);

	if (IsValid(SpawnedEquipActor))
	{
		SpawnedEquipActor->SetEquipmentType(EquipmentFragment->GetEquipmentType());
		SpawnedEquipActor->SetOwner(GetOwner());
		EquipmentFragment->SetEquippedActor(SpawnedEquipActor);
		DH_PRINT(EDH_Output::Both, 4.f, FLinearColor::Green,
			"[装备链路-EquipComp] SpawnEquippedActor: 生成成功! | Actor=%s | TypeTag=%s",
			*SpawnedEquipActor->GetName(), *EquipmentFragment->GetEquipmentType().ToString());
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 3.f, FLinearColor::Red,
			"[装备链路-EquipComp] SpawnEquippedActor: 生成失败! (EquipActorClass可能为空或Spawn失败)");
	}

	return SpawnedEquipActor;
}

AMIS_EquipActor* UMIS_EquipmentComponent::FindEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	auto FoundActor = EquippedActors.FindByPredicate([&EquipmentTypeTag](const AMIS_EquipActor* EquippedActor)
	{
		return EquippedActor->GetEquipmentType().MatchesTagExact(EquipmentTypeTag);
	});
	return FoundActor ? *FoundActor : nullptr;
}

void UMIS_EquipmentComponent::RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	if (AMIS_EquipActor* EquippedActor = FindEquippedActor(EquipmentTypeTag); IsValid(EquippedActor))
	{
		EquippedActors.Remove(EquippedActor);
		EquippedActor->Destroy();
	}
}

// ===================== 装备/卸下事件回调 =====================

void UMIS_EquipmentComponent::OnItemEquipped(UMIS_InventoryItem* EquippedItem)
{
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-EquipComp] ========== OnItemEquipped 触发! ==========");
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-EquipComp] Item=%s | bIsProxy=%d | HasAuth=%d | PC=%s | Mesh=%s",
		IsValid(EquippedItem) ? *EquippedItem->GetName() : TEXT("空"),
		bIsProxy,
		OwningPlayerController.IsValid() ? OwningPlayerController->HasAuthority() : -1,
		OwningPlayerController.IsValid() ? *OwningPlayerController->GetName() : TEXT("空"),
		OwningSkeletalMesh.IsValid() ? *OwningSkeletalMesh->GetName() : TEXT("空"));

	// ---- 步骤1: 验证输入 ----
	if (!IsValid(EquippedItem))
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-EquipComp] OnItemEquipped 退出: Item为空");
		return;
	}

	if (!OwningPlayerController.IsValid())
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-EquipComp] OnItemEquipped 退出: OwningPlayerController为空 (Init未调用?)");
		return;
	}

	if (!OwningPlayerController->HasAuthority())
	{
		DH_PRINT(EDH_Output::Both, 3.f, FLinearColor::Yellow,
			"[装备链路-EquipComp] OnItemEquipped 退出: 无Authority (非服务端),跳过Actor生成");
		return;
	}

	// ---- 步骤2: 提取 EquipmentFragment ----
	FMIS_ItemManifest& ItemManifest = EquippedItem->GetItemManifestMutable();
	FMIS_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FMIS_EquipmentFragment>();

	if (!EquipmentFragment)
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-EquipComp] OnItemEquipped 退出: 物品没有EquipmentFragment! (物品=%s)",
			*EquippedItem->GetName());
		return;
	}

	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
		"[装备链路-EquipComp] EquipmentFragment获取成功 | EquipmentType=%s | bEquipped=%d",
		*EquipmentFragment->GetEquipmentType().ToString(),
		EquipmentFragment->bEquipped);

	// ---- 步骤3: 执行装备效果 ----
	if (!bIsProxy)
	{
		DH_PRINT(EDH_Output::Both, 4.f, DHColors::Green,
			"[装备链路-EquipComp] 非代理模式,执行 OnEquip 效果");
		EquipmentFragment->OnEquip(OwningPlayerController.Get());
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 4.f, DHColors::Cyan,
			"[装备链路-EquipComp] 代理模式,跳过 OnEquip 效果");
	}

	// ---- 步骤4: 生成 3D 装备 Actor ----
	if (!OwningSkeletalMesh.IsValid())
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-EquipComp] OnItemEquipped 退出: OwningSkeletalMesh为空! (Init Mesh参数是否为nullptr?)");
		return;
	}

	AMIS_EquipActor* SpawnedEquipActor = SpawnEquippedActor(EquipmentFragment, ItemManifest, OwningSkeletalMesh.Get());

	if (IsValid(SpawnedEquipActor))
	{
		EquippedActors.Add(SpawnedEquipActor);
		DH_PRINT(EDH_Output::Both, 4.f, FLinearColor::Green,
			"[装备链路-EquipComp] ========== 装备完成! Actor=%s | 总数=%d ==========",
			*SpawnedEquipActor->GetName(), EquippedActors.Num());
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-EquipComp] OnItemEquipped 失败: SpawnEquippedActor返回nullptr!");
	}
}

void UMIS_EquipmentComponent::OnItemUnequipped(UMIS_InventoryItem* UnequippedItem)
{
	if (!IsValid(UnequippedItem)) return;
	if (!OwningPlayerController->HasAuthority()) return;

	FMIS_ItemManifest& ItemManifest = UnequippedItem->GetItemManifestMutable();
	FMIS_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FMIS_EquipmentFragment>();
	if (!EquipmentFragment) return;

	if (!bIsProxy)
	{
		EquipmentFragment->OnUnequip(OwningPlayerController.Get());
	}

	RemoveEquippedActor(EquipmentFragment->GetEquipmentType());
}
