// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/IC_GridTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"

#include "IC_ItemManifest.generated.h"

class UIC_InventoryItem;
struct FIC_ItemFragment;

/**
 * 物品清单 - 物品的完整配置数据容器
 * 包含所有 Fragment 片段,定义了物品的全部属性和行为
 */
USTRUCT(BlueprintType)
struct ITEMCORE_API FIC_ItemManifest
{
	GENERATED_BODY()

	TArray<TInstancedStruct<FIC_ItemFragment>>& GetFragmentsMutable() { return Fragments; }

	/** 从配置清单创建运行时 InventoryItem UObject */
	UIC_InventoryItem* Manifest(UObject* NewOuter);

	FGameplayTag GetItemType() const { return ItemType; }

	/** 在指定位置生成可拾取的 Actor */
	void SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation);

	/** 按类型和标签查找 Fragment (精确匹配标签) */
	template<typename T> requires std::derived_from<T, FIC_ItemFragment>
	const T* GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const;

	/** 按类型查找第一个匹配的 Fragment */
	template<typename T> requires std::derived_from<T, FIC_ItemFragment>
	const T* GetFragmentOfType() const;

	/** 按类型查找第一个匹配的可修改 Fragment */
	template<typename T> requires std::derived_from<T, FIC_ItemFragment>
	T* GetFragmentOfTypeMutable();

	/** 获取所有指定类型的 Fragment */
	template<typename T> requires std::derived_from<T, FIC_ItemFragment>
	TArray<const T*> GetAllFragmentsOfType() const;

private:
	UPROPERTY(EditAnywhere, Category = "ItemCore", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FIC_ItemFragment>> Fragments;

	UPROPERTY(EditAnywhere, Category = "ItemCore", meta = (Categories = "GameItems"))
	FGameplayTag ItemType;

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	TSubclassOf<AActor> PickupActorClass;

	void ClearFragments();
};

template<typename T> requires std::derived_from<T, FIC_ItemFragment>
const T* FIC_ItemManifest::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
{
	for (const TInstancedStruct<FIC_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			if (!FragmentPtr->GetFragmentTag().MatchesTagExact(FragmentTag)) continue;
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FIC_ItemFragment>
const T* FIC_ItemManifest::GetFragmentOfType() const
{
	for (const TInstancedStruct<FIC_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FIC_ItemFragment>
T* FIC_ItemManifest::GetFragmentOfTypeMutable()
{
	for (TInstancedStruct<FIC_ItemFragment>& Fragment : Fragments)
	{
		if (T* FragmentPtr = Fragment.GetMutablePtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FIC_ItemFragment>
TArray<const T*> FIC_ItemManifest::GetAllFragmentsOfType() const
{
	TArray<const T*> Result;
	for (const TInstancedStruct<FIC_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			Result.Add(FragmentPtr);
		}
	}
	return Result;
}