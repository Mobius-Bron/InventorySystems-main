#include "EquipmentManagement/ProxyMesh/OldMIS_ProxyMesh.h"

#include "EquipmentManagement/Components/OldMIS_EquipmentComponent.h"
#include "InventoryManagement/Components/OldMIS_InventoryComponent.h"
#include "GameFramework/Character.h"

AOldMIS_ProxyMesh::AOldMIS_ProxyMesh()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh");
	Mesh->SetupAttachment(RootComponent);

	EquipmentComponent = CreateDefaultSubobject<UOldMIS_EquipmentComponent>("Equipment");
	EquipmentComponent->SetOwningSkeletalMesh(Mesh);
	EquipmentComponent->SetIsProxy(true);
}

void AOldMIS_ProxyMesh::BeginPlay()
{
	Super::BeginPlay();
	DelayedInitializeOwner();
}

void AOldMIS_ProxyMesh::DelayedInitializeOwner()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		DelayedInitialization();
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!IsValid(PC))
	{
		DelayedInitialization();
		return;
	}

	ACharacter* Character = Cast<ACharacter>(PC->GetPawn());
	if (!IsValid(Character))
	{
		DelayedInitialization();
		return;
	}

	USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (!IsValid(CharacterMesh))
	{
		DelayedInitialization();
		return;
	}

	SourceMesh = CharacterMesh;
	Mesh->SetSkeletalMesh(SourceMesh->GetSkeletalMeshAsset());
	Mesh->SetAnimInstanceClass(SourceMesh->GetAnimInstance()->GetClass());

	// 查找 InventoryComponent (可能在 Character 或 PlayerController 上)
	UOldMIS_InventoryComponent* InvComp = Character->FindComponentByClass<UOldMIS_InventoryComponent>();
	if (!IsValid(InvComp))
	{
		InvComp = PC->FindComponentByClass<UOldMIS_InventoryComponent>();
	}

	EquipmentComponent->Init(PC, InvComp);
}

void AOldMIS_ProxyMesh::DelayedInitialization()
{
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &ThisClass::DelayedInitializeOwner);
	GetWorld()->GetTimerManager().SetTimerForNextTick(TimerDelegate);
}
