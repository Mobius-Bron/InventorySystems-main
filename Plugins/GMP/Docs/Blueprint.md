# GMP 蓝图（Blueprint）使用文档

## 集成

启用 GMP 插件（含 `GMPEditor` 编辑器模块）后，在蓝图编辑器里：

- 右键空白处 → 搜索 `GMP`，在 **`GMP | Message`** 分类下能找到相关节点。
- 也可在「我的蓝图 / 函数库」里搜 `GMP` 调用 `UGMPBPLib` 提供的函数节点。

---

## 节点一览

| 节点 | 类型 | 作用 |
|---|---|---|
| `ListenMessage` | 类事件节点 | 监听消息（事件图专用） |
| `NotifyMessage` | 函数节点 | 发送消息 |
| `RequestMessage` | 函数节点 | 请求消息（带响应） |
| `StoreMessage` | 函数节点 | 存储消息（新监听者立即收到历史值） |
| `ListenMessageRow` | 类事件节点 | 行监听（集合消息） |
| `Add Route` | 函数节点 | 添加路由 |

---

## 核心概念

- **消息键（Message Key）**：`MsgTag` / `EventName` 引脚，填字符串，如 `"Player.Hurt"`。
- **Sender**：发送节点上的「发送者对象」引脚。
- **WatchedObj**：监听节点上的「观察对象」引脚。
- **指定对象**：发送方的 `Sender` 与监听方的 `WatchedObj` 指向**同一对象**时，消息定向投递；两者都留空，则是全局广播。

---

## 发送消息（NotifyMessage）

### 不指定对象（全局广播）

1. 放置 `NotifyMessage` 节点。
2. `EventName` 填消息键，如 `"Player.Hurt"`。
3. 用节点上的 **Add Pin（+）** 添加参数引脚，填入参数类型与值。
4. `Sender` 留空。

### 指定对象（定向投递）

- `Sender` 引脚连接一个对象引用（如 `Self`、某个 Actor），监听方用相同对象即可收到。

---

## 监听消息（ListenMessage）

`ListenMessage` 是**类事件节点**，放在事件图，类似自定义事件：

1. 放置 `ListenMessage` 节点。
2. `EventName` 填与发送方一致的消息键。
3. 用 **Add Pin（+）** 添加参数引脚，**类型必须与发送方一致**。
4. 从节点的执行输出连接后续逻辑。

### 不指定对象

- `WatchedObj` 留空 → 监听全局消息。

### 指定对象

- `WatchedObj` 连接一个对象引用 → 只处理以该对象为 `Sender` 的消息。

> 监听侧可配置 **Times**（监听次数，`-1` 无限）与 **Order**（执行顺序，数值越大越晚执行）。

---

## 解绑（Unlisten Message）

用 `UGMPBPLib` 的函数节点 `Unlisten Message`，传入消息键与监听者对象，即可停止监听。

---

## 请求 / 响应（RequestMessage）

- **请求方**：`RequestMessage` 节点，填消息键、`Sender`、请求参数，并提供 `EventName` 回调事件接收响应。
- **响应方**：`ListenMessage` 节点收到请求后，调用 `Response Message` 函数节点，回传 `SeqId`（请求序号）与响应参数。

---

## 路由（Route）

`Add Route` 函数节点：把一个「行为键」扇出到一组「操作键」。

- `BehaviorKey`：行为键，如 `"Behavior.CloseBackpack"`。
- `OpKeys`：操作键数组（MakeArray），如 `["Op.CloseBagPanel", "Op.CloseTipA"]`。

向 `BehaviorKey` 发消息时，会自动展平为分别向各 `OpKeys` 发消息。

---

## 完整示例

```
[发送] NotifyMessage
   EventName = "Player.Hurt"
   Param0 (int32) = 10
   Sender    = Self          ← 指定对象：发送者是自己

[监听] ListenMessage
   EventName  = "Player.Hurt"
   WatchedObj = <同一个对象>   ← 指定对象：只收这个对象发的
   Param0 (int32)  →  执行打印 "收到伤害 10"
```
