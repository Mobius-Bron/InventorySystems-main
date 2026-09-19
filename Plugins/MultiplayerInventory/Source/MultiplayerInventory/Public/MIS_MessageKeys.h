#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GMPCore.h"

/**
 * 库存系统消息键 + 收发辅助 (GenericMessagePlugin)
 *
 * 设计约定:
 *  1. 数据层与 UI 之间的一切通信都通过 GMP 消息总线完成, 双方互不持有对方指针。
 *  2. "指定对象" 模式: 发送方与监听方都传入 FSigSource(InventoryComponent),
 *     保证同一进程内多个玩家/多套库存实例的消息互不串台。
 *  3. 键值必须用 #define 而非 constexpr 变量 —— MSGKEY 宏要求编译期字符串
 *     字面量作为非类型模板参数 (NTTP), 变量形式无法通过编译。
 *  4. GMP 的消息派发是**同步**的 (同一游戏线程、同一调用栈内完成), 因此
 *     消息化不会改变任何时序, 监听方在回调里可以直接拿到栈上实参的引用。
 *
 * 命名规范:
 *  MIS.Inv.*  —— 数据层 -> UI 的状态通知 (单向广播)
 *  MIS.Cmd.*  —— UI -> 数据层的意图命令 (UI 只发意图, 不关心谁处理)
 *  MIS.Ui.*   —— UI 内部控件之间的事件 (脱离父子引用)
 */

//======================================================================
// 数据层 -> UI : 状态通知
//======================================================================

/** 新增物品条目  参数: (UMIS_InventoryItem* Item) */
#define MIS_MSG_ITEM_ADDED          "MIS.Inv.ItemAdded"

/** 移除物品条目  参数: (UMIS_InventoryItem* Item) */
#define MIS_MSG_ITEM_REMOVED        "MIS.Inv.ItemRemoved"

/** 堆叠数量变化  参数: (const FMIS_SlotAvailabilityResult& Result) */
#define MIS_MSG_STACK_CHANGED       "MIS.Inv.StackChanged"

/** 背包已满      参数: 无 */
#define MIS_MSG_NO_ROOM             "MIS.Inv.NoRoom"

/** 物品被装备    参数: (UMIS_InventoryItem* Item) */
#define MIS_MSG_ITEM_EQUIPPED       "MIS.Inv.ItemEquipped"

/** 物品被卸下    参数: (UMIS_InventoryItem* Item) */
#define MIS_MSG_ITEM_UNEQUIPPED     "MIS.Inv.ItemUnequipped"

/** 背包界面开关  参数: (bool bOpen) */
#define MIS_MSG_MENU_TOGGLED        "MIS.Inv.MenuToggled"

/** 拾取提示变化  参数: (const FString& Message, bool bShow)
 *  两者合一, 替代原先 HUDWidget->ShowPickupMessage / HidePickupMessage 的直接调用。 */
#define MIS_MSG_PICKUP_PROMPT       "MIS.Inv.PickupPrompt"

//======================================================================
// UI -> 数据层 : 意图命令
//======================================================================

/** 丢弃物品      参数: (UMIS_InventoryItem* Item, int32 StackCount) */
#define MIS_CMD_DROP_ITEM           "MIS.Cmd.DropItem"

/** 消耗物品      参数: (UMIS_InventoryItem* Item) */
#define MIS_CMD_CONSUME_ITEM        "MIS.Cmd.ConsumeItem"

/** 装备槽点击    参数: (UMIS_InventoryItem* ItemToEquip, UMIS_InventoryItem* ItemToUnequip) */
#define MIS_CMD_EQUIP_SLOT          "MIS.Cmd.EquipSlotClicked"

//======================================================================
// UI 内部 : 控件事件 (发送方以自身作为 SigSource)
//======================================================================

/** 网格格子点击       参数: (int32 TileIndex, uint8 MouseButton) */
#define MIS_UI_GRID_SLOT_CLICKED    "MIS.Ui.GridSlotClicked"

/** 网格格子悬停/离开   参数: (int32 TileIndex) */
#define MIS_UI_GRID_SLOT_HOVERED    "MIS.Ui.GridSlotHovered"
#define MIS_UI_GRID_SLOT_UNHOVERED  "MIS.Ui.GridSlotUnhovered"

/** 已放置物品点击/悬停/离开  参数: (int32 GridIndex, uint8 MouseButton) / (int32 GridIndex) */
#define MIS_UI_SLOTTED_ITEM_CLICKED   "MIS.Ui.SlottedItemClicked"
#define MIS_UI_SLOTTED_ITEM_HOVERED   "MIS.Ui.SlottedItemHovered"
#define MIS_UI_SLOTTED_ITEM_UNHOVERED "MIS.Ui.SlottedItemUnhovered"

/** 已装备槽点击      参数: (UMIS_EquippedGridSlot* GridSlot, const FGameplayTag& EquipmentTypeTag) */
#define MIS_UI_EQUIPPED_GRID_SLOT_CLICKED "MIS.Ui.EquippedGridSlotClicked"

/** 已装备物品点击    参数: (UMIS_EquippedSlottedItem* SlottedItem) */
#define MIS_UI_EQUIPPED_SLOTTED_ITEM_CLICKED "MIS.Ui.EquippedSlottedItemClicked"

/** 弹出菜单操作      参数: (int32 Index, int32 Amount) */
#define MIS_UI_POPUP_SPLIT          "MIS.Ui.PopUpSplit"
#define MIS_UI_POPUP_DROP           "MIS.Ui.PopUpDrop"
#define MIS_UI_POPUP_CONSUME        "MIS.Ui.PopUpConsume"

/** 网格上物品悬停状态变化  参数: (UMIS_InventoryItem* Item, bool bHovered) */
#define MIS_UI_GRID_ITEM_HOVER_CHANGED "MIS.Ui.GridItemHoverChanged"

//======================================================================
// 收发辅助
//======================================================================

namespace MIS
{
	/**
	 * 鼠标键的消息化表示。
	 * FPointerEvent 内部持有 Slate 的共享引用, 不适合作为消息参数直接传递,
	 * 因此发送端只提取"哪个键被按下"这一必要信息。
	 */
	constexpr uint8 MouseButton_Left = 0;
	constexpr uint8 MouseButton_Right = 1;
	constexpr uint8 MouseButton_Other = 2;

	/** 从 Slate 鼠标事件提取按键编号。 */
	FORCEINLINE uint8 MouseButtonFromEvent(const FPointerEvent& MouseEvent)
	{
		const FKey& Button = MouseEvent.GetEffectingButton();
		if (Button == EKeys::LeftMouseButton) return MouseButton_Left;
		if (Button == EKeys::RightMouseButton) return MouseButton_Right;
		return MouseButton_Other;
	}

	/** 取消息中枢的管理器; 模块尚未就绪时返回 nullptr。 */
	FORCEINLINE UGMPManager* Manager()
	{
		return GMP::FMessageUtils::GetManager();
	}

	/**
	 * 发送一条库存消息。
	 * InSigSrc 建议统一传 FSigSource(InventoryComponent), 实现定向投递。
	 * 参数按地址零拷贝传递, 因此可以放心传 USTRUCT/引用 (派发是同步的)。
	 */
	template<typename... TArgs>
	FORCEINLINE void Emit(const GMP::FMSGKEYFind& Key, GMP::FSigSource InSigSrc, TArgs&&... Args)
	{
		if (UGMPManager* Mgr = Manager())
		{
			Mgr->GetHub().SendObjectMessage(Key, InSigSrc, Forward<TArgs>(Args)...);
		}
	}

	/**
	 * 监听一条库存消息。
	 * Listener 传 this 即可: GMP 内部保存弱引用, 对象销毁时自动解绑。
	 */
	template<typename T, typename F>
	FORCEINLINE FGMPKey Listen(const GMP::FMSGKEY& Key, GMP::FSigSource InSigSrc, T* Listener, F&& Func, GMP::FGMPListenOptions Options = {})
	{
		if (UGMPManager* Mgr = Manager())
		{
			return Mgr->GetHub().ListenObjectMessage(Key, InSigSrc, Listener, Forward<F>(Func), Options);
		}
		return FGMPKey(0);
	}

	/** 解绑某个监听者在指定键下的全部回调。 */
	FORCEINLINE void Unbind(const GMP::FMSGKEYFind& Key, const UObject* Listener)
	{
		if (UGMPManager* Mgr = Manager())
		{
			Mgr->GetHub().UnbindMessage(Key, Listener);
		}
	}

	/** 解绑某个监听者在"指定信号源"下的回调 (精确定位到某一个发送方)。 */
	FORCEINLINE void Unbind(const GMP::FMSGKEYFind& Key, const UObject* Listener, GMP::FSigSource InSigSrc)
	{
		if (UGMPManager* Mgr = Manager())
		{
			Mgr->GetHub().UnbindMessage(Key, Listener, InSigSrc);
		}
	}
}  // namespace MIS
