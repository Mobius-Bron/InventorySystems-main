#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "MIS_EquipActor.generated.h"

/**
 * 装备 3D Actor - 装备后附着在角色骨骼槽位上的可视 Actor
 * 携带 EquipmentTypeTag,标识自身属于什么装备类型
 * 由 EquipmentComponent 根据 EquipmentFragment 动态生成
 */
UCLASS()
class MULTIPLAYERINVENTORY_API AMIS_EquipActor : public AActor
{
	GENERATED_BODY()

public:
	AMIS_EquipActor();

	/** 获取装备类型标签 */
	FGameplayTag GetEquipmentType() const { return EquipmentType; }
	/** 设置装备类型标签 */
	void SetEquipmentType(FGameplayTag InEquipmentType) { EquipmentType = InEquipmentType; }

private:
	/** 装备类型标签 - 与 EquippedGridSlot 的 EquipmentTypeTag 匹配 */
	UPROPERTY(EditInstanceOnly, Category = "Inventory")
	FGameplayTag EquipmentType = FGameplayTag::EmptyTag;
};
