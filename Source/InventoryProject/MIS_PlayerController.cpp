#include "MIS_PlayerController.h"

#include "DH_DebugFunctionLibrary.h"
#include "MIS_PlayerCharacter.h"
#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "Widgets/HUD/MIS_HUDWidget.h"
#include "Widgets/Inventory/InventoryBase/MIS_InventoryWidget.h"

AMIS_PlayerController::AMIS_PlayerController()
{
	SetReplicates(true);
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
}

void AMIS_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;

	// [联机修复] 客户端执行顺序: PC 复制 -> BeginPlay -> Pawn 复制 -> ClientRestart。
	// 在 BeginPlay 里 GetPawn() 很可能为空, 不能在此时直接放弃。
	if (IsValid(GetPawn()))
	{
		CreateInventoryUI();
	}
	else
	{
		// OnNewPawn 是原生多播委托 (非动态), 必须用 AddUObject 而非 AddDynamic
		OnNewPawn.AddUObject(this, &ThisClass::HandleNewPawn);
	}
}

void AMIS_PlayerController::HandleNewPawn(APawn* NewPawn)
{
	OnNewPawn.RemoveAll(this);
	CreateInventoryUI();
}

void AMIS_PlayerController::CreateInventoryUI()
{
	if (bInventoryUICreated) return;

	AMIS_PlayerCharacter* MISCharacter = Cast<AMIS_PlayerCharacter>(GetPawn());
	if (!IsValid(MISCharacter))
	{
		DH_LOG_ERR("[背包PC] 创建 UI 中止: Pawn 未就绪或不是 AMIS_PlayerCharacter");
		return;
	}

	UMIS_InventoryComponent* InventoryComp = MISCharacter->GetInventoryComponent();
	if (!IsValid(InventoryComp))
	{
		DH_LOG_ERR("[背包PC] 创建 UI 中止: Character 上找不到 InventoryComponent");
		return;
	}

	bInventoryUICreated = true;

	if (IsValid(HUDWidgetClass))
	{
		UMIS_HUDWidget* HUDWidget = CreateWidget<UMIS_HUDWidget>(this, HUDWidgetClass);
		if (IsValid(HUDWidget))
		{
			HUDWidget->AddToViewport();
			// [解耦重构] 注入库存组件: HUD 内部通过 GMP 消息接收拾取提示 / 背包满 / 开关通知
			HUDWidget->SetInventoryComponent(InventoryComp);
		}
	}

	if (IsValid(InventoryWidgetClass))
	{
		UMIS_InventoryWidget* InventoryWidget = CreateWidget<UMIS_InventoryWidget>(this, InventoryWidgetClass);
		if (IsValid(InventoryWidget))
		{
			InventoryWidget->AddToViewport();
			// [解耦重构] 先注入组件(空间查询需要), 再注册消息监听
			InventoryComp->SetInventoryWidget(InventoryWidget);
			InventoryWidget->InitFromComponent(InventoryComp);
			InventoryWidget->CloseInventory();
		}
	}
}
