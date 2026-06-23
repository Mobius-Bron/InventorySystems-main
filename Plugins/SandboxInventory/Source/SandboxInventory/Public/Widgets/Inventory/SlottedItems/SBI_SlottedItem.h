// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SBI_SlottedItem.generated.h"

class UIC_InventoryItem;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSBI_SlottedItemClicked, int32, GridIndex, const FPointerEvent&, MouseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBI_SlottedItemHovered, int32, GridIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBI_SlottedItemUnhovered, int32, GridIndex);

UCLASS()
class SANDBOXINVENTORY_API USBI_SlottedItem : public UUserWidget
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
	void SetInventoryItem(UIC_InventoryItem* Item);
	UIC_InventoryItem* GetInventoryItem() const;
	void SetImageBrush(const FSlateBrush& Brush) const;
	void UpdateStackCount(int32 StackCount);

	FSBI_SlottedItemClicked OnSlottedItemClicked;
	FSBI_SlottedItemHovered OnSlottedItemHovered;
	FSBI_SlottedItemUnhovered OnSlottedItemUnhovered;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	int32 GridIndex{INDEX_NONE};
	TWeakObjectPtr<UIC_InventoryItem> InventoryItem;
	bool bIsStackable{false};
};