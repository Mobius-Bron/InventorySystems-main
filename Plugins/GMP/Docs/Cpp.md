# GMP C++ 使用文档

## 集成

1. 在模块的 `.Build.cs` 里加依赖：

```csharp
PublicDependencyModuleNames.AddRange(new string[] { "GMP" });
```

2. 引入头文件：

```cpp
#include "GMPCore.h"   // 核心：MSGKEY、路由、FMessageHub 声明
#include "GMPUtils.h"  // FMessageUtils 便捷封装
```

---

## 获取中枢（Hub）

```cpp
GMP::FMessageUtils::GetManager();   // -> UGMPManager*
GMP::FMessageUtils::GetManager()->GetHub();  // -> FMessageHub&
```

通常拿一次即可：

```cpp
auto& Hub = GMP::FMessageUtils::GetManager()->GetHub();
```

---

## 消息键

推荐用 `MSGKEY` 宏（编译期 `FName`，类型安全、可静态化）：

```cpp
MSGKEY("Player.Hurt")
```

也可以直接传 `FName`：

```cpp
FName Key = TEXT("Player.Hurt");
```

---

## 发送消息（Notify）

### 不指定对象（全局广播）

```cpp
Hub.SendMessage(MSGKEY("Player.Hurt"), int32(10), Causer);
```

### 指定对象（定向投递）

```cpp
Hub.SendObjectMessage(MSGKEY("Player.Hurt"), FSigSource(SenderActor), int32(10), Causer);
```

### 便捷封装

```cpp
GMP::FMessageUtils::NotifyObjectMessage(FSigSource(Src), MSGKEY("Key"), Args...);
GMP::FMessageUtils::SendObjectMessage(FSigSource(Src), MSGKEY("Key"), Args...);
```

---

## 信号源（FSigSource）

`FSigSource` 用于「指定对象」：包装一个 `UObject` / `UWorld` 作为消息的定向标识。

```cpp
FSigSource(SomeUObject);        // 从任意 UObject
FSigSource(GetWorld());         // 从 UWorld
FSigSource(WeakObjectPtr);      // 从弱引用
FSigSource::NullSigSrc;         // 空信号源 = 不指定对象
```

---

## 监听消息（Listen）

### 不指定对象

```cpp
FGMPKey Handle;
Hub.ListenObjectMessage(MSGKEY("Player.Hurt"), FSigSource::NullSigSrc, this,
    [this](int32 Damage, AActor* Causer)
    {
        // 处理伤害
    },
    &Handle);
```

### 指定对象

```cpp
Hub.ListenObjectMessage(MSGKEY("Player.Hurt"), FSigSource(SourceActor), this,
    [this](int32 Damage)
    {
        // 只处理来自 SourceActor 的消息
    });
```

### 世界消息

```cpp
Hub.ListenWorldMessage(GetWorld(), MSGKEY("Some.Key"), this,
    [this](int32 Value)
    {
        // ...
    });
```

### 行监听（监听集合元素）

带 `Index` 的监听，回调会额外收到「行号 + 集合元素」：

```cpp
Hub.ListenObjectMessage(MSGKEY("Items"), FSigSource(Src), /*Index*/ 0, this,
    [this](int32 Row, FItem Item)
    {
        // Row 为行号，Item 为元素
    });
```

### 监听次数与顺序

`FGMPListenOptions(Times, Order)`：
- `Times`：监听次数，`-1` 表示无限。
- `Order`：执行顺序，数值越大越晚执行（负数先执行）。

```cpp
FGMPListenOptions Opts(-1, 30);   // 无限次，顺序 30
Hub.ListenObjectMessage(Key, FSigSource::NullSigSrc, this, Func, &Handle, Opts);
```

### 便捷封装

```cpp
GMP::FMessageUtils::ListenObjectMessage(FSigSource(Src), MSGKEY("Key"), &Handle, Func);
```

---

## 解绑（Unbind）

```cpp
Hub.UnbindMessage(MSGKEY("Key"), Handle);   // 按句柄解绑
Hub.UnbindMessage(MSGKEY("Key"), this);     // 解绑某监听者的全部回调
```

---

## 请求 / 响应（Request / Response）

### 请求方

```cpp
FGMPKey SeqId = Hub.RequestMessage(MSGKEY("GetData"), FSigSource(Src),
    [](FGMPResponder& Rsp)
    {
        int32 Value = 0;
        if (Rsp.GetValue(Value))
        {
            // 收到响应
        }
    },
    RequestId);
```

### 响应方（在监听回调里响应）

```cpp
Hub.ListenObjectMessage(MSGKEY("GetData"), FSigSource(Src), this,
    [this](FGMPResponder& Rsp, int32 RequestId)
    {
        // 处理请求，然后响应
        Rsp.Response(SomeResult);
    });
```

---

## 存储消息（Store / Once）

存储消息：监听者**注册后立即**收到历史值（状态同步场景）。

```cpp
Hub.StoreObjectMessage(MSGKEY("State"), FSigSource(Src), Payload);   // 存储
Hub.RemoveStoredObjectMessage(MSGKEY("State"), FSigSource(Src));     // 移除存储
Hub.OnceObjectMessage(MSGKEY("Once.Key"), FSigSource(Src), Payload); // 只触发一次
```

---

## 路由（Route）

### 运行时添加 / 移除

```cpp
Hub.AddRoute(MSGKEY("Behavior.CloseBackpack"),
    { MSGKEY("Op.CloseBagPanel"), MSGKEY("Op.CloseTipA") });

Hub.RemoveRoute(MSGKEY("Behavior.CloseBackpack"));
```

### 声明式路由（推荐）

在任意编译单元的作用域（全局 / 命名空间 / 函数外）声明，无需手动在运行时注册，模块生命周期会自动处理：

```cpp
GMP_ROUTE(MSGKEY("Behavior.CloseBackpack"),
    GMP_FWD(MSGKEY("Op.CloseBagPanel")),
    GMP_FWD(MSGKEY("Op.CloseTipA")),
    GMP_FWD(MSGKEY("Op.CloseTipB"))
);
```

- `GMP_ROUTE(BehaviorKey, ...)`：行为键 + 一组操作键。
- `GMP_FWD(Key)`：把操作键包成 `FName`。
- 发送到 `Behavior.CloseBackpack` 的消息会**自动展平**发送到三个 `Op.*` 键。

---

## 完整示例

```cpp
// 发送方（某个 Actor）
void AMyActor::BroadcastHurt(int32 Damage)
{
    auto& Hub = GMP::FMessageUtils::GetManager()->GetHub();
    // 指定对象：只有监听 FSigSource(this) 的监听者收到
    Hub.SendObjectMessage(MSGKEY("Player.Hurt"), FSigSource(this), Damage);
}

// 接收方（某个 Actor）
void AMyListener::BeginPlay()
{
    Super::BeginPlay();
    auto& Hub = GMP::FMessageUtils::GetManager()->GetHub();
    Hub.ListenObjectMessage(MSGKEY("Player.Hurt"), FSigSource(SenderActor), this,
        [this](int32 Damage)
        {
            UE_LOG(LogTemp, Log, TEXT("收到伤害 %d"), Damage);
        },
        &Handle);
}
```
