#include "Items/Fragments/OldMIS_ItemFragment.h"

#include "DH_DebugFunctionLibrary.h"
#include "EquipmentManagement/EquipActor/OldMIS_EquipActor.h"
#include "Widgets/Composite/OldMIS_CompositeBase.h"
#include "Widgets/Composite/OldMIS_Leaf_Image.h"
#include "Widgets/Composite/OldMIS_Leaf_LabeledValue.h"
#include "Widgets/Composite/OldMIS_Leaf_Text.h"
#include "Windows/WindowsApplication.h"

void FOldMIS_InventoryItemFragment::Assimilate(UOldMIS_CompositeBase* Composite) const
{
	// 检查 Composite 的 FragmentTag 是否匹配此 Fragment 的标签
	if (!MatchesWidgetTag(Composite)) { return; }
	// 不匹配则保持折叠,匹配则展开该控件
	DH_SCREEN(2.f, DHColors::LightRed, "Tag匹配");
	Composite->Expand();
}

bool FOldMIS_InventoryItemFragment::MatchesWidgetTag(const UOldMIS_CompositeBase* Composite) const
{
	return Composite->GetFragmentTag().MatchesTagExact(GetFragmentTag());
}

void FOldMIS_ImageFragment::Assimilate(UOldMIS_CompositeBase* Composite) const
{
	// 先调用父类展开/折叠逻辑
	FOldMIS_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;

	// 将图片纹理和尺寸注入到 Leaf_Image 控件
	UOldMIS_Leaf_Image* Image = Cast<UOldMIS_Leaf_Image>(Composite);
	if (!IsValid(Image)) return;

	Image->SetImage(Icon);
	Image->SetBoxSize(IconDimensions);
	Image->SetImageSize(IconDimensions);
}

void FOldMIS_TextFragment::Assimilate(UOldMIS_CompositeBase* Composite) const
{
	FOldMIS_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;

	UOldMIS_Leaf_Text* LeafText = Cast<UOldMIS_Leaf_Text>(Composite);
	if (!IsValid(LeafText)) return;

	LeafText->SetText(FragmentText);
}

void FOldMIS_LabeledNumberFragment::Assimilate(UOldMIS_CompositeBase* Composite) const
{
	FOldMIS_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;

	UOldMIS_Leaf_LabeledValue* LabeledValue = Cast<UOldMIS_Leaf_LabeledValue>(Composite);
	if (!IsValid(LabeledValue)) return;

	// 设置标签文本 (如 "力量")
	LabeledValue->SetText_Label(Text_Label, bCollapseLabel);

	// 格式化数值并设置
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = MinFractionalDigits;
	Options.MaximumFractionalDigits = MaxFractionalDigits;
	LabeledValue->SetText_Value(FText::AsNumber(Value, &Options), bCollapseValue);
}

void FOldMIS_LabeledNumberFragment::Manifest()
{
	FOldMIS_InventoryItemFragment::Manifest();

	// 仅在首次创建时随机化数值 (后续网络复制保持一致性)
	if (bRandomizeOnManifest)
	{
		Value = FMath::FRandRange(Min, Max);
	}
	bRandomizeOnManifest = false;
}

// ===================== 消耗品 =====================

void FOldMIS_ConsumableFragment::OnConsume(APlayerController* PC)
{
	// 遍历所有消耗效果修饰器,依次执行
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.OnConsume(PC);
	}
}

void FOldMIS_ConsumableFragment::Assimilate(UOldMIS_CompositeBase* Composite) const
{
	FOldMIS_InventoryItemFragment::Assimilate(Composite);
	// 将每个消耗效果的数据注入到对应的 Composite 子控件
	for (const auto& Modifier : ConsumeModifiers)
	{
		const auto& ModRef = Modifier.Get();
		ModRef.Assimilate(Composite);
	}
}

void FOldMIS_ConsumableFragment::Manifest()
{
	FOldMIS_InventoryItemFragment::Manifest();
	// 初始化所有消耗效果 (如随机化生命恢复量)
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.Manifest();
	}
}

void FOldMIS_HealthPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Health Potion consumed! Healing by: %f"),
			GetValue()));
}

void FOldMIS_ManaPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Blue,
		FString::Printf(TEXT("Mana Potion consumed! Mana replenished by: %f"),
			GetValue()));
}

// ===================== 装备效果 =====================

void FOldMIS_StrengthModifier::OnEquip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Strength increased by: %f"),
			GetValue()));
}

void FOldMIS_StrengthModifier::OnUnequip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Item unequipped. Strength decreased by: %f"),
			GetValue()));
}

void FOldMIS_ArmorModifier::OnEquip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Item equipped. Armor increased by: %f"),
			GetValue()));
}

void FOldMIS_ArmorModifier::OnUnequip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Item unequipped. Armor decreased by: %f"),
			GetValue()));
}

void FOldMIS_DamageModifier::OnEquip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("Item equipped. Damage increased by: %f"),
			GetValue()));
}

void FOldMIS_DamageModifier::OnUnequip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Item unequipped. Damage decreased by: %f"),
			GetValue()));
}

// ===================== 装备片段 =====================

void FOldMIS_EquipmentFragment::OnEquip(APlayerController* PC)
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

void FOldMIS_EquipmentFragment::OnUnequip(APlayerController* PC)
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

void FOldMIS_EquipmentFragment::Assimilate(UOldMIS_CompositeBase* Composite) const
{
	FOldMIS_InventoryItemFragment::Assimilate(Composite);
	// 将每个装备效果的数据注入到对应的 Composite 子控件
	for (const auto& Modifier : EquipModifiers)
	{
		const auto& ModRef = Modifier.Get();
		ModRef.Assimilate(Composite);
	}
}

void FOldMIS_EquipmentFragment::Manifest()
{
	FOldMIS_InventoryItemFragment::Manifest();
	// 初始化所有装备效果 (如随机化属性加成值)
	for (auto& Modifier : EquipModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.Manifest();
	}
}

AOldMIS_EquipActor* FOldMIS_EquipmentFragment::SpawnAttachedActor(USkeletalMeshComponent* AttachMesh) const
{
	DH_PRINT(EDH_Output::Both, 4.f, DHColors::Cyan,
		"[装备链路-Fragment] >>> SpawnAttachedActor | AttachMesh=%s | EquipActorClass=%s | Socket=%s | World=%s",
		IsValid(AttachMesh) ? *AttachMesh->GetName() : TEXT("空"),
		EquipActorClass ? *EquipActorClass->GetName() : TEXT("空"),
		*SocketAttachPoint.ToString(),
		IsValid(AttachMesh) && AttachMesh->GetWorld() ? *AttachMesh->GetWorld()->GetName() : TEXT("空"));

	if (!IsValid(AttachMesh) || !EquipActorClass)
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-Fragment] SpawnAttachedActor 退出: AttachMesh=%d | EquipActorClass=%d",
			IsValid(AttachMesh), EquipActorClass != nullptr);
		return nullptr;
	}

	// 在世界中生成装备 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AttachMesh->GetOwner();
	AOldMIS_EquipActor* SpawnedActor = AttachMesh->GetWorld()->SpawnActor<AOldMIS_EquipActor>(EquipActorClass, SpawnParams);

	if (IsValid(SpawnedActor))
	{
		// 附着到骨骼网格体的指定槽位 (如 "hand_r" 右手)
		SpawnedActor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketAttachPoint);
		DH_PRINT(EDH_Output::Both, 4.f, FLinearColor::Green,
			"[装备链路-Fragment] SpawnAttachedActor: 生成并附着成功! | Actor=%s | Socket=%s",
			*SpawnedActor->GetName(), *SocketAttachPoint.ToString());
	}
	else
	{
		DH_PRINT(EDH_Output::Both, 2.f, FLinearColor::Red,
			"[装备链路-Fragment] SpawnAttachedActor: SpawnActor失败! Class=%s",
			*EquipActorClass->GetName());
	}

	return SpawnedActor;
}

void FOldMIS_EquipmentFragment::DestroyAttachedActor() const
{
	// 销毁已生成的装备 3D Actor
	if (EquippedActor.IsValid())
	{
		EquippedActor->Destroy();
	}
}

void FOldMIS_EquipmentFragment::SetEquippedActor(AOldMIS_EquipActor* EquipActor)
{
	EquippedActor = EquipActor;
}
