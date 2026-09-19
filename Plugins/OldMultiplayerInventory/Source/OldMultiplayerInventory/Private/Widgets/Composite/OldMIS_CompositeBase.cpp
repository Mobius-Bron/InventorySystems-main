#include "Widgets/Composite/OldMIS_CompositeBase.h"

void UOldMIS_CompositeBase::Collapse()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UOldMIS_CompositeBase::Expand()
{
	SetVisibility(ESlateVisibility::Visible);
}
