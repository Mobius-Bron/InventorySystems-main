# MultiplayerInventory 插件总结文档

## 一、插件概述

| 属性 | 值 |
|---|---|
| 名称 | MultiplayerInventory |
| 描述 | 适用于多人游戏的库存系统 |
| 版本 | 1.0 |
| 模块名 | MultiplayerInventory |
| 加载阶段 | Default (Runtime) |
| 类前缀 | `MIS_` |
| 日志分类 | `LogMIS` |
| 依赖 | `StructUtils`, `EnhancedInput`, `DebugHelper` |

**定位**: 多人库存系统，更灵活但需要手动集成，提供 ProxyMesh、双高亮、右键菜单等增强功能。

---

## 二、架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                        数据层 (Data Layer)                        │
│  FMIS_ItemFragment (基类) → InstancedStruct 多态存储              │
│  ├── FMIS_InventoryItemFragment (可同化到 UI 的基类)              │
│  │   ├── FMIS_ImageFragment          (物品图标)                   │
│  │   ├── FMIS_TextFragment           (物品文本)                   │
│  │   ├── FMIS_LabeledNumberFragment  (带标签数值,支持随机化)       │
│  │   ├── FMIS_ConsumableFragment     (消耗品)                     │
│  │   └── FMIS_EquipmentFragment      (装备行为)                   │
│  ├── FMIS_GridFragment               (网格占用尺寸)               │
│  ├── FMIS_StackableFragment          (可堆叠)                     │
│  ├── FMIS_ConsumeModifier            (消耗效果基类)                │
│  │   ├── FMIS_HealthPotionFragment   (生命药水)                   │
│  │   └── FMIS_ManaPotionFragment     (法力药水)                   │
│  └── FMIS_EquipModifier              (装备效果基类)                │
│      ├── FMIS_StrengthModifier       (力量加成)                   │
│      ├── FMIS_ArmorModifier          (护甲加成)                   │
│      └── FMIS_DamageModifier         (伤害加成)                   │
│                                                                   │
│  FMIS_ItemManifest → 物品配置容器                                  │
│  UMIS_InventoryItem → 运行时物品 UObject (可网络复制)               │
├─────────────────────────────────────────────────────────────────┤
│                       组件层 (Component Layer)                     │
│  UMIS_InventoryComponent  → 库存核心 ViewModel                     │
│     - Init(PC) 外部初始化                                         │
│     - TraceForItem() / PrimaryInteract() / ToggleInventory()      │
│     - 内置 DebugHelper 诊断输出                                     │
│  UMIS_EquipmentComponent  → 装备管理                               │
│     - Init(PC, InvComp, Mesh) 统一初始化入口                       │
│     - SetIsProxy(bProxy) 代理模式                                 │
│  UMIS_ItemComponent       → 世界可拾取物品                         │
├─────────────────────────────────────────────────────────────────┤
│                       网络层 (Network Layer)                       │
│  FMIS_InventoryFastArray → FastArray 增量复制                     │
│  RequestEquipSlotClicked / RequestDropItem / RequestConsumeItem   │
│  Server_* (Server RPC) + Multicast_EquipSlotClicked (Multicast)   │
├─────────────────────────────────────────────────────────────────┤
│                        UI 层 (Widget Layer)                        │
│  UMIS_InventoryWidget (库存主界面)                                  │
│  └── UMIS_InventoryGrid (核心网格)                                 │
│      ├── UMIS_GridSlot (格子)                                    │
│      ├── UMIS_SlottedItem (物品图标)                               │
│      ├── UMIS_EquippedGridSlot (装备栏槽位)                        │
│      └── UMIS_EquippedSlottedItem (装备栏物品)                     │
│  UMIS_HoverItem (拖拽跟随)                                         │
│  UMIS_ItemDescription (物品详情)                                    │
│  UMIS_ItemPopUp (右键菜单: 拆分/丢弃/消耗)                          │
│  UMIS_HUDWidget / UMIS_InfoMessage (HUD)                          │
│  UMIS_CharacterDisplay (角色旋转预览)                               │
│  UMIS_Composite / UMIS_Leaf (组合模式 UI)                          │
├─────────────────────────────────────────────────────────────────┤
│                       交互层 (Interaction Layer)                   │
│  IMIS_Highlightable (接口)                                        │
│  ├── UMIS_HighlightableStaticMesh (静态网格体高亮)                  │
│  └── UMIS_HighlightableSkeletalMesh (骨骼网格体高亮)               │
├─────────────────────────────────────────────────────────────────┤
│                       工具层 (Utility Layer)                       │
│  UMIS_InventoryFunctionLibrary (库存业务函数库 + ForEach2D)        │
│  UMIS_WidgetFunctionLibrary (Widget 坐标/尺寸计算)                  │
│  AMIS_ProxyMesh (角色预览代理 Actor)                               │
├─────────────────────────────────────────────────────────────────┤
│                       调试层 (Debug Layer)                         │
│  DebugHelper 插件 (DH_LOG / DH_SCREEN / DH_PRINT 宏)             │
│  - 装备链路: InvComp → EquipComp 全链路诊断                        │
│  - 拾取链路: TryAddItem → HasRoomForItem → Server_AddNewItem     │
│  - 拖拽链路: PickUp → AssignHoverItem → RemoveItemFromGrid       │
└─────────────────────────────────────────────────────────────────┘
```

---

## 三、核心数据流

### 3.1 物品拾取流程 (完整链路)

```
Input Action (在 Character 中绑定)
    │
    ▼
Character::PrimaryInteract()
    │  → InventoryComponent->PrimaryInteract()
    │
    ▼
UMIS_InventoryComponent::PrimaryInteract()
    │  [DebugHelper] DH_SCREEN: "拾取交互 | ThisActor=..."
    │  → ThisActor->FindComponentByClass<UMIS_ItemComponent>()
    │  [DebugHelper] DH_SCREEN: "找到物品组件: ..., 准备拾取"
    │
    ▼
UMIS_InventoryComponent::TryAddItem(ItemComponent)
    │  [DebugHelper] DH_SCREEN: ">>> 进入 TryAddItem"
    │
    ├── 1. InventoryWidget->HasRoomForItem(ItemComponent)
    │      → InventoryGrid::HasRoomForItem()
    │         → 遍历所有 GridSlot, 检查尺寸是否越界
    │         → CheckSlotConstraints() 检查堆叠约束
    │         → 返回 FMIS_SlotAvailabilityResult
    │  [DebugHelper] DH_SCREEN: "HasRoomForItem | 总空间=... | 槽位数=..."
    │
    ├── 2. InventoryList.FindFirstItemByType(ItemType)
    │      → 在 FastArray 中搜索同类型物品
    │
    ├── 3a. 可堆叠 + 已有同类型
    │      [DebugHelper] DH_LOG: "-> 堆叠已有物品"
    │      → OnStackChange.Broadcast()
    │      → Server_AddStacksToItem(ItemActor, Fill, Remainder)
    │         [DebugHelper] DH_SCREEN: "Server_AddStacks 收到"
    │         → Item->SetTotalStackCount(原有 + 新增)
    │         [DebugHelper] DH_PRINT: "堆叠完成, 世界Actor已销毁"
    │
    └── 3b. 新建物品
           [DebugHelper] DH_LOG: "-> 新建物品条目"
           → Server_AddNewItem(ItemActor, StackCount, Remainder)
              [DebugHelper] DH_SCREEN: "Server_AddNewItem 收到"
              → InventoryList.AddEntry(ItemComponent)
                 → ItemComponent->GetItemManifest().Manifest() 创建 UMIS_InventoryItem
                 → 各 Fragment::Manifest() (随机化数值)
              → NewItem->SetTotalStackCount(StackCount)
              [DebugHelper] DH_PRINT: "新物品创建完成"
              → FastArray 自动复制到所有客户端
              → PostReplicatedAdd → OnItemAdded.Broadcast()
              → ItemComponent->PickedUp() 销毁世界 Actor
```

### 3.2 物品拖拽流程

```
SlottedItem::NativeOnMouseButtonDown (左键)
    │
    ▼
InventoryGrid::OnSlottedItemClicked(GridIndex, MouseEvent)
    │  [DebugHelper] DH_SCREEN: "槽位点击 | idx=..."
    │
    ├── 情况1: 无 HoverItem + 左键 → PickUp()
    │      [DebugHelper] ">> 情况1: 拾起物品"
    │      → AssignHoverItem(Item, GridIndex, PreviousIndex)
    │         → CreateWidget<UMIS_HoverItem>
    │         → SetMouseCursorWidget(EMouseCursor::Default, HoverItem)
    │      → RemoveItemFromGrid(Item, GridIndex)
    │         → ForEach2D 清除所有占用格子
    │         → 删除 SlottedItem
    │
    ├── 情况2: 右键 → CreateItemPopUp()
    │      → 创建 UMIS_ItemPopUp (拆分/丢弃/消耗按钮)
    │      → 可堆叠物品还有 Slider 选择拆分数量
    │
    ├── 情况3: 同类型可堆叠物品
    │      → SwapStackCounts() / ConsumeHoverItemStacks() / FillInStack()
    │
    └── 情况4: 不同物品 → SwapWithHoverItem()
    
每帧 Tick:
    → UpdateTileParameters(CanvasPosition, MousePosition)
       → 计算鼠标所在网格坐标 + 象限
    → OnTileParametersUpdated()
       → CheckHoverPosition() 检测目标位置
           ├── bHasSpace = true → 绿色高亮
           ├── 单个 UpperLeftIndex 相同 → 灰色高亮 (可交换)
           └── 多个物品挡路 → 无高亮 (不可放置)
```

### 3.3 装备流程 (完整链路)

```
SlottedItem 被拖至 EquippedGridSlot 点击
    │
    ▼
InventoryGrid::EquippedGridSlotClicked(EquippedSlot, EquipmentTypeTag)
    │  → 校验 HoverItem 的 EquipmentFragment::EquipmentType 是否匹配
    │  → 提取已装备的 ItemToUnequip (如有)
    │
    ▼
UMIS_InventoryComponent::RequestEquipSlotClicked(ItemToEquip, ItemToUnequip)
    │  [DebugHelper] ">>> RequestEquipSlotClicked (Client→Server)"
    │
    ▼
Server_EquipSlotClicked(ItemToEquip, ItemToUnequip)  [Server RPC]
    │  [DebugHelper] ">>> Server_EquipSlotClicked (Server RPC)"
    │
    ▼
Multicast_EquipSlotClicked(ItemToEquip, ItemToUnequip)  [Multicast RPC]
    │  [DebugHelper] ">>> Multicast_EquipSlotClicked"
    │
    ├── OnItemEquipped.Broadcast(ItemToEquip)
    │      │
    │      ▼
    │   UMIS_EquipmentComponent::OnItemEquipped()
    │      [DebugHelper] "========== OnItemEquipped 触发! =========="
    │      │
    │      ├── 步骤1: 验证 HasAuthority() (非服务端跳过)
    │      │
    │      ├── 步骤2: 提取 EquipmentFragment
    │      │   [DebugHelper] "EquipmentFragment获取成功"
    │      │
    │      ├── 步骤3: 非代理模式 → EquipmentFragment->OnEquip(PC)
    │      │   → 遍历 EquipModifiers → 执行 OnEquip (属性修改)
    │      │
    │      └── 步骤4: SpawnAttachedActor(AttachMesh)
    │          → 在骨骼 SocketAttachPoint 上生成 3D 装备 Actor
    │          [DebugHelper] "装备完成! Actor=..."
    │
    └── OnItemUnequipped.Broadcast(ItemToUnequip)
           │
           ▼
        UMIS_EquipmentComponent::OnItemUnequipped()
           ├── EquipmentFragment->OnUnequip(PC) (移除属性修改)
           └── RemoveEquippedActor() (销毁 3D Actor)
```

### 3.4 ProxyMesh 角色预览流程

```
AMIS_ProxyMesh 构造时:
    → DelayedInitializeOwner() (延迟获取引用)
       → 获取 PlayerController 和 Character::GetMesh()
       → 复制骨骼网格体和动画蓝图
       → EquipmentComponent->Init(PC, InvComp, Mesh)
       → EquipmentComponent->SetIsProxy(true)  // 代理模式
       → 代理模式下只生成 3D Actor 显示装备外观, 不执行属性修改

UMIS_CharacterDisplay Widget:
    → 响应鼠标拖拽旋转 ProxyMesh Actor
    → 在库存 UI 中实时预览角色装备效果
```

---

## 四、关键设计特点

### 4.1 外部初始化模式
`UMIS_InventoryComponent` 和 `UMIS_EquipmentComponent` 不依赖 `Owner` 类型，通过外部调用 `Init()` 完成初始化:

```cpp
// 在 Character::NotifyControllerChanged() 中:
EquipmentComponent->Init(PC, InventoryComponent, GetMesh());
```

优势: 可挂在 Character / PlayerController / ProxyMesh 等任意 Actor 上。

### 4.2 完整 DebugHelper 诊断
在所有关键链路内置了 `DH_PRINT` / `DH_SCREEN` / `DH_LOG` 宏:
- 拾取链路: 从 `PrimaryInteract` → `TryAddItem` → `HasRoomForItem` → `Server_AddNewItem`
- 装备链路: 从 `RequestEquipSlotClicked` → `Server_EquipSlotClicked` → `Multicast_EquipSlotClicked` → `OnItemEquipped`
- 拖拽链路: 从 `OnSlottedItemClicked` → `PickUp` → `AssignHoverItem` → `RemoveItemFromGrid`

### 4.3 右键弹出菜单
`UMIS_ItemPopUp` 提供三个操作按钮:
- **拆分** (Split): 可堆叠物品显示 Slider 选择拆分数量
- **丢弃** (Drop): 调用 `InventoryComponent->RequestDropItem()`
- **消耗** (Consume): 调用 `InventoryComponent->RequestConsumeItem()`

### 4.4 双高亮支持
- `UMIS_HighlightableStaticMesh`: 通过 OverlayMaterial 实现静态网格高亮
- `UMIS_HighlightableSkeletalMesh`: 骨骼网格体高亮（Inventory 插件不支持）

### 4.5 完善的文档体系
- `FileStructure.txt`: 文件结构说明
- `ClassView.txt`: 类视图文档
- `DataFlow.txt`: 数据运行流文档
- `UsageGuide.txt`: 使用说明
- `error_log.txt`: 编译错误记录

---

## 五、网络架构

| 组件 | 复制方式 |
|---|---|
| `UMIS_InventoryComponent` | `bReplicateUsingRegisteredSubObjectList = true` |
| `FMIS_InventoryFastArray` | FastArray 增量序列化 (`NetDeltaSerialize`) |
| `UMIS_InventoryItem` | 作为 SubObject 复制 |
| RPC | `Server_*` (Server) + `Multicast_EquipSlotClicked` (Multicast) |
| 客户端请求 | `RequestEquipSlotClicked` / `RequestDropItem` / `RequestConsumeItem` |

FastArray 特殊处理:
- `PreReplicatedRemove` → 广播 `OnItemRemoved`
- `PostReplicatedAdd` → `AddRepSubObj()` 注册子对象 + 广播 `OnItemAdded`
- ListenServer 模式下额外手动广播 `OnItemAdded`

---

## 六、与 Inventory 的差异对比

| 方面 | Inventory (`Inv_`) | MultiplayerInventory (`MIS_`) |
|---|---|---|
| PlayerController | 内置 `AInv_PlayerController` | 无内置，需在 Character 中手动集成 |
| 输入/射线 | PC 每帧 Tick 自动调用 TraceForItem | 需在 Character::Look() 中手动调用 TraceForItem |
| 库存 UI | 分类标签页 (`UInv_SpatialInventory`) | 单网格界面 (`UMIS_InventoryWidget`) |
| 物品分类 | `EInv_ItemCategory` 枚举 | GameplayTag |
| 高亮支持 | 仅 StaticMesh | StaticMesh + SkeletalMesh |
| ProxyMesh | 无 | 有 (`AMIS_ProxyMesh`) |
| 装备组件初始化 | `InitializeOwner(PC)` | `Init(PC, InvComp, Mesh)` 三参数统一入口 |
| 调试输出 | 无 | 全链路内置 DebugHelper 诊断 |
| 文档 | 无 | 完整 4 份文档 |
| 函数库 | `UInv_InventoryStatics` (单文件) | `UMIS_InventoryFunctionLibrary` + `UMIS_WidgetFunctionLibrary` |
| 物品查询 | 无 | `GetAllItems()` / `FindFirstItemByType()` |
| 委托 | 无 `OnInventoryMenuToggled` | 有 `OnInventoryMenuToggled` |