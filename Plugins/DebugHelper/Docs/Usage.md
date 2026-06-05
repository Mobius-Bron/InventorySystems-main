# DebugHelper 插件使用文档

## 概述

DebugHelper 提供统一的调试输出，支持**日志**、**屏幕**和**双通道**输出，通过全局开关可一键关闭全部输出，打包后默认静默。

## 依赖

- 仅依赖 `Core`、`CoreUObject`、`Engine`（UE5 内置模块）
- 不依赖任何自定义插件

---

## 快速开始

### 1. 引用插件

在 `.Build.cs` 中添加依赖：

```csharp
PublicDependencyModuleNames.Add("DebugHelper");
```

### 2. 引入头文件

```cpp
#include "DH_DebugFunctionLibrary.h"
```

### 3. 使用宏

```cpp
// ---- 仅日志 ----
DH_LOG("[背包] 当前数量=%d", Count);
DH_LOG_WARN("[背包] 库存已满!");
DH_LOG_ERR("[背包] 空指针异常!");

// ---- 仅屏幕 ----
DH_SCREEN(2.f, FLinearColor::Green, "[背包] 物品拾取成功");

// ---- 日志 + 屏幕 ----
DH_PRINT(EDH_Output::Both, 3.f, FLinearColor::Magenta,
    "[背包] 堆叠已有物品 | 填充=%d | 剩余=%d", AmountToFill, Remainder);
```

### 4. 蓝图调用

搜索节点：`Print`（类别：`DebugHelper`）

| 参数 | 类型 | 说明 |
|---|---|---|
| WorldContextObject | Object | 传入 `self` |
| Message | String | 输出文本 |
| Output | EDH_Output | Log / Screen / Both |
| Color | LinearColor | 屏幕颜色 |
| Duration | float | 屏幕显示时长（秒） |

---

## 宏参考

| 宏 | 输出目标 | 说明 |
|---|---|---|
| `DH_LOG(Fmt, ...)` | 日志 | `UE_LOG(LogDH, Log, ...)` |
| `DH_LOG_WARN(Fmt, ...)` | 日志 | `UE_LOG(LogDH, Warning, ...)` |
| `DH_LOG_ERR(Fmt, ...)` | 日志 | `UE_LOG(LogDH, Error, ...)` |
| `DH_SCREEN(Duration, Color, Fmt, ...)` | 屏幕 | `GEngine->AddOnScreenDebugMessage` |
| `DH_PRINT(Output, Duration, Color, Fmt, ...)` | 日志+屏幕 | 双通道输出 |

---

## 枚举：EDH_Output

| 值 | 说明 |
|---|---|
| `EDH_Output::Log` | 仅 `UE_LOG` 输出到 Output Log |
| `EDH_Output::Screen` | 仅屏幕左上角浮窗 |
| `EDH_Output::Both` | 日志 + 屏幕，双通道 |

---

## 全局开关

### 控制台命令

```
debug.helper.enable 0    # 关闭全部调试输出
debug.helper.enable 1    # 开启
```

### 编辑器 vs 打包

| 环境 | 默认值 | 变更方式 |
|---|---|---|
| 编辑器 | `true` | 控制台 / DefaultEngine.ini |
| 打包开发版 | `false` | DefaultEngine.ini 写入 `debug.helper.enable=1` |
| Shipping | 编译消除 | 无法开启（宏展开为 `(void)0`） |

### 打包后开启

在项目 `Config/DefaultEngine.ini` 添加：

```ini
[ConsoleVariables]
debug.helper.enable=1
```

---

## 性能

| 环境 | 运行时开销 |
|---|---|
| 编辑器 / 打包开发版（开启） | 一次 `bool` 检查 + 格式化字符串 |
| 打包开发版（关闭） | 一次 `bool` 检查（分支预测几乎零开销） |
| Shipping | **零** — 宏展开为 `(void)0`，编译器完全消除 |

---

## 文件结构

```
DebugHelper/
├── DebugHelper.uplugin
├── Resources/
│   └── Icon128.png
├── Source/DebugHelper/
│   ├── Public/
│   │   ├── DebugHelper.h                  # 模块类
│   │   ├── DH_DebugSettings.h             # Shipping 守卫 + DH_ENABLED 宏
│   │   └── DH_DebugFunctionLibrary.h      # 枚举 + 蓝图函数库 + 5 个宏
│   └── Private/
│       ├── DebugHelper.cpp                 # 模块入口
│       └── DH_DebugFunctionLibrary.cpp     # CVar 定义 + Print 实现
└── Docs/
    └── Usage.md                            # 本文档
```
