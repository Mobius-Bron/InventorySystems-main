// Copyright AmberAeolian. All Rights Reserved.

#include "SBI_PlayerCharacter.h"

#include "DS_DebugFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InventoryManagement/Components/SBI_InventoryComponent.h"
#include "EquipmentManagement/Components/SBI_EquipmentComponent.h"
#include "Components/IC_ItemComponent.h"

ASBI_PlayerCharacter::ASBI_PlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	SetReplicates(true);
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	InventoryComponent = CreateDefaultSubobject<USBI_InventoryComponent>(TEXT("InventoryComponent"));
	EquipmentComponent = CreateDefaultSubobject<USBI_EquipmentComponent>(TEXT("EquipmentComponent"));
	FollowCamera->bUsePawnControlRotation = false;

	// 自动拾取范围球体
	PickupRange = CreateDefaultSubobject<USphereComponent>(TEXT("PickupRange"));
	PickupRange->SetupAttachment(RootComponent);
	PickupRange->SetSphereRadius(PickupRadius);
	PickupRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupRange->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	PickupRange->SetHiddenInGame(false);

	PrimaryActorTick.bCanEverTick = true;
}

void ASBI_PlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->Init(PC);
	}
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->Init(PC, InventoryComponent, GetMesh());
	}
}

void ASBI_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASBI_PlayerCharacter::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASBI_PlayerCharacter::Look);
	EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ASBI_PlayerCharacter::ToggleInventory);
}
	
void ASBI_PlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASBI_PlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerPitchInput(-LookAxisVector.Y);
		AddControllerYawInput(LookAxisVector.X);
	}
}

void ASBI_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 仅服务端执行吸附拾取逻辑
	if (HasAuthority())
	{
		AttractAndPickup(DeltaTime);
	}
}

void ASBI_PlayerCharacter::AttractAndPickup(float DeltaTime)
{
	if (!IsValid(InventoryComponent)) return;

	// 拾取碰撞体跟随角色位置
	if (IsValid(PickupRange))
	{
		PickupRange->SetSphereRadius(PickupRadius);
	}

	TArray<AActor*> OverlappingActors;
	PickupRange->GetOverlappingActors(OverlappingActors);

	const FVector PlayerLocation = GetActorLocation();

	for (AActor* OverlapActor : OverlappingActors)
	{
		if (!IsValid(OverlapActor)) continue;

		UIC_ItemComponent* ItemComp = OverlapActor->FindComponentByClass<UIC_ItemComponent>();
		if (!IsValid(ItemComp)) continue;

		const FVector ItemLocation = OverlapActor->GetActorLocation();
		const float Distance = FVector::Dist(PlayerLocation, ItemLocation);

		// 距离足够近 → 直接拾取
		if (Distance <= PickupThreshold)
		{
			DS_LOG(Inventory, "[沙盒自动拾取] 拾取 %s (距离=%.0f)", *OverlapActor->GetName(), Distance);
			InventoryComponent->TryAddItem(ItemComp);
			continue;
		}

		// 吸附: 物品向玩家移动
		const FVector Direction = (PlayerLocation - ItemLocation).GetSafeNormal();
		const FVector NewLocation = ItemLocation + Direction * AttractionSpeed * DeltaTime;

		// 防止超过玩家位置
		const float NewDistance = FVector::Dist(PlayerLocation, NewLocation);
		if (NewDistance <= PickupThreshold)
		{
			OverlapActor->SetActorLocation(PlayerLocation);
		}
		else
		{
			OverlapActor->SetActorLocation(NewLocation);
		}
	}
}

void ASBI_PlayerCharacter::ToggleInventory(const FInputActionValue& Value)
{
	DS_PRINT(Inventory, 2.f, DSColors::Cyan, "[沙盒角色] 按下背包键");

	if (IsValid(InventoryComponent))
	{
		InventoryComponent->ToggleInventory();
	}
	else
	{
		DS_LOG_ERR(Inventory, "[沙盒角色] 错误: InventoryComponent 无效");
	}
}