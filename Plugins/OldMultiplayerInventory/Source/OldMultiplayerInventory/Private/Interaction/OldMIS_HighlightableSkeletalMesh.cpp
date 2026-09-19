#include "Interaction/OldMIS_HighlightableSkeletalMesh.h"

void UOldMIS_HighlightableSkeletalMesh::Highlight_Implementation()
{
	SetOverlayMaterial(HighlightMaterial);
}

void UOldMIS_HighlightableSkeletalMesh::UnHighlight_Implementation()
{
	SetOverlayMaterial(nullptr);
}
