#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/MIS_GridTypes.h"

#include "MIS_InventoryWidget.generated.h"

class UMIS_InventoryGrid;
class UMIS_InventoryComponent;
class UMIS_ItemComponent;
class UCanvasPanel;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_InventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	UFUNCTION(BlueprintCallable, Category = "MIS|Init")
	void InitFromComponent(UMIS_InventoryComponent* InventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "Inventory UI")
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory UI")
	void CloseInventory();

	FMIS_SlotAvailabilityResult HasRoomForItem(UMIS_ItemComponent* ItemComponent) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMIS_InventoryGrid> InventoryGrid;

	TWeakObjectPtr<UMIS_InventoryComponent> InventoryComponent;
	bool bIsOpen{false};
};
