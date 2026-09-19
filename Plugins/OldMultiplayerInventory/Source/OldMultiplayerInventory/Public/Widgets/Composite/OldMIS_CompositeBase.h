#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "OldMIS_CompositeBase.generated.h"

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_CompositeBase : public UUserWidget
{
	GENERATED_BODY()
public:
	FGameplayTag GetFragmentTag() const { return FragmentTag; }
	void SetFragmentTag(const FGameplayTag& Tag) { FragmentTag = Tag; }
	virtual void Collapse();
	void Expand();

	using FuncType = TFunction<void(UOldMIS_CompositeBase*)>;
	virtual void ApplyFunction(FuncType Function) {}

private:

	UPROPERTY(EditAnywhere, Category = "OldMIS|Composite")
	FGameplayTag FragmentTag;
};
