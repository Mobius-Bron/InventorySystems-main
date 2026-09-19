#include "Widgets/Composite/OldMIS_Leaf_LabeledValue.h"
#include "Components/TextBlock.h"

void UOldMIS_Leaf_LabeledValue::SetText_Label(const FText& Text, bool bCollapse)
{
	Text_Label->SetText(Text);
	if (bCollapse)
	{
		Text_Label->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UOldMIS_Leaf_LabeledValue::SetText_Value(const FText& Text, bool bCollapse)
{
	Text_Value->SetText(Text);
	if (bCollapse)
	{
		Text_Value->SetVisibility(ESlateVisibility::Collapsed);
	}
}
