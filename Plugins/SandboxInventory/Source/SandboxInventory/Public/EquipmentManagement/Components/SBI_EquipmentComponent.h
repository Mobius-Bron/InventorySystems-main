// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SBI_EquipmentComponent.generated.h"

struct FGameplayTag;
class USBI_InventoryComponent;
class UIC_InventoryItem;
class ASBI_EquipActor;
class APlayerController;
class USkeletalMeshComponent;
struct FSBI_EquipmentFragment;
struct FIC_ItemManifest;

/**
 * 装备管理组件 - 负责装备 Actor 的生成/销毁
 *
 * 初始化: 外部调用 Init(PC, InvComp, Mesh) 完成所有必要引用绑定,
 * 不依赖 Owner 类型, 可挂在 Character / PlayerController / ProxyMesh 等任意 Actor 上。
 */
UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class SANDBOXINVENTORY_API USBI_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USBI_EquipmentComponent();

	/** 设置使用的骨骼网格体 (独立调用, 如构造时 Mesh 已就绪) */
	void SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh);
	/** 标记为代理模式 (ProxyMesh 专用, 跳过效果执行) */
	void SetIsProxy(bool bProxy) { bIsProxy = bProxy; }

	/**
	 * 统一初始化入口
	 * @param InPC         玩家控制器
	 * @param InInvComp    库存组件
	 * @param InMesh       骨骼网格体 (可选, 已通过 SetOwningSkeletalMesh 设置时可传 nullptr)
	 */
	void Init(APlayerController* InPC, USBI_InventoryComponent* InInvComp, USkeletalMeshComponent* InMesh = nullptr);

	/** 获取所有已装备的 Actor */
	const TArray<TObjectPtr<ASBI_EquipActor>>& GetEquippedActors() const { return EquippedActors; }

private:
	TWeakObjectPtr<USBI_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh;

	UFUNCTION()
	void OnItemEquipped(UIC_InventoryItem* EquippedItem);

	UFUNCTION()
	void OnItemUnequipped(UIC_InventoryItem* UnequippedItem);

	ASBI_EquipActor* SpawnEquippedActor(FSBI_EquipmentFragment* EquipmentFragment, const FIC_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh);

	UPROPERTY()
	TArray<TObjectPtr<ASBI_EquipActor>> EquippedActors;

	ASBI_EquipActor* FindEquippedActor(const FGameplayTag& EquipmentTypeTag);
	void RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag);

	bool bIsProxy{false};
};