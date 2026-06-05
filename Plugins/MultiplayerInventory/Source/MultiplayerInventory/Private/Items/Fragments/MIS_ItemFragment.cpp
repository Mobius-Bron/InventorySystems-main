#include "Items/Fragments/MIS_ItemFragment.h"

#include "DH_DebugFunctionLibrary.h"
#include "EquipmentManagement/EquipActor/MIS_EquipActor.h"
#include "Widgets/Composite/MIS_CompositeBase.h"
#include "Widgets/Composite/MIS_Leaf_Image.h"
#include "Widgets/Composite/MIS_Leaf_LabeledValue.h"
#include "Widgets/Composite/MIS_Leaf_Text.h"
#include "Windows/WindowsApplication.h"

void FMIS_InventoryItemFragment::Assimilate(UMIS_CompositeBase* Composite) const
{
	// 检查 Composite 的 FragmentTag 是否匹配此 Fragment 的标签
	if (!MatchesWidgetTag(Composite)) { return; }
	// 不匹配则保持折叠,匹配则展开该控件
	DH_SCREEN(2.f, DHColors::LightRed, "Tag匹配");
	Composite->Expand();
}

bool FMIS_InventoryItemFragment::MatchesWidgetTag(const UMIS_CompositeBase* Composite) const
{
	return Composite->GetFragmentTag().MatchesTagExact(GetFragmentTag());
}

void FMIS_ImageFragment::Assimilate(UMIS_CompositeBase* Composite) const
{
	// 先调用父类展开/折叠逻辑
	FMIS_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;

	// 将图片纹理和尺寸注入到 Leaf_Image 控件
	UMIS_Leaf_Image* Image = Cast<UMIS_Leaf_Image>(Composite);
	if (!IsValid(Image)) return;

	Image->SetImage(Icon);
	Image->SetBoxSize(IconDimensions);
	Image->SetImageSize(IconDimensions);
}

void FMIS_TextFragment::Assimilate(UMIS_CompositeBase* Composite) const
{
	FMIS_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;

	UMIS_Leaf_Text* LeafText = Cast<UMIS_Leaf_Text>(Composite);
	if (!IsValid(LeafText)) return;

	LeafText->SetText(FragmentText);
}

void FMIS_LabeledNumberFragment::Assimilate(UMIS_CompositeBase* Composite) const
{
	FMIS_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;

	UMIS_Leaf_LabeledValue* LabeledValue = Cast<UMIS_Leaf_LabeledValue>(Composite);
	if (!IsValid(LabeledValue)) return;

	// 设置标签文本 (如 "力量")
	LabeledValue->SetText_Label(Text_Label, bCollapseLabel);

	// 格式化数值并设置
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = MinFractionalDigits;
	Options.MaximumFractionalDigits = MaxFractionalDigits;
	LabeledValue->SetText_Value(FText::AsNumber(Value, &Options), bCollapseValue);
}

void FMIS_LabeledNumberFragment::Manifest()
{
	FMIS_InventoryItemFragment::Manifest();

	// 仅在首次创建时随机化数值 (后续网络复制保持一致性)
	if (bRandomizeOnManifest)
	{
		Value = FMath::FRandRange(Min, Max);
	}
	bRandomizeOnManifest = false;
}

// ===================== 消耗品 =====================

void FMIS_ConsumableFragment::OnConsume(APlayerController* PC)
{
	// 遍历所有消耗效果修饰器,依次执行
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.OnConsume(PC);
	}
}

void FMIS_ConsumableFragment::Assimilate(UMIS_CompositeBase* Composite) const
{
	FMIS_InventoryItemFragment::Assimilate(Composite);
	// 将每个消耗效果的数据注入到对应的 Composite 子控件
	for (const auto& Modifier : ConsumeModifiers)
	{
		const auto& ModRef = Modifier.Get();
		ModRef.Assimilate(Composite);
	}
}

void FMIS_ConsumableFragment::Manifest()
{
	FMIS_InventoryItemFragment::Manifest();
	// 初始化所有消耗效果 (如随机化生命恢复量)
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.Manifest();
	}
}

void FMIS_HealthPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Health Potion consumed! Healing by: %f"),
			GetValue()));
}

void FMIS_ManaPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Blue,
		FString::Printf(TEXT("Mana Potion consumed! Mana replenished by: %f"),
			GetValue()));
}

// ===================== 装备效果 =====================

void FMIS_StrengthModifier::OnEquip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Strength increased by: %f"),
			GetValue()));
}

void FMIS_StrengthModifier::OnUnequip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Item unequipped. Strength decreased by: %f"),
			GetValue()));
}

void FMIS_ArmorModifier::OnEquip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Item equipped. Armor increased by: %f"),
			GetValue()));
}

void FMIS_ArmorModifier::OnUnequip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Item unequipped. Armor decreased by: %f"),
			GetValue()));
}

void FMIS_DamageModifier::OnEquip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Item equipped. Damage increased by: %f"),
			GetValue()));
}

void FMIS_DamageModifier::OnUnequip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Item unequipped. Damage decreased by: %f"),
			GetValue()));
}

// ===================== 装备片段 =====================

void FMIS_EquipmentFragment::OnEquip(APlayerController* PC)
{
	// 防止重复装备
	if (bEquipped) return;
	bEquipped = true;

	// 遍历所有装备效果修饰器,依次执行 (如 +15 力量, +10 护甲)
	for (auto& Modifier : EquipModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.OnEquip(PC);
	}
}

void FMIS_EquipmentFragment::OnUnequip(APlayerController* PC)
{
	// 防止重复卸下
	if (!bEquipped) return;
	bEquipped = false;

	// 遍历所有装备效果修饰器,依次撤销 (如 -15 力量, -10 护甲)
	for (auto& Modifier : EquipModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.OnUnequip(PC);
	}
}

void FMIS_EquipmentFragment::Assimilate(UMIS_CompositeBase* Composite) const
{
	FMIS_InventoryItemFragment::Assimilate(Composite);
	// 将每个装备效果的数据注入到对应的 Composite 子控件
	for (const auto& Modifier : EquipModifiers)
	{
		const auto& ModRef = Modifier.Get();
		ModRef.Assimilate(Composite);
	}
}

void FMIS_EquipmentFragment::Manifest()
{
	FMIS_InventoryItemFragment::Manifest();
	// 初始化所有装备效果 (如随机化属性加成值)
	for (auto& Modifier : EquipModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.Manifest();
	}
}

AMIS_EquipActor* FMIS_EquipmentFragment::SpawnAttachedActor(USkeletalMeshComponent* AttachMesh) const
{
	if (!IsValid(AttachMesh) || !EquipActorClass) return nullptr;

	// 在世界中生成装备 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AttachMesh->GetOwner();
	AMIS_EquipActor* SpawnedActor = AttachMesh->GetWorld()->SpawnActor<AMIS_EquipActor>(EquipActorClass, SpawnParams);

	if (IsValid(SpawnedActor))
	{
		// 附着到骨骼网格体的指定槽位 (如 "hand_r" 右手)
		SpawnedActor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketAttachPoint);
	}

	return SpawnedActor;
}

void FMIS_EquipmentFragment::DestroyAttachedActor() const
{
	// 销毁已生成的装备 3D Actor
	if (EquippedActor.IsValid())
	{
		EquippedActor->Destroy();
	}
}

void FMIS_EquipmentFragment::SetEquippedActor(AMIS_EquipActor* EquipActor)
{
	EquippedActor = EquipActor;
}
