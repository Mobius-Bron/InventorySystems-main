#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"

#include "OldMIS_ItemComponent.generated.h"

class UOldMIS_InventoryItem;
struct FOldMIS_ItemManifest;
struct FOldMIS_ImageFragment;

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class OLDMULTIPLAYERINVENTORY_API UOldMIS_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOldMIS_ItemComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitItemManifest(FOldMIS_ItemManifest CopyOfManifest);
	const FOldMIS_ItemManifest& GetItemManifest() const;
	FOldMIS_ItemManifest& GetItemManifestMutable();
	FString GetPickupMessage() const { return PickupMessage; }
	void PickedUp();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnPickedUp();

private:
	UPROPERTY(Replicated, EditAnywhere, meta = (BaseStruct = "/Script/OldMultiplayerInventory.OldMIS_ItemManifest"))
	FInstancedStruct ItemManifest;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FString PickupMessage;
};
