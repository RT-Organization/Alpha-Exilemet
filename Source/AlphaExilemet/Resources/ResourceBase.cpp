// Fill out your copyright notice in the Description page of Project Settings.

#include "ResourceBase.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
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
	
	// Load stats from DT_Resources
	if (ResourceID.DataTable && !ResourceID.RowName.IsNone())
	{
		const FResourceRow* Row = ResourceID.DataTable->FindRow<FResourceRow>(
			ResourceID.RowName,
			TEXT("AResourceBase::BeginPlay — loading resource stats from DataTable")
		);

		if (Row)
		{
			// Override whatever the Blueprint default says
			Health = Row->Health;
			CurrencyValuePerUnit = static_cast<float>(Row->SellValue);
			VeinRegenerationTime = Row->RegenTime;
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("AResourceBase [%s]: Row '%s' not found in DataTable '%s'. Using Blueprint defaults."),
				*GetName(),
				*ResourceID.RowName.ToString(),
				*ResourceID.DataTable->GetName()
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AResourceBase [%s]: ResourceID is not set. Using Blueprint defaults for Health / SellValue."),
			*GetName()
		);
	}

	// Cache initial values AFTER loading from the DataTable
	InitialHealth = Health;
	InitialScale  = GetActorScale3D();
}

bool AResourceBase::ApplyResourceDamage(float DamageAmount)
{
	bool wasDepletedByThisDamage = false;
	
	if (bIsDepleted)
		return wasDepletedByThisDamage;
	
	Health -= DamageAmount;
	
	UpdateScale();
	
	if (Health <= 0.f)
	{
		DepleteResource();
		wasDepletedByThisDamage = true;
		
		GetWorldTimerManager().SetTimer(
			RegenTimer,
			this,
			&AResourceBase::RegenerateResource,
			VeinRegenerationTime,
			false
		);
	}
	
	return wasDepletedByThisDamage;
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
	bIsDepleted = true;
		
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AResourceBase::UpdateScale()
{
	float HealthRatio = Health / InitialHealth;
	
	HealthRatio = FMath::Clamp(HealthRatio, 0.1f, 1.f);
	
	SetActorScale3D(InitialScale * HealthRatio);
}

void AResourceBase::SetOutline(bool bEnable)
{
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);

	for (UStaticMeshComponent* MeshComp : Meshes)
	{
		if (MeshComp)
		{
			MeshComp->SetRenderCustomDepth(bEnable);
			MeshComp->SetCustomDepthStencilValue(1);
		}
	}
}