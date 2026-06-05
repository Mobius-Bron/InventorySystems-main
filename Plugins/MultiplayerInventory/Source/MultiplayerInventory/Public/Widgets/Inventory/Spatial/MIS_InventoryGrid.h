#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/MIS_GridTypes.h"

#include "MIS_InventoryGrid.generated.h"

class UMIS_ItemPopUp;
class UMIS_HoverItem;
struct FMIS_ImageFragment;
struct FMIS_GridFragment;
class UMIS_SlottedItem;
class UMIS_ItemComponent;
struct FMIS_ItemManifest;
class UCanvasPanel;
class UMIS_GridSlot;
class UMIS_InventoryComponent;
class UMIS_ItemDescription;
class UMIS_EquippedGridSlot;
class UMIS_EquippedSlottedItem;
struct FGameplayTag;
enum class EMIS_GridSlotState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMIS_GridItemHovered, UMIS_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMIS_GridItemUnhovered);

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
public:
	FMIS_SlotAvailabilityResult HasRoomForItem(const UMIS_ItemComponent* ItemComponent);
	void ShowCursor();
	void HideCursor();
	void SetOwningCanvas(UCanvasPanel* OwningCanvas);
	void DropItem();
	bool HasHoverItem() const;
	UMIS_HoverItem* GetHoverItem() const;
	float GetTileSize() const { return TileSize; }
	void ClearHoverItem();
	void AssignHoverItem(UMIS_InventoryItem* InventoryItem);
	void OnHide();

	void InitFromComponent(UMIS_InventoryComponent* InInventoryComponent, UCanvasPanel* InCanvasPanel);

	FMIS_GridItemHovered OnGridItemHovered;
	FMIS_GridItemUnhovered OnGridItemUnhovered;

	UFUNCTION()
	void AddItem(UMIS_InventoryItem* Item);

	UFUNCTION()
	void OnExternalItemRemoved(UMIS_InventoryItem* Item);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Widget")
	TArray<TObjectPtr<UMIS_EquippedGridSlot>> EquippedGridSlots;

public:
	void ConstructGrid();
	void ClearGrid();
	
private:
	TWeakObjectPtr<UMIS_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UCanvasPanel> OwningCanvasPanel;

	FMIS_SlotAvailabilityResult HasRoomForItem(const UMIS_InventoryItem* Item, const int32 StackAmountOverride = -1);
	FMIS_SlotAvailabilityResult HasRoomForItem(const FMIS_ItemManifest& Manifest, const int32 StackAmountOverride = -1);
	void AddItemToIndices(const FMIS_SlotAvailabilityResult& Result, UMIS_InventoryItem* NewItem);
	FVector2D GetDrawSize(const FMIS_GridFragment* GridFragment) const;
	void SetSlottedItemImage(const UMIS_SlottedItem* SlottedItem, const FMIS_GridFragment* GridFragment, const FMIS_ImageFragment* ImageFragment) const;
	void AddItemAtIndex(UMIS_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount);
	UMIS_SlottedItem* CreateSlottedItem(UMIS_InventoryItem* Item,
		const bool bStackable,
		const int32 StackAmount,
		const FMIS_GridFragment* GridFragment,
		const FMIS_ImageFragment* ImageFragment,
		const int32 Index);
	void AddSlottedItemToCanvas(const int32 Index, const FMIS_GridFragment* GridFragment, UMIS_SlottedItem* SlottedItem) const;
	void UpdateGridSlots(UMIS_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount);
	bool IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index) const;
	bool HasRoomAtIndex(const UMIS_GridSlot* GridSlot,
		const FIntPoint& Dimensions,
		const TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		const int32 MaxStackSize);
	bool CheckSlotConstraints(const UMIS_GridSlot* GridSlot,
		const UMIS_GridSlot* SubGridSlot,
		const TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		const int32 MaxStackSize) const;
	FIntPoint GetItemDimensions(const FMIS_ItemManifest& Manifest) const;
	bool HasValidItem(const UMIS_GridSlot* GridSlot) const;
	bool IsUpperLeftSlot(const UMIS_GridSlot* GridSlot, const UMIS_GridSlot* SubGridSlot) const;
	bool DoesItemTypeMatch(const UMIS_InventoryItem* SubItem, const FGameplayTag& ItemType) const;
	bool IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const;
	int32 DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize, const int32 AmountToFill, const UMIS_GridSlot* GridSlot) const;
	int32 GetStackAmount(const UMIS_GridSlot* GridSlot) const;
	bool IsRightClick(const FPointerEvent& MouseEvent) const;
	bool IsLeftClick(const FPointerEvent& MouseEvent) const;
	void PickUp(UMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	void AssignHoverItem(UMIS_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex);
	void RemoveItemFromGrid(UMIS_InventoryItem* InventoryItem, const int32 GridIndex);
	void UpdateTileParameters(const FVector2D& CanvasPosition, const FVector2D& MousePosition);
	FIntPoint CalculateHoveredCoordinates(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const;
	EMIS_TileQuadrant CalculateTileQuadrant(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const;
	void OnTileParametersUpdated(const FMIS_TileParameters& Parameters);
	FIntPoint CalculateStartingCoordinate(const FIntPoint& Coordinate, const FIntPoint& Dimensions, const EMIS_TileQuadrant Quadrant) const;
	FMIS_SpaceQueryResult CheckHoverPosition(const FIntPoint& Position, const FIntPoint& Dimensions);
	bool CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location);
	void HighlightSlots(const int32 Index, const FIntPoint& Dimensions);
	void UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions);
	void ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EMIS_GridSlotState GridSlotState);
	void PutDownOnIndex(const int32 Index);
	UUserWidget* GetVisibleCursorWidget();
	UUserWidget* GetHiddenCursorWidget();
	bool IsSameStackable(const UMIS_InventoryItem* ClickedInventoryItem) const;
	void SwapWithHoverItem(UMIS_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
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
	TSubclassOf<UMIS_ItemDescription> ItemDescriptionClass;

	UPROPERTY()
	TObjectPtr<UMIS_ItemDescription> ItemDescription;

	FTimerHandle DescriptionTimer;

	UPROPERTY(EditAnywhere, Category = "Inventory|Description", meta = (AllowPrivateAccess = "true"))
	float DescriptionTimerDelay = 0.5f;

	UMIS_ItemDescription* GetItemDescription();
	void SetItemDescriptionSizeAndPosition(UMIS_ItemDescription* Description, UCanvasPanel* Canvas) const;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UMIS_ItemDescription> EquippedItemDescriptionClass;

	UPROPERTY()
	TObjectPtr<UMIS_ItemDescription> EquippedItemDescription;

	FTimerHandle EquippedDescriptionTimer;

	UPROPERTY(EditAnywhere, Category = "Inventory|Description")
	float EquippedDescriptionTimerDelay = 0.5f;

	UMIS_ItemDescription* GetEquippedItemDescription();
	void SetEquippedItemDescriptionSizeAndPosition(UMIS_ItemDescription* Description, UCanvasPanel* Canvas) const;
	void ShowEquippedItemDescription(UMIS_InventoryItem* Item);
	void ClearEquippedItemDescription();

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UMIS_ItemPopUp> ItemPopUpClass;

	UPROPERTY()
	TObjectPtr<UMIS_ItemPopUp> ItemPopUp;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> VisibleCursorWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> HiddenCursorWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> VisibleCursorWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> HiddenCursorWidget;

	UFUNCTION()
	void AddStacks(const FMIS_SlotAvailabilityResult& Result);

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
	void EquippedGridSlotClicked(UMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag);

	UFUNCTION()
	void EquippedSlottedItemClicked(UMIS_EquippedSlottedItem* EquippedSlottedItem);

	bool CanEquipHoverItem(UMIS_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const;
	UMIS_EquippedGridSlot* FindSlotWithEquippedItem(UMIS_InventoryItem* EquippedItem) const;
	void ClearSlotOfItem(UMIS_EquippedGridSlot* EquippedGridSlot);
	void RemoveEquippedSlottedItem(UMIS_EquippedSlottedItem* EquippedSlottedItem);
	void MakeEquippedSlottedItem(UMIS_EquippedSlottedItem* OldSlottedItem, UMIS_EquippedGridSlot* EquippedGridSlot, UMIS_InventoryItem* ItemToEquip);
	void BroadcastSlotClickedDelegates(UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip) const;
	void BindEquippedGridSlotDelegates();

	UPROPERTY()
	TArray<TObjectPtr<UMIS_GridSlot>> GridSlots;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UMIS_GridSlot> GridSlotClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UMIS_SlottedItem> SlottedItemClass;

	UPROPERTY()
	TMap<int32, TObjectPtr<UMIS_SlottedItem>> SlottedItems;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Rows;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Columns;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UMIS_HoverItem> HoverItemClass;

	UPROPERTY()
	TObjectPtr<UMIS_HoverItem> HoverItem;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FVector2D ItemPopUpOffset;

	FMIS_TileParameters TileParameters;
	FMIS_TileParameters LastTileParameters;

	int32 ItemDropIndex{INDEX_NONE};
	FMIS_SpaceQueryResult CurrentQueryResult;
	bool bMouseWithinCanvas;
	bool bLastMouseWithinCanvas;
	int32 LastHighlightedIndex;
	FIntPoint LastHighlightedDimensions;
};
