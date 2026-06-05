#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "MIS_EquipmentComponent.generated.h"

struct FGameplayTag;
class UMIS_InventoryComponent;
class UMIS_InventoryItem;
class AMIS_EquipActor;
class APlayerController;
class USkeletalMeshComponent;
struct FMIS_EquipmentFragment;
struct FMIS_ItemManifest;

UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class MULTIPLAYERINVENTORY_API UMIS_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh);
	void SetIsProxy(bool bProxy) { bIsProxy = bProxy; }
	void InitializeOwner(APlayerController* PlayerController);
	void InitComponent(UMIS_InventoryComponent* InInventoryComponent);

	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UMIS_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh;

	UFUNCTION()
	void OnItemEquipped(UMIS_InventoryItem* EquippedItem);

	UFUNCTION()
	void OnItemUnequipped(UMIS_InventoryItem* UnequippedItem);

	void InitPlayerController();
	void InitInventoryComponent();
	AMIS_EquipActor* SpawnEquippedActor(FMIS_EquipmentFragment* EquipmentFragment, const FMIS_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh);

	UPROPERTY()
	TArray<TObjectPtr<AMIS_EquipActor>> EquippedActors;

	AMIS_EquipActor* FindEquippedActor(const FGameplayTag& EquipmentTypeTag);
	void RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag);

	UFUNCTION()
	void OnPossessedPawnChange(APawn* OldPawn, APawn* NewPawn);

	bool bIsProxy{false};
};
