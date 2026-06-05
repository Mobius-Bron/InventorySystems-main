#include "BlueprintFunctionLibraries/MIS_WidgetFunctionLibrary.h"

#include "Components/Widget.h"
#include "Blueprint/SlateBlueprintLibrary.h"

FVector2D UMIS_WidgetFunctionLibrary::GetWidgetPosition(UWidget* Widget)
{
	if (!IsValid(Widget)) return FVector2D::ZeroVector;
	return Widget->GetCachedGeometry().GetAbsolutePosition();
}

FVector2D UMIS_WidgetFunctionLibrary::GetWidgetSize(UWidget* Widget)
{
	if (!IsValid(Widget)) return FVector2D::ZeroVector;
	return Widget->GetCachedGeometry().GetLocalSize();
}

bool UMIS_WidgetFunctionLibrary::IsWithinBounds(const FVector2D& BoundaryPos, const FVector2D& WidgetSize, const FVector2D& MousePos)
{
	return MousePos.X >= BoundaryPos.X && MousePos.X <= BoundaryPos.X + WidgetSize.X &&
		   MousePos.Y >= BoundaryPos.Y && MousePos.Y <= BoundaryPos.Y + WidgetSize.Y;
}

FVector2D UMIS_WidgetFunctionLibrary::GetClampedWidgetPosition(const FVector2D& Boundary, const FVector2D& WidgetSize, const FVector2D& MousePos)
{
	FVector2D ClampedPos = MousePos;

	if (MousePos.X + WidgetSize.X > Boundary.X)
	{
		ClampedPos.X = MousePos.X - WidgetSize.X;
	}
	if (MousePos.Y + WidgetSize.Y > Boundary.Y)
	{
		ClampedPos.Y = MousePos.Y - WidgetSize.Y;
	}

	ClampedPos.X = FMath::Max(ClampedPos.X, 0.f);
	ClampedPos.Y = FMath::Max(ClampedPos.Y, 0.f);

	return ClampedPos;
}

int32 UMIS_WidgetFunctionLibrary::GetIndexFromPosition(const FIntPoint& Position, const int32 Columns)
{
	return Position.Y * Columns + Position.X;
}

FIntPoint UMIS_WidgetFunctionLibrary::GetPositionFromIndex(const int32 Index, const int32 Columns)
{
	return FIntPoint(Index % Columns, Index / Columns);
}
