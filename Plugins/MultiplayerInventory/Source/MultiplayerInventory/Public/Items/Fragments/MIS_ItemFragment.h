#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "MIS_ItemFragment.generated.h"

class APlayerController;

/**
 * 物品片段基类 - 所有物品属性的基类
 * 通过 GameplayTag 标识片段类型,存储在 ItemManifest 的 Fragment 列表中
 * 使用 InstancedStruct 实现多态存储
 */
USTRUCT(BlueprintType)
struct FMIS_ItemFragment
{
	GENERATED_BODY()

	FMIS_ItemFragment() {}
	FMIS_ItemFragment(const FMIS_ItemFragment&) = default;
	FMIS_ItemFragment& operator=(const FMIS_ItemFragment&) = default;
	FMIS_ItemFragment(FMIS_ItemFragment&&) = default;
	FMIS_ItemFragment& operator=(FMIS_ItemFragment&&) = default;
	virtual ~FMIS_ItemFragment() {}

	/** 获取片段标签 */
	FGameplayTag GetFragmentTag() const { return FragmentTag; }
	/** 设置片段标签 */
	void SetFragmentTag(FGameplayTag Tag) { FragmentTag = Tag; }
	/** 初始化片段 - 创建运行时物品时调用,可用于随机化值等 */
	virtual void Manifest() {}

private:
	/** 片段标签 - 标识片段类型 */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (Categories="FragmentTags"))
	FGameplayTag FragmentTag = FGameplayTag::EmptyTag;
};

class UMIS_CompositeBase;

/**
 * 可同化到UI的片段 - 数据可注入到 Composite UI 控件中的 Fragment
 * 通过 Assimilate() 方法将自身数据填充到对应的 Leaf 控件中
 */
USTRUCT(BlueprintType)
struct FMIS_InventoryItemFragment : public FMIS_ItemFragment
{
	GENERATED_BODY()

	/** 将片段数据同化到 Composite UI 控件中 */
	virtual void Assimilate(UMIS_CompositeBase* Composite) const;
protected:
	/** 检查 Composite 的 FragmentTag 是否匹配 */
	bool MatchesWidgetTag(const UMIS_CompositeBase* Composite) const;
};

/**
 * 网格片段 - 物品在库存网格中占用的尺寸
 * 例如 2x3 表示占2列3行共6格
 */
USTRUCT(BlueprintType)
struct FMIS_GridFragment : public FMIS_ItemFragment
{
	GENERATED_BODY()

	/** 获取网格占用尺寸 */
	FIntPoint GetGridSize() const { return GridSize; }
	/** 设置网格占用尺寸 */
	void SetGridSize(const FIntPoint& Size) { GridSize = Size; }
	/** 获取网格内边距 */
	float GetGridPadding() const { return GridPadding; }
	void SetGridPadding(float Padding) { GridPadding = Padding; }

private:
	/** 网格尺寸 (列数, 行数) */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FIntPoint GridSize{1, 1};

	/** 网格内边距 (物品图标与格子边缘的间距) */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float GridPadding{0.f};
};

/**
 * 图片片段 - 物品的图标
 */
USTRUCT(BlueprintType)
struct FMIS_ImageFragment : public FMIS_InventoryItemFragment
{
	GENERATED_BODY()

	/** 获取图标纹理 */
	UTexture2D* GetIcon() const { return Icon; }
	virtual void Assimilate(UMIS_CompositeBase* Composite) const override;

private:
	/** 图标纹理 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TObjectPtr<UTexture2D> Icon{nullptr};

	/** 图标显示尺寸 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FVector2D IconDimensions{44.f, 44.f};
};

/**
 * 文本片段 - 物品的文本描述
 */
USTRUCT(BlueprintType)
struct FMIS_TextFragment : public FMIS_InventoryItemFragment
{
	GENERATED_BODY()

	FText GetText() const { return FragmentText; }
	void SetText(const FText& Text) { FragmentText = Text; }
	virtual void Assimilate(UMIS_CompositeBase* Composite) const override;

private:
	/** 文本内容 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FText FragmentText;
};

/**
 * 标号数值片段 - 带标签的数值属性 (如: 伤害 +15)
 * 支持随机化: Manifest 时在 Min~Max 范围内随机取值
 */
USTRUCT(BlueprintType)
struct FMIS_LabeledNumberFragment : public FMIS_InventoryItemFragment
{
	GENERATED_BODY()

	virtual void Assimilate(UMIS_CompositeBase* Composite) const override;
	virtual void Manifest() override;
	float GetValue() const { return Value; }

	/** 首次 Manifest 时是否随机化数值 */
	bool bRandomizeOnManifest{true};

private:
	/** 标签文本 (如 "伤害") */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FText Text_Label{};

	/** 当前数值 */
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	float Value{0.f};

	/** 随机化最小值 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float Min{0};

	/** 随机化最大值 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float Max{0};

	/** 是否折叠标签 (不显示标签文本) */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	bool bCollapseLabel{false};

	/** 是否折叠数值 (不显示数值文本) */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	bool bCollapseValue{false};

	/** 最小小数位数 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MinFractionalDigits{1};

	/** 最大小数位数 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxFractionalDigits{1};
};

/**
 * 可堆叠片段 - 物品的堆叠能力
 */
USTRUCT(BlueprintType)
struct FMIS_StackableFragment : public FMIS_ItemFragment
{
	GENERATED_BODY()

	/** 最大堆叠数量 */
	int32 GetMaxStackSize() const { return MaxStackSize; }
	/** 当前堆叠数量 */
	int32 GetStackCount() const { return StackCount; }
	/** 设置当前堆叠数量 */
	void SetStackCount(int32 Count) { StackCount = Count; }

private:
	/** 最大堆叠数量 (如 20 表示最多20个同类型物品堆在一起) */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxStackSize{1};

	/** 当前堆叠数量 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 StackCount{1};
};

// ---------- 消耗品相关 Fragment ----------

/**
 * 消耗效果基类 - 物品消耗时执行的效果
 */
USTRUCT(BlueprintType)
struct FMIS_ConsumeModifier : public FMIS_LabeledNumberFragment
{
	GENERATED_BODY()

	/** 消耗时执行 - 在子类中实现具体效果 */
	virtual void OnConsume(APlayerController* PC) {}
};

/**
 * 可消耗片段 - 标记物品可被消耗,包含消耗效果列表
 */
USTRUCT(BlueprintType)
struct FMIS_ConsumableFragment : public FMIS_InventoryItemFragment
{
	GENERATED_BODY()

	/** 执行消耗 - 遍历所有 ConsumeModifier */
	virtual void OnConsume(APlayerController* PC);
	virtual void Assimilate(UMIS_CompositeBase* Composite) const override;
	virtual void Manifest() override;
private:
	/** 消耗效果列表 */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FMIS_ConsumeModifier>> ConsumeModifiers;
};

/** 生命药水效果 - 消耗时恢复生命值 */
USTRUCT(BlueprintType)
struct FMIS_HealthPotionFragment : public FMIS_ConsumeModifier
{
	GENERATED_BODY()
	virtual void OnConsume(APlayerController* PC) override;
};

/** 法力药水效果 - 消耗时恢复法力值 */
USTRUCT(BlueprintType)
struct FMIS_ManaPotionFragment : public FMIS_ConsumeModifier
{
	GENERATED_BODY()
	virtual void OnConsume(APlayerController* PC) override;
};

// ---------- 装备相关 Fragment ----------

/**
 * 装备效果基类 - 装备/卸下时执行的效果
 */
USTRUCT(BlueprintType)
struct FMIS_EquipModifier : public FMIS_LabeledNumberFragment
{
	GENERATED_BODY()

	/** 装备时执行 */
	virtual void OnEquip(APlayerController* PC) {}
	/** 卸下时执行 */
	virtual void OnUnequip(APlayerController* PC) {}
};

/** 力量加成 - 装备时增加力量,卸下时减少 */
USTRUCT(BlueprintType)
struct FMIS_StrengthModifier : public FMIS_EquipModifier
{
	GENERATED_BODY()
	virtual void OnEquip(APlayerController* PC) override;
	virtual void OnUnequip(APlayerController* PC) override;
};

/** 护甲加成 - 装备时增加护甲,卸下时减少 */
USTRUCT(BlueprintType)
struct FMIS_ArmorModifier : public FMIS_EquipModifier
{
	GENERATED_BODY()
	virtual void OnEquip(APlayerController* PC) override;
	virtual void OnUnequip(APlayerController* PC) override;
};

/** 伤害加成 - 装备时增加伤害,卸下时减少 */
USTRUCT(BlueprintType)
struct FMIS_DamageModifier : public FMIS_EquipModifier
{
	GENERATED_BODY()
	virtual void OnEquip(APlayerController* PC) override;
	virtual void OnUnequip(APlayerController* PC) override;
};

class AMIS_EquipActor;

/**
 * 装备片段 - 物品可装备到角色身上
 * 包含装备效果列表、3D装备Actor类、附着骨骼点
 * 装备时会在角色骨骼上生成 EquipActor 并显示
 */
USTRUCT(BlueprintType)
struct FMIS_EquipmentFragment : public FMIS_InventoryItemFragment
{
	GENERATED_BODY()

	bool bEquipped{false}; // 是否已装备

	/** 装备效果入口 - 遍历 EquipModifiers 执行 OnEquip */
	void OnEquip(APlayerController* PC);
	/** 卸下效果入口 - 遍历 EquipModifiers 执行 OnUnequip */
	void OnUnequip(APlayerController* PC);
	virtual void Assimilate(UMIS_CompositeBase* Composite) const override;
	virtual void Manifest() override;

	/** 在骨骼网格体上生成并附着装备 Actor */
	AMIS_EquipActor* SpawnAttachedActor(USkeletalMeshComponent* AttachMesh) const;
	/** 销毁已生成的装备 Actor */
	void DestroyAttachedActor() const;
	/** 获取装备类型 Tag (用于匹配装备槽位) */
	FGameplayTag GetEquipmentType() const { return EquipmentType; }
	/** 设置当前装备 Actor 引用 */
	void SetEquippedActor(AMIS_EquipActor* EquipActor);

private:
	/** 装备效果列表 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<TInstancedStruct<FMIS_EquipModifier>> EquipModifiers;

	/** 装备 3D Actor 蓝图类 - 装备时生成的 Actor 类型 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<AMIS_EquipActor> EquipActorClass = nullptr;

	/** 当前已生成的装备 Actor 弱引用 */
	TWeakObjectPtr<AMIS_EquipActor> EquippedActor = nullptr;

	/** 骨骼附着点名称 (如 "hand_r" 表示右手) */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FName SocketAttachPoint{NAME_None};

	/** 装备类型标签 - 用于匹配 EquippedGridSlot */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FGameplayTag EquipmentType = FGameplayTag::EmptyTag;
};
