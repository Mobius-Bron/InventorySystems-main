#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interaction/MIS_Highlightable.h"

#include "MIS_HighlightableSkeletalMesh.generated.h"

/**
 * 骨骼网格高亮组件 - 支持拾取高亮的骨骼网格体 (如武器/盔甲)
 * 原理: 通过调用父类 SetOverlayMaterial() 叠加一层半透明高亮材质
 * 用于需要骨骼动画的可拾取物品
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class MULTIPLAYERINVENTORY_API UMIS_HighlightableSkeletalMesh : public USkeletalMeshComponent, public IMIS_Highlightable
{
	GENERATED_BODY()

public:
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;

private:
	/** 高亮叠加材质 - 不能命名为 OverlayMaterial,与父类 UMeshComponent 冲突 */
	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	TObjectPtr<UMaterialInterface> HighlightMaterial = nullptr;

	/** 是否当前已高亮 */
	bool bHighlighted{false};
};
