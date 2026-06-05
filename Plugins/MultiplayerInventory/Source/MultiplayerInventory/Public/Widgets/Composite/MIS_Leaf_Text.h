#pragma once

#include "CoreMinimal.h"
#include "MIS_Leaf.h"
#include "MIS_Leaf_Text.generated.h"

class UTextBlock;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_Leaf_Text : public UMIS_Leaf
{
	GENERATED_BODY()

public:
	void SetText(const FText& Text) const;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Value;
};
