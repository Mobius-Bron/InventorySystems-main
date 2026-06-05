#include "Widgets/Composite/MIS_Composite.h"
#include "Blueprint/WidgetTree.h"

void UMIS_Composite::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Children.Empty();

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UMIS_CompositeBase* Composite = Cast<UMIS_CompositeBase>(Widget); IsValid(Composite))
		{
			Children.Add(Composite);
			Composite->Collapse();
		}
	});
}

void UMIS_Composite::ApplyFunction(FuncType Function)
{
	for (auto& Child : Children)
	{
		if (IsValid(Child))
		{
			Function(Child);
			Child->ApplyFunction(Function);
		}
	}
}

void UMIS_Composite::Collapse()
{
	UMIS_CompositeBase::Collapse();
	for (auto& Child : Children)
	{
		if (IsValid(Child))
		{
			Child->Collapse();
		}
	}
}
