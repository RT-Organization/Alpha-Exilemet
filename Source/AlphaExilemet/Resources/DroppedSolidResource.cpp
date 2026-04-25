#include "DroppedSolidResource.h"
#include "AlphaExilemet/Tools/PickaxeTool.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"

ADroppedSolidResource::ADroppedSolidResource()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
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
		Destroy();
	}
}