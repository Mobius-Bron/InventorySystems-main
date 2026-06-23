// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SBI_GridSlot.generated.h"

class USBI_ItemPopUp;
class UIC_InventoryItem;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSBI_GridSlotEvent, int32, GridIndex, const FPointerEvent&, MouseEvent);

UENUM(BlueprintType)
enum class ESBI_GridSlotState : uint8
{
	Unoccupied,
	Occupied,
	Selected
};

UCLASS()
class SANDBOXINVENTORY_API USBI_GridSlot : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	void SetTileIndex(int32 Index) { TileIndex = Index; }
	int32 GetTileIndex() const { return TileIndex; }
	ESBI_GridSlotState GetGridSlotState() const { return GridSlotState; }
	void SetInventoryItem(UIC_InventoryItem* Item);
	UIC_InventoryItem* GetInventoryItem() const;
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(int32 Count) { StackCount = Count; }
	void SetItemPopUp(USBI_ItemPopUp* PopUp);
	USBI_ItemPopUp* GetItemPopUp() const;

	void SetOccupiedTexture();
	void SetUnoccupiedTexture();
	void SetSelectedTexture();

	FSBI_GridSlotEvent GridSlotClicked;
	FSBI_GridSlotEvent GridSlotHovered;
	FSBI_GridSlotEvent GridSlotUnhovered;

private:
	int32 StackCount{0};
	int32 TileIndex{INDEX_NONE};
	TWeakObjectPtr<UIC_InventoryItem> InventoryItem;
	TWeakObjectPtr<USBI_ItemPopUp> ItemPopUp;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GridSlot;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FSlateBrush Brush_Unoccupied;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FSlateBrush Brush_Occupied;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FSlateBrush Brush_Selected;

	ESBI_GridSlotState GridSlotState;

	UFUNCTION()
	void OnItemPopUpDestruct(UUserWidget* Menu);
};