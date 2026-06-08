#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "MIS_EquipmentComponent.generated.h"

struct FGameplayTag;
class UMIS_InventoryComponent;
class UMIS_InventoryItem;
class AMIS_EquipActor;
class APlayerController;
class USkeletalMeshComponent;
struct FMIS_EquipmentFragment;
struct FMIS_ItemManifest;

/**
 * 装备管理组件 - 负责装备 Actor 的生成/销毁与装备效果触发
 *
 * 初始化: 外部调用 Init(PC, InvComp, Mesh) 完成所有必要引用绑定,
 * 不依赖 Owner 类型,可挂在 Character / PlayerController / ProxyMesh 等任意 Actor 上。
 */
UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class MULTIPLAYERINVENTORY_API UMIS_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIS_EquipmentComponent();

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
	void Init(APlayerController* InPC, UMIS_InventoryComponent* InInvComp, USkeletalMeshComponent* InMesh = nullptr);

private:
	TWeakObjectPtr<UMIS_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh;

	UFUNCTION()
	void OnItemEquipped(UMIS_InventoryItem* EquippedItem);

	UFUNCTION()
	void OnItemUnequipped(UMIS_InventoryItem* UnequippedItem);

	AMIS_EquipActor* SpawnEquippedActor(FMIS_EquipmentFragment* EquipmentFragment, const FMIS_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh);

	UPROPERTY()
	TArray<TObjectPtr<AMIS_EquipActor>> EquippedActors;

	AMIS_EquipActor* FindEquippedActor(const FGameplayTag& EquipmentTypeTag);
	void RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag);

	bool bIsProxy{false};
};
