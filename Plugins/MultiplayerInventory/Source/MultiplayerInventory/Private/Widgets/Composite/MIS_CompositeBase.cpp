#include "Widgets/Composite/MIS_CompositeBase.h"

void UMIS_CompositeBase::Collapse()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMIS_CompositeBase::Expand()
{
	SetVisibility(ESlateVisibility::Visible);
}
