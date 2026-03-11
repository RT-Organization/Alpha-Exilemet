// Fill out your copyright notice in the Description page of Project Settings.

#include "ResourceBase.h"
#include "TimerManager.h"

AResourceBase::AResourceBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AResourceBase::BeginPlay()
{
	Super::BeginPlay();
	
	// Save initial values
	InitialHealth = Health;
	InitialScale = GetActorScale3D();
}

void AResourceBase::ApplyResourceDamage(float DamageAmount)
{
	if (bIsDepleted)
		return;
	
	Health -= DamageAmount;
	
	UpdateScale();
	
	if (Health <= 0.f)
	{
		DepleteResource();
		
		GetWorldTimerManager().SetTimer(
			RegenTimer,
			this,
			&AResourceBase::RegenerateResource,
			VeinRegenerationTime,
			false
		);
	}
}

void AResourceBase::RegenerateResource()
{
	Health = InitialHealth;
	
	bIsDepleted = false;
	
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	
	SetActorScale3D(FVector(1.f));
}

void AResourceBase::DepleteResource()
{
	// Health = 0 // doesn't matter
	
	bIsDepleted = true;
		
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AResourceBase::UpdateScale()
{
	float HealthRatio = Health / InitialHealth;
	
	HealthRatio = FMath::Clamp(HealthRatio, 0.1f, 1.f);
	
	SetActorScale3D(FVector(HealthRatio));
}