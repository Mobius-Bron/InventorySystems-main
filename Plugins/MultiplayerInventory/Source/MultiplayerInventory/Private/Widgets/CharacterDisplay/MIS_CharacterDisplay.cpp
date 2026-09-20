#include "Widgets/CharacterDisplay/MIS_CharacterDisplay.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "DH_DebugFunctionLibrary.h"
#include "EquipmentManagement/ProxyMesh/MIS_ProxyMesh.h"
#include "Kismet/GameplayStatics.h"

FReply UMIS_CharacterDisplay::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	CurrentPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	LastPosition = CurrentPosition;

	bIsDragging = true;
	return FReply::Handled();
}

FReply UMIS_CharacterDisplay::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	bIsDragging = false;
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UMIS_CharacterDisplay::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bIsDragging = false;
}

void UMIS_CharacterDisplay::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 全场景查找本地 ProxyMesh。客户端上通常只有一个, 取首项即可;
	// 若关卡内存在多个 (例如多份 UI 各自生成), 无法判断归属, 输出告警便于定位。
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, AMIS_ProxyMesh::StaticClass(), Actors);

	if (!Actors.IsValidIndex(0))
	{
		DH_LOG_WARN("[角色预览] 场景中没有 ProxyMesh, 拖拽旋转功能不可用");
		return;
	}

	if (Actors.Num() > 1)
	{
		DH_LOG_WARN("[角色预览] 场景存在 %d 个 ProxyMesh, 当前绑定首项 %s, 可能绑错对象",
			Actors.Num(), *Actors[0]->GetName());
	}

	AMIS_ProxyMesh* ProxyMesh = Cast<AMIS_ProxyMesh>(Actors[0]);
	if (!IsValid(ProxyMesh)) return;

	Mesh = ProxyMesh->GetMesh();
}

void UMIS_CharacterDisplay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bIsDragging) return;

	LastPosition = CurrentPosition;
	CurrentPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	const float HorizontalDelta = LastPosition.X - CurrentPosition.X;

	if (!Mesh.IsValid()) return;
	Mesh->AddRelativeRotation(FRotator(0.f, HorizontalDelta, 0.f));
}
