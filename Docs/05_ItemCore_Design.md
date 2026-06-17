# ItemCore 插件设计文档

## 一、插件概述

| 属性 | 值 |
|---|---|
| 名称 | ItemCore |
| 全称 | Item Core System |
| 描述 | 共享物品核心系统，提供物品数据模型、Fragment 片段系统、FastArray 网络同步，供所有背包插件引用 |
| 版本 | 1.0 |
| 模块名 | ItemCore |
| 加载阶段 | Default (Runtime) |
| 类前缀 | `IC_` |
| 日志分类 | `LogItemCore` |
| 依赖 | `Core`, `CoreUObject`, `Engine`, `StructUtils`, `GameplayTags`, `DebugSystem` |

**命名理由**: `ItemCore` = Item Core，直接表达"物品核心"的含义。插件名与模块名一致，遵循 UE 主流命名规范。`IC_` 前缀简洁，在代码中辨识度高。

---

## 二、拆分动机

### 2.1 现状问题

```
当前状态:
┌──────────────────────────────────────────────────────────────┐
│  Inventory 插件                 MultiplayerInventory 插件    │
│  ├── Inv_ItemFragment (基类)     ├── MIS_ItemFragment (基类) │
│  ├── Inv_ItemManifest             ├── MIS_ItemManifest        │
│  ├── UInv_InventoryItem           ├── UMIS_InventoryItem      │
│  ├── UInv_ItemComponent           ├── UMIS_ItemComponent      │
│  ├── Inv_FastArray                ├── MIS_FastArray           │
│  ├── Inv_GridTypes                ├── MIS_GridTypes           │
│  ├── Inv_ItemTags                 ├── MIS_ItemTags            │
│  ├── Inv_FragmentTags             ├── MIS_FragmentTags        │
│  ├── InventoryComponent           ├── InventoryComponent      │
│  ├── EquipmentComponent           ├── EquipmentComponent      │
│  └── UI Widgets                   └── UI Widgets              │
│                                                               │
│  问题: 两套完全重复的代码，各自维护，无法共享                  │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 拆分后架构

```
目标状态:
┌──────────────────────────────────────────────────────────────┐
│                      ItemCore 插件 (公共)                     │
│  ├── FIC_ItemFragment (基类)                                  │
│  ├── FIC_InventoryItemFragment (UI 可同化)                    │
│  ├── FIC_GridFragment / FIC_ImageFragment / FIC_TextFragment  │
│  ├── FIC_LabeledNumberFragment / FIC_StackableFragment        │
│  ├── FIC_ItemManifest (物品配置)                               │
│  ├── UIC_InventoryItem (运行时物品 UObject)                    │
│  ├── UIC_ItemComponent (世界可拾取物品)                        │
│  ├── FIC_InventoryEntry + FIC_InventoryFastArray (网络同步)    │
│  ├── EIC_TileQuadrant / FIC_TileParameters 等 (网格类型)       │
│  └── IC_FragmentTags (片段标签)                                │
├──────────────────────────────────────────────────────────────┤
│              ▲                        ▲                       │
│              │ 引用                    │ 引用                  │
│  ┌───────────┴──────────┐  ┌──────────┴──────────┐           │
│  │ Inventory 插件 (单机)  │  │ M-Inventory 插件     │           │
│  │ + PlayerController    │  │ + ProxyMesh          │           │
│  │ + EquipmentComponent  │  │ + EquipmentComponent  │           │
│  │ + ConsumableFragment  │  │ + ConsumableFragment  │           │
│  │ + UI Widgets          │  │ + UI Widgets          │           │
│  │ + 内置 PC 集成        │  │ + Init() 外部初始化   │           │
│  └───────────────────────┘  └───────────────────────┘          │
│                                                                │
│                        未来扩展:                                │
│          ┌──────────────┐  ┌──────────────┐                   │
│          │ 新背包插件 A  │  │ 新背包插件 B  │  ...              │
│          └──────────────┘  └──────────────┘                   │
└──────────────────────────────────────────────────────────────┘
```

---

## 三、纳入 / 不纳入 ItemCore 的边界

### 3.1 纳入 ItemCore（通用、可共享）

| 类 | 说明 |
|---|---|
| `FIC_ItemFragment` | Fragment 基类，所有物品属性的根 |
| `FIC_InventoryItemFragment` | UI 可同化 Fragment 基类 |
| `FIC_GridFragment` | 网格占用尺寸 |
| `FIC_ImageFragment` | 物品图标 |
| `FIC_TextFragment` | 物品文本 |
| `FIC_LabeledNumberFragment` | 带标签数值（支持随机化） |
| `FIC_StackableFragment` | 可堆叠属性 |
| `FIC_ItemManifest` | 物品配置容器（含模板方法） |
| `UIC_InventoryItem` | 运行时物品 UObject |
| `UIC_ItemComponent` | 世界可拾取物品 ActorComponent |
| `FIC_InventoryEntry` + `FIC_InventoryFastArray` | FastArray 网络同步 |
| `EIC_TileQuadrant` / `FIC_TileParameters` | 网格悬停参数 |
| `FIC_SlotAvailability` / `FIC_SlotAvailabilityResult` | 空间查询结果 |
| `FIC_SpaceQueryResult` | 空间查询结果 |
| `IC_FragmentTags` | Fragment 标签常量 |

### 3.2 不纳入 ItemCore（游戏特定，留在各背包插件）

| 类 | 说明 | 原因 |
|---|---|---|
| `FIC_ConsumableFragment` | 消耗品 | 每个游戏消耗逻辑不同 |
| `FIC_EquipmentFragment` | 装备 | 装备行为紧密耦合于游戏 |
| `FIC_HealthPotionFragment` 等 | 具体效果 | 纯游戏逻辑 |
| `FIC_EquipModifier` / `FIC_ConsumeModifier` | 属性修改器 | 游戏特定 |
| `UIC_InventoryComponent` | 库存 ViewModel | 网格/列表/标签页等 UI 布局不同 |
| `UIC_EquipmentComponent` | 装备管理 | 装备系统差异大 |
| 所有 Widget 类 | UI 层 | 每种背包 UI 风格不同 |
| `AIC_PlayerController` | 内置 PC | 集成方式不同 |
| 高亮组件 | 交互 | 高亮效果不同 |
| ProxyMesh | 预览 | 非核心功能 |

---

## 四、文件结构

```
ItemCore/
├── ItemCore.uplugin
├── Resources/
│   └── Icon128.png
├── Source/ItemCore/
│   ├── ItemCore.Build.cs
│   ├── Public/
│   │   ├── ItemCore.h                          (模块入口 + Log 声明)
│   │   │
│   │   ├── Items/
│   │   │   ├── IC_ItemFragment.h               (Fragment 基类 + 所有核心 Fragment)
│   │   │   ├── IC_ItemManifest.h               (物品配置容器)
│   │   │   ├── IC_InventoryItem.h              (运行时物品 UObject)
│   │   │   └── IC_ItemTags.h                   (Fragment 标签常量)
│   │   │
│   │   ├── Components/
│   │   │   └── IC_ItemComponent.h              (世界可拾取物品)
│   │   │
│   │   ├── FastArray/
│   │   │   └── IC_FastArray.h                  (InventoryEntry + InventoryFastArray)
│   │   │
│   │   └── Types/
│   │       └── IC_GridTypes.h                  (网格枚举 + 结构体)
│   │
│   └── Private/
│       ├── ItemCore.cpp                        (模块 StartupModule)
│       ├── Items/
│       │   ├── IC_ItemFragment.cpp
│       │   ├── IC_ItemManifest.cpp
│       │   ├── IC_InventoryItem.cpp
│       │   └── IC_ItemTags.cpp
│       ├── Components/
│       │   └── IC_ItemComponent.cpp
│       ├── FastArray/
│       │   └── IC_FastArray.cpp
│       └── Types/
│           └── IC_GridTypes.cpp (如果实现不为空)
```

---

## 五、核心类设计

### 5.1 Fragment 片段系统

```cpp
// ===== IC_ItemFragment.h =====

/**
 * 物品片段基类
 * 所有物品属性的根。通过 FragmentTag 标识片段类型。
 * 存储在 FIC_ItemManifest 的 Fragment 列表中，使用 InstancedStruct 多态存储。
 */
USTRUCT(BlueprintType)
struct FIC_ItemFragment
{
    GENERATED_BODY()

    FGameplayTag GetFragmentTag() const { return FragmentTag; }
    void SetFragmentTag(FGameplayTag Tag) { FragmentTag = Tag; }

    /** 初始化 - 创建运行时物品时调用，可用于随机化 */
    virtual void Manifest() {}

private:
    UPROPERTY(EditAnywhere, meta = (Categories = "FragmentTags"))
    FGameplayTag FragmentTag = FGameplayTag::EmptyTag;
};

/**
 * 可同化到 UI 的 Fragment
 * 继承此类表示片段数据可注入到 Composite UI 控件中
 */
USTRUCT(BlueprintType)
struct FIC_InventoryItemFragment : public FIC_ItemFragment
{
    GENERATED_BODY()

    virtual void Assimilate(UIC_CompositeBase* Composite) const;

protected:
    bool MatchesWidgetTag(const UIC_CompositeBase* Composite) const;
};

// 具体 Fragment: Grid/Image/Text/LabeledNumber/Stackable
// (结构与现有 MIS_ 版本一致，仅命名改为 IC_)
```

**继承关系**:
```
FIC_ItemFragment (基类)
├── FIC_InventoryItemFragment (UI 可同化)
│   ├── FIC_ImageFragment         (图标)
│   ├── FIC_TextFragment          (文本)
│   └── FIC_LabeledNumberFragment (带标签数值)
├── FIC_GridFragment              (网格尺寸)
└── FIC_StackableFragment         (可堆叠)

在背包插件中继承:
FIC_InventoryItemFragment
├── FXX_ConsumableFragment        (消耗品)
└── FXX_EquipmentFragment         (装备)
```

### 5.2 物品配置容器

```cpp
// ===== IC_ItemManifest.h =====

USTRUCT(BlueprintType)
struct ITEMCORE_API FIC_ItemManifest
{
    GENERATED_BODY()

    TArray<TInstancedStruct<FIC_ItemFragment>>& GetFragmentsMutable();
    UIC_InventoryItem* Manifest(UObject* NewOuter);
    FGameplayTag GetItemType() const { return ItemType; }

    void AssimilateInventoryFragments(UIC_CompositeBase* Composite) const;

    // 模板方法: GetFragmentOfType / GetFragmentOfTypeWithTag / GetFragmentOfTypeMutable / GetAllFragmentsOfType
    // (与现有 MIS_ 版本一致)

    void SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation);

private:
    UPROPERTY(EditAnywhere, meta = (ExcludeBaseStruct))
    TArray<TInstancedStruct<FIC_ItemFragment>> Fragments;

    UPROPERTY(EditAnywhere, meta = (Categories = "GameItems"))
    FGameplayTag ItemType;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> PickupActorClass;

    void ClearFragments();
};
```

### 5.3 运行时物品 UObject

```cpp
// ===== IC_InventoryItem.h =====

UCLASS()
class ITEMCORE_API UIC_InventoryItem : public UObject
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsSupportedForNetworking() const override { return true; }

    const FIC_ItemManifest& GetItemManifest() const { return ItemManifest.Get<FIC_ItemManifest>(); }
    FIC_ItemManifest& GetItemManifestMutable() { return ItemManifest.GetMutable<FIC_ItemManifest>(); }
    void SetItemManifest(const FIC_ItemManifest& Manifest);

    int32 GetTotalStackCount() const { return TotalStackCount; }
    void SetTotalStackCount(int32 Count) { TotalStackCount = Count; }

    // 便捷查询方法
    bool IsStackable() const;
    FGameplayTag GetItemType() const;

private:
    UPROPERTY(VisibleAnywhere, meta = (BaseStruct = "/Script/ItemCore.IC_ItemManifest"), Replicated)
    FInstancedStruct ItemManifest;

    UPROPERTY(Replicated)
    int32 TotalStackCount{0};
};

// 便捷模板函数
template <typename FragmentType>
const FragmentType* GetFragment(const UIC_InventoryItem* Item, const FGameplayTag& Tag);
```

### 5.4 世界可拾取物品组件

```cpp
// ===== IC_ItemComponent.h =====

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ITEMCORE_API UIC_ItemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UIC_ItemComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitItemManifest(FIC_ItemManifest CopyOfManifest);
    const FIC_ItemManifest& GetItemManifest() const;
    FIC_ItemManifest& GetItemManifestMutable();
    FString GetPickupMessage() const { return PickupMessage; }
    void PickedUp();

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "ItemCore")
    void OnPickedUp();

private:
    UPROPERTY(Replicated, EditAnywhere, meta = (BaseStruct = "/Script/ItemCore.IC_ItemManifest"))
    FInstancedStruct ItemManifest;

    UPROPERTY(EditAnywhere)
    FString PickupMessage;
};
```

### 5.5 FastArray 网络同步

```cpp
// ===== IC_FastArray.h =====

/**
 * 库存条目 - FastArray 中的单个条目
 */
USTRUCT(BlueprintType)
struct FIC_InventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    FIC_InventoryEntry() {}

private:
    friend struct FIC_InventoryFastArray;

    UPROPERTY()
    TObjectPtr<UIC_InventoryItem> Item = nullptr;
};

/**
 * 库存快速数组 - 使用 UE5 FastArray 机制进行网络增量复制
 *
 * 设计要点:
 * - OwnerComponent 支持任意 UActorComponent，通过构造函数或 Init() 设置
 * - 不依赖特定 InventoryComponent 类型
 * - PreReplicatedRemove / PostReplicatedAdd 通过委托通知外部
 */
USTRUCT(BlueprintType)
struct FIC_InventoryFastArray : public FFastArraySerializer
{
    GENERATED_BODY()

    FIC_InventoryFastArray() : OwnerComponent(nullptr) {}
    FIC_InventoryFastArray(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}

    /** 通过 Init() 在任何位置完成初始化 */
    void Init(UActorComponent* InOwnerComponent) { OwnerComponent = InOwnerComponent; }

    /** 获取所有物品 */
    TArray<UIC_InventoryItem*> GetAllItems() const;

    // FFastArraySerializer contract
    void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FastArrayDeltaSerialize<FIC_InventoryEntry, FIC_InventoryFastArray>(Entries, DeltaParams, *this);
    }

    /** 从 ItemComponent 创建新条目 (拾取新物品) */
    UIC_InventoryItem* AddEntry(UIC_ItemComponent* ItemComponent);

    /** 从已有 InventoryItem 添加条目 */
    UIC_InventoryItem* AddEntry(UIC_InventoryItem* Item);

    /** 移除指定物品 */
    void RemoveEntry(UIC_InventoryItem* Item);

    /** 按物品类型查找第一个匹配 (用于堆叠合并) */
    UIC_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType);

    // ─── 委托 (替代原有的对具体 InventoryComponent 的依赖) ───
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemChanged, UIC_InventoryItem*);
    FOnItemChanged OnItemAdded;
    FOnItemChanged OnItemRemoved;

private:
    UPROPERTY()
    TArray<FIC_InventoryEntry> Entries;

    UPROPERTY(NotReplicated)
    TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FIC_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FIC_InventoryFastArray>
{
    enum { WithNetDeltaSerializer = true };
};
```

### 5.6 网格类型

```cpp
// ===== IC_GridTypes.h =====

UENUM(BlueprintType)
enum class EIC_TileQuadrant : uint8
{
    TopLeft, TopRight, BottomLeft, BottomRight, None
};

USTRUCT(BlueprintType)
struct FIC_TileParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FIntPoint TileCoordinats{};

    UPROPERTY(BlueprintReadWrite)
    int32 TileIndex{INDEX_NONE};

    UPROPERTY(BlueprintReadWrite)
    EIC_TileQuadrant TileQuadrant{EIC_TileQuadrant::None};
};

USTRUCT()
struct FIC_SlotAvailability
{
    GENERATED_BODY()

    FIC_SlotAvailability() {}
    FIC_SlotAvailability(int32 ItemIndex, int32 Room, bool bHasItem)
        : Index(ItemIndex), AmountToFill(Room), bItemAtIndex(bHasItem) {}

    int32 Index{INDEX_NONE};
    int32 AmountToFill{0};
    bool bItemAtIndex{false};
};

USTRUCT()
struct FIC_SlotAvailabilityResult
{
    GENERATED_BODY()

    TWeakObjectPtr<UIC_InventoryItem> Item;
    int32 TotalRoomToFill{0};
    int32 Remainder{0};
    bool bStackable{false};
    TArray<FIC_SlotAvailability> SlotAvailabilities;
};

USTRUCT()
struct FIC_SpaceQueryResult
{
    GENERATED_BODY()

    bool bHasSpace{false};
    bool bCanSwap{false};
    int32 SwapIndex{INDEX_NONE};
    FIntPoint SwapDimensions{};
};
```

### 5.7 Fragment 标签常量

```cpp
// ===== IC_ItemTags.h =====

namespace IC_FragmentTags
{
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GridFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(IconFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(StackableFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemNameFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(PrimaryStatFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemTypeFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlavorTextFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SellValueFragment);
    ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(RequiredLevelFragment);

    namespace StatMod
    {
        ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(StatMod_1);
        ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(StatMod_2);
        ITEMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(StatMod_3);
    }
}

// 注意: 具体的物品类型标签 (如 GameItems.Equipment.Weapons.Sword)
// 不放在 ItemCore 中，而是留在各背包插件中定义，因为它们是游戏特定的。
```

---

## 六、调试系统集成

所有 ItemCore 源文件使用 DebugSystem 宏进行调试输出：

```cpp
#include "DS_DebugFunctionLibrary.h"

// 示例:
DS_LOG(Inventory, "ItemManifest::Manifest | ItemType=%s | Fragments=%d",
    *ItemType.ToString(), Fragments.Num());

DS_PRINT(Network, 3.f, DSColors::Cyan,
    "FastArray::PostReplicatedAdd | 新增 %d 个物品", AddedIndices.Num());

DS_SCREEN(Interaction, 2.f, DSColors::Green,
    "ItemComponent::PickedUp | %s", *GetName());
```

对应 DebugSystem 的系统掩码：
- `Inventory` (0x01) - 物品创建/销毁
- `Network` (0x08) - FastArray 同步
- `Interaction` (0x10) - 拾取交互

---

## 七、Build.cs 依赖

```csharp
// ItemCore.Build.cs

PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "StructUtils",        // InstancedStruct
    "GameplayTags",       // GameplayTag
    "DebugSystem",        // DS_* 宏
});

PrivateDependencyModuleNames.AddRange(new string[]
{
    "CoreUObject",
    "Engine",
    "NetCore",            // FFastArraySerializer
});
```

---

## 八、Init() 模式设计

### 8.1 FastArray 的 Init()

```cpp
// 在任何位置初始化 FastArray，不依赖特定组件类型
FIC_InventoryFastArray InventoryList;
InventoryList.Init(this);  // this = 任意 UActorComponent*
```

### 8.2 背包插件的 InventoryComponent 使用方式

```cpp
// 在背包插件中:
class UXX_InventoryComponent : public UActorComponent
{
    void BeginPlay() override
    {
        // FastArray 在声明时已通过 Init() 绑定到 this
        // 无需依赖特定父类
    }

private:
    UPROPERTY(Replicated)
    FIC_InventoryFastArray InventoryList{this};  // 构造函数绑定
};
```

### 8.3 外部初始化约定

所有新背包插件遵循统一约定：
- `InventoryComponent::Init(APlayerController*)` 进行外部初始化
- 不依赖 `Owner` 的具体类型
- 可挂在 Character / PlayerController / ProxyMesh 等任意 Actor

---

## 九、对现有插件的影响

### 9.1 Inventory 插件改造

```cpp
// 之前:
class INVENTORY_API UInv_InventoryComponent : public UActorComponent
{
    UPROPERTY(Replicated)
    FInv_InventoryFastArray InventoryList;  // 自己的 FastArray
};

// 之后:
class INVENTORY_API UInv_InventoryComponent : public UActorComponent
{
    UPROPERTY(Replicated)
    FIC_InventoryFastArray InventoryList{this};  // 使用 ItemCore 的 FastArray
};
```

- `FInv_ItemFragment` → 继承 `FIC_ItemFragment`
- `FInv_ItemManifest` → `FIC_ItemManifest`
- `UInv_InventoryItem` → `UIC_InventoryItem`
- `UInv_ItemComponent` → `UIC_ItemComponent`
- `FInv_InventoryFastArray` → `FIC_InventoryFastArray`

### 9.2 MultiplayerInventory 插件同样改造

- `FMIS_ItemFragment` → 继承 `FIC_ItemFragment`
- `FMIS_ItemManifest` → `FIC_ItemManifest`
- 等等

### 9.3 迁移策略

| 阶段 | 内容 |
|---|---|
| 1 | 创建 ItemCore 插件，实现所有核心类 |
| 2 | 修改 Inventory 插件，引用 ItemCore，替换基类 |
| 3 | 修改 MultiplayerInventory 插件，引用 ItemCore，替换基类 |
| 4 | 删除 Inventory 和 MultiplayerInventory 中已提取的重复代码 |
| 5 | 编译验证 + 功能测试 |

---

## 十、新背包插件开发约定

基于 ItemCore 开发新背包插件时，遵循以下约定：

| 约定 | 说明 |
|---|---|
| 引用 ItemCore | Build.cs 中添加 `"ItemCore"` 到 PublicDependencyModuleNames |
| 使用 DebugSystem | 所有调试用 `DS_*` 宏，系统选 `Inventory` / `UI` / `Network` |
| Init() 模式 | InventoryComponent 通过 `Init(APlayerController*)` 外部初始化 |
| FastArray 绑定 | 在 InventoryComponent 构造时 `InventoryList{this}` 绑定 |
| Fragment 扩展 | 继承 `FIC_ItemFragment` 或 `FIC_InventoryItemFragment` 添加新属性 |
| 多人同步 | 使用 ItemCore 的 FastArray，自动获得增量复制能力 |
| 物品类型标签 | 在各自插件中定义 GameplayTag，不放入 ItemCore |
| 前缀约定 | 新插件使用统一的 2-3 字母前缀 (如 `MIS_`, `XX_`) |

---

## 十一、实施计划

| 阶段 | 内容 | 文件数 |
|---|---|---|
| 1 | 创建 ItemCore 插件骨架 | 3 (uplugin + Build.cs + 模块入口) |
| 2 | 实现 Fragment 系统 | 4 (IC_ItemFragment.h/.cpp + IC_ItemTags.h/.cpp) |
| 3 | 实现 ItemManifest + InventoryItem | 4 (IC_ItemManifest.h/.cpp + IC_InventoryItem.h/.cpp) |
| 4 | 实现 ItemComponent | 2 (IC_ItemComponent.h/.cpp) |
| 5 | 实现 FastArray | 2 (IC_FastArray.h/.cpp) |
| 6 | 实现 GridTypes | 1 (IC_GridTypes.h) |
| 7 | 编译验证 | - |
| 8 | 改造 Inventory 插件引用 ItemCore | 修改现有文件 |
| 9 | 改造 MultiplayerInventory 插件引用 ItemCore | 修改现有文件 |
| 10 | 删除重复代码，最终验证 | - |