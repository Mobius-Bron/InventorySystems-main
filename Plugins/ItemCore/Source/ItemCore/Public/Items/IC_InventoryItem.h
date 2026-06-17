// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Items/IC_ItemManifest.h"

#include "IC_InventoryItem.generated.h"

/**
 * 运行时物品对象 - 库存中每个物品的运行时实例
 * 持有 ItemManifest 配置和当前堆叠数量
 * 支持网络复制,作为 FastArray 的条目内容
 */
UCLASS()
class ITEMCORE_API UIC_InventoryItem : public UObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }

	void SetItemManifest(const FIC_ItemManifest& Manifest);
	const FIC_ItemManifest& GetItemManifest() const { return ItemManifest.Get<FIC_ItemManifest>(); }
	FIC_ItemManifest& GetItemManifestMutable() { return ItemManifest.GetMutable<FIC_ItemManifest>(); }

	bool IsStackable() const;

	int32 GetTotalStackCount() const { return TotalStackCount; }
	void SetTotalStackCount(int32 Count) { TotalStackCount = Count; }

	FGameplayTag GetItemType() const;

private:
	UPROPERTY(VisibleAnywhere, meta = (BaseStruct = "/Script/ItemCore.IC_ItemManifest"), Replicated)
	FInstancedStruct ItemManifest;

	UPROPERTY(Replicated)
	int32 TotalStackCount{0};
};

/**
 * 便捷模板函数: 从物品中按标签获取指定类型的 Fragment
 */
template <typename FragmentType>
const FragmentType* GetFragment(const UIC_InventoryItem* Item, const FGameplayTag& Tag)
{
	if (!IsValid(Item)) return nullptr;

	const FIC_ItemManifest& Manifest = Item->GetItemManifest();
	return Manifest.GetFragmentOfTypeWithTag<FragmentType>(Tag);
}