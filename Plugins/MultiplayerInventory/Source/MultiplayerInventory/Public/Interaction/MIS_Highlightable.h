#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "MIS_Highlightable.generated.h"

/**
 * 高亮接口 - 可被拾取射线检测到的物品必须实现的接口
 * 提供 Highlight/UnHighlight 蓝图事件
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UMIS_Highlightable : public UInterface
{
	GENERATED_BODY()
};

class MULTIPLAYERINVENTORY_API IMIS_Highlightable
{
	GENERATED_BODY()

public:
	/** 开启高亮效果 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Highlightable")
	void Highlight();

	/** 关闭高亮效果 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Highlightable")
	void UnHighlight();
};
