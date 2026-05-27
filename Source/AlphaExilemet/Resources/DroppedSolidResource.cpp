#include "DroppedSolidResource.h"

#include "AlphaExilemet/Tools/PickaxeTool.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADroppedSolidResource::ADroppedSolidResource()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Effect(
		TEXT("/Game/AlphaExilemet/VFX/FXS_Warp.FXS_Warp")
	);
	
	if (Effect.Succeeded())
	{
		ObtainEffect = Effect.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<USoundBase> Sound(
		TEXT("/Game/AlphaExilemet/SFX/XFX/Base/Spatial/ElectricWarp.ElectricWarp")
	);

	if (Sound.Succeeded())
	{
		ObtainSound = Sound.Object;
	}
}

void ADroppedSolidResource::InitDroppedResource(UStaticMesh* InMesh, FDataTableRowHandle InID)
{
	ResourceID = InID;

	if (Mesh && InMesh)
	{
		Mesh->SetStaticMesh(InMesh);
	}
}

void ADroppedSolidResource::Interact_Implementation(AAlphaExilemetCharacter* Interactor)
{
	if (!Interactor || !Interactor->CurrentTool) return;

	APickaxeTool* Pickaxe = Cast<APickaxeTool>(Interactor->CurrentTool);
	if (!Pickaxe) return;

	bool bAdded = Pickaxe->TryAddOre(ResourceID);

	if (bAdded)
	{
		if (ObtainEffect)
		{
			UNiagaraComponent* NiagaraComp =
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					ObtainEffect,
					GetActorLocation()
				);

			if (NiagaraComp)
			{
				FDataTableRowHandle RowHandle = ResourceID;

				if (const FResourceRow* Row =
					RowHandle.GetRow<FResourceRow>(TEXT("DroppedSolidResource")))
				{
					FLinearColor EffectColor = Row->PrimaryColor;
					EffectColor.A = 0.25f;

					NiagaraComp->SetVariableLinearColor(
						FName("User.Color"),
						EffectColor
					);
				}
			}
		}
		
		if (ObtainSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				ObtainSound,
				GetActorLocation()
			);
		}
		
		Destroy();
	}
}