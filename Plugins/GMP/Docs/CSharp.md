# GMP C#（UnrealCSharp）使用文档

## 集成

1. 在 `UnrealCSharp` / `UnrealCSharpCore` 模块的 `.Build.cs` 里加依赖：

```csharp
PrivateDependencyModuleNames.Add("GMP");
```

2. 在 UnrealCSharp 模块的某个 `.cpp`（如 `FRegisterGMP.cpp`）里包含头文件并注册：

```cpp
#include "CSharpSupport.h"

// 通过 FClassBuilder 注册（CoreCLR 与 Mono 双后端通用），native 注册类名为 FGMP：
//   FGMP.NotifyObjectMessage
//   FGMP.NotifyObjectMessageTyped
//   FGMP.ListenObjectMessage
//   FGMP.UnbindObjectMessage
```

3. 汇编卸载时清理 domain-keyed 监听：

```cpp
FUnrealCSharpModuleDelegates::OnUnrealCSharpModuleInActive.AddLambda(...);
```

> C# 侧通过 interop 层 `Interop.GMPBridge` 桥接到 native 的 `FGMP` 类；下面的示例以 native 注册类名 `FGMP` 为调用入口，若你的 C# 侧封装成了别的类名（如 `GMPBridge` / `GMP`），替换对应类名即可。

---

## 参数类型映射

C# 侧通过类型标签 `EArgType`（对应 `Interop.GMPBridge.EArgType`）区分参数：

| C# 类型 | 标签 | 说明 |
|---|---|---|
| `UObject`（含 Actor 等） | `Object` | 走 managed handle |
| `long` / `int` | `Int64` | `Payloads[i]` |
| `double` | `Double` | `Payloads[i]` 按位存 double |
| `bool` | `Bool` | `Payloads[i]` 0/1 |
| `string` | `String` | `Payloads[i]` 为 UTF8 指针 |

---

## 发送消息（NotifyObjectMessage）

```csharp
// FGMP.NotifyObjectMessage(sender, msgKey, ...args) -> bool
// sender = 发送者对象；传 null 表示「不指定对象」（全局广播）

// 不指定对象（全局广播）
FGMP.NotifyObjectMessage(null, "Player.Hurt", 42L, causerActor);

// 指定对象（定向投递）
FGMP.NotifyObjectMessage(this, "Player.Hurt", 42L, causerActor);
```

---

## 监听消息（ListenObjectMessage）

```csharp
// FGMP.ListenObjectMessage(watchedObj, msgKey, weakObj, cbHandle [, leftTimes]) -> ulong (key)
ulong key = FGMP.ListenObjectMessage(watchedActor, "Player.Hurt", null, cbHandle);
```

参数说明：

| 参数 | 含义 |
|---|---|
| `watchedObj` | 观察对象（指定对象过滤）；传 `null` = 不指定对象（全局） |
| `msgKey` | 消息键 |
| `weakObj` | 回调宿主（弱引用，宿主销毁自动解绑） |
| `cbHandle` | 回调的 managed 侧 id（C# 侧持有 delegate 并 pin 住，native 只存不透明 id） |
| `leftTimes` | 监听次数，`-1` 无限（默认） |

### 不指定对象

```csharp
FGMP.ListenObjectMessage(null, "Player.Hurt", this, cbHandle);
```

### 指定对象

```csharp
FGMP.ListenObjectMessage(sourceActor, "Player.Hurt", this, cbHandle);
```

> 强类型 C# 监听（泛型回调）会在首次监听时把回调参数类型注册为 tag 的签名（editor/dev 下做签名校验）。

---

## 解绑（UnbindObjectMessage）

```csharp
// 按监听者对象解绑
FGMP.UnbindObjectMessage("Player.Hurt", this, 0);

// 按监听返回的 key 解绑
FGMP.UnbindObjectMessage("Player.Hurt", null, key);
```

---

## 完整示例

```csharp
// 监听（指定对象）
ulong key = FGMP.ListenObjectMessage(enemyActor, "Enemy.Dead", this, cbHandle);

// 监听（全局）
FGMP.ListenObjectMessage(null, "Game.Over", this, cbHandle2);

// 发送（指定对象）
FGMP.NotifyObjectMessage(this, "Player.Hurt", 42L, attackerActor);

// 解绑
FGMP.UnbindObjectMessage("Enemy.Dead", this, key);
```

> 路由（Route）：C# 侧未直接暴露 `AddRoute`，请在 C++/蓝图侧添加路由，或用 C++ 的 `GMP_ROUTE` 宏声明式注册；向行为键发消息会自动展平到各操作键。
