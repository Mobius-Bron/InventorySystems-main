#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "BlueprintFunctionLibraries/MIS_WidgetFunctionLibrary.h"

#include "MIS_InventoryFunctionLibrary.generated.h"

class UMIS_InventoryComponent;
class UMIS_InventoryItem;
class APlayerController;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_InventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static UMIS_InventoryComponent* GetInventoryComponent(const APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static UMIS_InventoryItem* FindFirstItemByType(APlayerController* PC, FGameplayTag ItemType);

	template<typename T, typename FuncT>
	static void ForEach2D(TArray<T>& Array, int32 Index, const FIntPoint& Range2D, int32 GridColumns, const FuncT& Function);

	template <typename LambdaType>
	static void ForEach2D(int32 Columns, int32 Rows, LambdaType Lambda)
	{
		for (int32 Row = 0; Row < Rows; Row++)
		{
			for (int32 Column = 0; Column < Columns; Column++)
			{
				const int32 Index = Row * Columns + Column;
				Lambda(Row, Column, Index);
			}
		}
	}
};

template<typename T, typename FuncT>
void UMIS_InventoryFunctionLibrary::ForEach2D(TArray<T>& Array, int32 Index, const FIntPoint& Range2D, int32 GridColumns, const FuncT& Function)
{
	for (int32 j = 0; j < Range2D.Y; ++j)
	{
		for (int32 i = 0; i < Range2D.X; ++i)
		{
			const FIntPoint Coordinates = UMIS_WidgetFunctionLibrary::GetPositionFromIndex(Index, GridColumns) + FIntPoint(i, j);
			const int32 TileIndex = UMIS_WidgetFunctionLibrary::GetIndexFromPosition(Coordinates, GridColumns);
			if (Array.IsValidIndex(TileIndex))
			{
				Function(Array[TileIndex]);
			}
		}
	}
}
