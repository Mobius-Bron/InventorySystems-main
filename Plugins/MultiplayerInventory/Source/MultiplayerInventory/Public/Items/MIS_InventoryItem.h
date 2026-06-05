#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Items/Manifest/MIS_ItemManifest.h"

#include "MIS_InventoryItem.generated.h"

/**
 * 运行时物品对象 - 库存中每个物品的运行时实例
 * 持有 ItemManifest 配置和当前堆叠数量
 * 支持网络复制,作为 FastArray 的条目内容
 */
UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_InventoryItem : public UObject
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }

	/** 设置物品配置清单 */
	void SetItemManifest(const FMIS_ItemManifest& Manifest);

	/** 获取物品配置清单(只读) */
	const FMIS_ItemManifest& GetItemManifest() const { return ItemManifest.Get<FMIS_ItemManifest>(); }

	/** 获取可修改的物品配置清单 */
	FMIS_ItemManifest& GetItemManifestMutable() { return ItemManifest.GetMutable<FMIS_ItemManifest>(); }

	/** 是否可堆叠 (Manifest 中是否有 StackableFragment) */
	bool IsStackable() const;

	/** 是否可消耗 (Manifest 中是否有 ConsumableFragment) */
	bool IsConsumable() const;

	/** 当前总堆叠数量 */
	int32 GetTotalStackCount() const { return TotalStackCount; }

	/** 设置当前总堆叠数量 */
	void SetTotalStackCount(int32 Count) { TotalStackCount = Count; }
private:

	/** 物品配置清单 - 使用 InstancedStruct 存储,支持网络复制 */
	UPROPERTY(VisibleAnywhere, meta = (BaseStruct = "/Script/MultiplayerInventory.MIS_ItemManifest"), Replicated)
	FInstancedStruct ItemManifest;

	/** 当前总堆叠数量 */
	UPROPERTY(Replicated)
	int32 TotalStackCount{0};
};

/**
 * 便捷模板函数: 从物品中按标签获取指定类型的 Fragment
 * @param Item 物品对象
 * @param Tag  片段标签
 * @return 找到的 Fragment 指针,未找到返回 nullptr
 */
template <typename FragmentType>
const FragmentType* GetFragment(const UMIS_InventoryItem* Item, const FGameplayTag& Tag)
{
	if (!IsValid(Item)) return nullptr;

	const FMIS_ItemManifest& Manifest = Item->GetItemManifest();
	return Manifest.GetFragmentOfTypeWithTag<FragmentType>(Tag);
}
