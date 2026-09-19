#pragma once

#include "CoreMinimal.h"
#include "OldMIS_Leaf.h"
#include "OldMIS_Leaf_Image.generated.h"

class UImage;
class USizeBox;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API UOldMIS_Leaf_Image : public UOldMIS_Leaf
{
	GENERATED_BODY()

public:
	void SetImage(UTexture2D* Texture) const;
	void SetBoxSize(const FVector2D& Size) const;
	void SetImageSize(const FVector2D& Size) const;
	FVector2D GetImageSize() const;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Icon;
};
