# DebugHelper 插件总结文档

## 一、插件概述

| 属性 | 值 |
|---|---|
| 名称 | DebugHelper |
| 描述 | 封装方便调试的函数 |
| 版本 | 1.0 |
| 模块名 | DebugHelper |
| 加载阶段 | Default (Runtime) |
| 类前缀 | `DH_` |
| 日志分类 | `LogDH` |
| 依赖 | 仅 `Core`、`Engine`（UE5 内置模块，无自定义插件依赖） |

**定位**: 轻量级统一调试输出工具，封装 UE_LOG 和屏幕消息，通过全局开关一键控制。

---

## 二、架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                    DebugHelper 插件                           │
│                                                               │
│  Source/DebugHelper/                                          │
│  ├── Public/                                                  │
│  │   ├── DH_DebugFunctionLibrary.h  (核心: 宏 + 枚举 + 颜色)   │
│  │   ├── DH_DebugSettings.h        (全局开关: DH_ENABLED)     │
│  │   └── DebugHelper.h             (模块入口)                  │
│  ├── Private/                                                 │
│  │   ├── DH_DebugFunctionLibrary.cpp  (实现)                  │
│  │   └── DebugHelper.cpp             (StartupModule/Shutdown) │
│  └── DebugHelper.Build.cs                                     │
│                                                               │
│  无 Content 资源、无 UI、无依赖其他插件                          │
└─────────────────────────────────────────────────────────────┘
```

---

## 三、核心机制

### 3.1 全局开关机制

```
┌──────────────────────────────────────────────────────────────┐
│                    DH_ENABLED 宏判断流程                       │
│                                                               │
│  #if UE_BUILD_SHIPPING                                        │
│      #define DH_ENABLED (false)    ← Shipping 编译时硬编码     │
│  #else                                                        │
│      #define DH_ENABLED (DH::IsDebugEnabled())                │
│          ↓                                                    │
│      DH::IsDebugEnabled()                                     │
│          ↓                                                    │
│      CVarDHEnable.GetValueOnAnyThread()                       │
│          ↓                                                    │
│      TAutoConsoleVariable<bool> CVarDHEnable                  │
│      (TEXT("debug.helper.enable"), false, ...)                │
│          ↓                                                    │
│      默认值: false (关闭)                                      │
│      控制台: debug.helper.enable 1  (开启)                     │
│      配置文件: DefaultEngine.ini 中写入                        │
└──────────────────────────────────────────────────────────────┘
```

| 环境 | DH_ENABLED 值 | 说明 |
|---|---|---|
| 编辑器 | `DH::IsDebugEnabled()` | 默认 false，可通过控制台/配置文件开启 |
| 打包开发版 | `DH::IsDebugEnabled()` | 默认 false，需在 DefaultEngine.ini 中配置 |
| Shipping | `false` (硬编码) | 所有宏展开为 `(void)0`，编译时完全消除 |

### 3.2 宏体系

```cpp
// ─── 仅日志 ───
#define DH_LOG(Format, ...)
    ↓
    do { if (DH_ENABLED) { UE_LOG(LogDH, Log, TEXT(Format), ##__VA_ARGS__); } } while(0)

// ─── 仅日志 (警告级别) ───
#define DH_LOG_WARN(Format, ...)
    ↓
    do { if (DH_ENABLED) { UE_LOG(LogDH, Warning, TEXT(Format), ##__VA_ARGS__); } } while(0)

// ─── 仅日志 (错误级别) ───
#define DH_LOG_ERR(Format, ...)
    ↓
    do { if (DH_ENABLED) { UE_LOG(LogDH, Error, TEXT(Format), ##__VA_ARGS__); } } while(0)

// ─── 仅屏幕 ───
#define DH_SCREEN(Duration, Color, Format, ...)
    ↓
    do { if (DH_ENABLED && GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, Duration, Color.ToFColor(false),
            FString::Printf(TEXT(Format), ##__VA_ARGS__));
    } } while(0)

// ─── 日志 + 屏幕 (双通道) ───
#define DH_PRINT(Output, Duration, Color, Format, ...)
    ↓
    do { if (DH_ENABLED) {
        UDH_DebugFunctionLibrary::PrintInternal(Output, Color, Duration,
            FString::Printf(TEXT(Format), ##__VA_ARGS__));
    } } while(0)
```

### 3.3 输出枚举

```cpp
UENUM(BlueprintType)
enum class EDH_Output : uint8
{
    Log     UMETA(DisplayName = "仅日志"),    // 仅 UE_LOG 输出到 Output Log
    Screen  UMETA(DisplayName = "仅屏幕"),    // 仅屏幕左上角浮窗
    Both    UMETA(DisplayName = "日志+屏幕"), // 双通道输出
};
```

### 3.4 PrintInternal 实现

```cpp
void UDH_DebugFunctionLibrary::PrintInternal(EDH_Output Output, FLinearColor Color,
    float Duration, const FString& Message)
{
    // 日志通道
    if (Output == EDH_Output::Log || Output == EDH_Output::Both)
    {
        UE_LOG(LogDH, Log, TEXT("%s"), *Message);
    }

    // 屏幕通道
    if ((Output == EDH_Output::Screen || Output == EDH_Output::Both) && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, Duration, Color.ToFColor(false), Message);
    }
}
```

### 3.5 颜色常量

`DHColors` 命名空间内置 20+ 种颜色常量:

| 类别 | 颜色 |
|---|---|
| 基本色 | White, Black, Red, Green, Blue |
| 扩展色 | Cyan, Magenta, Yellow, Orange, Pink, Purple |
| 浅色系 | LightRed, LightGreen, LightBlue, LightCyan, LightYellow, LightGray |
| 深色系 | DarkRed, DarkGreen, DarkBlue, DarkCyan, DarkMagenta, DarkYellow, DarkGray |
| 命名色 | Lime, Olive, Teal, Navy, Maroon, Silver, Gold, Coral, SkyBlue, Salmon |

---

## 四、使用方式

### 4.1 C++ 集成

```cpp
// 1. Build.cs 中添加依赖
PublicDependencyModuleNames.Add("DebugHelper");

// 2. 引入头文件
#include "DH_DebugFunctionLibrary.h"

// 3. 使用宏
DH_LOG("[背包] 当前数量=%d", Count);
DH_LOG_WARN("[背包] 库存已满!");
DH_LOG_ERR("[背包] 空指针异常!");

DH_SCREEN(2.f, FLinearColor::Green, "[背包] 物品拾取成功");

DH_PRINT(EDH_Output::Both, 3.f, DHColors::Magenta,
    "[背包] 堆叠已有物品 | 填充=%d | 剩余=%d", Amount, Remainder);
```

### 4.2 蓝图调用

搜索节点: `Print` (类别: `DebugHelper`)

| 参数 | 类型 | 说明 |
|---|---|---|
| WorldContextObject | Object | 传入 `self` |
| Message | String | 输出文本 |
| Output | EDH_Output | Log / Screen / Both |
| Color | LinearColor | 屏幕颜色 |
| Duration | float | 屏幕显示时长（秒） |

---

## 五、设计缺陷 (对标新版需求)

| 缺陷 | 描述 |
|---|---|
| 无分系统控制 | 只有全局开关，无法单独开关某个系统（如只开 Inventory 调试，关 UI 调试） |
| 无默认输出设置 | `DH_PRINT` 必须显式传入 `EDH_Output`，无默认值 |
| 颜色命名空间 | `DHColors` 命名空间在头文件中定义，跨编译单元可能有 ODR 风险 |
| 宏命名无前缀 | `DH_LOG` 与 UE 内置 `UE_LOG` 易混淆 |
| 无编译期开关 | 无法在编译选项中按模块选择性启用/禁用 |
| 使用 `constexpr` 在头文件中 | UE 对 `constexpr` 支持有版本差异风险 |