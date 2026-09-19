#pragma once

#include "CoreMinimal.h"
#include "Widgets/Composite/OldMIS_Composite.h"

#include "OldMIS_ItemDescription.generated.h"

class USizeBox;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_ItemDescription : public UOldMIS_Composite
{
	GENERATED_BODY()

public:
	FVector2D GetBoxSize() const;
	virtual void SetVisibility(ESlateVisibility InVisibility) override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;
};
