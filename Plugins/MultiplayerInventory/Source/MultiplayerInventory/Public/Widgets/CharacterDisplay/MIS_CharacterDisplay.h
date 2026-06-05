#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MIS_CharacterDisplay.generated.h"

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_CharacterDisplay : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	bool bIsDragging{false};
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;

	FVector2D CurrentPosition;
	FVector2D LastPosition;
};
