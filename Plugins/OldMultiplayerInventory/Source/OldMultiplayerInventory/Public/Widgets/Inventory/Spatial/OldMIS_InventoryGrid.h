#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/OldMIS_GridTypes.h"

#include "OldMIS_InventoryGrid.generated.h"

class UOldMIS_ItemPopUp;
class UOldMIS_HoverItem;
struct FOldMIS_ImageFragment;
struct FOldMIS_GridFragment;
class UOldMIS_SlottedItem;
class UOldMIS_ItemComponent;
struct FOldMIS_ItemManifest;
class UCanvasPanel;
class UOldMIS_GridSlot;
class UOldMIS_InventoryComponent;
class UOldMIS_ItemDescription;
class UOldMIS_EquippedGridSlot;
class UOldMIS_EquippedSlottedItem;
struct FGameplayTag;
enum class EOldMIS_GridSlotState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOldMIS_GridItemHovered, UOldMIS_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOldMIS_GridItemUnhovered);

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
public:
	FOldMIS_SlotAvailabilityResult HasRoomForItem(const UOldMIS_ItemComponent* ItemComponent);
	void ShowCursor();
	void HideCursor();
	void SetOwningCanvas(UCanvasPanel* OwningCanvas);
	void DropItem();
	bool HasHoverItem() const;
	UOldMIS_HoverItem* GetHoverItem() const;
	float GetTileSize() const { return TileSize; }
	void ClearHoverItem();
	void AssignHoverItem(UOldMIS_InventoryItem* InventoryItem);
	void OnHide();

	void InitFromComponent(UOldMIS_InventoryComponent* InInventoryComponent, UCanvasPanel* InCanvasPanel);

	FOldMIS_GridItemHovered OnGridItemHovered;
	FOldMIS_GridItemUnhovered OnGridItemUnhovered;

	UFUNCTION()
	void AddItem(UOldMIS_InventoryItem* Item);

	UFUNCTION()
	void OnExternalItemRemoved(UOldMIS_InventoryItem* Item);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Widget")
	TArray<TObjectPtr<UOldMIS_EquippedGridSlot>> EquippedGridSlots;

public:
	void ConstructGrid();
	void ClearGrid();
	
private:
	TWeakObjectPtr<UOldMIS_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UCanvasPanel> OwningCanvasPanel;

	FOldMIS_SlotAvailabilityResult HasRoomForItem(const UOldMIS_InventoryItem* Item, const int32 StackAmountOverride = -1);
	FOldMIS_SlotAvailabilityResult HasRoomForItem(const FOldMIS_ItemManifest& Manifest, const int32 StackAmountOverride = -1);
	void AddItemToIndices(const FOldMIS_SlotAvailabilityResult& Result, UOldMIS_InventoryItem* NewItem);
	FVector2D GetDrawSize(const FOldMIS_GridFragment* GridFragment) const;
	void SetSlottedItemImage(const UOldMIS_SlottedItem* SlottedItem, const FOldMIS_GridFragment* GridFragment, const FOldMIS_ImageFragment* ImageFragment) const;
	void AddItemAtIndex(UOldMIS_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount);
	UOldMIS_SlottedItem* CreateSlottedItem(UOldMIS_InventoryItem* Item,
		const bool bStackable,
		const int32 StackAmount,
		const FOldMIS_GridFragment* GridFragment,
		const FOldMIS_ImageFragment* ImageFragment,
		const int32 Index);
	void AddSlottedItemToCanvas(const int32 Index, const FOldMIS_GridFragment* GridFragment, UOldMIS_SlottedItem* SlottedItem) const;
	void UpdateGridSlots(UOldMIS_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount);
	bool IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index) const;
	bool HasRoomAtIndex(const UOldMIS_GridSlot* GridSlot,
		const FIntPoint& Dimensions,
		const TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		const int32 MaxStackSize);
	bool CheckSlotConstraints(const UOldMIS_GridSlot* GridSlot,
		const UOldMIS_GridSlot* SubGridSlot,
		const TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		const int32 MaxStackSize) const;
	FIntPoint GetItemDimensions(const FOldMIS_ItemManifest& Manifest) const;
	bool HasValidItem(const UOldMIS_GridSlot* GridSlot) const;
	bool IsUpperLeftSlot(const UOldMIS_GridSlot* GridSlot, const UOldMIS_GridSlot* SubGridSlot) const;
	bool DoesItemTypeMatch(const UOldMIS_InventoryItem* SubItem, const FGameplayTag& ItemType) const;
	bool IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const;
	int32 DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize, const int32 AmountToFill, const UOldMIS_GridSlot* GridSlot) const;
	int32 GetStackAmount(const UOldMIS_GridSlot* GridSlot) const;
	bool IsRightClick(const FPointerEvent& MouseEvent) const;
	bool IsLeftClick(const FPointerEvent& MouseEvent) const;
	void PickUp(UOldMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	void AssignHoverItem(UOldMIS_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex);
	void RemoveItemFromGrid(UOldMIS_InventoryItem* InventoryItem, const int32 GridIndex);
	void UpdateTileParameters(const FVector2D& CanvasPosition, const FVector2D& MousePosition);
	FIntPoint CalculateHoveredCoordinates(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const;
	EOldMIS_TileQuadrant CalculateTileQuadrant(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const;
	void OnTileParametersUpdated(const FOldMIS_TileParameters& Parameters);
	FIntPoint CalculateStartingCoordinate(const FIntPoint& Coordinate, const FIntPoint& Dimensions, const EOldMIS_TileQuadrant Quadrant) const;
	FOldMIS_SpaceQueryResult CheckHoverPosition(const FIntPoint& Position, const FIntPoint& Dimensions);
	bool CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location);
	void HighlightSlots(const int32 Index, const FIntPoint& Dimensions);
	void UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions);
	void ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EOldMIS_GridSlotState GridSlotState);
	void PutDownOnIndex(const int32 Index);
	UUserWidget* GetVisibleCursorWidget();
	UUserWidget* GetHiddenCursorWidget();
	bool IsSameStackable(const UOldMIS_InventoryItem* ClickedInventoryItem) const;
	void SwapWithHoverItem(UOldMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	bool ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount, const int32 MaxStackSize) const;
	void SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index);
	bool ShouldConsumeHoverItemStacks(const int32 HoveredStackCount, const int32 RoomInClickedSlot) const;
	void ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index);
	bool ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount) const;
	void FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index);
	void CreateItemPopUp(const int32 GridIndex);
	void PutHoverItemBack();
	void OnItemUnhovered();

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOldMIS_ItemDescription> ItemDescriptionClass;

	UPROPERTY()
	TObjectPtr<UOldMIS_ItemDescription> ItemDescription;

	FTimerHandle DescriptionTimer;

	UPROPERTY(EditAnywhere, Category = "Inventory|Description", meta = (AllowPrivateAccess = "true"))
	float DescriptionTimerDelay = 0.5f;

	UOldMIS_ItemDescription* GetItemDescription();
	void SetItemDescriptionSizeAndPosition(UOldMIS_ItemDescription* Description, UCanvasPanel* Canvas) const;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOldMIS_ItemDescription> EquippedItemDescriptionClass;

	UPROPERTY()
	TObjectPtr<UOldMIS_ItemDescription> EquippedItemDescription;

	FTimerHandle EquippedDescriptionTimer;

	UPROPERTY(EditAnywhere, Category = "Inventory|Description")
	float EquippedDescriptionTimerDelay = 0.5f;

	UOldMIS_ItemDescription* GetEquippedItemDescription();
	void SetEquippedItemDescriptionSizeAndPosition(UOldMIS_ItemDescription* Description, UCanvasPanel* Canvas) const;
	void ShowEquippedItemDescription(UOldMIS_InventoryItem* Item);
	void ClearEquippedItemDescription();

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOldMIS_ItemPopUp> ItemPopUpClass;

	UPROPERTY()
	TObjectPtr<UOldMIS_ItemPopUp> ItemPopUp;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> VisibleCursorWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> HiddenCursorWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> VisibleCursorWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> HiddenCursorWidget;

	UFUNCTION()
	void AddStacks(const FOldMIS_SlotAvailabilityResult& Result);

	UFUNCTION()
	void OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnPopUpMenuSplit(int32 SplitAmount, int32 Index);

	UFUNCTION()
	void OnPopUpMenuDrop(int32 Index);

	UFUNCTION()
	void OnPopUpMenuConsume(int32 Index);

	UFUNCTION()
	void OnSlottedItemHovered(int32 GridIndex);

	UFUNCTION()
	void OnSlottedItemUnhovered(int32 GridIndex);

	UFUNCTION()
	void OnInventoryMenuToggled(bool bOpen);

	UFUNCTION()
	void EquippedGridSlotClicked(UOldMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag);

	UFUNCTION()
	void EquippedSlottedItemClicked(UOldMIS_EquippedSlottedItem* EquippedSlottedItem);

	bool CanEquipHoverItem(UOldMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const;
	UOldMIS_EquippedGridSlot* FindSlotWithEquippedItem(UOldMIS_InventoryItem* EquippedItem) const;
	void ClearSlotOfItem(UOldMIS_EquippedGridSlot* EquippedGridSlot);
	void RemoveEquippedSlottedItem(UOldMIS_EquippedSlottedItem* EquippedSlottedItem);
	void MakeEquippedSlottedItem(UOldMIS_EquippedSlottedItem* OldSlottedItem, UOldMIS_EquippedGridSlot* EquippedGridSlot, UOldMIS_InventoryItem* ItemToEquip);
	void BroadcastSlotClickedDelegates(UOldMIS_InventoryItem* ItemToEquip, UOldMIS_InventoryItem* ItemToUnequip) const;
	void BindEquippedGridSlotDelegates();

	UPROPERTY()
	TArray<TObjectPtr<UOldMIS_GridSlot>> GridSlots;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOldMIS_GridSlot> GridSlotClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOldMIS_SlottedItem> SlottedItemClass;

	UPROPERTY()
	TMap<int32, TObjectPtr<UOldMIS_SlottedItem>> SlottedItems;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Rows;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Columns;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOldMIS_HoverItem> HoverItemClass;

	UPROPERTY()
	TObjectPtr<UOldMIS_HoverItem> HoverItem;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FVector2D ItemPopUpOffset;

	FOldMIS_TileParameters TileParameters;
	FOldMIS_TileParameters LastTileParameters;

	int32 ItemDropIndex{INDEX_NONE};
	FOldMIS_SpaceQueryResult CurrentQueryResult;
	bool bMouseWithinCanvas;
	bool bLastMouseWithinCanvas;
	int32 LastHighlightedIndex;
	FIntPoint LastHighlightedDimensions;
};
