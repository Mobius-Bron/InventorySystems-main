#include "Interaction/OldMIS_HighlightableStaticMesh.h"

void UOldMIS_HighlightableStaticMesh::Highlight_Implementation()
{
	SetOverlayMaterial(HighlightMaterial);
}

void UOldMIS_HighlightableStaticMesh::UnHighlight_Implementation()
{
	SetOverlayMaterial(nullptr);
}
