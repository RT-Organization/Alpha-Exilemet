#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "BaseCamp.generated.h"

class USphereComponent;

// ─────────────────────────────────────────────────────────────────────────────
// Delegates — fired when the player enters or exits the oxygen bubble.
// ShipActor binds to these to drive its open/close animation.
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerEnteredCamp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerExitedCamp);

UCLASS()
class ALPHAEXILEMET_API ABaseCamp : public AActor
{
	GENERATED_BODY()
	
public:
	ABaseCamp();
	
protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// ── COMPONENTS ────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Base|Components")
	USphereComponent* OxygenSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base|Components")
	float OxygenRegenRadius;

	// ── PROXIMITY DELEGATES ───────────────────────────────────────────────────

	/**
	 * Fired when the player's capsule enters the OxygenSphere.
	 * ShipActor binds to this in its BeginPlay to trigger the open animation.
	 * You can also bind to this in BP for any other "player is home" logic.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Base|Events")
	FOnPlayerEnteredCamp OnPlayerEnteredCamp;

	/**
	 * Fired when the player's capsule exits the OxygenSphere.
	 * ShipActor binds to this to trigger the close animation.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Base|Events")
	FOnPlayerExitedCamp OnPlayerExitedCamp;

	// ── SHIP REPAIR PROGRESSION ───────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Ship Repairs")
	TMap<EShipSystem, int32> ShipRepairLevels;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression ScrubberProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression ScannerProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression DampenerProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression RetrieverProgression;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base|Ship Repairs|Progression")
	FStatProgression RefinerProgression;

	UFUNCTION(BlueprintPure, Category = "Base|Ship Repairs")
	int32 GetShipSystemLevel(EShipSystem SystemID);

	UFUNCTION(BlueprintCallable, Category = "Base|Ship Repairs")
	void UpgradeShipSystem(EShipSystem SystemID);

	UFUNCTION(BlueprintCallable, Category = "Base|Ship Repairs")
	void ApplyShipUpgrades();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Base|Ship Repairs")
	void BP_UpdateForcefieldRadius(float NewRadius);
	
	// ── SHOP PROGRESSION / UNLOCKS ─────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Shop Unlocks")
	TArray<EToolType> UnlockedTools;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Shop Unlocks")
	TArray<ESpecialItem> UnlockedSpecialItems;

	UFUNCTION(BlueprintPure, Category = "Base|Shop Unlocks")
	bool IsToolUnlocked(EToolType ToolID);

	UFUNCTION(BlueprintPure, Category = "Base|Shop Unlocks")
	bool IsSpecialItemUnlocked(ESpecialItem ItemID);

	UFUNCTION(BlueprintCallable, Category = "Base|Shop Unlocks")
	void UnlockTool(EToolType ToolID);

	UFUNCTION(BlueprintCallable, Category = "Base|Shop Unlocks")
	void UnlockSpecialItem(ESpecialItem ItemID);

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};