#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PooledObject.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTAUNREALMASTER_API UPooledObject : public UActorComponent
{
	GENERATED_BODY()

public:	

	void InitWithSubsystem(class UObjectPoolSubsystem* Owner);

	UFUNCTION(BlueprintCallable)
	void RecycleSelf();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	bool bIsPoolActive;

private:

	TObjectPtr<class UObjectPoolSubsystem> ObjectPoolSubsystem;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
};
