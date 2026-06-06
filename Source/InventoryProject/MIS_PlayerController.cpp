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
			InventoryComp->SetHUDWidget(HUDWidget);
		}
	}

	if (IsValid(InventoryWidgetClass))
	{
		UMIS_InventoryWidget* InventoryWidget = CreateWidget<UMIS_InventoryWidget>(this, InventoryWidgetClass);
		if (IsValid(InventoryWidget))
		{
			InventoryWidget->AddToViewport();
			InventoryWidget->InitFromComponent(InventoryComp);
			InventoryWidget->CloseInventory();
			InventoryComp->SetInventoryWidget(InventoryWidget);
		}
	}
}
