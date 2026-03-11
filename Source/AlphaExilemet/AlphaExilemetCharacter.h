#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlphaExilemetTypes.h"
#include "AlphaExilemetCharacter.generated.h"

class AToolBase;
class UCameraComponent;

// -------------------------------------------------------------------------
// STRUCTS (Scalable AAA Progression)
// -------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FStatProgression
{
	GENERATED_BODY()

	// The starting value at Level 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression")
	float BaseValue;

	// Flat amount added per level (e.g., +20 Health)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression")
	float AdditivePerLevel;

	// Multiplier applied per level (e.g., 1.1 for +10%, or 0.9 for -10%)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression")
	float MultiplierPerLevel;

	// Default Constructor
	FStatProgression()
	{
		BaseValue = 100.0f;
		AdditivePerLevel = 0.0f;
		MultiplierPerLevel = 1.0f;
	}

	// Helper function to calculate the exact value at any given level
	float GetValueAtLevel(int32 Level) const
	{
		// Formula: (Base + (Additive * Level)) * (Multiplier ^ Level)
		float FlatTotal = BaseValue + (AdditivePerLevel * Level);
		float MultipliedTotal = FlatTotal * FMath::Pow(MultiplierPerLevel, Level);
		return MultipliedTotal;
	}
};

// -------------------------------------------------------------------------
// DELEGATES
// -------------------------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChangedSignature, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolEquippedSignature, AToolBase*, NewTool);

UCLASS()
class ALPHAEXILEMET_API AAlphaExilemetCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAlphaExilemetCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
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
	// CURRENT UPGRADE LEVELS (0 - 5)
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Levels", meta = (ClampMin = "0", ClampMax = "5"))
	int32 OxygenLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Levels", meta = (ClampMin = "0", ClampMax = "5"))
	int32 HealthLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Levels", meta = (ClampMin = "0", ClampMax = "5"))
	int32 AgilityLevel;

	// -------------------------------------------------------------------------
	// STRUCT PROGRESSION CONFIGURATION
	// -------------------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression HealthProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression OxygenDrainProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression AgilityProgression;

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
	
	// -------------------------------------------------------------------------
	// EQUIPMENT & INTERACTION
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Equipment")
	AToolBase* CurrentTool;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	bool bIsLookingAtInteractable;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	float InteractionDistance;
	
	// -------------------------------------------------------------------------
	// METHODS
	// -------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="AlphaExilemet|Equipment")
	void Equip(AToolBase* NewTool);
	virtual void Equip_Implementation(AToolBase* NewTool);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="AlphaExilemet|Equipment")
	void Unequip();
	virtual void Unequip_Implementation();
	
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Interaction")
	void TryInteract();

	// -------------------------------------------------------------------------
	// UPGRADE SYSTEM METHODS
	// -------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void UpgradeStat(EPlayerStat StatToUpgrade);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void RecalculateStats();
};