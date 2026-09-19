#include "Widgets/Composite/OldMIS_Leaf.h"

void UOldMIS_Leaf::ApplyFunction(FuncType Function)
{
	Function(this);
}
