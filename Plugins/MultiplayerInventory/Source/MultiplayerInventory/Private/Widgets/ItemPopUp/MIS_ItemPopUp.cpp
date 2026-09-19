#include "Widgets/ItemPopUp/MIS_ItemPopUp.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "MIS_MessageKeys.h"

void UMIS_ItemPopUp::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Split->OnClicked.AddDynamic(this, &ThisClass::SplitButtonClicked);
	Button_Drop->OnClicked.AddDynamic(this, &ThisClass::DropButtonClicked);
	Button_Consume->OnClicked.AddDynamic(this, &ThisClass::ConsumeButtonClicked);
	Slider_Split->OnValueChanged.AddDynamic(this, &ThisClass::SliderValueChanged);
}

void UMIS_ItemPopUp::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	RemoveFromParent();
}

int32 UMIS_ItemPopUp::GetSplitAmount() const
{
	return FMath::FloorToInt(Slider_Split->GetValue());
}

void UMIS_ItemPopUp::CollapseSplitButton() const
{
	Button_Split->SetVisibility(ESlateVisibility::Collapsed);
	Slider_Split->SetVisibility(ESlateVisibility::Collapsed);
	Text_SplitAmount->SetVisibility(ESlateVisibility::Collapsed);
}

void UMIS_ItemPopUp::CollapseConsumeButton() const
{
	Button_Consume->SetVisibility(ESlateVisibility::Collapsed);
}

void UMIS_ItemPopUp::SetSliderParams(const float Max, const float Value) const
{
	Slider_Split->SetMaxValue(Max);
	Slider_Split->SetMinValue(1);
	Slider_Split->SetValue(Value);
	Text_SplitAmount->SetText(FText::AsNumber(FMath::FloorToInt(Value)));
}

FVector2D UMIS_ItemPopUp::GetBoxSize() const
{
	return SizeBox_Root->GetWidthOverride() > 0.f
		? FVector2D(SizeBox_Root->GetWidthOverride(), SizeBox_Root->GetHeightOverride())
		: FVector2D(150.f, 120.f);
}

void UMIS_ItemPopUp::SplitButtonClicked()
{
	// [解耦重构] 广播消息后自行关闭。原先靠委托返回值决定是否关闭, 现在不再依赖调用方是否绑定。
	MIS::Emit(MSGKEY(MIS_UI_POPUP_SPLIT), GMP::FSigSource(this), GridIndex, GetSplitAmount());
	RemoveFromParent();
}

void UMIS_ItemPopUp::DropButtonClicked()
{
	MIS::Emit(MSGKEY(MIS_UI_POPUP_DROP), GMP::FSigSource(this), GridIndex);
	RemoveFromParent();
}

void UMIS_ItemPopUp::ConsumeButtonClicked()
{
	MIS::Emit(MSGKEY(MIS_UI_POPUP_CONSUME), GMP::FSigSource(this), GridIndex);
	RemoveFromParent();
}

void UMIS_ItemPopUp::SliderValueChanged(float Value)
{
	Text_SplitAmount->SetText(FText::AsNumber(FMath::FloorToInt(Value)));
}
