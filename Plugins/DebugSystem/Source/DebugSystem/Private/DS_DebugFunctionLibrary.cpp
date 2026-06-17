// Copyright AmberAeolian. All Rights Reserved.

#include "DS_DebugFunctionLibrary.h"
#include "DebugSystem.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogDS);

// ============================================================================
// 控制台变量
// ============================================================================

/**
 * 全局调试开关
 * 控制台: debug.system.enable 0/1
 * 配置文件: [/Script/DebugSystem.DebugSystem] 段下 debug.system.enable=1
 */
static TAutoConsoleVariable<bool> CVarDSEnable(
	TEXT("debug.system.enable"),
	false,
	TEXT("0 = 关闭所有 DebugSystem 输出\n")
	TEXT("1 = 开启 (需结合 debug.system.systems 控制具体哪些系统输出)"),
	ECVF_Default
);

/**
 * 系统位掩码开关
 * 控制台: debug.system.systems <位掩码>
 * 示例:
 *   debug.system.systems 0x00000001  → 仅 Inventory
 *   debug.system.systems 0x0000001F  → Inventory+Equipment+UI+Network+Interaction
 *   debug.system.systems 0xFFFFFFFF  → 全部系统
 *   debug.system.systems 0           → 等同于全局关闭
 */
static TAutoConsoleVariable<int32> CVarDSSystems(
	TEXT("debug.system.systems"),
	0xFFFFFFFF,
	TEXT("位掩码控制哪些系统输出调试信息\n")
	TEXT("  0x00000001 = Inventory\n")
	TEXT("  0x00000002 = Equipment\n")
	TEXT("  0x00000004 = UI\n")
	TEXT("  0x00000008 = Network\n")
	TEXT("  0x00000010 = Interaction\n")
	TEXT("  0x00000020 = Combat\n")
	TEXT("  0x00000040 = AI\n")
	TEXT("  0x00000080 = SaveLoad\n")
	TEXT("  0x00000100 = DataAsset\n")
	TEXT("  0x00000200 = Config\n")
	TEXT("  0xFFFFFFFF = 全部系统"),
	ECVF_Default
);

// ============================================================================
// 开关实现
// ============================================================================

namespace DS
{
	bool IsDebugEnabled()
	{
		return CVarDSEnable.GetValueOnAnyThread();
	}

	bool IsSystemEnabled(EDS_System System)
	{
		return (CVarDSSystems.GetValueOnAnyThread() & static_cast<uint32>(System)) != 0;
	}
}

// ============================================================================
// 蓝图函数实现
// ============================================================================

void UDS_DebugFunctionLibrary::Print(const UObject* WorldContextObject, EDS_System System,
	const FString& Message, FLinearColor Color, float Duration)
{
	if (!DS::IsDebugEnabled() || !DS::IsSystemEnabled(System))
	{
		return;
	}
	PrintInternal(EDS_Output::Both, Color, Duration,
		FString::Printf(TEXT("[%s] %s"), *StaticEnum<EDS_System>()->GetNameStringByValue(static_cast<uint32>(System)), *Message));
}

void UDS_DebugFunctionLibrary::PrintEx(const UObject* WorldContextObject, EDS_System System,
	EDS_Output Output, const FString& Message, FLinearColor Color, float Duration)
{
	if (!DS::IsDebugEnabled() || !DS::IsSystemEnabled(System))
	{
		return;
	}
	PrintInternal(Output, Color, Duration,
		FString::Printf(TEXT("[%s] %s"), *StaticEnum<EDS_System>()->GetNameStringByValue(static_cast<uint32>(System)), *Message));
}

void UDS_DebugFunctionLibrary::PrintInternal(EDS_Output Output, FLinearColor Color,
	float Duration, const FString& Message)
{
	// 日志通道
	if (Output == EDS_Output::LogOnly || Output == EDS_Output::Both)
	{
		UE_LOG(LogDS, Log, TEXT("%s"), *Message);
	}

	// 屏幕通道
	if ((Output == EDS_Output::ScreenOnly || Output == EDS_Output::Both) && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color.ToFColor(false), Message);
	}
}