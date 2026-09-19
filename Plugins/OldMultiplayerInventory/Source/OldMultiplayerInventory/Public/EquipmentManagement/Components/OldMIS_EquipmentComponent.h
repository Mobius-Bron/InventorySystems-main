#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "OldMIS_EquipmentComponent.generated.h"

struct FGameplayTag;
class UOldMIS_InventoryComponent;
class UOldMIS_InventoryItem;
class AOldMIS_EquipActor;
class APlayerController;
class USkeletalMeshComponent;
struct FOldMIS_EquipmentFragment;
struct FOldMIS_ItemManifest;

/**
 * 装备管理组件 - 负责装备 Actor 的生成/销毁与装备效果触发
 *
 * 初始化: 外部调用 Init(PC, InvComp, Mesh) 完成所有必要引用绑定,
 * 不依赖 Owner 类型,可挂在 Character / PlayerController / ProxyMesh 等任意 Actor 上。
 */
UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class OLDMULTIPLAYERINVENTORY_API UOldMIS_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOldMIS_EquipmentComponent();

	/** 设置使用的骨骼网格体 (独立调用,如构造时 Mesh 已就绪) */
	void SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh);
	/** 标记为代理模式 (ProxyMesh 专用,跳过 OnEquip 效果执行) */
	void SetIsProxy(bool bProxy) { bIsProxy = bProxy; }

	/**
	 * 统一初始化入口
	 * @param InPC         玩家控制器
	 * @param InInvComp    库存组件
	 * @param InMesh       骨骼网格体 (可选,已通过 SetOwningSkeletalMesh 设置时可传 nullptr)
	 */
	void Init(APlayerController* InPC, UOldMIS_InventoryComponent* InInvComp, USkeletalMeshComponent* InMesh = nullptr);

private:
	TWeakObjectPtr<UOldMIS_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh;

	UFUNCTION()
	void OnItemEquipped(UOldMIS_InventoryItem* EquippedItem);

	UFUNCTION()
	void OnItemUnequipped(UOldMIS_InventoryItem* UnequippedItem);

	AOldMIS_EquipActor* SpawnEquippedActor(FOldMIS_EquipmentFragment* EquipmentFragment, const FOldMIS_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh);

	UPROPERTY()
	TArray<TObjectPtr<AOldMIS_EquipActor>> EquippedActors;

	AOldMIS_EquipActor* FindEquippedActor(const FGameplayTag& EquipmentTypeTag);
	void RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag);

	bool bIsProxy{false};
};
