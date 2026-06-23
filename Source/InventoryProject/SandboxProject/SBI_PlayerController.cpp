// Copyright AmberAeolian. All Rights Reserved.

#include "SBI_PlayerController.h"

#include "DS_DebugFunctionLibrary.h"
#include "SBI_PlayerCharacter.h"
#include "InventoryManagement/Components/SBI_InventoryComponent.h"
#include "Widgets/Inventory/InventoryBase/SBI_InventoryWidget.h"
#include "Widgets/Equipment/SBI_EquipmentWidget.h"

ASBI_PlayerController::ASBI_PlayerController()
{
	SetReplicates(true);
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
}

void ASBI_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;

	ASBI_PlayerCharacter* SBICharacter = Cast<ASBI_PlayerCharacter>(GetPawn());
	if (!IsValid(SBICharacter)) return;

	USBI_InventoryComponent* InventoryComp = SBICharacter->GetInventoryComponent();
	if (!IsValid(InventoryComp)) return;

	if (IsValid(InventoryWidgetClass))
	{
		USBI_InventoryWidget* InventoryWidget = CreateWidget<USBI_InventoryWidget>(this, InventoryWidgetClass);
		if (IsValid(InventoryWidget))
		{
			InventoryWidget->AddToViewport();
			InventoryWidget->InitFromComponent(InventoryComp);
			InventoryWidget->CloseInventory();
			InventoryComp->SetInventoryWidget(InventoryWidget);

			DS_LOG(Inventory, "[沙盒Controller] 背包UI已创建");
		}
	}

	if (IsValid(EquipmentWidgetClass))
	{
		USBI_EquipmentWidget* EquipmentWidget = CreateWidget<USBI_EquipmentWidget>(this, EquipmentWidgetClass);
		if (IsValid(EquipmentWidget))
		{
			EquipmentWidget->AddToViewport();
			EquipmentWidget->InitFromComponent(InventoryComp, nullptr);
			EquipmentWidget->CloseEquipment();

			DS_LOG(Inventory, "[沙盒Controller] 装备UI已创建");
		}
	}
}