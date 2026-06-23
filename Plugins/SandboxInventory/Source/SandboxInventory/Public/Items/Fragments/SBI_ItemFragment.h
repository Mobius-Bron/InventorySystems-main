// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/IC_ItemFragment.h"
#include "GameplayTagContainer.h"

#include "SBI_ItemFragment.generated.h"

class ASBI_EquipActor;
class APlayerController;
class USkeletalMeshComponent;

/**
 * 装备片段 - 物品可装备到角色身上
 * 包含 3D 装备 Actor 类、附着骨骼点、装备类型标签
 * 装备时会在角色骨骼上生成 EquipActor 并显示
 */
USTRUCT(BlueprintType)
struct FSBI_EquipmentFragment : public FIC_ItemFragment
{
	GENERATED_BODY()

	/** 在骨骼网格体上生成并附着装备 Actor */
	ASBI_EquipActor* SpawnAttachedActor(USkeletalMeshComponent* AttachMesh) const;
	/** 销毁已生成的装备 Actor */
	void DestroyAttachedActor() const;
	/** 获取装备类型 Tag (用于匹配装备槽位) */
	FGameplayTag GetEquipmentType() const { return EquipmentType; }
	/** 设置当前装备 Actor 引用 */
	void SetEquippedActor(ASBI_EquipActor* EquipActor);

	/** 是否已装备 */
	bool bEquipped{false};

private:
	/** 装备 3D Actor 蓝图类 - 装备时生成的 Actor 类型 */
	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	TSubclassOf<ASBI_EquipActor> EquipActorClass = nullptr;

	/** 当前已生成的装备 Actor 弱引用 */
	TWeakObjectPtr<ASBI_EquipActor> EquippedActor = nullptr;

	/** 骨骼附着点名称 (如 "hand_r" 表示右手) */
	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FName SocketAttachPoint{NAME_None};

	/** 装备类型标签 - 用于匹配装备槽位 */
	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FGameplayTag EquipmentType = FGameplayTag::EmptyTag;
};