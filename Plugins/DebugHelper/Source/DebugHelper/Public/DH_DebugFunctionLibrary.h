#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DH_DebugSettings.h"
#include "DH_DebugFunctionLibrary.generated.h"

DEBUGHELPER_API DECLARE_LOG_CATEGORY_EXTERN(LogDH, Log, All);

UENUM(BlueprintType)
enum class EDH_Output : uint8
{
	Log     UMETA(DisplayName = "仅日志"),
	Screen  UMETA(DisplayName = "仅屏幕"),
	Both    UMETA(DisplayName = "日志+屏幕"),
};

UCLASS()
class DEBUGHELPER_API UDH_DebugFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DebugHelper", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static void Print(const UObject* WorldContextObject, const FString& Message,
		EDH_Output Output = EDH_Output::Both, FLinearColor Color = FLinearColor::White, float Duration = 2.f);

	static void PrintInternal(EDH_Output Output, FLinearColor Color, float Duration, const FString& Message);
};

// ---- 颜色常量 (FLinearColor 原生不含 Cyan/Magenta/Orange 等) ----
namespace DHColors
{
    // 基本颜色
    inline constexpr FLinearColor White(1.f, 1.f, 1.f);           // 白色
    inline constexpr FLinearColor Black(0.f, 0.f, 0.f);           // 黑色
    inline constexpr FLinearColor Red(1.f, 0.f, 0.f);             // 红色
    inline constexpr FLinearColor Green(0.f, 1.f, 0.f);           // 绿色
    inline constexpr FLinearColor Blue(0.f, 0.f, 1.f);            // 蓝色
    
    // 扩展颜色 (原生不含)
    inline constexpr FLinearColor Cyan(0.f, 1.f, 1.f);            // 青色
    inline constexpr FLinearColor Magenta(1.f, 0.f, 1.f);         // 品红/洋红
    inline constexpr FLinearColor Yellow(1.f, 1.f, 0.f);          // 黄色
    inline constexpr FLinearColor Orange(1.f, 0.5f, 0.f);         // 橙色
    inline constexpr FLinearColor Pink(1.f, 0.75f, 0.8f);         // 粉色
    inline constexpr FLinearColor Purple(0.5f, 0.f, 0.5f);        // 紫色
    
    // 浅色系
    inline constexpr FLinearColor LightRed(1.f, 0.5f, 0.5f);      // 浅红色
    inline constexpr FLinearColor LightGreen(0.5f, 1.f, 0.5f);    // 浅绿色
    inline constexpr FLinearColor LightBlue(0.5f, 0.5f, 1.f);     // 浅蓝色
    inline constexpr FLinearColor LightCyan(0.5f, 1.f, 1.f);      // 浅青色
    inline constexpr FLinearColor LightYellow(1.f, 1.f, 0.5f);    // 浅黄色
    inline constexpr FLinearColor LightGray(0.75f, 0.75f, 0.75f); // 浅灰色
    
    // 深色系
    inline constexpr FLinearColor DarkRed(0.5f, 0.f, 0.f);        // 暗红色
    inline constexpr FLinearColor DarkGreen(0.f, 0.5f, 0.f);      // 暗绿色
    inline constexpr FLinearColor DarkBlue(0.f, 0.f, 0.5f);       // 暗蓝色
    inline constexpr FLinearColor DarkCyan(0.f, 0.5f, 0.5f);      // 暗青色
    inline constexpr FLinearColor DarkMagenta(0.5f, 0.f, 0.5f);   // 暗品红
    inline constexpr FLinearColor DarkYellow(0.5f, 0.5f, 0.f);    // 暗黄色
    inline constexpr FLinearColor DarkGray(0.25f, 0.25f, 0.25f);  // 暗灰色
    
    // 常见命名颜色
    inline constexpr FLinearColor Lime(0.f, 1.f, 0.f);            // 亮绿色/柠檬绿
    inline constexpr FLinearColor Olive(0.5f, 0.5f, 0.f);         // 橄榄绿
    inline constexpr FLinearColor Teal(0.f, 0.5f, 0.5f);          // 蓝绿色/水鸭色
    inline constexpr FLinearColor Navy(0.f, 0.f, 0.5f);           // 海军蓝/藏青色
    inline constexpr FLinearColor Maroon(0.5f, 0.f, 0.f);         // 栗色/褐红色
    inline constexpr FLinearColor Silver(0.75f, 0.75f, 0.75f);    // 银色/银灰色
    inline constexpr FLinearColor Gold(1.f, 0.843f, 0.f);         // 金色
    inline constexpr FLinearColor Coral(1.f, 0.5f, 0.25f);        // 珊瑚色
    inline constexpr FLinearColor SkyBlue(0.529f, 0.808f, 0.922f);// 天蓝色
    inline constexpr FLinearColor Salmon(1.f, 0.549f, 0.412f);    // 鲑鱼色/橙红色
}

// ---- 宏 ----

#define DH_LOG(Format, ...) \
	do { if (DH_ENABLED) { UE_LOG(LogDH, Log, TEXT(Format), ##__VA_ARGS__); } } while(0)

#define DH_LOG_WARN(Format, ...) \
	do { if (DH_ENABLED) { UE_LOG(LogDH, Warning, TEXT(Format), ##__VA_ARGS__); } } while(0)

#define DH_LOG_ERR(Format, ...) \
	do { if (DH_ENABLED) { UE_LOG(LogDH, Error, TEXT(Format), ##__VA_ARGS__); } } while(0)

#define DH_SCREEN(Duration, Color, Format, ...) \
	do { if (DH_ENABLED && GEngine) { \
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color.ToFColor(false), FString::Printf(TEXT(Format), ##__VA_ARGS__)); \
	} } while(0)

#define DH_PRINT(Output, Duration, Color, Format, ...) \
	do { if (DH_ENABLED) { \
		UDH_DebugFunctionLibrary::PrintInternal(Output, Color, Duration, FString::Printf(TEXT(Format), ##__VA_ARGS__)); \
	} } while(0)
