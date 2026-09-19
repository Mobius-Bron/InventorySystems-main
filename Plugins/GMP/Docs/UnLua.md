# GMP UnLua（Lua）使用文档

## 集成

1. 在 `UnLua` 模块的 `.Build.cs` 里加依赖：

```csharp
PrivateDependencyModuleNames.Add("GMP");
```

2. 在 UnLua 模块的某个 `.cpp`（如 `LuaCore.cpp`）里包含头文件，GMP 会自动注册：

```cpp
#include "UnLuaSupport.h"
```

注册后，Lua 侧可用的全局函数：

| 函数 | 说明 |
|---|---|
| `GMP.NotifyObjectMessage` | 发送消息 |
| `GMP.ListenObjectMessage` | 监听消息 |
| `GMP.ListenRowMessage` | 行监听（集合消息） |
| `GMP.UnbindObjectMessage` / `GMP.UnListenObjectMessage` | 解绑 |
| `GMP.AddRoute` | 添加路由 |
| `GMP.Mixin(table)` | 给 table 混入对象方法 |

---

## 发送消息（NotifyObjectMessage）

```lua
-- GMP.NotifyObjectMessage(obj, msgkey, ...)
-- obj = 发送者对象；传 nil 表示「不指定对象」（全局广播）

-- 不指定对象（全局广播）
GMP.NotifyObjectMessage(nil, "Player.Hurt", 42, causer)

-- 指定对象（定向投递，发送者 = self）
GMP.NotifyObjectMessage(self, "Player.Hurt", 42, causer)
```

对象方法形式（`Mixin` 后，`self` 自动作为发送者）：

```lua
self:NotifyObjectMessage("Player.Hurt", 42, causer)
```

---

## 监听消息（ListenObjectMessage）

```lua
-- GMP.ListenObjectMessage(watchedobj, msgkey, weakobj, function|string [, times])
-- 返回一个 key（integer），供解绑使用
local key = GMP.ListenObjectMessage(watched, "Player.Hurt", self, function(damage, causer)
    print("收到伤害", damage)
end)
```

参数说明：

| 参数 | 含义 |
|---|---|
| `watchedobj` | 观察对象（指定对象过滤）；传 `nil` = 不指定对象（全局） |
| `msgkey` | 消息键 |
| `weakobj` | 回调宿主（弱引用，宿主销毁自动解绑） |
| `function\|string` | 回调函数，或成员函数名字符串 |
| `times` | 监听次数，`-1` 无限（默认） |

### 不指定对象

```lua
GMP.ListenObjectMessage(nil, "Player.Hurt", self, function(damage)
    print("全局伤害", damage)
end)
```

### 指定对象

```lua
-- 只处理以 SourceActor 为发送者的消息
GMP.ListenObjectMessage(SourceActor, "Player.Hurt", self, function(damage)
    print("来自 SourceActor 的伤害", damage)
end)
```

### 对象方法形式（推荐，Mixin 后）

```lua
-- self:ListenObjectMessage(watchedobj, msgkey, func [, times])
self:ListenObjectMessage(SourceActor, "Player.Hurt", function(damage)
    print("伤害", damage)
end)

-- self:ListenWorldMessage(msgkey, func [, times])  -- 监听世界消息
self:ListenWorldMessage("World.Tick", function(dt)
    -- ...
end)
```

> 回调若写成员函数名（字符串），GMP 会从 `weakobj` 表里取同名方法调用。

---

## 行监听（ListenRowMessage）

```lua
-- GMP.ListenRowMessage(watchedobj, msgkey, weakobj, index, function(row, item) [, times])
-- index >= 0 跟随该行；index < 0 监听 ~index 行（含删除通知）；GMP.AllRows 监听每一行
GMP.ListenRowMessage(SourceActor, "Items", self, GMP.AllRows, function(row, item)
    print("第", row, "行变化:", item)
end)
```

---

## 解绑（UnbindObjectMessage）

```lua
-- 按监听者对象解绑
GMP.UnbindObjectMessage("Player.Hurt", self)

-- 按监听返回的 key 解绑
GMP.UnbindObjectMessage("Player.Hurt", key)

-- 对象方法形式
self:UnbindObjectMessage("Player.Hurt", key)
```

---

## 路由（AddRoute）

```lua
-- GMP.AddRoute(behaviorkey, opkeys_table)
GMP.AddRoute("Behavior.CloseBackpack", {
    "Op.CloseBagPanel",
    "Op.CloseTipA",
    "Op.CloseTipB",
})

-- 之后向行为键发消息，会自动展平为向各操作键发消息
GMP.NotifyObjectMessage(nil, "Behavior.CloseBackpack")
```

---

## Mixin

把对象方法混入某个 table（通常是 UnLua 绑定的对象表）：

```lua
local obj = GMP.Mixin(your_table)
obj:NotifyObjectMessage("Key", 1, 2)
obj:ListenObjectMessage(nil, "Key", function(...) end)
```

---

## 完整示例

```lua
-- 监听（指定对象 + 全局各一）
GMP.ListenObjectMessage(EnemyActor, "Enemy.Dead", self, function() print("敌人死亡") end)
GMP.ListenObjectMessage(nil, "Game.Over", self, function(reason) print("游戏结束", reason) end)

-- 发送（指定对象）
GMP.NotifyObjectMessage(self, "Player.Hurt", 42, attacker)

-- 路由
GMP.AddRoute("Behavior.OpenMainUI", { "Op.OpenBag", "Op.OpenSkill" })
GMP.NotifyObjectMessage(self, "Behavior.OpenMainUI")

-- 解绑
GMP.UnbindObjectMessage("Enemy.Dead", self)
```
