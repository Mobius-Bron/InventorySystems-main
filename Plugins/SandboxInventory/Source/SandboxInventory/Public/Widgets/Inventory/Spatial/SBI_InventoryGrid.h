// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SBI_Types.h"

#include "SBI_InventoryGrid.generated.h"

class USBI_ItemPopUp;
class USBI_HoverItem;
class USBI_SlottedItem;
class UIC_ItemComponent;
class UIC_InventoryItem;
class UCanvasPanel;
class USBI_GridSlot;
class USBI_InventoryComponent;
struct FIC_ImageFragment;
struct FGameplayTag;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBI_GridItemHovered, UIC_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSBI_GridItemUnhovered);

UCLASS()
class SANDBOXINVENTORY_API USBI_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	FSBI_SlotAvailabilityResult HasRoomForItem(const UIC_ItemComponent* ItemComponent);
	void ShowCursor();
	void HideCursor();
	void SetOwningCanvas(UCanvasPanel* OwningCanvas);
	void DropItem();
	bool HasHoverItem() const;
	USBI_HoverItem* GetHoverItem() const;
	float GetTileSize() const { return TileSize; }
	void ClearHoverItem();
	void AssignHoverItem(UIC_InventoryItem* InventoryItem);
	void OnHide();

	void InitFromComponent(USBI_InventoryComponent* InInventoryComponent, UCanvasPanel* InCanvasPanel);

	FSBI_GridItemHovered OnGridItemHovered;
	FSBI_GridItemUnhovered OnGridItemUnhovered;

	UFUNCTION()
	void AddItem(UIC_InventoryItem* Item);

	UFUNCTION()
	void OnExternalItemRemoved(UIC_InventoryItem* Item);

public:
	/** 构建网格 */
	void ConstructGrid();
	void ClearGrid();

	/** 网格配置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Grid")
	int32 Rows = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Grid")
	int32 Columns = 9;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Grid")
	float TileSize = 64.f;

	/** 预制类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Classes")
	TSubclassOf<USBI_GridSlot> GridSlotClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Classes")
	TSubclassOf<USBI_SlottedItem> SlottedItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Classes")
	TSubclassOf<USBI_HoverItem> HoverItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SandboxInventory|Classes")
	TSubclassOf<USBI_ItemPopUp> ItemPopUpClass;

	/** 网格槽位列表 */
	UPROPERTY()
	TArray<TObjectPtr<USBI_GridSlot>> GridSlots;

private:
	TWeakObjectPtr<USBI_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UCanvasPanel> OwningCanvasPanel;

	UPROPERTY()
	TObjectPtr<USBI_HoverItem> HoverItem;

	/** 物品槽位映射 (槽位索引 -> SlottedItem) */
	UPROPERTY()
	TMap<int32, TObjectPtr<USBI_SlottedItem>> SlottedItems;

	int32 LastHighlightedIndex{INDEX_NONE};
	int32 ItemDropIndex{INDEX_NONE};
	bool bMouseWithinCanvas{false};

	void AddItemAtIndex(UIC_InventoryItem* Item, int32 Index, bool bStackable, int32 StackAmount);
	USBI_SlottedItem* CreateSlottedItem(UIC_InventoryItem* Item, bool bStackable, int32 StackAmount, int32 Index);
	void SetSlottedItemImage(const USBI_SlottedItem* SlottedItem, const FIC_ImageFragment* ImageFragment) const;

	UFUNCTION()
	void OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnSlottedItemHovered(int32 GridIndex);

	UFUNCTION()
	void OnSlottedItemUnhovered(int32 GridIndex);

	UFUNCTION()
	void OnItemSplit(int32 SplitAmount, int32 Index);
	UFUNCTION()
	void OnItemDrop(int32 Index);
	UFUNCTION()
	void OnItemConsume(int32 Index);
};