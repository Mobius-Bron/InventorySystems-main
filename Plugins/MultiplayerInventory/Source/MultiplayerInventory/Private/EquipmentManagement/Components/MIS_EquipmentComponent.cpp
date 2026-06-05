#include "EquipmentManagement/Components/MIS_EquipmentComponent.h"

#include "EquipmentManagement/EquipActor/MIS_EquipActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "BlueprintFunctionLibraries/MIS_InventoryFunctionLibrary.h"
#include "Items/MIS_InventoryItem.h"
#include "Items/Fragments/MIS_ItemFragment.h"

void UMIS_EquipmentComponent::SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh)
{
	OwningSkeletalMesh = OwningMesh;
}

void UMIS_EquipmentComponent::InitializeOwner(APlayerController* PlayerController)
{
	if (IsValid(PlayerController))
	{
		OwningPlayerController = PlayerController;
	}
	InitInventoryComponent();
}

void UMIS_EquipmentComponent::InitComponent(UMIS_InventoryComponent* InInventoryComponent)
{
	// MVVM 绑定入口: 订阅库存组件的装备/卸下事件
	InventoryComponent = InInventoryComponent;
	if (!InventoryComponent.IsValid()) return;

	if (!InventoryComponent->OnItemEquipped.IsAlreadyBound(this, &ThisClass::OnItemEquipped))
	{
		InventoryComponent->OnItemEquipped.AddDynamic(this, &ThisClass::OnItemEquipped);
	}

	if (!InventoryComponent->OnItemUnequipped.IsAlreadyBound(this, &ThisClass::OnItemUnequipped))
	{
		InventoryComponent->OnItemUnequipped.AddDynamic(this, &ThisClass::OnItemUnequipped);
	}
}

void UMIS_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	InitPlayerController();
}

void UMIS_EquipmentComponent::InitPlayerController()
{
	if (OwningPlayerController.IsValid()) return;

	if (OwningPlayerController = Cast<APlayerController>(GetOwner()); OwningPlayerController.IsValid())
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwningPlayerController->GetPawn()); IsValid(OwnerCharacter))
		{
			OnPossessedPawnChange(nullptr, OwnerCharacter);
		}
		else
		{
			OwningPlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChange);
		}
	}
}

void UMIS_EquipmentComponent::OnPossessedPawnChange(APawn* OldPawn, APawn* NewPawn)
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(NewPawn); IsValid(OwnerCharacter))
	{
		// 获取角色的骨骼网格体 → 装备 Actor 附着目标
		OwningSkeletalMesh = OwnerCharacter->GetMesh();
	}
	InitInventoryComponent();
}

void UMIS_EquipmentComponent::InitInventoryComponent()
{
	if (InventoryComponent.IsValid()) return;

	// 通过函数库从 PlayerController 获取库存组件
	InventoryComponent = UMIS_InventoryFunctionLibrary::GetInventoryComponent(OwningPlayerController.Get());
	if (!InventoryComponent.IsValid()) return;

	// 绑定装备/卸下事件 (MVVM)
	if (!InventoryComponent->OnItemEquipped.IsAlreadyBound(this, &ThisClass::OnItemEquipped))
	{
		InventoryComponent->OnItemEquipped.AddDynamic(this, &ThisClass::OnItemEquipped);
	}

	if (!InventoryComponent->OnItemUnequipped.IsAlreadyBound(this, &ThisClass::OnItemUnequipped))
	{
		InventoryComponent->OnItemUnequipped.AddDynamic(this, &ThisClass::OnItemUnequipped);
	}
}

AMIS_EquipActor* UMIS_EquipmentComponent::SpawnEquippedActor(FMIS_EquipmentFragment* EquipmentFragment, const FMIS_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh)
{
	// 在骨骼网格体上生成并附着装备 3D Actor
	AMIS_EquipActor* SpawnedEquipActor = EquipmentFragment->SpawnAttachedActor(AttachMesh);
	SpawnedEquipActor->SetEquipmentType(EquipmentFragment->GetEquipmentType());
	SpawnedEquipActor->SetOwner(GetOwner());
	EquipmentFragment->SetEquippedActor(SpawnedEquipActor);
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
		EquippedActor->Destroy(); // 销毁 3D 装备 Actor
	}
}

void UMIS_EquipmentComponent::OnItemEquipped(UMIS_InventoryItem* EquippedItem)
{
	// ---- 步骤1: 获取装备片段 ----
	if (!IsValid(EquippedItem)) return;
	if (!OwningPlayerController->HasAuthority()) return; // 仅服务端执行

	FMIS_ItemManifest& ItemManifest = EquippedItem->GetItemManifestMutable();
	FMIS_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FMIS_EquipmentFragment>();
	if (!EquipmentFragment) return;

	// ---- 步骤2: 执行装备效果 (代理模式跳过) ----
	if (!bIsProxy)
	{
		// 遍历所有 EquipModifier,依次执行 OnEquip (如 +15 伤害)
		EquipmentFragment->OnEquip(OwningPlayerController.Get());
	}

	// ---- 步骤3: 在角色骨骼上生成 3D 装备 Actor ----
	if (!OwningSkeletalMesh.IsValid()) return;
	AMIS_EquipActor* SpawnedEquipActor = SpawnEquippedActor(EquipmentFragment, ItemManifest, OwningSkeletalMesh.Get());

	// 记录到映射表 (同类型替换时先查找并销毁旧的)
	EquippedActors.Add(SpawnedEquipActor);
}

void UMIS_EquipmentComponent::OnItemUnequipped(UMIS_InventoryItem* UnequippedItem)
{
	// ---- 步骤1: 获取装备片段 ----
	if (!IsValid(UnequippedItem)) return;
	if (!OwningPlayerController->HasAuthority()) return; // 仅服务端执行

	FMIS_ItemManifest& ItemManifest = UnequippedItem->GetItemManifestMutable();
	FMIS_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FMIS_EquipmentFragment>();
	if (!EquipmentFragment) return;

	// ---- 步骤2: 执行卸下效果 (代理模式跳过) ----
	if (!bIsProxy)
	{
		// 遍历所有 EquipModifier,依次执行 OnUnequip (如 -15 伤害)
		EquipmentFragment->OnUnequip(OwningPlayerController.Get());
	}

	// ---- 步骤3: 销毁 3D 装备 Actor ----
	RemoveEquippedActor(EquipmentFragment->GetEquipmentType());
}
