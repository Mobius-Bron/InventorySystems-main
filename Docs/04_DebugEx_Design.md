# DebugSystem 插件设计文档

> **实际实现**: 插件名为 `DebugSystem`，模块名为 `DebugSystem`，前缀 `DS_`，日志分类 `LogDS`。
> 本文档为设计阶段的原始方案，命名以 `DebugEx`/`DX_` 为占位，实际代码已按 `DebugSystem`/`DS_` 实现。

## 一、插件概述

| 属性 | 设计名 | 实际实现 |
|---|---|---|
| 名称 | DebugEx | DebugSystem |
| 模块名 | DebugEx | DebugSystem |
| 类前缀 | `DX_` | `DS_` |
| 日志分类 | `LogDX` | `LogDS` |
| CVar 前缀 | `debug.ex.*` | `debug.system.*` |
| 描述 | 增强型调试输出系统，支持分系统开关和全局控制 |
| 版本 | 1.0 |
| 加载阶段 | Default (Runtime) |
| 依赖 | 仅 `Core`、`Engine`（UE5 内置模块，无自定义插件依赖） |

---

## 二、核心设计目标

| 目标 | 描述 |
|---|---|
| 全局开关 | 一个 CVar 控制所有调试输出的总开关 |
| 分系统开关 | 每个子系统独立可开关，通过位掩码实现 |
| 三种输出模式 | LogOnly / ScreenOnly / Both (默认 Both) |
| 编译期消除 | Shipping 构建时所有宏展开为 `(void)0`，零运行时开销 |
| 蓝图支持 | 提供 `Print` 蓝图节点，可在蓝图中选择系统和输出模式 |
| 零侵入 | 仅需 `#include "DX_DebugFunctionLibrary.h"` 即可使用 |
| 向后兼容 | 保留 DebugHelper 的所有宏风格，降低迁移成本 |

---

## 三、系统标识设计 (位掩码)

### 3.1 系统枚举

```cpp
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDX_System : uint32
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
    Custom1     = 1 << 28,  // 0x10000000  自定义1
    Custom2     = 1 << 29,  // 0x20000000  自定义2
    Custom3     = 1 << 30,  // 0x40000000  自定义3
    Custom4     = 1 << 31,  // 0x80000000  自定义4

    All         = 0xFFFFFFFF, // 全部系统
};
ENUM_CLASS_FLAGS(EDX_System)
```

### 3.2 位掩码工作原理

```
全局开关: debug.ex.enable = 1
系统掩码: debug.ex.systems = 0x0000001F  (Inventory + Equipment + UI + Network + Interaction)

判断是否输出:
    DX_ENABLED(System) = 全局开关 && (系统掩码 & System) != 0

示例:
    DX_ENABLED(Inventory)   = true  && (0x1F & 0x01) != 0 = true   ✓ 输出
    DX_ENABLED(Combat)      = true  && (0x1F & 0x20) != 0 = false  ✗ 不输出
    DX_ENABLED(Inventory)   = false && ...                 = false  ✗ 全局关闭
```

---

## 四、宏体系设计

### 4.1 宏定义

```cpp
// ─── 核心判断宏 ───
#if UE_BUILD_SHIPPING
    #define DX_ENABLED(System) (false)
#else
    #define DX_ENABLED(System) \
        (DX::IsDebugEnabled() && DX::IsSystemEnabled(EDX_System::System))
#endif

// ─── 仅日志 (Log) ───
#define DX_LOG(System, Format, ...) \
    do { if (DX_ENABLED(System)) { \
        UE_LOG(LogDX, Log, TEXT("[%s] ") TEXT(Format), \
            TEXT(#System), ##__VA_ARGS__); \
    } } while(0)

// ─── 仅日志 (Warning) ───
#define DX_LOG_WARN(System, Format, ...) \
    do { if (DX_ENABLED(System)) { \
        UE_LOG(LogDX, Warning, TEXT("[%s] ") TEXT(Format), \
            TEXT(#System), ##__VA_ARGS__); \
    } } while(0)

// ─── 仅日志 (Error) ───
#define DX_LOG_ERR(System, Format, ...) \
    do { if (DX_ENABLED(System)) { \
        UE_LOG(LogDX, Error, TEXT("[%s] ") TEXT(Format), \
            TEXT(#System), ##__VA_ARGS__); \
    } } while(0)

// ─── 仅屏幕 (Screen) ───
#define DX_SCREEN(System, Duration, Color, Format, ...) \
    do { if (DX_ENABLED(System) && GEngine) { \
        GEngine->AddOnScreenDebugMessage(-1, Duration, Color.ToFColor(false), \
            FString::Printf(TEXT("[%s] ") TEXT(Format), \
                TEXT(#System), ##__VA_ARGS__)); \
    } } while(0)

// ─── 日志 + 屏幕 (Both, 默认) ───
#define DX_PRINT(System, Duration, Color, Format, ...) \
    do { if (DX_ENABLED(System)) { \
        UDX_DebugFunctionLibrary::PrintInternal( \
            EDX_Output::Both, Color, Duration, \
            FString::Printf(TEXT("[%s] ") TEXT(Format), \
                TEXT(#System), ##__VA_ARGS__)); \
    } } while(0)

// ─── 指定输出模式 ───
#define DX_PRINT_EX(System, Output, Duration, Color, Format, ...) \
    do { if (DX_ENABLED(System)) { \
        UDX_DebugFunctionLibrary::PrintInternal( \
            Output, Color, Duration, \
            FString::Printf(TEXT("[%s] ") TEXT(Format), \
                TEXT(#System), ##__VA_ARGS__)); \
    } } while(0)
```

### 4.2 宏对比 (DebugHelper vs DebugEx)

| 功能 | DebugHelper | DebugEx |
|---|---|---|
| 日志 | `DH_LOG(Fmt, ...)` | `DX_LOG(System, Fmt, ...)` |
| 日志警告 | `DH_LOG_WARN(Fmt, ...)` | `DX_LOG_WARN(System, Fmt, ...)` |
| 日志错误 | `DH_LOG_ERR(Fmt, ...)` | `DX_LOG_ERR(System, Fmt, ...)` |
| 屏幕 | `DH_SCREEN(Dur, Color, Fmt, ...)` | `DX_SCREEN(System, Dur, Color, Fmt, ...)` |
| 双通道 | `DH_PRINT(Output, Dur, Color, Fmt, ...)` | `DX_PRINT(System, Dur, Color, Fmt, ...)` |
| 双通道(指定模式) | 无 | `DX_PRINT_EX(System, Output, Dur, Color, Fmt, ...)` |

**关键差异**: 
- DebugEx 所有宏都多了 `System` 参数，用于分系统控制
- `DX_PRINT` 默认输出模式为 `Both`，不需要显式传入 `EDX_Output`
- `DX_PRINT_EX` 允许显式指定输出模式（向后兼容 DebugHelper 的 `DH_PRINT`）

---

## 五、核心实现

### 5.1 文件结构

```
DebugEx/
├── DebugEx.uplugin
├── Source/DebugEx/
│   ├── DebugEx.Build.cs
│   ├── Public/
│   │   ├── DX_DebugFunctionLibrary.h    (宏 + 枚举 + 函数声明)
│   │   ├── DX_DebugSettings.h           (全局开关 + 系统开关)
│   │   └── DebugEx.h                    (模块入口)
│   └── Private/
│       ├── DX_DebugFunctionLibrary.cpp  (实现)
│       └── DebugEx.cpp                  (StartupModule/Shutdown)
```

### 5.2 DX_DebugSettings.h (开关控制)

```cpp
#pragma once

namespace DX
{
    /** 全局调试开关 */
    DEBUGEX_API bool IsDebugEnabled();

    /** 检查指定系统是否启用 */
    DEBUGEX_API bool IsSystemEnabled(EDX_System System);
}

#if UE_BUILD_SHIPPING
    #define DX_ENABLED(System) (false)
#else
    #define DX_ENABLED(System) \
        (DX::IsDebugEnabled() && DX::IsSystemEnabled(EDX_System::System))
#endif
```

### 5.3 DX_DebugFunctionLibrary.cpp (CVar 注册)

```cpp
#include "DX_DebugFunctionLibrary.h"

DEFINE_LOG_CATEGORY(LogDX);

// ─── 全局开关 ───
static TAutoConsoleVariable<bool> CVarDXEnable(
    TEXT("debug.ex.enable"),
    false,  // 默认关闭
    TEXT("0 = 关闭所有 DebugEx 输出  1 = 开启\n")
    TEXT("开启后结合 debug.ex.systems 控制具体哪些系统输出"),
    ECVF_Default
);

// ─── 系统位掩码开关 ───
static TAutoConsoleVariable<int32> CVarDXSystems(
    TEXT("debug.ex.systems"),
    0xFFFFFFFF,  // 默认全部开启（但需要全局开关也开启）
    TEXT("位掩码控制哪些系统输出调试信息\n")
    TEXT("示例:\n")
    TEXT("  debug.ex.systems 0x00000001  → 仅 Inventory\n")
    TEXT("  debug.ex.systems 0x0000001F  → Inventory+Equipment+UI+Network+Interaction\n")
    TEXT("  debug.ex.systems 0xFFFFFFFF  → 全部系统\n")
    TEXT("  debug.ex.systems 0           → 等同于全局关闭"),
    ECVF_Default
);

namespace DX
{
    bool IsDebugEnabled()
    {
        return CVarDXEnable.GetValueOnAnyThread();
    }

    bool IsSystemEnabled(EDX_System System)
    {
        return (CVarDXSystems.GetValueOnAnyThread() & static_cast<uint32>(System)) != 0;
    }
}
```

### 5.4 DX_DebugFunctionLibrary.h (输出函数)

```cpp
UENUM(BlueprintType)
enum class EDX_Output : uint8
{
    LogOnly     UMETA(DisplayName = "仅日志"),
    ScreenOnly  UMETA(DisplayName = "仅屏幕"),
    Both        UMETA(DisplayName = "日志+屏幕"),
};

UCLASS()
class DEBUGEX_API UDX_DebugFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 蓝图调用入口 (默认 Both 输出) */
    UFUNCTION(BlueprintCallable, Category = "DebugEx",
        meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
    static void Print(
        const UObject* WorldContextObject,
        EDX_System System,
        const FString& Message,
        FLinearColor Color = FLinearColor::White,
        float Duration = 2.f);

    /** 蓝图调用入口 (指定输出模式) */
    UFUNCTION(BlueprintCallable, Category = "DebugEx",
        meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
    static void PrintEx(
        const UObject* WorldContextObject,
        EDX_System System,
        EDX_Output Output,
        const FString& Message,
        FLinearColor Color = FLinearColor::White,
        float Duration = 2.f);

    /** 内部实现 - 被宏调用 */
    static void PrintInternal(
        EDX_Output Output,
        FLinearColor Color,
        float Duration,
        const FString& Message);
};
```

---

## 六、使用示例

### 6.1 C++ 使用

```cpp
#include "DX_DebugFunctionLibrary.h"

// ─── 库存系统调试 ───
DX_LOG(Inventory, "背包已打开, 物品数=%d", ItemCount);
DX_LOG_WARN(Inventory, "库存空间不足!");
DX_SCREEN(Inventory, 3.f, DXColors::Cyan, "物品拾取成功: %s", *ItemName);

// ─── 装备系统调试 ───
DX_PRINT(Equipment, 4.f, DXColors::Green, 
    "装备完成 | Actor=%s | TypeTag=%s", *ActorName, *TagName);

// ─── 网络调试 ───
DX_LOG(Network, "Server_AddNewItem | ItemActor=%s | StackCount=%d", *Name, Count);

// ─── 显式指定输出模式 ───
DX_PRINT_EX(Combat, EDX_Output::ScreenOnly, 2.f, DXColors::Red,
    "玩家受到伤害: %f", Damage);
```

### 6.2 蓝图使用

```
节点: Print
├── WorldContextObject: self
├── System: Inventory (下拉选择)
├── Message: "物品拾取成功"
├── Color: (R=0,G=1,B=0)
└── Duration: 2.0

节点: PrintEx
├── WorldContextObject: self
├── System: Equipment
├── Output: Both
├── Message: "装备完成"
├── Color: (R=0,G=1,B=0)
└── Duration: 4.0
```

### 6.3 控制台命令

```
debug.ex.enable 1                          # 开启全局调试
debug.ex.systems 0x0000001F                # 开启 Inventory+Equipment+UI+Network+Interaction
debug.ex.systems 0x00000002                # 仅开启 Equipment
debug.ex.systems 0                         # 关闭所有系统
debug.ex.enable 0                          # 全局关闭 (无视系统掩码)
```

---

## 七、输出格式

所有宏输出自动前缀系统名称，便于在 Output Log 中过滤:

```
// 输出示例:
LogDX: [Inventory] 背包已打开, 物品数=5
LogDX: Warning: [Inventory] 库存空间不足!
LogDX: Error: [Network] 服务器连接超时

// 屏幕输出:
[Inventory] 物品拾取成功: 铁剑
[Equipment] 装备完成 | Actor=EquipActor_3 | TypeTag=GameItems.Equipment.Weapons.Sword
```

---

## 八、编译期行为

| 构建配置 | DX_ENABLED(System) | 行为 |
|---|---|---|
| Development Editor | `IsDebugEnabled() && IsSystemEnabled(System)` | 运行时检查 |
| Development Game | `IsDebugEnabled() && IsSystemEnabled(System)` | 运行时检查 |
| Shipping | `false` (硬编码) | 所有宏展开为 `(void)0`，编译器优化后完全消除 |
| Test | `IsDebugEnabled() && IsSystemEnabled(System)` | 运行时检查 |

---

## 九、与 DebugHelper 的兼容迁移

### 迁移映射

| DebugHelper 写法 | DebugEx 等价写法 |
|---|---|
| `DH_LOG("背包物品=%d", n)` | `DX_LOG(Inventory, "背包物品=%d", n)` |
| `DH_LOG_WARN("库存满")` | `DX_LOG_WARN(Inventory, "库存满")` |
| `DH_SCREEN(2.f, Green, "拾取")` | `DX_SCREEN(Inventory, 2.f, Green, "拾取")` |
| `DH_PRINT(Both, 3.f, Magenta, "堆叠=%d", n)` | `DX_PRINT(Inventory, 3.f, Magenta, "堆叠=%d", n)` |

### 颜色常量

DebugEx 复用 DebugHelper 的 `DHColors` 命名空间中的颜色常量，并重命名为 `DXColors`:

```cpp
namespace DXColors
{
    // 与 DHColors 相同的颜色常量，增加:
    inline constexpr FLinearColor DefaultScreen(1.f, 1.f, 1.f);    // 默认白色
    inline constexpr FLinearColor DefaultLog(1.f, 1.f, 1.f);       // 默认白色
    inline constexpr FLinearColor Info(0.f, 1.f, 1.f);             // 青色 (信息)
    inline constexpr FLinearColor Success(0.f, 1.f, 0.f);          // 绿色 (成功)
    inline constexpr FLinearColor Warning(1.f, 1.f, 0.f);          // 黄色 (警告)
    inline constexpr FLinearColor Error(1.f, 0.f, 0.f);            // 红色 (错误)
}
```

---

## 十、扩展性设计

### 10.1 添加新系统

```cpp
// 在 EDX_System 枚举中新增:
enum class EDX_System : uint32
{
    // ... 现有系统 ...
    NewSystem    = 1 << 10,  // 0x00000400  新系统
};
```

无需修改任何宏或实现代码，新系统即可使用所有宏。

### 10.2 运行时动态注册系统

可选的扩展功能，允许在运行时动态注册自定义系统名称:

```cpp
// 蓝图或 C++ 中注册:
DX::RegisterSystem("MyCustomSystem", 1 << 10);
DX::UnregisterSystem(1 << 10);
```

### 10.3 输出过滤

未来可扩展输出过滤器，如按日志级别、屏幕时长等过滤:

```cpp
debug.ex.filter.loglevel Warning    # 仅输出 Warning 及以上级别
debug.ex.filter.minduration 2.0     # 仅输出屏幕时长 ≥ 2.0 秒的消息
```

---

## 十一、实施计划

| 阶段 | 内容 | 文件 |
|---|---|---|
| 1 | 创建插件基础结构 | `DebugEx.uplugin`, `DebugEx.Build.cs`, `DebugEx.h/.cpp` |
| 2 | 实现开关系统 | `DX_DebugSettings.h`, `DX_DebugFunctionLibrary.cpp` (CVar) |
| 3 | 实现宏体系 | `DX_DebugFunctionLibrary.h` (全部宏 + 枚举 + 颜色) |
| 4 | 实现蓝图支持 | `UDX_DebugFunctionLibrary::Print` / `PrintEx` |
| 5 | 测试验证 | 各系统宏输出验证、Shipping 编译消除验证 |
| 6 | 迁移现有代码 | 将 MultiplayerInventory 中的 `DH_*` 宏替换为 `DX_*` |