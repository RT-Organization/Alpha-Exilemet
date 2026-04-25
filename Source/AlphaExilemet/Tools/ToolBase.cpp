// Fill out your copyright notice in the Description page of Project Settings.

#include "ToolBase.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"

AToolBase::AToolBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AToolBase::BeginPlay()
{
	Super::BeginPlay();
}

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

int32 AToolBase::RemoveResource(FName InResourceID, int32 Amount)
{
	return 0;
}

int32 AToolBase::GetResourceAmount(FName InResourceID) const
{
	return 0;
}

TMap<FName, int32> AToolBase::GetAllResources() const
{
	// Base implementation — child classes override this to return their specific inventory map.
	return TMap<FName, int32>();
}

int32 AToolBase::GetToolStatLevel(FName StatName)
{
	if (ToolUpgradeLevels.Contains(StatName))
	{
		return ToolUpgradeLevels[StatName];
	}
	return 0;
}

float AToolBase::GetMaxCapacity() const
{
	return 0.0f;
}

void AToolBase::UpgradeStat(FName StatName)
{
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
	// Default empty, designed in Blueprint
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
			for (int32 i = 0; i < MatCache.Materials.Num(); ++i)
			{
				Comp->SetMaterial(i, MatCache.Materials[i]);
			}
		}
	}
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
