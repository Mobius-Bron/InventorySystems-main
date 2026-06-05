#pragma once

#include "CoreMinimal.h"
#include "Types/MIS_GridTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"

#include "MIS_ItemManifest.generated.h"

class UMIS_InventoryItem;
struct FMIS_ItemFragment;
class UMIS_CompositeBase;

/**
 * 物品清单 - 物品的完整配置数据容器
 * 包含所有 Fragment 片段,定义了物品的全部属性和行为
 * 可以在蓝图中编辑,作为物品的数据模板
 */
USTRUCT(BlueprintType)
struct MULTIPLAYERINVENTORY_API FMIS_ItemManifest
{
	GENERATED_BODY()

	/** 获取可修改的 Fragment 列表 */
	TArray<TInstancedStruct<FMIS_ItemFragment>>& GetFragmentsMutable() { return Fragments; }

	/** 从配置清单创建运行时 InventoryItem UObject */
	UMIS_InventoryItem* Manifest(UObject* NewOuter);

	/** 获取物品类型 GameplayTag (用于堆叠匹配等) */
	FGameplayTag GetItemType() const { return ItemType; }

	/** 将所有 InventoryItemFragment 的数据注入到 UI 组合控件中 */
	void AssimilateInventoryFragments(UMIS_CompositeBase* Composite) const;

	/** 按类型和标签查找 Fragment (精确匹配标签) */
	template<typename T> requires std::derived_from<T, FMIS_ItemFragment>
	const T* GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const;

	/** 按类型查找第一个匹配的 Fragment */
	template<typename T> requires std::derived_from<T, FMIS_ItemFragment>
	const T* GetFragmentOfType() const;

	/** 按类型查找第一个匹配的可修改 Fragment */
	template<typename T> requires std::derived_from<T, FMIS_ItemFragment>
	T* GetFragmentOfTypeMutable();

	/** 获取所有指定类型的 Fragment */
	template<typename T> requires std::derived_from<T, FMIS_ItemFragment>
	TArray<const T*> GetAllFragmentsOfType() const;

	/** 在指定位置生成可拾取的 Actor */
	void SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation);

private:

	/** Fragment 片段列表 - 物品的所有属性片段集合 */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FMIS_ItemFragment>> Fragments;

	/** 物品类型 GameplayTag - 用于分类和堆叠匹配 */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (Categories="GameItems"))
	FGameplayTag ItemType;

	/** 拾取 Actor 蓝图类 - 掉落时生成的 Actor */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<AActor> PickupActorClass;

	/** 清空所有 Fragment */
	void ClearFragments();
};

template<typename T>
requires std::derived_from<T, FMIS_ItemFragment>
const T* FMIS_ItemManifest::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
{
	for (const TInstancedStruct<FMIS_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			if (!FragmentPtr->GetFragmentTag().MatchesTagExact(FragmentTag)) continue;
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FMIS_ItemFragment>
const T* FMIS_ItemManifest::GetFragmentOfType() const
{
	for (const TInstancedStruct<FMIS_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FMIS_ItemFragment>
T* FMIS_ItemManifest::GetFragmentOfTypeMutable()
{
	for (TInstancedStruct<FMIS_ItemFragment>& Fragment : Fragments)
	{
		if (T* FragmentPtr = Fragment.GetMutablePtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FMIS_ItemFragment>
TArray<const T*> FMIS_ItemManifest::GetAllFragmentsOfType() const
{
	TArray<const T*> Result;
	for (const TInstancedStruct<FMIS_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			Result.Add(FragmentPtr);
		}
	}
	return Result;
}
