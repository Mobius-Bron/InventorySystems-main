#include "MIS_PlayerController.h"

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

	AMIS_PlayerCharacter* MISCharacter = Cast<AMIS_PlayerCharacter>(GetPawn());
	if (!IsValid(MISCharacter)) return;

	UMIS_InventoryComponent* InventoryComp = MISCharacter->GetInventoryComponent();
	if (!IsValid(InventoryComp)) return;

	if (IsValid(HUDWidgetClass))
	{
		UMIS_HUDWidget* HUDWidget = CreateWidget<UMIS_HUDWidget>(this, HUDWidgetClass);
		if (IsValid(HUDWidget))
		{
			HUDWidget->AddToViewport();
			// [解耦重构] 改为注入库存组件: HUD 内部通过 GMP 消息接收拾取提示/背包满/开关通知
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
