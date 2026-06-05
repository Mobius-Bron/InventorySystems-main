#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MIS_ProxyMesh.generated.h"

class UMIS_EquipmentComponent;
class APlayerController;

UCLASS()
class MULTIPLAYERINVENTORY_API AMIS_ProxyMesh : public AActor
{
	GENERATED_BODY()

public:
	AMIS_ProxyMesh();
	USkeletalMeshComponent* GetMesh() const { return Mesh; }

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMIS_EquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	FTimerHandle TimerForNextTick;
	void DelayedInitializeOwner();
	void DelayedInitialization();
};
