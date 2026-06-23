// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "SBI_PlayerController.generated.h"

class USBI_InventoryWidget;
class USBI_EquipmentWidget;

UCLASS()
class INVENTORYPROJECT_API ASBI_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASBI_PlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USBI_InventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USBI_EquipmentWidget> EquipmentWidgetClass;
};