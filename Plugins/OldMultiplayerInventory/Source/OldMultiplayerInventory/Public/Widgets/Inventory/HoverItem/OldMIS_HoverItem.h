#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "OldMIS_HoverItem.generated.h"

class UOldMIS_InventoryItem;
class UImage;
class UTextBlock;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_HoverItem : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetImageBrush(const FSlateBrush& Brush) const;
	void UpdateStackCount(const int32 Count);

	FGameplayTag GetItemType() const;
	int32 GetStackCount() const { return StackCount; }
	bool IsStackable() const { return bIsStackable; }
	void SetIsStackable(bool bStacks);
	int32 GetPreviousGridIndex() const { return PreviousGridIndex; }
	void SetPreviousGridIndex(int32 Index) { PreviousGridIndex = Index; }
	FIntPoint GetGridDimensions() const { return GridDimensions; }
	void SetGridDimensions(const FIntPoint& Dimensions) { GridDimensions = Dimensions; }
	UOldMIS_InventoryItem* GetInventoryItem() const;
	void SetInventoryItem(UOldMIS_InventoryItem* Item);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	int32 PreviousGridIndex;
	FIntPoint GridDimensions;
	TWeakObjectPtr<UOldMIS_InventoryItem> InventoryItem;
	bool bIsStackable{false};
	int32 StackCount{0};
};
