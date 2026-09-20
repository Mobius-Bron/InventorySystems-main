#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "MIS_PlayerController.generated.h"

class UMIS_InventoryWidget;
class UMIS_HUDWidget;

UCLASS()
class INVENTORYPROJECT_API AMIS_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMIS_PlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMIS_HUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMIS_InventoryWidget> InventoryWidgetClass;

private:
	/**
	 * [联机修复] 创建背包相关 UI (幂等)。
	 *
	 * 客户端上的执行顺序是: PC 复制 -> PC BeginPlay -> Pawn 复制 -> ClientRestart(装上 Pawn)。
	 * 因此在 BeginPlay 里取 Pawn 很可能为空。原实现在此直接 return, 导致纯客户端下
	 * 背包 UI 永不创建; 而 TryAddItem 又依赖 UI 计算空间, 连锁结果是客户端无法拾取物品。
	 * 现在改为: Pawn 就绪则立即创建, 否则挂起等待 OnNewPawn 回调。
	 */
	void CreateInventoryUI();

	/**
	 * [联机修复] 客户端 Pawn 复制到位后的补建回调。
	 * 注意: APlayerController::OnNewPawn 是原生多播委托 (TMulticastDelegate), 不是动态委托,
	 * 因此绑定用 AddUObject / 解绑用 RemoveAll, 不能使用 AddDynamic/RemoveDynamic。
	 */
	void HandleNewPawn(APawn* NewPawn);

	/** 防止 BeginPlay 与 OnNewPawn 两条路径重复创建 UI。 */
	bool bInventoryUICreated{false};
};
