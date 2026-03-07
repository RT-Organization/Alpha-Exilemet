#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlphaExilemetCharacter.generated.h"

// Forward declarations
class AToolBase;
class UCameraComponent;

UCLASS()
class ALPHAEXILEMET_API AAlphaExilemetCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAlphaExilemetCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// -------------------------------------------------------------------------
	// COMPONENTS
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Camera")
	UCameraComponent* FirstPersonCameraComponent;

	// -------------------------------------------------------------------------
	// BASE STATS (Levels 0 - 5)
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats", meta = (ClampMin = "0", ClampMax = "5"))
	int32 OxygenLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats", meta = (ClampMin = "0", ClampMax = "5"))
	int32 HealthLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats", meta = (ClampMin = "0", ClampMax = "5"))
	int32 AgilityLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats", meta = (ClampMin = "0", ClampMax = "5"))
	int32 CapacityLevel;

	// -------------------------------------------------------------------------
	// RUNTIME VARIABLES
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Runtime")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Runtime")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Runtime")
	float Oxygen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Runtime")
	float MaxOxygen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Runtime")
	float Currency;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Equipment")
	AToolBase* CurrentTool;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	bool bIsLookingAtInteractable;
	
	// -------------------------------------------------------------------------
	// EXTRA VARIABLES
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	float InteractionDistance;
	
	// -------------------------------------------------------------------------
	// METHODS
	// -------------------------------------------------------------------------
	// Equips a new tool, optionally handling the unequipping of the old one
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void Equip(AToolBase* NewTool);

	// Unequips the currently held tool
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void Unequip();
	
	// Fires the raycast to interact with terminals/items
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Interaction")
	void TryInteract();
};