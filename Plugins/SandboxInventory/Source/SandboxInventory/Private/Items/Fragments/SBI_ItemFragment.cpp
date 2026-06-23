// Copyright AmberAeolian. All Rights Reserved.

#include "Items/Fragments/SBI_ItemFragment.h"
#include "DS_DebugFunctionLibrary.h"
#include "EquipmentManagement/EquipActor/SBI_EquipActor.h"
#include "Engine/World.h"

ASBI_EquipActor* FSBI_EquipmentFragment::SpawnAttachedActor(USkeletalMeshComponent* AttachMesh) const
{
	DS_LOG(Inventory, "装备片段 SpawnAttachedActor | Mesh=%s | Class=%s | Socket=%s",
		IsValid(AttachMesh) ? *AttachMesh->GetName() : TEXT("空"),
		EquipActorClass ? *EquipActorClass->GetName() : TEXT("空"),
		*SocketAttachPoint.ToString());

	if (!IsValid(AttachMesh) || !EquipActorClass)
	{
		DS_LOG_WARN(Inventory, "SpawnAttachedActor 失败: Mesh=%d | Class=%d",
			IsValid(AttachMesh), EquipActorClass != nullptr);
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AttachMesh->GetOwner();
	ASBI_EquipActor* SpawnedActor = AttachMesh->GetWorld()->SpawnActor<ASBI_EquipActor>(EquipActorClass, SpawnParams);

	if (IsValid(SpawnedActor))
	{
		SpawnedActor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketAttachPoint);
		DS_PRINT(Inventory, 4.f, FLinearColor::Green,
			"[沙盒装备] 生成装备Actor: %s | Socket=%s",
			*SpawnedActor->GetName(), *SocketAttachPoint.ToString());
	}
	else
	{
		DS_LOG_ERR(Inventory, "SpawnAttachedActor 失败: SpawnActor 返回空");
	}

	return SpawnedActor;
}

void FSBI_EquipmentFragment::DestroyAttachedActor() const
{
	if (EquippedActor.IsValid())
	{
		DS_LOG(Inventory, "销毁装备Actor: %s", *EquippedActor->GetName());
		EquippedActor->Destroy();
	}
}

void FSBI_EquipmentFragment::SetEquippedActor(ASBI_EquipActor* EquipActor)
{
	EquippedActor = EquipActor;
}