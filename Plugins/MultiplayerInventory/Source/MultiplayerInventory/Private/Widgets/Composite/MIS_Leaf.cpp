#include "Widgets/Composite/MIS_Leaf.h"

void UMIS_Leaf::ApplyFunction(FuncType Function)
{
	Function(this);
}
