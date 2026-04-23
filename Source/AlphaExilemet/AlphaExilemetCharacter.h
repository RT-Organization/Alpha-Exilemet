#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlphaExilemetTypes.h"
#include "AlphaExilemetCharacter.generated.h"

class AToolBase;
class UCameraComponent;
class USpringArmComponent;
class USphereComponent;
class UAudioComponent;
class ABaseCamp;

// -------------------------------------------------------------------------
// DELEGATES
// -------------------------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChangedSignature, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolEquippedSignature, AToolBase*, NewTool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolWieldedSignature, int32, ActiveSlotIndex);

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Components")
	UCameraComponent* FirstPersonCameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Components")
	USpringArmComponent* DeathCameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Components")
	UCameraComponent* DeathCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Components")
	USphereComponent* ScannerSphere;

	// -------------------------------------------------------------------------
	// PROGRESSION & STATS CONFIGURATION
	// -------------------------------------------------------------------------
	// Maps the Stat (e.g., Health, Oxygen) to its current level (0-5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Levels")
	TMap<EPlayerStat, int32> SystemUpgradeLevels;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Math")
	FStatProgression HealthProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Math")
	FStatProgression OxygenDrainProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Math")
	FStatProgression AgilityProgression;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Math")
	FStatProgression JumpProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Math")
	FStatProgression SprintMultiplierProgression;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Math")
	FStatProgression GravityScaleProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Progression|Math")
	FStatProgression AirControlProgression;

	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Progression")
	int32 GetSystemStatLevel(EPlayerStat StatName);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void UpgradeStat(EPlayerStat StatToUpgrade);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Progression")
	void RecalculateStats();

	// -------------------------------------------------------------------------
	// RUNTIME SURVIVAL VARIABLES
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	bool bIsSurvivalActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	bool bIsInSafeZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float Oxygen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float MaxOxygen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float OxygenDrainRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float OxygenRegenRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float SuffocationDamageRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Survival")
	float Currency;
	
	// -------------------------------------------------------------------------
	// MOVEMENT & HAZARD PHYSICS
	// -------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Movement")
	bool bIsSprinting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Movement")
	float BaseWalkSpeed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Movement")
	float CurrentSprintMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Movement|Base Boost")
	float MaxAngleForBaseAcceleration = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Movement|Base Boost")
	float SecondsBeforeBaseAccelerationOccurs = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Movement|Base Boost")
	float BaseAccelerationMultiplier = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Movement|Base Boost")
	float TimeSpentMovingTowardsBase = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Movement|Base Boost")
	float CurrentBaseBoostMultiplier = 1.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Movement|Hazards")
	float HazardSpeedMultiplier = 1.0f;

	// Cached physics values to restore when leaving ice
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Movement|Hazards")
	float DefaultGroundFriction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Movement|Hazards")
	float DefaultBrakingDeceleration;

	// -------------------------------------------------------------------------
	// EQUIPMENT & INVENTORY
	// -------------------------------------------------------------------------

	// --- Variables ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Equipment")
	AToolBase* CurrentTool;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Inventory")
	TArray<AToolBase*> OwnedTools;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Inventory")
	int32 ActiveToolIndex = -1;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Inventory")
	int32 PendingToolIndex = -1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaExilemet|Inventory")
	int32 MaxInventorySize = 3;

	// --- Special Items ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Inventory")
	bool bHasSpecialItem = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AlphaExilemet|Inventory")
	ESpecialItem EquippedSpecialItem;

	// --- Core Inventory Functions ---
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void AddToolToInventory(AToolBase* NewTool);

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void StartWieldTool(int32 Index); // use this
	
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void WieldPendingTool();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void HolsterCurrentTool();

	// --- Animation Helper Functions ---
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void SnapPendingToolToHand();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Equipment")
	void SnapCurrentToolToHolster();

	// --- Delegates/Events ---
	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Events")
	FOnInventoryUpdatedSignature OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "AlphaExilemet|Events")
	FOnToolWieldedSignature OnToolWielded;
	
	// -------------------------------------------------------------------------
	// ECONOMY & SELLING
	// -------------------------------------------------------------------------

	// Calculates the final payout with the Molecular Refiner multiplier applied
	UFUNCTION(BlueprintPure, Category = "AlphaExilemet|Economy")
	int32 GetRefinedSellValue(int32 BaseTotalValue);

	// Adds the refined currency to the player's wallet
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Economy")
	void ProcessSale(int32 BaseTotalValue);
	
	// -------------------------------------------------------------------------
	// INTERACTION & SCANNING
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	bool bIsLookingAtInteractable;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Interaction")
	float InteractionDistance;
	
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Interaction")
	void TryInteract();

	UFUNCTION()
	void OnScannerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnScannerOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	// -------------------------------------------------------------------------
	// DEATH & RESPAWN
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Runtime")
	bool bIsDead = false;

	void Die();

	UFUNCTION(BlueprintImplementableEvent, Category = "AlphaExilemet|Events")
	void BP_OnPlayerDied();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Runtime")
	void RespawnPlayer(FVector SpawnLocation, FRotator SpawnRotation);
	
	public:
	// -------------------------------------------------------------------------
	// SAVE & LOAD (DATA EXTRACTION)
	// -------------------------------------------------------------------------

	// Extracts the tool upgrades and inventories from the player's tools and pushes them to the Save Object
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void SaveToolDataToSaveObject(class UAlphaExilemetSaveGame* SaveObject);

	// Injects the tool upgrades and inventories from the Save Object back into the player's tools
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|SaveLoad")
	void LoadToolDataFromSaveObject(class UAlphaExilemetSaveGame* SaveObject);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Audio")
	TEnumAsByte<EPhysicalSurface> SurfaceOverride = SurfaceType_Default;
	
	// -------------------------------------------------------------------------
	// AUDIO & SURVIVAL EFFECTS
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaExilemet|Components")
	UAudioComponent* BreathingAudioComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Audio")
	USoundBase* RecoveryBreathingSound;

	FTimerHandle BreathingFadeTimerHandle;

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Survival")
	void EnterSafeZone();

	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Survival")
	void ExitSafeZone();

	void FadeOutBreathingSound();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Audio")
	USoundBase* DeathSound;

	// The percentage of oxygen (0.0 to 1.0) where vision starts to fade
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Survival|Vignette")
	float OxygenVignetteThreshold = 0.3f; 

	// The maximum intensity of the vignette just before death
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AlphaExilemet|Survival|Vignette")
	float MaxVignetteIntensity = 2.0f;
	
private:
	// Cached reference
	UPROPERTY()
	ABaseCamp* BaseCampRef;
};