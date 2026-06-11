#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PooledObjectData.h"
#include "ObjectPoolSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolerCleanupSignature);

USTRUCT(BlueprintType)
struct FSingleObjectPool
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<TObjectPtr<class UPooledObject>> PooledObjects;
};

UCLASS()
class SPARTAUNREALMASTER_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void InitializePool(const TArray<FPooledObjectData>& InPooledObjectData);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void Broadcast_PoolerCleanup();

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	AActor* GetPooledActor(FString Name);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void RecyclePooledObject(class UPooledObject* PoolCompRef);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void RecycleActor(AActor* PooledActor);

	UPROPERTY()
	FPoolerCleanupSignature OnPoolerCleanup;

protected:
	UPROPERTY()
	TArray<FPooledObjectData> PooledObjectData;

	UPROPERTY()
	TArray<FSingleObjectPool> Pools;

	UPROPERTY()
	TObjectPtr<AActor> PoolRoot;

private:
	void RegenItem(int32 PoolIndex, int32 PositionIndex);
	void EnsurePoolRoot();
};
