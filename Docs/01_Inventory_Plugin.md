# Inventory 插件总结文档

## 一、插件概述

| 属性 | 值 |
|---|---|
| 名称 | Inventory |
| 描述 | Inventory System |
| 作者 | Druid Mechanics |
| 版本 | 1.0 |
| 模块名 | Inventory |
| 加载阶段 | Default (Runtime) |
| 类前缀 | `Inv_` |
| 日志分类 | `LogInventory` |
| 依赖 | `StructUtils`, `EnhancedInput` |

**定位**: 单机库存系统，内置 PlayerController，开箱即用。

---

## 二、架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                        数据层 (Data Layer)                        │
│  FInv_ItemFragment (基类) → InstancedStruct 多态存储              │
│  ├── FInv_GridFragment           (网格占用尺寸)                   │
│  ├── FInv_ImageFragment          (物品图标)                       │
│  ├── FInv_TextFragment           (物品文本)                       │
│  ├── FInv_LabeledNumberFragment  (带标签数值,支持随机化)           │
│  ├── FInv_StackableFragment      (可堆叠)                         │
│  ├── FInv_ConsumableFragment     (消耗品,含 ConsumeModifier 数组)  │
│  │   ├── FInv_HealthPotionFragment (生命药水)                     │
│  │   └── FInv_ManaPotionFragment  (法力药水)                      │
│  └── FInv_EquipmentFragment      (装备,含 EquipModifier 数组)     │
│      ├── FInv_StrengthModifier    (力量加成)                      │
│      ├── FInv_ArmorModifier       (护甲加成)                      │
│      └── FInv_DamageModifier      (伤害加成)                      │
│                                                                   │
│  FInv_ItemManifest → 物品配置容器 (持有所有 Fragment)              │
│  UInv_InventoryItem → 运行时物品 UObject (持有 Manifest + 堆叠计数) │
├─────────────────────────────────────────────────────────────────┤
│                       组件层 (Component Layer)                     │
│  UInv_InventoryComponent  → 库存核心 ViewModel (挂在 PlayerController) │
│  UInv_EquipmentComponent  → 装备管理 (挂在 Character)              │
│  UInv_ItemComponent       → 世界可拾取物品 ActorComponent           │
├─────────────────────────────────────────────────────────────────┤
│                       网络层 (Network Layer)                       │
│  FInv_InventoryFastArray → FastArray 增量复制                     │
│  Server_AddNewItem / Server_AddStacksToItem / Server_DropItem     │
│  Server_ConsumeItem / Server_EquipSlotClicked / Multicast_EquipSlotClicked │
├─────────────────────────────────────────────────────────────────┤
│                        UI 层 (Widget Layer)                        │
│  UInv_InventoryBase (抽象基类)                                     │
│  └── UInv_SpatialInventory (分类标签页: 装备/消耗品/可制作)         │
│      ├── UInv_InventoryGrid (网格)                                │
│      │   ├── UInv_GridSlot (格子)                                │
│      │   └── UInv_SlottedItem (物品图标)                          │
│      ├── UInv_EquippedGridSlot (装备栏槽位)                        │
│      └── UInv_EquippedSlottedItem (装备栏物品)                     │
│  UInv_HoverItem (拖拽跟随)                                         │
│  UInv_ItemDescription (物品详情提示)                                │
│  UInv_ItemPopUp (右键菜单: 拆分/丢弃/消耗)                          │
│  UInv_HUDWidget (HUD)                                              │
│  UInv_InfoMessage (消息提示)                                        │
│  UInv_CharacterDisplay (角色旋转预览)                               │
│  UInv_Composite / UInv_Leaf (组合模式 UI)                          │
├─────────────────────────────────────────────────────────────────┤
│                       交互层 (Interaction Layer)                   │
│  IInv_Highlightable (接口)                                        │
│  UInv_HighlightableStaticMesh (仅静态网格体高亮)                    │
├─────────────────────────────────────────────────────────────────┤
│                       工具层 (Utility Layer)                       │
│  UInv_InventoryStatics (蓝图函数库 + ForEach2D)                     │
│  UInv_WidgetUtils (Widget 坐标/尺寸计算)                            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 三、核心数据流

### 3.1 物品拾取流程

```
玩家按下 PrimaryInteract
    │
    ▼
AInv_PlayerController::PrimaryInteract()
    │  → ThisActor->FindComponentByClass<UInv_ItemComponent>()
    │
    ▼
UInv_InventoryComponent::TryAddItem(ItemComponent)
    │
    ├── 1. InventoryMenu->HasRoomForItem(ItemComponent)
    │      → 在网格中搜索可用空间
    │      → 返回 FInv_SlotAvailabilityResult (总空间/各槽位填充量/剩余)
    │
    ├── 2. 查找同类型物品 (用于堆叠)
    │      → InventoryList.FindFirstItemByType(ItemType)
    │
    ├── 3a. 可堆叠 + 已有同类型 → Server_AddStacksToItem(RPC)
    │      → 增加堆叠计数 → OnStackChange.Broadcast()
    │
    └── 3b. 新建物品 → Server_AddNewItem(RPC)
           → InventoryList.AddEntry() 创建 UInv_InventoryItem
           → Manifest() 调用各 Fragment 的 Manifest() (随机化数值)
           → FastArray 自动复制到客户端
           → PostReplicatedAdd → OnItemAdded.Broadcast()
           → ItemComponent->PickedUp() 销毁世界 Actor
```

### 3.2 物品拖拽流程

```
SlottedItem::OnMouseButtonDown (左键)
    │
    ▼
InventoryGrid::OnSlottedItemClicked()
    │
    ├── 情况1: 无 HoverItem + 左键 → PickUp()
    │      → AssignHoverItem() 设置鼠标跟随图标
    │      → RemoveItemFromGrid() 清除网格占用
    │
    ├── 情况2: 右键 → CreateItemPopUp() 弹出菜单
    │
    ├── 情况3: 同类型可堆叠 → SwapStackCounts / FillInStack
    │
    └── 情况4: 不同物品交换 → SwapWithHoverItem()
    
每帧 Tick:
    → UpdateTileParameters() 计算鼠标所在网格坐标
    → CheckHoverPosition() 检测目标位置
        ├── 有空位 → 绿色高亮
        ├── 单个物品挡路 → 灰色高亮(可交换)
        └── 多个物品挡路 → 无高亮(不可放置)
```

### 3.3 装备流程

```
SlottedItem 被拖至 EquippedGridSlot
    │
    ▼
InventoryGrid::EquippedGridSlotClicked()
    │  → 校验装备类型 Tag 匹配
    │
    ▼
InventoryComponent::Server_EquipSlotClicked(ItemToEquip, ItemToUnequip)
    │
    ▼
Multicast_EquipSlotClicked() → 广播到所有客户端
    │
    ├── OnItemEquipped.Broadcast(ItemToEquip)
    │      → EquipmentComponent::OnItemEquipped()
    │         ├── 提取 EquipmentFragment
    │         ├── 非代理模式 → EquipmentFragment->OnEquip(PC) 执行属性修改
    │         └── SpawnAttachedActor() 在骨骼上生成 3D 装备 Actor
    │
    └── OnItemUnequipped.Broadcast(ItemToUnequip)
           → EquipmentComponent::OnItemUnequipped()
              ├── EquipmentFragment->OnUnequip(PC) 移除属性修改
              └── RemoveEquippedActor() 销毁 3D Actor
```

### 3.4 消耗/丢弃流程

```
消耗:
  InventoryComponent::Server_ConsumeItem()
    → 减少堆叠计数 (归零则 RemoveEntry)
    → ConsumableFragment->OnConsume(PC)
       → 遍历 ConsumeModifiers → 执行具体效果 (如 HealthPotion)

丢弃:
  InventoryComponent::Server_DropItem()
    → 减少堆叠计数 (归零则 RemoveEntry)
    → SpawnDroppedItem() 在角色前方随机位置生成世界 Actor
```

---

## 四、关键设计特点

### 4.1 Fragment 数据片段系统
- 物品属性通过 `InstancedStruct` 多态存储，每个 Fragment 是独立的数据单元
- `FInv_ItemManifest` 是物品的完整配置模板，`Manifest()` 方法创建运行时 `UInv_InventoryItem`
- `AssimilateInventoryFragments()` 将数据注入 UI 组合控件

### 4.2 内置 PlayerController
- `AInv_PlayerController` 自带 `TraceForItem()` (每帧 Tick 射线检测)、`PrimaryInteract()`、`ToggleInventory()`
- 输入通过 `EnhancedInput` 绑定，配置 `InputMappingContext` 即可
- 无需额外在 Character 中编写拾取逻辑

### 4.3 分类标签页 UI
- `UInv_SpatialInventory` 使用 `WidgetSwitcher` 实现装备/消耗品/可制作三个标签页
- 每个标签页有独立的 `UInv_InventoryGrid`
- 使用 `EInv_ItemCategory` 枚举分类物品

### 4.4 EquipmentComponent 初始化
- `InitializeOwner(APlayerController*)` 简化初始化
- 通过 `OnPossessedPawnChange` 自动处理角色切换时重新绑定骨骼

---

## 五、网络架构

| 组件 | 复制方式 |
|---|---|
| `UInv_InventoryComponent` | `bReplicateUsingRegisteredSubObjectList = true` |
| `FInv_InventoryFastArray` | FastArray 增量序列化 (`NetDeltaSerialize`) |
| `UInv_InventoryItem` | 作为 SubObject 复制 |
| RPC | `Server_*` (Server RPC) + `Multicast_EquipSlotClicked` (Multicast) |

---

## 六、与 MultiplayerInventory 的主要差异

| 方面 | Inventory | MultiplayerInventory |
|---|---|---|
| PlayerController | 内置 `AInv_PlayerController` | 无内置，需手动集成 |
| UI 结构 | 分类标签页 (`UInv_SpatialInventory`) | 单网格界面 (`UMIS_InventoryWidget`) |
| 物品分类 | `EInv_ItemCategory` 枚举 | GameplayTag |
| 高亮 | 仅 StaticMesh | StaticMesh + SkeletalMesh |
| ProxyMesh | 无 | 有 (`AMIS_ProxyMesh`) |
| 装备组件初始化 | `InitializeOwner(PC)` | `Init(PC, InvComp, Mesh)` |
| 调试输出 | 无 | 内置 DebugHelper 诊断 |
| 文档 | 无 | 完整文档 (FileStructure/ClassView/DataFlow/UsageGuide) |