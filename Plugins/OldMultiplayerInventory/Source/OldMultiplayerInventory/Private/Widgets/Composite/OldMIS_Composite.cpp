#include "Widgets/Composite/OldMIS_Composite.h"
#include "Blueprint/WidgetTree.h"

void UOldMIS_Composite::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Children.Empty();

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UOldMIS_CompositeBase* Composite = Cast<UOldMIS_CompositeBase>(Widget); IsValid(Composite))
		{
			Children.Add(Composite);
			Composite->Collapse();
		}
	});
}

void UOldMIS_Composite::ApplyFunction(FuncType Function)
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

void UOldMIS_Composite::Collapse()
{
	UOldMIS_CompositeBase::Collapse();
	for (auto& Child : Children)
	{
		if (IsValid(Child))
		{
			Child->Collapse();
		}
	}
}
