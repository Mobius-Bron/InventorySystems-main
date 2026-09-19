#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "OldMIS_HUDWidget.generated.h"

class UOldMIS_InfoMessage;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_HUDWidget : public UUserWidget
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
	TObjectPtr<UOldMIS_InfoMessage> InfoMessage;

	UFUNCTION()
	void OnNoRoom();
};
