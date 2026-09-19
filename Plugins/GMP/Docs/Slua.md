# GMP slua（Lua）使用文档

## 集成

1. 在 `slua_unreal` 模块的 `.Build.cs` 里加依赖：

```csharp
PrivateDependencyModuleNames.Add("GMP");
```

2. 在 slua 模块的某个 `.cpp` 里包含头文件，并对每个 `lua_State` 调用一次注册：

```cpp
#include "SluaSupport.h"

// 每个 lua_State 创建后调用一次
SluaSupport::GMP_RegisterToSlua(L);
```

注册后，Lua 侧可用的全局函数：

| 函数 | 说明 |
|---|---|
| `NotifyObjectMessage` | 发送消息（返回 bool） |
| `ListenObjectMessage` | 监听消息 |
| `ListenRowMessage` | 行监听（集合消息） |
| `UnbindObjectMessage` / `UnListenObjectMessage` | 解绑 |
| `AddRoute` | 添加路由 |

> slua 与 UnLua 的 API 基本一致，区别在于：slua 是**纯全局函数**形式（无 Mixin），`NotifyObjectMessage` 返回 `bool`。

---

## 发送消息（NotifyObjectMessage）

```lua
-- NotifyObjectMessage(sender, msgkey, ...) -> bool
-- sender = 发送者对象；传 nil 表示「不指定对象」（全局广播）

-- 不指定对象（全局广播）
NotifyObjectMessage(nil, "Player.Hurt", 42, causer)

-- 指定对象（定向投递）
NotifyObjectMessage(self, "Player.Hurt", 42, causer)
```

---

## 监听消息（ListenObjectMessage）

```lua
-- ListenObjectMessage(watchedobj, msgkey, weakobj, function [, times]) -> key
local key = ListenObjectMessage(watched, "Player.Hurt", self, function(damage, causer)
    print("收到伤害", damage)
end)
```

参数说明：

| 参数 | 含义 |
|---|---|
| `watchedobj` | 观察对象（指定对象过滤）；传 `nil` = 不指定对象（全局） |
| `msgkey` | 消息键 |
| `weakobj` | 回调宿主（弱引用，宿主销毁自动解绑） |
| `function` | 回调函数 |
| `times` | 监听次数，`-1` 无限（默认） |

### 不指定对象

```lua
ListenObjectMessage(nil, "Player.Hurt", self, function(damage)
    print("全局伤害", damage)
end)
```

### 指定对象

```lua
ListenObjectMessage(SourceActor, "Player.Hurt", self, function(damage)
    print("来自 SourceActor 的伤害", damage)
end)
```

---

## 行监听（ListenRowMessage）

```lua
-- ListenRowMessage(watchedobj, msgkey, weakobj, index, function(row, item) [, times]) -> key
-- index >= 0 跟随该行；index < 0 监听 ~index 行（含删除通知）；GMP.AllRows 监听每一行
ListenRowMessage(SourceActor, "Items", self, GMP.AllRows, function(row, item)
    print("第", row, "行变化:", item)
end)
```

---

## 解绑（UnbindObjectMessage）

```lua
-- 按监听者对象解绑
UnbindObjectMessage("Player.Hurt", self)

-- 按监听返回的 key 解绑
UnbindObjectMessage("Player.Hurt", key)

-- UnListenObjectMessage 是别名
UnListenObjectMessage("Player.Hurt", self)
```

---

## 路由（AddRoute）

```lua
-- AddRoute(behaviorkey, opkeys_table)
AddRoute("Behavior.CloseBackpack", {
    "Op.CloseBagPanel",
    "Op.CloseTipA",
    "Op.CloseTipB",
})

NotifyObjectMessage(nil, "Behavior.CloseBackpack")
```

---

## 完整示例

```lua
-- 监听（指定对象 + 全局各一）
ListenObjectMessage(EnemyActor, "Enemy.Dead", self, function() print("敌人死亡") end)
ListenObjectMessage(nil, "Game.Over", self, function(reason) print("游戏结束", reason) end)

-- 发送（指定对象）
NotifyObjectMessage(self, "Player.Hurt", 42, attacker)

-- 路由
AddRoute("Behavior.OpenMainUI", { "Op.OpenBag", "Op.OpenSkill" })
NotifyObjectMessage(self, "Behavior.OpenMainUI")

-- 解绑
UnbindObjectMessage("Enemy.Dead", self)
```
