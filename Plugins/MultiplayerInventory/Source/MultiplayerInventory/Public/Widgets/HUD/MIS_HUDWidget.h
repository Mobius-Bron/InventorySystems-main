#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MIS_HUDWidget.generated.h"

class UMIS_InfoMessage;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_HUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void ShowPickupMessage(const FString& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HidePickupMessage();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMIS_InfoMessage> InfoMessage;

	UFUNCTION()
	void OnNoRoom();
};
