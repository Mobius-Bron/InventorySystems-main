#include "Widgets/HUD/MIS_HUDWidget.h"

#include "InventoryManagement/Components/MIS_InventoryComponent.h"
#include "MIS_MessageKeys.h"
#include "Widgets/HUD/MIS_InfoMessage.h"

void UMIS_HUDWidget::SetInventoryComponent(UMIS_InventoryComponent* InInventoryComponent)
{
	if (!IsValid(InInventoryComponent)) return;
	if (InventoryComponent.Get() == InInventoryComponent) return;  // 重复注入直接忽略

	InventoryComponent = InInventoryComponent;

	// [解耦重构] 全部改为监听消息, SigSource 与数据层发送侧保持一致 (定向投递)。
	// 监听者传入 this, GMP 内部持弱引用, Widget 销毁自动解绑。
	const GMP::FSigSource InventorySource(InInventoryComponent);

	MIS::Listen(MSGKEY(MIS_MSG_NO_ROOM), InventorySource, this,
		[this]()
		{
			OnNoRoom();
		});

	MIS::Listen(MSGKEY(MIS_MSG_PICKUP_PROMPT), InventorySource, this,
		[this](const FString& Message, bool bShow)
		{
			if (bShow)
			{
				ShowPickupMessage(Message);
			}
			else
			{
				HidePickupMessage();
			}
		});

	// 背包打开时隐藏 HUD, 关闭时恢复
	MIS::Listen(MSGKEY(MIS_MSG_MENU_TOGGLED), InventorySource, this,
		[this](bool bOpen)
		{
			SetVisibility(bOpen ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
		});
}

void UMIS_HUDWidget::OnNoRoom()
{
	if (!IsValid(InfoMessage)) return;
	InfoMessage->SetMessage(FText::FromString("No Room In Inventory."));
}
