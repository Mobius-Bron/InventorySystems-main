# GMP (GenericMessagePlugin) 使用文档

GMP 是一个通用的消息传递插件，提供一套**跨语言统一**的消息发送 / 监听 / 路由机制，支持 C++、蓝图、Lua（UnLua / slua）、TypeScript / JavaScript（Puerts）、C#（UnrealCSharp）、AngelScript 等多种语言，并且各语言之间可以互相收发消息。

---

## 核心概念

### 1. 消息键（Message Key）

一条消息由 `FName` 类型的键唯一标识。各语言中通常用字符串字面量表示：

| 语言 | 写法 |
|---|---|
| C++ | `MSGKEY("Player.Hurt")` |
| 蓝图 | `Gameplay Message Key` 参数传 `"Player.Hurt"` |
| Lua / TS / C# / AS | `"Player.Hurt"` |

### 2. 中枢（Message Hub）

`FMessageHub` 是全局消息路由器，负责消息的注册、分发、展平（路由）。各语言最终都打到同一个 Hub 上。

### 3. 信号源（SigSource）与「是否指定对象」

这是 GMP 消息最核心的区分维度：

- **指定对象**：消息绑定到一个具体的 `UObject`（发送者 / 观察对象）。监听时用**相同的对象**做过滤，实现对象级别的精准投递（不会串台）。
- **不指定对象**：使用空信号源（C++ 的 `FSigSource::NullSigSrc`，脚本的 `nil` / `null`），消息做全局广播，所有监听该键的监听者都能收到。

> 直观理解：`指定对象` = 一对一 / 定向通知；`不指定对象` = 全局事件。

### 4. 监听者（Listener / WeakObj）

监听回调的宿主对象。采用**弱引用**，宿主销毁后自动解绑，避免悬挂回调。脚本语言里通常额外传一个 `weakobj` 参数，就是这个宿主。

### 5. 路由（Route）

「行为键（behavior key）」扇出到一组「操作键（op key）」。向行为键发消息时，会**自动展平**为直接向各操作键发消息。

- 声明式路由是**纯数据**，没有转发 lambda。
- 典型用途：一个业务动作（如 `Behavior.CloseBackpack`）驱动多个 UI 操作（`Op.CloseBagPanel`、`Op.CloseTipA`…）。

---

## 统一 API 模式

所有语言遵循同一套命名与语义，只是语法糖不同：

| 操作 | C++ | 脚本（Lua/TS/C#/AS） |
|---|---|---|
| 发送（指定对象） | `SendObjectMessage(Key, SigSource, Args...)` | `NotifyObjectMessage(sender, key, ...)` |
| 发送（不指定对象） | `SendMessage(Key, Args...)` | `NotifyObjectMessage(nil, key, ...)` |
| 监听（指定对象） | `ListenObjectMessage(Key, SigSource, Listener, Func)` | `ListenObjectMessage(watched, key, weak, cb, times)` |
| 解绑 | `UnbindMessage(Key, ...)` | `UnbindObjectMessage(key, listened)` |
| 路由 | `AddRoute(BehaviorKey, OpKeys)` | `AddRoute(behaviorKey, opKeys)` |

> 「是否指定对象」在监听侧由 `watched` 对象决定、在发送侧由 `sender` 对象决定；两者**同时为空**表示不指定对象（全局），**同时为同一对象**表示指定对象（定向）。

---

## 支持的语言与文档

| 语言 | 文档 |
|---|---|
| C++ | [Cpp.md](./Cpp.md) |
| 蓝图（Blueprint） | [Blueprint.md](./Blueprint.md) |
| UnLua（Lua） | [UnLua.md](./UnLua.md) |
| slua（Lua） | [Slua.md](./Slua.md) |
| Puerts（TypeScript / JavaScript） | [Puerts.md](./Puerts.md) |
| C#（UnrealCSharp） | [CSharp.md](./CSharp.md) |
| AngelScript | [AngelScript.md](./AngelScript.md) |
