// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DS_DebugSettings.h"
#include "DS_DebugFunctionLibrary.generated.h"

// ============================================================================
// 输出模式枚举
// ============================================================================

UENUM(BlueprintType)
enum class EDS_Output : uint8
{
	LogOnly     UMETA(DisplayName = "仅日志"),
	ScreenOnly  UMETA(DisplayName = "仅屏幕"),
	Both        UMETA(DisplayName = "日志+屏幕"),
};

// ============================================================================
// 函数库
// ============================================================================

UCLASS()
class DEBUGSYSTEM_API UDS_DebugFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 蓝图调用入口 (默认 Both 输出) */
	UFUNCTION(BlueprintCallable, Category = "DebugSystem",
		meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, AutoCreateRefTerm = "Color"))
	static void Print(
		const UObject* WorldContextObject,
		EDS_System System,
		const FString& Message,
		FLinearColor Color = FLinearColor::White,
		float Duration = 2.f);

	/** 蓝图调用入口 (指定输出模式) */
	UFUNCTION(BlueprintCallable, Category = "DebugSystem",
		meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, AutoCreateRefTerm = "Color"))
	static void PrintEx(
		const UObject* WorldContextObject,
		EDS_System System,
		EDS_Output Output,
		const FString& Message,
		FLinearColor Color = FLinearColor::White,
		float Duration = 2.f);

	/** 内部实现 - 被宏调用 */
	static void PrintInternal(EDS_Output Output, FLinearColor Color, float Duration, const FString& Message);
};

// ============================================================================
// 颜色常量
// ============================================================================

namespace DSColors
{
	// 基本色
	inline const FLinearColor White(1.f, 1.f, 1.f);
	inline const FLinearColor Black(0.f, 0.f, 0.f);
	inline const FLinearColor Red(1.f, 0.f, 0.f);
	inline const FLinearColor Green(0.f, 1.f, 0.f);
	inline const FLinearColor Blue(0.f, 0.f, 1.f);

	// 扩展色
	inline const FLinearColor Cyan(0.f, 1.f, 1.f);
	inline const FLinearColor Magenta(1.f, 0.f, 1.f);
	inline const FLinearColor Yellow(1.f, 1.f, 0.f);
	inline const FLinearColor Orange(1.f, 0.5f, 0.f);
	inline const FLinearColor Pink(1.f, 0.4f, 0.7f);
	inline const FLinearColor Purple(0.5f, 0.f, 0.5f);

	// 浅色系
	inline const FLinearColor LightRed(1.f, 0.5f, 0.5f);
	inline const FLinearColor LightGreen(0.5f, 1.f, 0.5f);
	inline const FLinearColor LightBlue(0.5f, 0.5f, 1.f);
	inline const FLinearColor LightCyan(0.5f, 1.f, 1.f);
	inline const FLinearColor LightYellow(1.f, 1.f, 0.5f);
	inline const FLinearColor LightGray(0.75f, 0.75f, 0.75f);

	// 深色系
	inline const FLinearColor DarkRed(0.5f, 0.f, 0.f);
	inline const FLinearColor DarkGreen(0.f, 0.5f, 0.f);
	inline const FLinearColor DarkBlue(0.f, 0.f, 0.5f);
	inline const FLinearColor DarkCyan(0.f, 0.5f, 0.5f);
	inline const FLinearColor DarkMagenta(0.5f, 0.f, 0.5f);
	inline const FLinearColor DarkYellow(0.5f, 0.5f, 0.f);
	inline const FLinearColor DarkGray(0.25f, 0.25f, 0.25f);

	// 命名色
	inline const FLinearColor Lime(0.f, 1.f, 0.2f);
	inline const FLinearColor Olive(0.5f, 0.5f, 0.f);
	inline const FLinearColor Teal(0.f, 0.5f, 0.5f);
	inline const FLinearColor Navy(0.f, 0.f, 0.5f);
	inline const FLinearColor Maroon(0.5f, 0.f, 0.f);
	inline const FLinearColor Silver(0.75f, 0.75f, 0.75f);
	inline const FLinearColor Gold(1.f, 0.84f, 0.f);
	inline const FLinearColor Coral(1.f, 0.5f, 0.31f);
	inline const FLinearColor SkyBlue(0.53f, 0.8f, 1.f);
	inline const FLinearColor Salmon(1.f, 0.55f, 0.41f);

	// 语义色
	inline const FLinearColor Info(0.f, 1.f, 1.f);
	inline const FLinearColor Success(0.f, 1.f, 0.f);
	inline const FLinearColor Warning(1.f, 1.f, 0.f);
	inline const FLinearColor Error(1.f, 0.f, 0.f);
}

// ============================================================================
// 宏体系
// ============================================================================

//
// 仅日志 (Log)
//
#define DS_LOG(System, Format, ...) \
	do { if (DS_ENABLED(System)) { \
		UE_LOG(LogDS, Log, TEXT("[%s] ") TEXT(Format), TEXT(#System), ##__VA_ARGS__); \
	} } while(0)

//
// 仅日志 (Warning)
//
#define DS_LOG_WARN(System, Format, ...) \
	do { if (DS_ENABLED(System)) { \
		UE_LOG(LogDS, Warning, TEXT("[%s] ") TEXT(Format), TEXT(#System), ##__VA_ARGS__); \
	} } while(0)

//
// 仅日志 (Error)
//
#define DS_LOG_ERR(System, Format, ...) \
	do { if (DS_ENABLED(System)) { \
		UE_LOG(LogDS, Error, TEXT("[%s] ") TEXT(Format), TEXT(#System), ##__VA_ARGS__); \
	} } while(0)

//
// 仅屏幕 (Screen)
//
#define DS_SCREEN(System, Duration, Color, Format, ...) \
	do { if (DS_ENABLED(System)) { \
		UDS_DebugFunctionLibrary::PrintInternal(EDS_Output::ScreenOnly, Color, Duration, \
			FString::Printf(TEXT("[%s] ") TEXT(Format), TEXT(#System), ##__VA_ARGS__)); \
	} } while(0)

//
// 日志 + 屏幕 (Both, 默认)
//
#define DS_PRINT(System, Duration, Color, Format, ...) \
	do { if (DS_ENABLED(System)) { \
		UDS_DebugFunctionLibrary::PrintInternal(EDS_Output::Both, Color, Duration, \
			FString::Printf(TEXT("[%s] ") TEXT(Format), TEXT(#System), ##__VA_ARGS__)); \
	} } while(0)

//
// 指定输出模式
//
#define DS_PRINT_EX(System, Output, Duration, Color, Format, ...) \
	do { if (DS_ENABLED(System)) { \
		UDS_DebugFunctionLibrary::PrintInternal(Output, Color, Duration, \
			FString::Printf(TEXT("[%s] ") TEXT(Format), TEXT(#System), ##__VA_ARGS__)); \
	} } while(0)