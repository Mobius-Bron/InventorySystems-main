#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MIS_HUDWidget.generated.h"

class UMIS_InfoMessage;
class UMIS_InventoryComponent;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * [解耦重构] 由外部 (PlayerController) 注入库存组件。
	 *
	 * 注入后本 Widget 通过 GMP 消息接收:
	 *   MIS.Inv.NoRoom        -> 背包已满提示
	 *   MIS.Inv.PickupPrompt  -> 拾取提示显隐 (替代原先数据层直接调用 Show/HidePickupMessage)
	 *   MIS.Inv.MenuToggled   -> 背包开关时自身显隐
	 * 不再直接绑定数据层委托, 也不再反向查找组件。重复注入会被忽略。
	 */
	void SetInventoryComponent(UMIS_InventoryComponent* InInventoryComponent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void ShowPickupMessage(const FString& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HidePickupMessage();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMIS_InfoMessage> InfoMessage;

	TWeakObjectPtr<UMIS_InventoryComponent> InventoryComponent;

	UFUNCTION()
	void OnNoRoom();
};
