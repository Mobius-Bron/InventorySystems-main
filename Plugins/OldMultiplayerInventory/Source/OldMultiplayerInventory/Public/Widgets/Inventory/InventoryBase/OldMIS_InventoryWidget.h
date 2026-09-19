#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/OldMIS_GridTypes.h"

#include "OldMIS_InventoryWidget.generated.h"

class UOldMIS_InventoryGrid;
class UOldMIS_InventoryComponent;
class UOldMIS_ItemComponent;
class UCanvasPanel;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_InventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	UFUNCTION(BlueprintCallable, Category = "OldMIS|Init")
	void InitFromComponent(UOldMIS_InventoryComponent* InventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "Inventory UI")
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory UI")
	void CloseInventory();

	FOldMIS_SlotAvailabilityResult HasRoomForItem(UOldMIS_ItemComponent* ItemComponent) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOldMIS_InventoryGrid> InventoryGrid;

	TWeakObjectPtr<UOldMIS_InventoryComponent> InventoryComponent;
	bool bIsOpen{false};
};
