#include "Widgets/Composite/MIS_Leaf_Text.h"
#include "Components/TextBlock.h"

void UMIS_Leaf_Text::SetText(const FText& Text) const
{
	Text_Value->SetText(Text);
}
