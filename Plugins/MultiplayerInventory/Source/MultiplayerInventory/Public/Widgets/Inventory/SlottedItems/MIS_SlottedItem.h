#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MIS_SlottedItem.generated.h"

class UMIS_InventoryItem;
class UImage;
class UTextBlock;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_SlottedItem : public UUserWidget
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
	void SetInventoryItem(UMIS_InventoryItem* Item);
	UMIS_InventoryItem* GetInventoryItem() const;
	void SetImageBrush(const FSlateBrush& Brush) const;
	void UpdateStackCount(int32 StackCount);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	int32 GridIndex;
	FIntPoint GridDimensions;
	TWeakObjectPtr<UMIS_InventoryItem> InventoryItem;
	bool bIsStackable{false};
};
