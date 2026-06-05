#pragma once

#include "CoreMinimal.h"
#include "Widgets/Composite/MIS_Composite.h"

#include "MIS_ItemDescription.generated.h"

class USizeBox;

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_ItemDescription : public UMIS_Composite
{
	GENERATED_BODY()

public:
	FVector2D GetBoxSize() const;
	virtual void SetVisibility(ESlateVisibility InVisibility) override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;
};
