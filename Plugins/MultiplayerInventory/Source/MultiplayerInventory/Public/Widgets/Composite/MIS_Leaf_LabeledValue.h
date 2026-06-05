#pragma once

#include "CoreMinimal.h"
#include "MIS_Leaf.h"
#include "MIS_Leaf_LabeledValue.generated.h"

class UTextBlock;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_Leaf_LabeledValue : public UMIS_Leaf
{
	GENERATED_BODY()

public:
	void SetText_Label(const FText& Text, bool bCollapse);
	void SetText_Value(const FText& Text, bool bCollapse);

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Label;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Value;
};
