#include "Widgets/Composite/OldMIS_Leaf_Text.h"
#include "Components/TextBlock.h"

void UOldMIS_Leaf_Text::SetText(const FText& Text) const
{
	Text_Value->SetText(Text);
}
