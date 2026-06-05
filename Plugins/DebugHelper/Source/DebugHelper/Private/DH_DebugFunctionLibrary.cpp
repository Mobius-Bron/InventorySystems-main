#include "DH_DebugFunctionLibrary.h"

#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogDH);

static TAutoConsoleVariable<bool> CVarDHEnable(
	TEXT("debug.helper.enable"),
	false,
	TEXT("0 = 关闭所有DebugHelper输出（默认）  1 = 开启\n")
	TEXT("控制台输入 debug.helper.enable 1 即可开启调试输出。"),
	ECVF_Default
);

namespace DH
{
	bool IsDebugEnabled()
	{
		return CVarDHEnable.GetValueOnAnyThread();
	}
}

void UDH_DebugFunctionLibrary::Print(const UObject* WorldContextObject, const FString& Message,
	EDH_Output Output, FLinearColor Color, float Duration)
{
	PrintInternal(Output, Color, Duration, Message);
}

void UDH_DebugFunctionLibrary::PrintInternal(EDH_Output Output, FLinearColor Color, float Duration, const FString& Message)
{
	if (Output == EDH_Output::Log || Output == EDH_Output::Both)
	{
		UE_LOG(LogDH, Log, TEXT("%s"), *Message);
	}

	if ((Output == EDH_Output::Screen || Output == EDH_Output::Both) && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color.ToFColor(false), Message);
	}
}
