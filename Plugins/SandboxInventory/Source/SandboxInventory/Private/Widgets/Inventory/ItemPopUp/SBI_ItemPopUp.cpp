// Copyright AmberAeolian. All Rights Reserved.

#include "Widgets/Inventory/ItemPopUp/SBI_ItemPopUp.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void USBI_ItemPopUp::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Split)
	{
		Button_Split->OnClicked.AddDynamic(this, &ThisClass::OnSplitClicked);
	}
	if (Button_Drop)
	{
		Button_Drop->OnClicked.AddDynamic(this, &ThisClass::OnDropClicked);
	}
	if (Button_Consume)
	{
		Button_Consume->OnClicked.AddDynamic(this, &ThisClass::OnConsumeClicked);
	}
	if (Slider_Split)
	{
		Slider_Split->OnValueChanged.AddDynamic(this, &ThisClass::OnSliderValueChanged);
	}
}

void USBI_ItemPopUp::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	RemoveFromParent();
}

void USBI_ItemPopUp::OnSplitClicked()
{
	if (OnSplit.IsBound())
	{
		OnSplit.Execute(GetSplitAmount(), GridIndex);
	}
	RemoveFromParent();
}

void USBI_ItemPopUp::OnDropClicked()
{
	if (OnDrop.IsBound())
	{
		OnDrop.Execute(GridIndex);
	}
	RemoveFromParent();
}

void USBI_ItemPopUp::OnConsumeClicked()
{
	if (OnConsume.IsBound())
	{
		OnConsume.Execute(GridIndex);
	}
	RemoveFromParent();
}

void USBI_ItemPopUp::OnSliderValueChanged(float Value)
{
	if (Text_SplitAmount)
	{
		Text_SplitAmount->SetText(FText::AsNumber(FMath::FloorToInt(Value)));
	}
}

int32 USBI_ItemPopUp::GetSplitAmount() const
{
	if (Slider_Split)
	{
		return FMath::FloorToInt(Slider_Split->GetValue());
	}
	return 1;
}

void USBI_ItemPopUp::CollapseSplitButton() const
{
	if (Button_Split)
	{
		Button_Split->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Slider_Split)
	{
		Slider_Split->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_SplitAmount)
	{
		Text_SplitAmount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USBI_ItemPopUp::CollapseConsumeButton() const
{
	if (Button_Consume)
	{
		Button_Consume->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USBI_ItemPopUp::SetSliderParams(const float Max, const float Value) const
{
	if (Slider_Split)
	{
		Slider_Split->SetMaxValue(Max);
		Slider_Split->SetValue(Value);
	}
	if (Text_SplitAmount)
	{
		Text_SplitAmount->SetText(FText::AsNumber(FMath::FloorToInt(Value)));
	}
}

FVector2D USBI_ItemPopUp::GetBoxSize() const
{
	if (SizeBox_Root)
	{
		return FVector2D(SizeBox_Root->GetWidthOverride(), SizeBox_Root->GetHeightOverride());
	}
	return FVector2D(100.f, 100.f);
}