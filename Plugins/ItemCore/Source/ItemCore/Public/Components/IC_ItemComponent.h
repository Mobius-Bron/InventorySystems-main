// Copyright AmberAeolian. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"

#include "IC_ItemComponent.generated.h"

struct FIC_ItemManifest;

/**
 * 世界可拾取物品组件
 * 挂载在 Actor 上,表示该 Actor 代表一个可拾取的物品
 * 持有 ItemManifest 配置,支持网络复制
 */
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

	UPROPERTY(EditAnywhere, Category = "ItemCore")
	FString PickupMessage;
};