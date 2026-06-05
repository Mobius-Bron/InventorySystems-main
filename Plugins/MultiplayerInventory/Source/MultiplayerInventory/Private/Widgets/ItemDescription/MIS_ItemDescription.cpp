#include "Widgets/ItemDescription/MIS_ItemDescription.h"

#include "Components/SizeBox.h"

FVector2D UMIS_ItemDescription::GetBoxSize() const
{
	return SizeBox->GetDesiredSize();
}

void UMIS_ItemDescription::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);
}
