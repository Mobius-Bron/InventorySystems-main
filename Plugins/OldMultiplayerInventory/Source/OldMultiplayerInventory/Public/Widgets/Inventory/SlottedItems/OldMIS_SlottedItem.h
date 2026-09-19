#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "OldMIS_SlottedItem.generated.h"

class UOldMIS_InventoryItem;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOldMIS_SlottedItemClicked, int32, GridIndex, const FPointerEvent&, MouseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOldMIS_SlottedItemHovered, int32, GridIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOldMIS_SlottedItemUnhovered, int32, GridIndex);

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_SlottedItem : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;

public:
	bool IsStackable() const { return bIsStackable; }
	void SetIsStackable(bool bStackable) { bIsStackable = bStackable; }
	UImage* GetImageIcon() const { return Image_Icon; }
	void SetGridIndex(int32 Index) { GridIndex = Index; }
	int32 GetGridIndex() const { return GridIndex; }
	void SetGridDimensions(const FIntPoint& Dimensions) { GridDimensions = Dimensions; }
	FIntPoint GetGridDimensions() const { return GridDimensions; }
	void SetInventoryItem(UOldMIS_InventoryItem* Item);
	UOldMIS_InventoryItem* GetInventoryItem() const;
	void SetImageBrush(const FSlateBrush& Brush) const;
	void UpdateStackCount(int32 StackCount);

	FOldMIS_SlottedItemClicked OnSlottedItemClicked;
	FOldMIS_SlottedItemHovered OnSlottedItemHovered;
	FOldMIS_SlottedItemUnhovered OnSlottedItemUnhovered;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	int32 GridIndex;
	FIntPoint GridDimensions;
	TWeakObjectPtr<UOldMIS_InventoryItem> InventoryItem;
	bool bIsStackable{false};
};
