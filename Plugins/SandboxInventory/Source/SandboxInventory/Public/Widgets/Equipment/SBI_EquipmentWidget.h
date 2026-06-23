// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "SBI_EquipmentWidget.generated.h"

class USBI_EquipmentSlot;
class USBI_InventoryComponent;
class USBI_InventoryGrid;
class UIC_InventoryItem;
class UCanvasPanel;
class UUniformGridPanel;
struct FPointerEvent;

/**
 * 装备界面 - 管理多个装备槽位
 * 拖拽物品到匹配的槽位即可装备, 点击已装备的槽位卸下
 */
UCLASS()
class SANDBOXINVENTORY_API USBI_EquipmentWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

public:
	/** 初始化 */
	void InitFromComponent(USBI_InventoryComponent* InInventoryComponent, USBI_InventoryGrid* InInventoryGrid);

	/** 打开/关闭 */
	void OpenEquipment();
	void CloseEquipment();

	/** 刷新所有装备槽位 */
	void RefreshEquipmentSlots();

	/** 尝试装备物品到槽位 */
	void TryEquipItem(UIC_InventoryItem* Item, const FGameplayTag& EquipmentTypeTag);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	/** 装备槽位列表 (在蓝图中通过 BindWidget 绑定) */
	UPROPERTY(meta = (BindWidget))
	TArray<TObjectPtr<USBI_EquipmentSlot>> EquipmentSlots;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	TSubclassOf<USBI_EquipmentSlot> EquipmentSlotClass;

	TWeakObjectPtr<USBI_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<USBI_InventoryGrid> InventoryGrid;

	UFUNCTION()
	void OnEquipmentSlotClicked(const FGameplayTag& EquipmentTypeTag, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnItemEquipped(UIC_InventoryItem* Item);

	UFUNCTION()
	void OnItemUnequipped(UIC_InventoryItem* Item);
};