#pragma once

#include "CoreMinimal.h"
#include "OldMIS_Leaf.h"
#include "OldMIS_Leaf_Text.generated.h"

class UTextBlock;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_Leaf_Text : public UOldMIS_Leaf
{
	GENERATED_BODY()

public:
	void SetText(const FText& Text) const;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Value;
};
