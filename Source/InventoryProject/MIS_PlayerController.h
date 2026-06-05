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
};
