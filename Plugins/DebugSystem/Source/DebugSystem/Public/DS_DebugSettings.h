// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Platform.h"

// ============================================================================
// 系统标识枚举 (位掩码)
// ============================================================================

UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDS_System : uint32
{
	None        = 0,

	// ─── 核心系统 ───
	Inventory   = 1 << 0,   // 0x00000001  库存系统
	Equipment   = 1 << 1,   // 0x00000002  装备系统
	UI          = 1 << 2,   // 0x00000004  用户界面
	Network     = 1 << 3,   // 0x00000008  网络复制

	// ─── 交互系统 ───
	Interaction = 1 << 4,   // 0x00000010  交互/拾取
	Combat      = 1 << 5,   // 0x00000020  战斗系统
	AI          = 1 << 6,   // 0x00000040  人工智能

	// ─── 数据系统 ───
	SaveLoad    = 1 << 7,   // 0x00000080  存档/读档
	DataAsset   = 1 << 8,   // 0x00000100  数据资产加载
	Config      = 1 << 9,   // 0x00000200  配置系统

	// ─── 扩展预留 ───
	Custom1     = 1u << 28,  // 0x10000000  自定义1
	Custom2     = 1u << 29,  // 0x20000000  自定义2
	Custom3     = 1u << 30,  // 0x40000000  自定义3
	Custom4     = 1u << 31,  // 0x80000000  自定义4

	All         = 0xFFFFFFFF, // 全部系统
};
ENUM_CLASS_FLAGS(EDS_System)

// ============================================================================
// 开关控制函数
// ============================================================================

namespace DS
{
	/** 全局调试开关 */
	DEBUGSYSTEM_API bool IsDebugEnabled();

	/** 检查指定系统是否启用 */
	DEBUGSYSTEM_API bool IsSystemEnabled(EDS_System System);
}

// ============================================================================
// 编译期消除: Shipping 构建时所有宏展开为 (void)0
// ============================================================================

#if UE_BUILD_SHIPPING
	#define DS_ENABLED(System) (false)
#else
	#define DS_ENABLED(System) \
		(DS::IsDebugEnabled() && DS::IsSystemEnabled(EDS_System::System))
#endif