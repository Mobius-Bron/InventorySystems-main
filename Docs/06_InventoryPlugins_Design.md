# 背包插件群设计文档（v3 — 四种背包 + 独立功能模块）

## 一、总体架构

### 1.1 插件全景

```
                          ItemCore (已有)
                  物品数据 / Fragment / FastArray
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
          ▼                    ▼                    ▼
    ┌──────────┐       ┌──────────────┐      ┌──────────────┐
    │ 四种背包  │       │  独立功能模块  │      │  独立功能模块  │
    │ (选其一)  │       │  (按需组合)    │      │  (按需组合)    │
    └──────────┘       └──────────────┘      └──────────────┘
    │                  │                     │
    │ Sandbox          │ HotbarSystem        │ EquipmentSystem
    │ Inventory        │ (手持物快捷栏)       │ (装备/纸娃娃)
    │ (沙盒背包)        │                     │
    │                  │ RadialMenu          │ ContainerSystem
    │ Survival         │ (轮盘菜单)           │ (外部容器/嵌套)
    │ Inventory        │                     │
    │ (生存背包)        │ SortSystem          │
    │                  │ (排序)              │
    │ RPG              │                     │
    │ Inventory        │ FilterSystem        │
    │ (角色扮演背包)    │ (过滤)              │
    │                  │                     │
    │ Tactical         │ ExpandSystem        │
    │ Inventory        │ (自动扩展)           │
    │ (战术背包)        │                     │
    └──────────┘       │ WeightSystem        │
                       │ (负重)              │
                       │                     │
                       │ TabSystem           │
                       │ (分类标签)           │
                       └──────────────┘
```

### 1.2 核心原则

| 原则 | 说明 |
|------|------|
| **Component/UI 分离** | 数据存储在 Component 中，UI 只负责展示。Component 可被多个 UI 同时引用（如玩家背包 + 箱子面板并排打开） |
| **ItemCore 统一数据层** | 四种背包共用 `FIC_InventoryFastArray`，差异在存储算法和 UI 布局 |
| **功能模块独立** | Hotbar/Equipment/Container/Sort/Filter/Tab/Weight/Expand 各自独立，可跨背包类型组合 |
| **容量模型统一** | 行列+锁定+翻页/滚动 的配置模型适用于所有背包（见 2.0 节） |

---

## 二、通用容量配置模型

四种背包共享此模型，区别仅在于物品是否占据多格。

```cpp
USTRUCT(BlueprintType)
struct FInventoryCapacityConfig
{
    // 每行槽位数（列数）
    UPROPERTY(EditAnywhere) int32 Columns = 9;

    // 总行数（物理上限）
    UPROPERTY(EditAnywhere) int32 TotalRows = 6;

    // 初始解锁行数
    UPROPERTY(EditAnywhere) int32 InitialUnlockedRows = 3;

    // 是否允许解锁更多行
    UPROPERTY(EditAnywhere) bool bCanUnlockRows = true;

    // 每次解锁行数（1 = 逐行解锁，3 = 整页解锁）
    UPROPERTY(EditAnywhere) int32 RowsPerUnlock = 1;

    // 解锁消耗
    UPROPERTY(EditAnywhere) TArray<FInventoryUnlockCost> UnlockCosts;

    // UI 显示：分页 or 滚动
    UPROPERTY(EditAnywhere) EInventoryDisplayMode DisplayMode = EInventoryDisplayMode::Pagination;

    // 每页行数（分页模式）
    UPROPERTY(EditAnywhere) int32 RowsPerPage = 4;

    // 是否可扩展总容量
    UPROPERTY(EditAnywhere) bool bCanExpand = false;

    // 最大扩展行数
    UPROPERTY(EditAnywhere) int32 MaxTotalRows = 20;
};
```

---

## 三、四种背包详细设计

### 3.1 SandboxInventory — 沙盒背包

**参考**: 我的世界、星露谷、泰拉瑞亚、模拟经营类游戏

**定位**: 最通用的背包，覆盖沙盒/生存/模拟经营/建造类游戏。

**核心特征**:
- 一个物品一个格子（SlotStorage）
- 固定行列 + 可解锁 + 可扩展
- UI 分页或滚动
- 手持物快捷栏（1-9 数字键，物品从背包拖入）
- 装备系统（纸娃娃 + 属性面板）
- 外部容器交互（箱子/NPC交易/工作台）

**Component/UI 分离示意**:
```
┌──────────────────────────────────────────────────────┐
│  SandboxInventoryComponent (数据层，可被多人引用)      │
│  ├── FIC_InventoryFastArray ItemList                 │
│  ├── FInventoryCapacityConfig CapacityConfig         │
│  └── FInventoryCapacityState CapacityState           │
│                                                      │
│  被以下 UI 同时引用:                                   │
│  ┌─────────────────┐  ┌─────────────────┐            │
│  │ 玩家背包 UI      │  │ 箱子 UI          │            │
│  │ (下半屏)         │  │ (上半屏)         │            │
│  │ 9x4 网格         │  │ 9x3 网格         │            │
│  └─────────────────┘  └─────────────────┘            │
│  ┌─────────────────┐  ┌─────────────────┐            │
│  │ 装备 UI          │  │ NPC交易 UI       │            │
│  │ 纸娃娃 + 属性    │  │ 买卖面板         │            │
│  └─────────────────┘  └─────────────────┘            │
└──────────────────────────────────────────────────────┘
```

**交互场景**:
```
打开箱子:   玩家背包(下半屏) + 箱子面板(上半屏)
NPC交易:    玩家背包(下半屏) + NPC商店(上半屏)
工作台:     玩家背包(下半屏) + 合成界面(上半屏)
装备界面:   玩家背包(下半屏) + 装备纸娃娃(上半屏)
```

**数据结构**:
```cpp
UCLASS()
class USandboxInventoryComponent : public UActorComponent
{
    // 统一存储
    UPROPERTY(Replicated)
    FIC_InventoryFastArray ItemList{this};

    // 容量配置
    UPROPERTY(EditAnywhere)
    FInventoryCapacityConfig CapacityConfig;

    // 运行时容量状态
    UPROPERTY(Replicated)
    FInventoryCapacityState CapacityState;

    // 尝试解锁下一批行
    UFUNCTION(BlueprintCallable)
    bool TryUnlockRows();

    // 获取指定页的物品（用于 UI 分页显示）
    TArray<UIC_InventoryItem*> GetItemsForPage(int32 PageIndex) const;

    // 获取指定行范围的物品（用于 UI 滚动显示）
    TArray<UIC_InventoryItem*> GetItemsForRowRange(int32 StartRow, int32 EndRow) const;
};
```

**TryAddItem 算法**:
```
1. 先尝试堆叠已有同类物品（遍历已解锁的槽位）
2. 在已解锁的槽位中找第一个空位
3. 如果无空位 + bCanUnlockRows → 提示解锁
4. 如果无空位 + bCanExpand → 自动扩展一行
5. 放入
```

**组合的功能模块**:
- HotbarSystem（手持物 1-9）
- EquipmentSystem（装备纸娃娃）
- ContainerSystem（外部容器）
- ExpandSystem（可选，自动扩展）

---

### 3.2 SurvivalInventory — 生存背包

**参考**: PUBG、DayZ、绝地求生类大逃杀游戏

**定位**: 以负重为核心限制的背包，物品纵向排列，自动紧凑。

**核心特征**:
- 一个物品一个格子（SlotStorage）
- 物品纵向单列或双列排列，自动紧凑不留空位
- 负重限制：超重即无法拾取新物品
- 装备系统（头盔/护甲/背包）
- 背包等级影响负重上限

**UI 布局**:
```
┌──────────────────────────┐
│ 生存背包                  │
│ ┌──────────────────────┐ │
│ │ [物品1]  [物品2]      │ │  ← 双列纵向排列
│ │ [物品3]  [物品4]      │ │     自动紧凑，不留空位
│ │ [物品5]  [物品6]      │ │
│ │ [物品7]  [物品8]      │ │
│ │ ...                   │ │
│ └──────────────────────┘ │
│ ┌──────────────────────┐ │
│ │ ████████████░░░░░░░░ │ │  ← 负重条
│ │ 32.5 / 50.0 kg       │ │
│ │ 状态: 正常            │ │
│ └──────────────────────┘ │
│ ┌──────────────────────┐ │
│ │ 装备栏                │ │
│ │ [头盔] [护甲] [背包]  │ │
│ └──────────────────────┘ │
└──────────────────────────┘
```

**数据结构**:
```cpp
UCLASS()
class USurvivalInventoryComponent : public UActorComponent
{
    UPROPERTY(Replicated)
    FIC_InventoryFastArray ItemList{this};

    // 负重配置
    UPROPERTY(EditAnywhere)
    float MaxWeight = 50.0f;

    UPROPERTY(Replicated)
    float CurrentWeight = 0.0f;

    // 背包等级（影响 MaxWeight）
    UPROPERTY(Replicated)
    int32 BackpackLevel = 1;

    // 每级背包的负重上限
    UPROPERTY(EditAnywhere)
    TArray<float> WeightPerLevel = { 30.0f, 50.0f, 70.0f, 100.0f };

    // 是否允许超重拾取
    UPROPERTY(EditAnywhere)
    bool bAllowOverweightPickup = false;

    // 物品列数（1 或 2）
    UPROPERTY(EditAnywhere)
    int32 Columns = 2;

    // 自动紧凑
    void CompactItems();
};
```

**TryAddItem 算法**:
```
1. 获取物品的 WeightFragment → ItemWeight
2. 计算添加后的总重量 = CurrentWeight + ItemWeight * Count
3. 如果总重量 > MaxWeight:
   - 如果 bAllowOverweightPickup = false → 返回失败
   - 否则标记超重状态，继续
4. 先尝试堆叠已有同类物品
5. 添加物品到 ItemList
6. 触发 CompactItems() 自动排列
7. 更新 CurrentWeight
```

**组合的功能模块**:
- WeightSystem（负重限制）
- EquipmentSystem（头盔/护甲/背包）
- HotbarSystem（1-5 武器/物品快捷栏）
- RadialMenu（手雷轮盘 + 药品轮盘）

---

### 3.3 RPGInventory — 角色扮演背包

**参考**: 原神、崩坏星穹铁道、暗黑破坏神、大多数 RPG

**定位**: 大容量分类背包，按物品类型分标签页，支持排序和动态空行。

**核心特征**:
- 一个物品一个格子（SlotStorage）
- 每页大容量 + 动态空行（可配置预留 N 行空行）
- 多规则排序
- 物品自动紧凑排列
- 装备系统（武器/圣遗物/角色面板）

#### 两种存储模式

RPG 背包标签页有 **两种根本不同的存储模式**，通过 `ERPGStorageMode` 配置：

```
┌─────────────────────────────────────────────────────────┐
│                    RPGInventory                          │
│                                                         │
│  ┌─────────────────────┐    ┌─────────────────────────┐ │
│  │ 模式A: 统一存储       │    │ 模式B: 分容器存储        │ │
│  │ (原神风格)           │    │ (暗黑破坏神风格)          │ │
│  │                     │    │                         │ │
│  │ 所有物品 → 一个数组  │    │ 武器 → 武器容器          │ │
│  │ 标签页 = UI 过滤     │    │ 圣遗物 → 圣遗物容器      │ │
│  │                     │    │ 食物 → 食物容器          │ │
│  │ ItemList             │    │ 材料 → 材料容器          │ │
│  │  ├─ 武器_A          │    │ 剧情 → 剧情容器          │ │
│  │  ├─ 圣遗物_B        │    │                         │ │
│  │  ├─ 食物_C          │    │ 标签页 = 切换到不同容器  │ │
│  │  ├─ 材料_D          │    │ 每个容器有独立容量       │ │
│  │  └─ 剧情_E          │    │                         │ │
│  │                     │    │ 参考: 暗黑4、POE、       │ │
│  │ 参考: 原神、星穹铁道  │    │       现有 Inventory 插件│ │
│  └─────────────────────┘    └─────────────────────────┘ │
│                                                         │
│  两种模式共用同一套 UI 交互（标签栏、排序、动态空行）      │
└─────────────────────────────────────────────────────────┘
```

#### 模式A: 统一存储 + UI 分类过滤

所有物品存入同一个 `FIC_InventoryFastArray`，标签页只是 UI 层的过滤视图。

```
数据层:                          UI 层:
┌──────────────────────┐        ┌──────────────────────┐
│ ItemList (全部物品)    │        │ 武器标签: 筛选 Tag=   │
│ ├─ 武器_A  Tag=Weapon │  ───→  │   Weapon 的物品      │
│ ├─ 武器_B  Tag=Weapon │        │ 显示: 武器_A, 武器_B  │
│ ├─ 圣遗物_C Tag=Relic │        │                      │
│ ├─ 圣遗物_D Tag=Relic │        │ 圣遗物标签: 筛选      │
│ ├─ 食物_E  Tag=Food   │        │   Tag=Relic 的物品   │
│ ├─ 材料_F  Tag=Mat    │        │ 显示: 圣遗物_C, _D    │
│ └─ 材料_G  Tag=Mat    │        │                      │
└──────────────────────┘        │ 全部标签: 不过滤       │
                                │ 显示所有物品           │
                                └──────────────────────┘
```

- 容量共享：所有物品共用一个 `FInventoryCapacityConfig`
- "全部"标签页有意义：显示所有物品，总数 = 各类之和
- 物品跨标签移动：只需修改 `GameplayTag`，数据仍在同一数组
- 排序：在过滤后的子集上排序

#### 模式B: 分容器独立存储

每个标签页对应一个独立的 `FIC_InventoryFastArray`，标签页切换意味着切换到不同容器。

```
数据层:                              UI 层:
┌──────────────────────────┐        ┌──────────────────────┐
│ 武器容器 (ItemList_Weapon) │        │ 武器标签 → 展示       │
│ Capacity: 9x8, 20 件      │  ───→  │ 武器容器的内容        │
│ ├─ 武器_A                 │        │ 当前页: 1/3, 20 件   │
│ └─ 武器_B                 │        │                      │
│                           │        │ 圣遗物标签 → 展示     │
│ 圣遗物容器 (ItemList_Relic)│  ───→  │ 圣遗物容器的内容      │
│ Capacity: 9x8, 45 件      │        │ 当前页: 1/6, 45 件   │
│ ├─ 圣遗物_C               │        │                      │
│ └─ 圣遗物_D               │        │ 全部标签 → 展示       │
│                           │  ───→  │ 合并所有容器的内容    │
│ 食物容器 (ItemList_Food)  │        │ (可选，性能开销大)    │
│ Capacity: 9x6, 8 件       │        │                      │
│ ...                       │        │                      │
└──────────────────────────┘        └──────────────────────┘
```

- 容量独立：每个容器有自己的 `FInventoryCapacityConfig`（武器 9x8，材料 9x6 等）
- "全部"标签页可选：合并所有容器显示，但数据量大时性能开销高
- 物品跨标签移动：需要从一个容器移除，加入另一个容器
- 排序：每个容器独立排序
- 网络同步：每个容器独立 FastArray，互不干扰

**参考现有 Inventory 插件**: 项目中已有的 `Inventory` 插件就是模式B的实现，每个类别（武器/圣遗物/材料等）是独立的 `InventoryComponent`。

#### 数据结构

```cpp
// 存储模式
UENUM(BlueprintType)
enum class ERPGStorageMode : uint8
{
    Unified,   // 模式A: 统一存储 + UI 过滤
    PerTab,    // 模式B: 每个标签页独立容器
};

// 标签页配置（两种模式共用）
USTRUCT(BlueprintType)
struct FRPGTabConfig
{
    UPROPERTY(EditAnywhere)
    FText TabName;

    // 该标签页对应的物品类别（模式A: 过滤用；模式B: 容器绑定用）
    UPROPERTY(EditAnywhere, meta = (Categories = "GameItems"))
    FGameplayTag CategoryTag;

    UPROPERTY(EditAnywhere)
    int32 SortOrder = 0;

    UPROPERTY(EditAnywhere)
    bool bShowItemCount = true;

    // 模式B 专用：该标签页容器的容量配置
    UPROPERTY(EditAnywhere, meta = (EditCondition = "StorageMode == ERPGStorageMode::PerTab"))
    FInventoryCapacityConfig CapacityConfig;
};

UCLASS()
class URPGInventoryComponent : public UActorComponent
{
    // 存储模式
    UPROPERTY(EditAnywhere)
    ERPGStorageMode StorageMode = ERPGStorageMode::Unified;

    // ===== 模式A: 统一存储 =====
    UPROPERTY(Replicated, meta = (EditCondition = "StorageMode == ERPGStorageMode::Unified"))
    FIC_InventoryFastArray ItemList{this};

    UPROPERTY(EditAnywhere, meta = (EditCondition = "StorageMode == ERPGStorageMode::Unified"))
    FInventoryCapacityConfig UnifiedCapacityConfig;

    // ===== 模式B: 分容器存储 =====
    // 每个标签页对应一个独立容器
    UPROPERTY(Replicated, meta = (EditCondition = "StorageMode == ERPGStorageMode::PerTab"))
    TMap<FGameplayTag, FIC_InventoryFastArray> TabContainers;

    // ===== 通用配置 =====
    UPROPERTY(EditAnywhere)
    TArray<FRPGTabConfig> TabConfigs;

    // 预留空行数
    UPROPERTY(EditAnywhere)
    int32 ReservedEmptyRows = 2;

    // 启用"全部"标签页
    UPROPERTY(EditAnywhere)
    bool bEnableAllTab = true;

    // 默认排序规则
    UPROPERTY(EditAnywhere)
    ESortRule DefaultSortRule = ESortRule::ByType;

    // 获取指定标签页 + 指定页的物品
    TArray<UIC_InventoryItem*> GetItemsForTabAndPage(
        const FGameplayTag& TabTag, int32 PageIndex) const;

    // 获取每个标签页的物品数量
    TMap<FGameplayTag, int32> GetTabItemCounts() const;

    // 模式B: 在标签页间移动物品
    bool MoveItemBetweenTabs(UIC_InventoryItem* Item,
        const FGameplayTag& FromTab, const FGameplayTag& ToTab);
};
```

**动态空行逻辑**:
```
// 当前标签页显示的行数 = 物品占满的行数 + 预留空行数
// 但不超过 RowsPerPage

例如: 标签页有 20 件物品，每行 9 格，预留 2 行空行
  物品占满的行数 = ceil(20 / 9) = 3 行
  显示行数 = min(3 + 2, RowsPerPage=4) = 4 行
  → 1 行物品 + 3 行空行（含预留）

如果物品数量变为 36 件:
  物品占满的行数 = ceil(36 / 9) = 4 行
  显示行数 = min(4 + 2, 4) = 4 行
  → 4 行物品，无空行，需翻页
```

#### 模式A vs 模式B 对比

| 维度 | 模式A: 统一存储 | 模式B: 分容器存储 |
|------|---------------|-----------------|
| 数据存储 | 一个 FastArray | 每个标签一个 FastArray |
| 容量 | 共享一个容量配置 | 每个容器独立容量 |
| "全部"标签 | 自然支持，不过滤 | 需合并所有容器（开销大） |
| 跨标签移动 | 修改 Tag 即可 | 移除+加入不同容器 |
| 网络同步 | 一个数组同步 | 多个数组独立同步 |
| 排序 | 过滤后排序 | 各容器独立排序 |
| 典型游戏 | 原神、星穹铁道 | 暗黑4、POE、现有 Inventory 插件 |

**组合的功能模块**:
- TabSystem（分类标签，两种模式下行为不同）
- SortSystem（多规则排序）
- FilterSystem（类型过滤）
- ExpandSystem（自动扩展，预留空行）
- EquipmentSystem（装备/角色面板）

---

### 3.4 TacticalInventory — 战术背包

**参考**: 逃离塔科夫、生化危机4

**定位**: 物品有不同尺寸的格子背包，支持物品旋转，多个可装备的容器。

**核心特征**:
- 物品有不同尺寸（1x1 弹药, 2x3 武器, 3x3 头盔）
- 支持物品旋转（行列数不同时可旋转放入）
- 多个容器：背包/弹挂/口袋/安全箱，每个是独立的存储
- 容器本身是可装备的物品，切换装备后物品留在原容器中
- 每个容器有自己的网格（可能不连续：多个 2x2 区域，或一个 5x5 大区域）
- 嵌套容器：背包里放弹挂
- 容器有物品类型限制（弹挂只能放弹匣和药品）

**多容器架构**:
```
玩家 TacticalInventoryComponent
├── Container: 口袋 (2x2)
│   └── [钥匙] [钱]
├── Container: 弹挂 (4x4, 限弹药+药品)
│   ├── [弹匣] [弹匣] [绷带] [止痛药]
│   ├── [弹匣] [空]   [空]   [空]
│   └── ...
├── Container: 背包_A (5x6)
│   ├── [2x3武器] [1x1弹药] [1x1弹药]
│   ├── [头盔3x3]
│   └── Container: 弹挂_B (3x3, 嵌套在背包内)
│       └── [弹匣] [药品] ...
└── Container: 安全箱 (2x2)
    └── [贵重物品] [钥匙]
```

**容器配置**:
```cpp
// 容器区域定义（支持不连续网格）
USTRUCT(BlueprintType)
struct FTacticalGridRegion
{
    // 区域在容器内的起始位置
    UPROPERTY(EditAnywhere)
    FIntPoint StartPos{0, 0};

    // 区域大小
    UPROPERTY(EditAnywhere)
    FIntPoint RegionSize{2, 2};
};

// 容器配置
USTRUCT(BlueprintType)
struct FTacticalContainerConfig
{
    // 容器总尺寸（包围盒）
    UPROPERTY(EditAnywhere)
    FIntPoint TotalSize{5, 5};

    // 可用区域（如果不连续，定义多个子区域）
    // 如果为空，则整个 TotalSize 都是可用区域
    UPROPERTY(EditAnywhere)
    TArray<FTacticalGridRegion> Regions;

    // 容器类型
    UPROPERTY(EditAnywhere)
    EContainerType ContainerType;  // Pockets, Vest, Backpack, SecureCase

    // 允许放入的物品类型（空 = 全部允许）
    UPROPERTY(EditAnywhere, meta = (Categories = "GameItems"))
    FGameplayTag AllowedItemTag;

    // 禁止放入的物品类型
    UPROPERTY(EditAnywhere)
    TArray<FGameplayTag> BlacklistTags;

    // 是否允许放入其他容器（嵌套）
    UPROPERTY(EditAnywhere)
    bool bCanNest = false;

    // 允许放入的容器最大尺寸
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bCanNest"))
    FIntPoint MaxNestedContainerSize{0, 0};
};

// 容器实例
UCLASS()
class UTacticalContainer : public UObject
{
    UPROPERTY(Replicated)
    FTacticalContainerConfig Config;

    UPROPERTY(Replicated)
    FIC_InventoryFastArray ItemList{nullptr};

    // 占用矩阵（平铺一维数组，True = 被占用）
    TArray<bool> OccupancyGrid;

    // 物品 → 放置位置映射
    TMap<UIC_InventoryItem*, FIntPoint> ItemPositions;

    // 检查指定区域是否可用
    bool CanPlaceItem(const FIntPoint& TopLeft, const FIntPoint& ItemSize, bool bRotated) const;

    // 放置物品
    bool TryPlaceItem(UIC_InventoryItem* Item, const FIntPoint& TopLeft, bool bRotated);

    // 旋转物品（交换宽高）
    void RotateItem(UIC_InventoryItem* Item);
};

UCLASS()
class UTacticalInventoryComponent : public UActorComponent
{
    // 所有容器
    UPROPERTY(Replicated)
    TArray<TObjectPtr<UTacticalContainer>> Containers;

    // 装备槽位（每个槽位绑定一个容器）
    UPROPERTY(Replicated)
    TMap<FGameplayTag, TObjectPtr<UTacticalContainer>> EquippedContainers;

    // 装备容器到槽位
    void EquipContainer(FGameplayTag SlotTag, UTacticalContainer* Container);

    // 卸下容器（物品留在容器中）
    void UnequipContainer(FGameplayTag SlotTag);

    // 切换背包（新背包装备，旧背包保留物品放入仓库）
    void SwapContainer(FGameplayTag SlotTag, UTacticalContainer* NewContainer);
};
```

**物品旋转逻辑**:
```
物品尺寸: 2x3 (宽x高)

正常状态: 占用 2 列 x 3 行
旋转后:   占用 3 列 x 2 行  (交换宽高)

旋转触发: 拖拽时按 R 键，或右键菜单选择旋转

适用场景: 容器宽度不够但高度够时，旋转后可以放入
  例: 容器 3x10，武器 2x4 → 旋转后 4x2，可以放入
```

**容器切换逻辑**:
```
玩家有: 背包_A (5x6) 和 背包_B (4x8)

当前装备背包_A:
  背包_A 中有 [武器] [弹药] [头盔]
  背包_B 在仓库中，里面有 [医疗用品] [食物]

玩家切换到背包_B:
  1. 背包_A 从装备槽卸下
  2. 背包_A 中的物品全部保留在背包_A 内
  3. 背包_B 装备到装备槽
  4. 背包_B 中的物品重新可用
  5. 背包_A 存入仓库/背包列表

物品不会丢失，每个容器独立保存自己的物品。
```

**TryAddItem 算法**:
```
1. 确定目标容器（按优先级: 口袋 > 弹挂 > 背包 > 安全箱）
2. 获取物品的 GridFragment → ItemSize
3. 检查容器类型限制（AllowedItemTag / BlacklistTags）
4. 在可用区域中扫描:
   - 遍历 OccupancyGrid，从左上到右下
   - 检查 CanPlaceItem(位置, ItemSize, 当前旋转状态)
   - 如果失败 + 行列不同 → 尝试旋转后再扫描
5. 如果找到位置 → 标记占用矩阵 + 添加物品
6. 如果本容器无空间 → 尝试下一个容器
7. 全部容器无空间 → 返回失败
```

**组合的功能模块**:
- WeightSystem（负重限制）
- HotbarSystem（1-6 数字键）
- RadialMenu（手雷轮盘 + 药品轮盘）
- EquipmentSystem（头盔/耳机/护甲/弹挂/背包/眼镜）
- ContainerSystem（嵌套容器）

---

## 四、四种背包差异对比

| 维度 | Sandbox | Survival | RPG | Tactical |
|------|---------|----------|-----|----------|
| **存储方式** | SlotStorage | SlotStorage | SlotStorage | GridStorage |
| **物品尺寸** | 1x1 | 1x1 | 1x1 | 可变 (1x1 ~ 5x5) |
| **物品旋转** | 不需要 | 不需要 | 不需要 | 支持 |
| **容量模型** | 行列+锁定+扩展 | 负重上限 | 行列+分页+空行 | 多容器+各自网格 |
| **排列方式** | 固定位置 | 自动紧凑 | 自动紧凑 | 手动放置 |
| **分类方式** | 无 | 无 | 标签页 | 容器限制 |
| **UI 布局** | 网格 + 上半屏交互 | 纵向列表 | 标签页 + 网格 | 多区域网格 |
| **核心限制** | 槽位数量 | 重量 | 槽位数量 | 物理空间 |
| **装备系统** | 纸娃娃 | 头盔/护甲/背包 | 武器/圣遗物 | 全套战术装备 |
| **容器** | 外部箱子 | 无 | 无 | 多容器+嵌套 |
| **典型游戏** | 我的世界/星露谷 | PUBG/DayZ | 原神/暗黑 | 塔科夫/生化4 |

---

## 五、独立功能模块摘要

这些模块可与任意背包类型组合：

| 模块 | 功能 | 依赖 |
|------|------|------|
| **HotbarSystem** | 1-9 数字快捷栏，引用模式 | ItemCore |
| **RadialMenu** | 长按弹出轮盘，扇区选择 | ItemCore |
| **EquipmentSystem** | 纸娃娃预览 + 属性面板 + 拖拽装备 | ItemCore |
| **ContainerSystem** | 外部容器交互 + 嵌套容器 | ItemCore |
| **SortSystem** | 多规则排序（名称/类型/稀有度/数量/重量/价值/时间） | ItemCore |
| **FilterSystem** | 按名称/类型/稀有度过滤 | ItemCore |
| **TabSystem** | 按 GameplayTag 分类标签页 | ItemCore + GameplayTag |
| **WeightSystem** | 负重计算 + 超重惩罚 | ItemCore + FIC_WeightFragment |
| **ExpandSystem** | 自动扩展容量 + 预留空行 | ItemCore |

---

## 六、插件命名与类前缀

| 插件名 | 类前缀 | 说明 |
|--------|--------|------|
| ItemCore | IC_ | 物品核心（已有） |
| SandboxInventory | SBI_ | 沙盒背包 |
| SurvivalInventory | SVI_ | 生存背包 |
| RPGInventory | RPGI_ | 角色扮演背包 |
| TacticalInventory | TCI_ | 战术背包 |
| HotbarSystem | HBS_ | 手持物快捷栏 |
| RadialMenu | RDM_ | 轮盘菜单 |
| EquipmentSystem | EQS_ | 装备系统 |
| ContainerSystem | CNS_ | 容器系统 |
| SortSystem | SRT_ | 排序系统 |
| FilterSystem | FLT_ | 过滤系统 |
| TabSystem | TBS_ | 分类标签 |
| WeightSystem | WGT_ | 负重系统 |
| ExpandSystem | EXP_ | 自动扩展 |

---

## 七、开发建议顺序

| 优先级 | 内容 | 理由 |
|--------|------|------|
| 1 | **SandboxInventory** | 最通用，验证 Component/UI 分离架构，覆盖最多游戏类型 |
| 2 | **HotbarSystem** | Sandbox 和 Survival 都需要 |
| 3 | **EquipmentSystem** | 所有背包都需要 |
| 4 | **ContainerSystem** | Sandbox 和 Tactical 需要 |
| 5 | **SurvivalInventory** | 在此基础上增加负重 |
| 6 | **RPGInventory** | 增加标签/排序 |
| 7 | **TacticalInventory** | 最复杂，多容器+物品旋转+嵌套 |