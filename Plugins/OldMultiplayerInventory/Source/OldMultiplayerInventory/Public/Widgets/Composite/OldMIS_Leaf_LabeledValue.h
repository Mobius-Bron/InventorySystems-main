#pragma once

#include "CoreMinimal.h"
#include "OldMIS_Leaf.h"
#include "OldMIS_Leaf_LabeledValue.generated.h"

class UTextBlock;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_Leaf_LabeledValue : public UOldMIS_Leaf
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
