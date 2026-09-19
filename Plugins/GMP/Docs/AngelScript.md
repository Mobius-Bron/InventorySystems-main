# GMP AngelScript 使用文档

## 集成

1. 在 `AngelscriptRuntime` 模块的 `.Build.cs` 里加依赖：

```csharp
PrivateDependencyModuleNames.Add("GMP");
```

2. 在 AngelscriptRuntime 模块的某个 `.cpp` 里包含头文件，`AS_FORCE_LINK` 会自动注册弱类型 `GMP` 命名空间全局函数：

```cpp
#include "AngelScriptSupport.h"
```

3. AS 引擎创建后，调用一次强类型绑定（为每个 tag 生成 `asListen_<id>` / `asNotify_<id>`）：

```cpp
AngelScriptSupport::GMP_RegisterTypedBinds(WorldContext);
```

4. （可选）启动时安装预处理器 hook，让「弱类型 + 字面量 key」的调用自动重写为强类型调用：

```cpp
AngelScriptSupport::GMP_As_InstallPreprocessorHook();
```

---

## 两种 API 风格

GMP 在 AngelScript 里提供**弱类型**（动态 key）与**强类型**（每 tag 生成带类型检查的绑定）两套，语义一致：

| 风格 | 特点 |
|---|---|
| 弱类型 | `GMP::ListenObjectMessage(WatchedObj, "key", WeakObj, cb)`，key 是字符串，运行时解析 |
| 强类型 | `GMP::asListen_Player_Hurt(...)`，key 在函数名里，编译期类型检查 |

> 推荐强类型；弱类型适合 key 是变量、动态拼接的场景。

---

## 发送消息（NotifyObjectMessage）

### 强类型（推荐）

```angelscript
// GMP::asNotify_<id>(Sender, <typed args...>)
// 以 tag "Player.Hurt" 为例，id = "Player_Hurt"
GMP::asNotify_Player_Hurt(this, 42, causer);          // 指定对象：Sender = this
GMP::asNotify_Player_Hurt(null, 42, causer);          // 不指定对象：Sender = null（全局广播）
```

### 弱类型

```angelscript
// GMP::NotifyObjectMessage(Sender, MsgKey, Params) -> bool
// Params 为 TArray<FGMPTypedAddr>（弱类型参数容器）
```

---

## 监听消息（ListenObjectMessage）

### 强类型（推荐）

```angelscript
// GMP::asListen_<id>(WatchedObj, WeakObj, FOn_<id>@ cb, Times = -1) -> int64
int64 Key = GMP::asListen_Player_Hurt(null, this, function(int Damage, AActor Causer)
{
    Print("hurt " + Damage);
});
```

参数说明：

| 参数 | 含义 |
|---|---|
| `WatchedObj` | 观察对象（指定对象过滤）；`null` = 不指定对象（全局） |
| `WeakObj` | 回调宿主（弱引用，宿主销毁自动解绑） |
| `cb` | 回调（`FOn_<id>` funcdef，签名与 tag 参数一致） |
| `Times` | 监听次数，`-1` 无限（默认） |

### 指定对象

```angelscript
int64 Key = GMP::asListen_Player_Hurt(SourceActor, this, function(int Damage, AActor Causer)
{
    Print("来自 SourceActor 的伤害 " + Damage);
});
```

### 弱类型

```angelscript
// GMP::ListenObjectMessage(WatchedObj, MsgKey, WeakObj, Callback, Times = -1) -> int64
int64 Key = GMP::ListenObjectMessage(SourceActor, "Player.Hurt", this, function(int Damage, AActor Causer)
{
    Print("hurt " + Damage);
});
```

### 命名方法监听（ABI-B 快速路径）

用对象上的具名 AS 方法作为回调，绕过 ProcessEvent 直接 JIT 调用：

```angelscript
// GMP::ListenObjectMessageMethod(WeakObj, MsgKey, MethodName) -> int64
int64 Key = GMP::ListenObjectMessageMethod(this, "Player.Hurt", "OnHurt");
// 触发时调用 this.OnHurt(<匹配签名的方法>)
```

---

## 行监听（asListenRow_<id>）

集合 tag（参数为单个 `TArray<T>`）会额外生成行监听：

```angelscript
// GMP::asListenRow_<id>(WatchedObj, WeakObj, Index, FOnRow_<id>@ cb, Times = -1) -> int64
int64 Key = GMP::asListenRow_Items(null, this, 0, function(int Row, FItem Item)
{
    Print("第 " + Row + " 行变化");
});
```

---

## 解绑（UnbindObjectMessage）

```angelscript
// 按监听者对象解绑
GMP::UnbindObjectMessage("Player.Hurt", this);

// 按监听返回的 key 解绑
GMP::UnbindObjectMessage("Player.Hurt", null, Key);
```

---

## 路由（Route）

AngelScript 侧未直接暴露 `AddRoute`，请在 C++/蓝图侧添加路由，或用 C++ 的 `GMP_ROUTE` 宏声明式注册；向行为键发消息会自动展平到各操作键。

---

## 完整示例

```angelscript
// 强类型监听（指定对象）
int64 Key = GMP::asListen_Enemy_Dead(enemyActor, this, function()
{
    Print("敌人死亡");
});

// 弱类型监听（全局）
GMP::ListenObjectMessage(null, "Game.Over", this, function(FString Reason)
{
    Print("游戏结束 " + Reason);
});

// 发送（指定对象）
GMP::asNotify_Player_Hurt(this, 42, attacker);

// 解绑
GMP::UnbindObjectMessage("Enemy.Dead", this, Key);
```
