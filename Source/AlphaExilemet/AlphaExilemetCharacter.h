#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlphaExilemetCharacter.generated.h"

// Forward declarations
class AToolBase;
class UCameraComponent;

// -------------------------------------------------------------------------
// DELEGATES (Event Dispatchers for the UI)
// -------------------------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChangedSignature, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolEquippedSignature, AToolBase*, NewTool);

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

	// Timer for handling survival logic
	FTimerHandle SurvivalTimerHandle;

	// The function called every second to manage Oxygen and Health
	UFUNCTION()
	void HandleSurvivalStats();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// -------------------------------------------------------------------------
	// EVENT DISPATCHERS
	// -------------------------------------------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Events")
	FOnStatChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Events")
	FOnStatChangedSignature OnOxygenChanged;

	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Events")
	FOnToolEquippedSignature OnToolEquipped;

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
	// RUNTIME SURVIVAL VARIABLES
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
	bool bIsInSafeZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats")
	float OxygenDrainRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats")
	float OxygenRegenRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats")
	float SuffocationDamageRate;

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
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="AlphaExilemet|Equipment")
	void Equip(AToolBase* NewTool);
	virtual void Equip_Implementation(AToolBase* NewTool);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="AlphaExilemet|Equipment")
	void Unequip();
	virtual void Unequip_Implementation();
	
	// Fires the raycast to interact with terminals/items
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Interaction")
	void TryInteract();
};