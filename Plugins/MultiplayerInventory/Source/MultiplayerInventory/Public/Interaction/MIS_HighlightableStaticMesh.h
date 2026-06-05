#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/MIS_Highlightable.h"

#include "MIS_HighlightableStaticMesh.generated.h"

/**
 * 静态网格高亮组件 - 支持拾取高亮的静态网格体
 * 原理: 通过调用父类 SetOverlayMaterial() 叠加一层半透明高亮材质
 * 实现 IMIS_Highlightable 接口,用于拾取系统的射线检测高亮
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class MULTIPLAYERINVENTORY_API UMIS_HighlightableStaticMesh : public UStaticMeshComponent, public IMIS_Highlightable
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
