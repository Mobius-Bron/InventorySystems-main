#include "Widgets/Composite/MIS_Leaf_LabeledValue.h"
#include "Components/TextBlock.h"

void UMIS_Leaf_LabeledValue::SetText_Label(const FText& Text, bool bCollapse)
{
	Text_Label->SetText(Text);
	if (bCollapse)
	{
		Text_Label->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMIS_Leaf_LabeledValue::SetText_Value(const FText& Text, bool bCollapse)
{
	Text_Value->SetText(Text);
	if (bCollapse)
	{
		Text_Value->SetVisibility(ESlateVisibility::Collapsed);
	}
}
