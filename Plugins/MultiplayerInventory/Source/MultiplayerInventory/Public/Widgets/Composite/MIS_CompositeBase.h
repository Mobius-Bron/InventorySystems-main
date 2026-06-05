#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "MIS_CompositeBase.generated.h"

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_CompositeBase : public UUserWidget
{
	GENERATED_BODY()
public:
	FGameplayTag GetFragmentTag() const { return FragmentTag; }
	void SetFragmentTag(const FGameplayTag& Tag) { FragmentTag = Tag; }
	virtual void Collapse();
	void Expand();

	using FuncType = TFunction<void(UMIS_CompositeBase*)>;
	virtual void ApplyFunction(FuncType Function) {}

private:

	UPROPERTY(EditAnywhere, Category = "MIS|Composite")
	FGameplayTag FragmentTag;
};
