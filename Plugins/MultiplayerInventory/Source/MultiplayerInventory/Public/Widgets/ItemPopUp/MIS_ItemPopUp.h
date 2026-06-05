#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MIS_ItemPopUp.generated.h"

class UButton;
class USlider;
class UTextBlock;
class USizeBox;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FMIS_PopUpMenuSplit, int32, SplitAmount, int32, Index);
DECLARE_DYNAMIC_DELEGATE_OneParam(FMIS_PopUpMenuDrop, int32, Index);
DECLARE_DYNAMIC_DELEGATE_OneParam(FMIS_PopUpMenuConsume, int32, Index);

UCLASS()
class MULTIPLAYERINVENTORY_API UMIS_ItemPopUp : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

public:
	FMIS_PopUpMenuSplit OnSplit;
	FMIS_PopUpMenuDrop OnDrop;
	FMIS_PopUpMenuConsume OnConsume;

	int32 GetSplitAmount() const;
	void CollapseSplitButton() const;
	void CollapseConsumeButton() const;
	void SetSliderParams(const float Max, const float Value) const;
	FVector2D GetBoxSize() const;
	void SetGridIndex(int32 Index) { GridIndex = Index; }
	int32 GetGridIndex() const { return GridIndex; }

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Split;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Drop;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Consume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Split;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SplitAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Root;

	int32 GridIndex{INDEX_NONE};

	UFUNCTION()
	void SplitButtonClicked();

	UFUNCTION()
	void DropButtonClicked();

	UFUNCTION()
	void ConsumeButtonClicked();

	UFUNCTION()
	void SliderValueChanged(float Value);
};
