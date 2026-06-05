#include "Interaction/MIS_HighlightableSkeletalMesh.h"

void UMIS_HighlightableSkeletalMesh::Highlight_Implementation()
{
	SetOverlayMaterial(HighlightMaterial);
}

void UMIS_HighlightableSkeletalMesh::UnHighlight_Implementation()
{
	SetOverlayMaterial(nullptr);
}
