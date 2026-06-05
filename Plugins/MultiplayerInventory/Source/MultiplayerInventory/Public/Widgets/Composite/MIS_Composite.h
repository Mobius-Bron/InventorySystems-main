#pragma once

#include "CoreMinimal.h"
#include "MIS_CompositeBase.h"
#include "MIS_Composite.generated.h"

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_Composite : public UMIS_CompositeBase
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void ApplyFunction(FuncType Function) override;
	virtual void Collapse() override;
	TArray<UMIS_CompositeBase*> GetChildren() { return Children; }

private:
	UPROPERTY()
	TArray<TObjectPtr<UMIS_CompositeBase>> Children;
};
