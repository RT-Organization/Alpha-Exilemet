#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "AlphaExilemet/Interfaces/Interactable.h"
#include "DroppedSolidResource.generated.h"

class AAlphaExilemetCharacter;

UCLASS(Blueprintable)
class ALPHAEXILEMET_API ADroppedSolidResource : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	ADroppedSolidResource();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resource")
	FDataTableRowHandle ResourceID;

public:
	void InitDroppedResource(UStaticMesh* InMesh, FDataTableRowHandle InID);

	virtual void Interact_Implementation(AAlphaExilemetCharacter* Interactor) override;
};