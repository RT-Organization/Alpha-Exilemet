// Fill out your copyright notice in the Description page of Project Settings.


#include "ToolBase.h"
#include "AlphaExilemetCharacter.h"

// Sets default values
AToolBase::AToolBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

// Called when the game starts or when spawned
void AToolBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AToolBase::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	if (Interactor)
	{
		//Add tool to the player's inventory
		Interactor->OwnedTools.AddUnique(this);

		//Equip the tool
		Interactor->Equip(this);

		//Disable world collision so the player doesn't trip over it or interact with it again
		SetActorEnableCollision(false);
	}
}

void AToolBase::StartUsing_Implementation()
{
	// default empty
}

void AToolBase::StopUsing_Implementation()
{
	// default empty
}

int32 AToolBase::GetToolStatLevel(FName StatName)
{
	if (ToolUpgradeLevels.Contains(StatName))
	{
		return ToolUpgradeLevels[StatName];
	}
	return 0; // If not found, it is Level 0
}

void AToolBase::UpgradeStat(FName StatName)
{
	// Add 1 to the level if it exists, otherwise initialize it at Level 1
	if (ToolUpgradeLevels.Contains(StatName))
	{
		ToolUpgradeLevels[StatName]++;
	}
	else
	{
		ToolUpgradeLevels.Add(StatName, 1);
	}
}