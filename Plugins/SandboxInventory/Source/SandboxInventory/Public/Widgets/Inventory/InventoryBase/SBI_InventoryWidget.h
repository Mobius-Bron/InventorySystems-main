// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SBI_Types.h"

#include "SBI_InventoryWidget.generated.h"

class USBI_InventoryGrid;
class USBI_InventoryComponent;
class UIC_ItemComponent;
class UCanvasPanel;

UCLASS()
class SANDBOXINVENTORY_API USBI_InventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	UFUNCTION(BlueprintCallable, Category = "SandboxInventory|Init")
	void InitFromComponent(USBI_InventoryComponent* InventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory|UI")
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory|UI")
	void CloseInventory();

	FSBI_SlotAvailabilityResult HasRoomForItem(UIC_ItemComponent* ItemComponent) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USBI_InventoryGrid> InventoryGrid;

	TWeakObjectPtr<USBI_InventoryComponent> InventoryComponent;
	bool bIsOpen{false};
};