#include "ObjectPoolSubsystem.h"
#include "PooledObject.h"
#include "Engine/World.h"

void UObjectPoolSubsystem::Deinitialize()
{
	OnPoolerCleanup.Clear();
	Pools.Empty();
	Super::Deinitialize();
}

void UObjectPoolSubsystem::InitializePool(const TArray<FPooledObjectData>& InPooledObjectData)
{
	PooledObjectData = InPooledObjectData;
	EnsurePoolRoot();

	FActorSpawnParameters SpawnParams;
	for (int32 PoolIndex = 0; PoolIndex < PooledObjectData.Num(); PoolIndex++)
	{
		FSingleObjectPool CurrentPool;
		
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (int32 ObjectIndex = 0; ObjectIndex < PooledObjectData[PoolIndex].PoolSize; ObjectIndex++)
		{
			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(PooledObjectData[PoolIndex].ActorTemplate, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (SpawnedActor)
			{
				SpawnedActor->SetActorLabel(FString::Printf(TEXT("%s_%d"), *PooledObjectData[PoolIndex].ActorName, ObjectIndex));
				
				UPooledObject* PoolComp = NewObject<UPooledObject>(SpawnedActor);
				PoolComp->RegisterComponent();
				SpawnedActor->AddInstanceComponent(PoolComp);
				PoolComp->InitWithSubsystem(this);
				
				CurrentPool.PooledObjects.Add(PoolComp);
				
				SpawnedActor->SetActorHiddenInGame(true);
				SpawnedActor->SetActorEnableCollision(false);
				SpawnedActor->SetActorTickEnabled(false);
				
				if (PoolRoot)
				{
					SpawnedActor->AttachToActor(PoolRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				}
			}
		}
		Pools.Add(CurrentPool);
	}
}

void UObjectPoolSubsystem::Broadcast_PoolerCleanup()
{
	OnPoolerCleanup.Broadcast();
}

AActor* UObjectPoolSubsystem::GetPooledActor(FString Name)
{
	int32 CurrentPoolIndex = -1;
	for (int32 i = 0; i < PooledObjectData.Num(); i++)
	{
		if (PooledObjectData[i].ActorName == Name)
		{
			CurrentPoolIndex = i;
			break;
		}
	}

	if (CurrentPoolIndex == -1) return nullptr;

	int32 FirstAvailable = -1;
	TArray<TObjectPtr<UPooledObject>>& Objects = Pools[CurrentPoolIndex].PooledObjects;

	for (int32 i = 0; i < Objects.Num(); i++)
	{
		if (Objects[i] && !Objects[i]->bIsPoolActive)
		{
			FirstAvailable = i;
			break;
		}
		else if (!Objects[i])
		{
			RegenItem(CurrentPoolIndex, i);
			FirstAvailable = i;
			break;
		}
	}

	if (FirstAvailable >= 0)
	{
		UPooledObject* ToReturn = Objects[FirstAvailable];
		ToReturn->bIsPoolActive = true;
		OnPoolerCleanup.AddUniqueDynamic(ToReturn, &UPooledObject::RecycleSelf);

		AActor* ToReturnActor = ToReturn->GetOwner();
		ToReturnActor->SetActorHiddenInGame(false);
		ToReturnActor->SetActorEnableCollision(true);
		ToReturnActor->SetActorTickEnabled(true);
		ToReturnActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		
		return ToReturnActor;
	}

	if (PooledObjectData[CurrentPoolIndex].bCanGrow)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(PooledObjectData[CurrentPoolIndex].ActorTemplate, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (SpawnedActor)
		{
			SpawnedActor->SetActorLabel(PooledObjectData[CurrentPoolIndex].ActorName + TEXT("_Grown"));
			UPooledObject* PoolComp = NewObject<UPooledObject>(SpawnedActor);
			PoolComp->RegisterComponent();
			SpawnedActor->AddInstanceComponent(PoolComp);
			PoolComp->InitWithSubsystem(this);
			
			Objects.Add(PoolComp);
			PoolComp->bIsPoolActive = true;
			OnPoolerCleanup.AddUniqueDynamic(PoolComp, &UPooledObject::RecycleSelf);
			return SpawnedActor;
		}
	}

	return nullptr;
}

void UObjectPoolSubsystem::RecyclePooledObject(UPooledObject* PoolCompRef)
{
	if (!PoolCompRef) return;

	OnPoolerCleanup.RemoveDynamic(PoolCompRef, &UPooledObject::RecycleSelf);
	PoolCompRef->bIsPoolActive = false;

	AActor* ReturningActor = PoolCompRef->GetOwner();
	if (ReturningActor)
	{
		ReturningActor->SetActorHiddenInGame(true);
		ReturningActor->SetActorEnableCollision(false);
		ReturningActor->SetActorTickEnabled(false);
		
		EnsurePoolRoot();
		if (PoolRoot)
		{
			ReturningActor->AttachToActor(PoolRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}
}

void UObjectPoolSubsystem::RecycleActor(AActor* PooledActor)
{
	if (PooledActor)
	{
		if (UPooledObject* PoolComp = PooledActor->FindComponentByClass<UPooledObject>())
		{
			RecyclePooledObject(PoolComp);
		}
	}
}

void UObjectPoolSubsystem::RegenItem(int32 PoolIndex, int32 PositionIndex)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(PooledObjectData[PoolIndex].ActorTemplate, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (SpawnedActor)
	{
		SpawnedActor->SetActorLabel(PooledObjectData[PoolIndex].ActorName + TEXT("_Regen"));
		UPooledObject* PoolComp = NewObject<UPooledObject>(SpawnedActor);
		PoolComp->RegisterComponent();
		SpawnedActor->AddInstanceComponent(PoolComp);
		PoolComp->InitWithSubsystem(this);

		Pools[PoolIndex].PooledObjects[PositionIndex] = PoolComp;
		
		SpawnedActor->SetActorHiddenInGame(true);
		SpawnedActor->SetActorEnableCollision(false);
		SpawnedActor->SetActorTickEnabled(false);
		
		EnsurePoolRoot();
		if (PoolRoot)
		{
			SpawnedActor->AttachToActor(PoolRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}
}

void UObjectPoolSubsystem::EnsurePoolRoot()
{
	if (!PoolRoot)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = TEXT("ObjectPoolRoot");
		SpawnParams.ObjectFlags |= RF_Transient;
		PoolRoot = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
#if WITH_EDITOR
		PoolRoot->SetActorLabel(TEXT("ObjectPoolRoot"));
#endif
	}
}
