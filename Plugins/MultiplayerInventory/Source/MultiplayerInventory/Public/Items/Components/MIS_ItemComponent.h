#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"

#include "MIS_ItemComponent.generated.h"

class UMIS_InventoryItem;
struct FMIS_ItemManifest;
struct FMIS_ImageFragment;

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MULTIPLAYERINVENTORY_API UMIS_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIS_ItemComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitItemManifest(FMIS_ItemManifest CopyOfManifest);
	const FMIS_ItemManifest& GetItemManifest() const;
	FMIS_ItemManifest& GetItemManifestMutable();
	FString GetPickupMessage() const { return PickupMessage; }
	void PickedUp();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnPickedUp();

private:
	UPROPERTY(Replicated, EditAnywhere, meta = (BaseStruct = "/Script/MultiplayerInventory.MIS_ItemManifest"))
	FInstancedStruct ItemManifest;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FString PickupMessage;
};
