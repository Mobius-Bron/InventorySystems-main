#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OldMIS_ProxyMesh.generated.h"

class UOldMIS_EquipmentComponent;
class APlayerController;

UCLASS()
class OLDMULTIPLAYERINVENTORY_API AOldMIS_ProxyMesh : public AActor
{
	GENERATED_BODY()

public:
	AOldMIS_ProxyMesh();
	USkeletalMeshComponent* GetMesh() const { return Mesh; }

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UOldMIS_EquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	FTimerHandle TimerForNextTick;
	void DelayedInitializeOwner();
	void DelayedInitialization();
};
