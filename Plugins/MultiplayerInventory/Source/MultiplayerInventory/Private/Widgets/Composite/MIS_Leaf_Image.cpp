#include "Widgets/Composite/MIS_Leaf_Image.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

void UMIS_Leaf_Image::SetImage(UTexture2D* Texture) const
{
	Image_Icon->SetBrushFromTexture(Texture);
}

void UMIS_Leaf_Image::SetBoxSize(const FVector2D& Size) const
{
	SizeBox_Icon->SetWidthOverride(Size.X);
	SizeBox_Icon->SetHeightOverride(Size.Y);
}

void UMIS_Leaf_Image::SetImageSize(const FVector2D& Size) const
{
	Image_Icon->SetDesiredSizeOverride(Size);
}

FVector2D UMIS_Leaf_Image::GetImageSize() const
{
	return Image_Icon->GetBrush().ImageSize;
}
