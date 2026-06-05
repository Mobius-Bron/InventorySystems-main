#pragma once

#include "CoreMinimal.h"
#include "MIS_CompositeBase.h"
#include "MIS_Leaf.generated.h"

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_Leaf : public UMIS_CompositeBase
{
	GENERATED_BODY()
public:
	virtual void ApplyFunction(FuncType Function) override;
};
