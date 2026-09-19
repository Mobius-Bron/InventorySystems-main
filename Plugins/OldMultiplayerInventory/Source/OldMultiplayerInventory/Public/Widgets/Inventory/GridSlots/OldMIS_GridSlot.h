#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "OldMIS_GridSlot.generated.h"

class UOldMIS_ItemPopUp;
class UOldMIS_InventoryItem;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOldMIS_GridSlotEvent, int32, GridIndex, const FPointerEvent&, MouseEvent);

UENUM(BlueprintType)
enum class EOldMIS_GridSlotState : uint8
{
	Unoccupied,
	Occupied,
	Selected,
	GrayedOut
};

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_GridSlot : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	void SetTileIndex(int32 Index) { TileIndex = Index; }
	int32 GetTileIndex() const { return TileIndex; }
	EOldMIS_GridSlotState GetGridSlotState() const { return GridSlotState; }
	TWeakObjectPtr<UOldMIS_InventoryItem> GetInventoryItem() const { return InventoryItem; }
	void SetInventoryItem(UOldMIS_InventoryItem* Item);
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(int32 Count) { StackCount = Count; }
	int32 GetIndex() const { return TileIndex; }
	void SetIndex(int32 Index) { TileIndex = Index; }
	int32 GetUpperLeftIndex() const { return UpperLeftIndex; }
	void SetUpperLeftIndex(int32 Index) { UpperLeftIndex = Index; }
	bool IsAvailable() const { return bAvailable; }
	void SetAvailable(bool bIsAvailable) { bAvailable = bIsAvailable; }
	void SetItemPopUp(UOldMIS_ItemPopUp* PopUp);
	UOldMIS_ItemPopUp* GetItemPopUp() const;

	void SetOccupiedTexture();
	void SetUnoccupiedTexture();
	void SetSelectedTexture();
	void SetGrayedOutTexture();

	FOldMIS_GridSlotEvent GridSlotClicked;
	FOldMIS_GridSlotEvent GridSlotHovered;
	FOldMIS_GridSlotEvent GridSlotUnhovered;

private:
	int32 StackCount{0};
	bool bAvailable{true};
	int32 TileIndex{INDEX_NONE};
	int32 UpperLeftIndex{INDEX_NONE};
	TWeakObjectPtr<UOldMIS_InventoryItem> InventoryItem;
	TWeakObjectPtr<UOldMIS_ItemPopUp> ItemPopUp;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GridSlot;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Unoccupied;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Occupied;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Selected;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_GrayedOut;

	EOldMIS_GridSlotState GridSlotState;

	UFUNCTION()
	void OnItemPopUpDestruct(UUserWidget* Menu);
};
