# GMP Puerts（TypeScript / JavaScript）使用文档

## 集成

1. 在 `JsEnv` 模块的 `.Build.cs` 里加依赖：

```csharp
PrivateDependencyModuleNames.Add("GMP");
```

2. 在 `JsEnv` 模块的某个 `.cpp`（如 `JsEnvImpl.cpp`）里包含头文件，GMP 会自动注册 `"GMP"` addon：

```cpp
#include "PuertsSupport.h"
```

无需手动生命周期管理，`require('GMP')` 即可解析；Isolate 销毁时自动清理监听。

3. （可选）类型声明 `GMP.d.ts`：

```typescript
declare module "GMP" {
    function ListenObjectMessage(watchedObj: object, msgKey: string, weakObj: object | null, callback: Function | string, times?: number): number;
    function ListenRowMessage(watchedobj: object, msgkey: string, weakobj: object | null, index: number, callback: (row: number, item: any) => void, times?: number): number;
    function UnbindObjectMessage(msgKey: string, listenedObj: object | number, key?: number): void;
    function UnListenObjectMessage(msgKey: string, listenedObj: object | number, key?: number): void;
    function NotifyObjectMessage(sender: object, msgKey: string, ...args: any[]): boolean;
}
```

---

## 发送消息（NotifyObjectMessage）

```typescript
import { NotifyObjectMessage } from 'GMP';

// NotifyObjectMessage(sender, msgKey, ...args) -> boolean
// sender = 发送者对象；传 null 表示「不指定对象」（全局广播）

// 不指定对象（全局广播）
NotifyObjectMessage(null, 'Player.Hurt', 42, causerActor);

// 指定对象（定向投递）
NotifyObjectMessage(this, 'Player.Hurt', 42, causerActor);
```

参数类型自动推断（未注册的 tag 按 JS 值推断）：

| JS 值 | UE 类型 |
|---|---|
| `boolean` | `bool` |
| `number`(整数) / `bigint` | `int64` |
| `number`(浮点) | `double` |
| `string` | `FString` |
| `UObject` | `UObject*` |

---

## 监听消息（ListenObjectMessage）

```typescript
import { ListenObjectMessage } from 'GMP';

// ListenObjectMessage(watchedObj, msgKey, weakObj, callback|string [, times]) -> number (key)
const key = ListenObjectMessage(watchedActor, 'Player.Hurt', null, (damage: number, causer: object) => {
    console.log('收到伤害', damage, causer);
});
```

参数说明：

| 参数 | 含义 |
|---|---|
| `watchedObj` | 观察对象（指定对象过滤）；传 `null` = 不指定对象（全局） |
| `msgKey` | 消息键 |
| `weakObj` | 回调宿主（弱引用，宿主销毁自动解绑）；传 `null` 则由回调对象驱动生命周期 |
| `callback\|string` | 回调函数，或全局函数名字符串 |
| `times` | 监听次数，`-1` 无限（默认） |

### 不指定对象

```typescript
ListenObjectMessage(null, 'Player.Hurt', null, (damage: number) => {
    console.log('全局伤害', damage);
});
```

### 指定对象

```typescript
ListenObjectMessage(sourceActor, 'Player.Hurt', this, (damage: number) => {
    console.log('来自 sourceActor 的伤害', damage);
});
```

---

## 行监听（ListenRowMessage）

```typescript
import { ListenRowMessage } from 'GMP';

// ListenRowMessage(watchedobj, msgkey, weakobj, index, (row, item) => void [, times]) -> number
// index >= 0 跟随该行；index < 0 监听 ~index 行（含删除通知，被删的行 row 为负）
ListenRowMessage(sourceActor, 'Items', null, 0, (row: number, item: any) => {
    console.log('第', row, '行变化:', item);
});
```

---

## 解绑（UnbindObjectMessage）

```typescript
import { UnbindObjectMessage } from 'GMP';

// 按监听者对象解绑
UnbindObjectMessage('Player.Hurt', this);

// 按监听返回的 key 解绑
UnbindObjectMessage('Player.Hurt', key);

// UnListenObjectMessage 是别名
UnListenObjectMessage('Player.Hurt', this);
```

---

## 完整示例

```typescript
import { ListenObjectMessage, UnbindObjectMessage, NotifyObjectMessage } from 'GMP';

// 监听（指定对象）
const key = ListenObjectMessage(enemyActor, 'Enemy.Dead', this, () => {
    console.log('敌人死亡');
});

// 监听（全局）
ListenObjectMessage(null, 'Game.Over', this, (reason: string) => {
    console.log('游戏结束', reason);
});

// 发送（指定对象）
NotifyObjectMessage(this, 'Player.Hurt', 42, attackerActor);

// 解绑
UnbindObjectMessage('Enemy.Dead', this, key);
```

> 路由（Route）：Puerts 侧未直接暴露 `AddRoute`，路由请使用 C++/蓝图/其他语言侧添加，或在 C++ 中用 `GMP_ROUTE` 宏声明式注册；向行为键发消息同样会自动展平到各操作键。
