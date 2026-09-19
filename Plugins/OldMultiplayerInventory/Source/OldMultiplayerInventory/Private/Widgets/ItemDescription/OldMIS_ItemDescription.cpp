#include "Widgets/ItemDescription/OldMIS_ItemDescription.h"

#include "Components/SizeBox.h"

FVector2D UOldMIS_ItemDescription::GetBoxSize() const
{
	return SizeBox->GetDesiredSize();
}

void UOldMIS_ItemDescription::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);
}
