// Fill out your copyright notice in the Description page of Project Settings.


#include "PooledObject.h"
#include "ObjectPoolSubsystem.h"



void UPooledObject::InitWithSubsystem(UObjectPoolSubsystem* Owner)
{
	bIsPoolActive = false;
	ObjectPoolSubsystem = Owner;
}

void UPooledObject::RecycleSelf()
{
	if (ObjectPoolSubsystem)
	{
		ObjectPoolSubsystem->RecyclePooledObject(this);
	}
}

void UPooledObject::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (ObjectPoolSubsystem)
	{
		ObjectPoolSubsystem->OnPoolerCleanup.RemoveDynamic(this, &UPooledObject::RecycleSelf);
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

