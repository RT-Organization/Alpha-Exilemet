// Fill out your copyright notice in the Description page of Project Settings.


#include "ToolBase.h"
#include "AlphaExilemetSaveGame.h"
#include "AlphaExilemetCharacter.h"

// Sets default values
AToolBase::AToolBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	
	// Force the default state of the tool to ALWAYS block the interaction laser
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

// Called when the game starts or when spawned
void AToolBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// In ToolBase.cpp
void AToolBase::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	if (Interactor)
	{
		SetActorEnableCollision(false);
		Interactor->AddToolToInventory(this);
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

void AToolBase::ClearInventory(float RetainedFraction)
{
	// Default empty
}

int32 AToolBase::GetToolStatLevel(FName StatName)
{
	if (ToolUpgradeLevels.Contains(StatName))
	{
		return ToolUpgradeLevels[StatName];
	}
	return 0; // If not found, it is Level 0
}

float AToolBase::GetMaxCapacity() const
{
	return 0.0f;
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

void AToolBase::MaterializeItem_Implementation()
{
	// Default empty, we will design the effect in Blueprint
}

void AToolBase::StartMaterialize()
{
	if (!MaterializeMaterial) return;
	
	if (Mesh)
	{
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* Comp : MeshComponents)
	{
		if (Comp)
		{
			FMaterialCache MatCache;
			// Save every material slot on this specific mesh piece
			for (int32 i = 0; i < Comp->GetNumMaterials(); ++i)
			{
				MatCache.Materials.Add(Comp->GetMaterial(i));
				Comp->SetMaterial(i, MaterializeMaterial);
			}
			CachedMaterials.Add(Comp, MatCache);
		}
	}
}

void AToolBase::UpdateMaterialize(float Alpha)
{
	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents);
	
	for (UMeshComponent* Comp : MeshComponents)
	{
		if (Comp)
		{
			// Updates the "Disolve" parameter on all pieces simultaneously
			Comp->SetScalarParameterValueOnMaterials(FName("Disolve"), Alpha);
		}
	}
}

void AToolBase::FinishMaterialize()
{
	for (auto& Pair : CachedMaterials)
	{
		UMeshComponent* Comp = Pair.Key;
		FMaterialCache& MatCache = Pair.Value;

		if (Comp)
		{
			// Restore every material slot to its exact original texture
			for (int32 i = 0; i < MatCache.Materials.Num(); ++i)
			{
				Comp->SetMaterial(i, MatCache.Materials[i]);
			}
		}
	}
	// Clear the memory
	CachedMaterials.Empty();
	
	if (Mesh)
	{
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
}

/* ----------------------------- */
/* SAVE & LOAD                   */
/* ----------------------------- */
void AToolBase::SaveToolData(UAlphaExilemetSaveGame* SaveObject)
{
	if (!SaveObject) return;
	SaveObject->SavedToolUpgrades.Append(ToolUpgradeLevels);
}

void AToolBase::LoadToolData(UAlphaExilemetSaveGame* SaveObject)
{
	if (!SaveObject) return;
	ToolUpgradeLevels = SaveObject->SavedToolUpgrades;
}