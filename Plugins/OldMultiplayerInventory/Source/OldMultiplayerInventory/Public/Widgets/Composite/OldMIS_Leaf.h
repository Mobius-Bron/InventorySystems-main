#pragma once

#include "CoreMinimal.h"
#include "OldMIS_CompositeBase.h"
#include "OldMIS_Leaf.generated.h"

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_Leaf : public UOldMIS_CompositeBase
{
	GENERATED_BODY()
public:
	virtual void ApplyFunction(FuncType Function) override;
};
