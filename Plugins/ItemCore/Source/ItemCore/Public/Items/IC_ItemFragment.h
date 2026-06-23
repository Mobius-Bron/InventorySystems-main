// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "IC_ItemFragment.generated.h"

/**
 * 物品片段基类 - 所有物品属性的基类
 * 通过 GameplayTag 标识片段类型,存储在 ItemManifest 的 Fragment 列表中
 * 使用 InstancedStruct 实现多态存储
 */
USTRUCT(BlueprintType)
struct FIC_ItemFragment
{
	GENERATED_BODY()

	FIC_ItemFragment() {}
	FIC_ItemFragment(const FIC_ItemFragment&) = default;
	FIC_ItemFragment& operator=(const FIC_ItemFragment&) = default;
	FIC_ItemFragment(FIC_ItemFragment&&) = default;
	FIC_ItemFragment& operator=(FIC_ItemFragment&&) = default;
	virtual ~FIC_ItemFragment() {}

	/** 获取片段标签 */
	FGameplayTag GetFragmentTag() const { return FragmentTag; }
	/** 设置片段标签 */
	void SetFragmentTag(FGameplayTag Tag) { FragmentTag = Tag; }
	/** 初始化片段 - 创建运行时物品时调用,可用于随机化值等 */
	virtual void Manifest() {}

private:
	UPROPERTY(EditAnywhere, Category = "ItemCore", meta = (Categories = "FragmentTags"))
	FGameplayTag FragmentTag = FGameplayTag::EmptyTag;
};

/**
 * UI可同化片段基类 (标记型)
 * 继承此类的 Fragment 表示其数据可注入到 UI Composite 控件中
 * 具体的 Assimilate 实现在各背包插件中通过继承此类完成
 */
USTRUCT(BlueprintType)
struct FIC_InventoryItemFragment : public FIC_ItemFragment
{
	GENERATED_BODY()
};

/**
 * 网格片段 - 物品在库存网格中占用的尺寸
 */
USTRUCT(BlueprintType)
struct FIC_GridFragment : public FIC_ItemFragment
{
	GENERATED_BODY()

	FIntPoint GetGridSize() const { return GridSize; }
	void SetGridSize(const FIntPoint& Size) { GridSize = Size; }
	float GetGridPadding() const { return GridPadding; }
	void SetGridPadding(float Padding) { GridPadding = Padding; }

private:
	UPROPERTY(EditAnywhere, Category = "ItemCore")
	FIntPoint GridSize{1, 1};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	float GridPadding{0.f};
};

/**
 * 图片片段 - 物品的图标
 */
USTRUCT(BlueprintType)
struct FIC_ImageFragment : public FIC_InventoryItemFragment
{
	GENERATED_BODY()

	UTexture2D* GetIcon() const { return Icon; }
	FVector2D GetIconDimensions() const { return IconDimensions; }

private:
	UPROPERTY(EditAnywhere, Category = "ItemCore")
	TObjectPtr<UTexture2D> Icon{nullptr};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	FVector2D IconDimensions{44.f, 44.f};
};

/**
 * 文本片段 - 物品的文本描述
 */
USTRUCT(BlueprintType)
struct FIC_TextFragment : public FIC_InventoryItemFragment
{
	GENERATED_BODY()

	FText GetText() const { return FragmentText; }
	void SetText(const FText& Text) { FragmentText = Text; }

private:
	UPROPERTY(EditAnywhere, Category = "ItemCore")
	FText FragmentText;
};

/**
 * 标号数值片段 - 带标签的数值属性 (如: 伤害 +15)
 * 支持随机化: Manifest 时在 Min~Max 范围内随机取值
 */
USTRUCT(BlueprintType)
struct FIC_LabeledNumberFragment : public FIC_InventoryItemFragment
{
	GENERATED_BODY()

	virtual void Manifest() override;
	float GetValue() const { return Value; }

	bool bRandomizeOnManifest{true};

private:
	UPROPERTY(EditAnywhere, Category = "ItemCore")
	FText Text_Label{};

	UPROPERTY(VisibleAnywhere, Category = "ItemCore")
	float Value{0.f};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	float Min{0};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	float Max{0};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	bool bCollapseLabel{false};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	bool bCollapseValue{false};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	int32 MinFractionalDigits{1};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	int32 MaxFractionalDigits{1};
};

/**
 * 可堆叠片段 - 物品的堆叠能力
 */
USTRUCT(BlueprintType)
struct FIC_StackableFragment : public FIC_ItemFragment
{
	GENERATED_BODY()

	int32 GetMaxStackSize() const { return MaxStackSize; }
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(int32 Count) { StackCount = Count; }

private:
	UPROPERTY(EditAnywhere, Category = "ItemCore")
	int32 MaxStackSize{1};

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	int32 StackCount{1};
};