#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlphaExilemetTypes.h"
#include "AlphaExilemetCharacter.generated.h"

class AToolBase;
class UCameraComponent;

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
	// CURRENT UPGRADE LEVELS
	// -------------------------------------------------------------------------
	// Maps the Stat (e.g., Health, Oxygen) to its current level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Levels")
	TMap<EPlayerStat, int32> SystemUpgradeLevels;

	// Helper function to get a stat level safely
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Progression")
	int32 GetSystemStatLevel(EPlayerStat StatName);

	// -------------------------------------------------------------------------
	// STRUCT PROGRESSION CONFIGURATION
	// -------------------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression HealthProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression OxygenDrainProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression AgilityProgression;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression JumpProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Stats|Progression")
	FStatProgression SprintMultiplierProgression;

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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Movement")
	float BaseWalkSpeed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Runtime")
	float CurrentSprintMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Runtime")
	float Currency;
	
	// -------------------------------------------------------------------------
	// EQUIPMENT & INTERACTION
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Equipment")
	AToolBase* CurrentTool;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Inventory")
	TArray<AToolBase*> OwnedTools;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	bool bIsLookingAtInteractable;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	float InteractionDistance;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="AlphaExilemet|Equipment")
	void Equip(AToolBase* NewTool);
	virtual void Equip_Implementation(AToolBase* NewTool);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="AlphaExilemet|Equipment")
	void Unequip();
	virtual void Unequip_Implementation();
	
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Interaction")
	void TryInteract();
	
	// -------------------------------------------------------------------------
	// SPECIAL ITEM INVENTORY
	// -------------------------------------------------------------------------
	
	// True if the player is currently carrying a special item
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Inventory")
	bool bHasSpecialItem = false;

	// Which specific special item are they holding?
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Inventory")
	ESpecialItem EquippedSpecialItem;

	// -------------------------------------------------------------------------
	// UPGRADE SYSTEM METHODS
	// -------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void UpgradeStat(EPlayerStat StatToUpgrade);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void RecalculateStats();
};