#pragma once

#include "CoreMinimal.h"
#include "OldMIS_CompositeBase.h"
#include "OldMIS_Composite.generated.h"

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_Composite : public UOldMIS_CompositeBase
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void ApplyFunction(FuncType Function) override;
	virtual void Collapse() override;
	TArray<UOldMIS_CompositeBase*> GetChildren() { return Children; }

private:
	UPROPERTY()
	TArray<TObjectPtr<UOldMIS_CompositeBase>> Children;
};
