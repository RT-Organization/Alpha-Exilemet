#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipActor.generated.h"

class ABaseTerminal;
class ABaseCamp;

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API AShipActor : public AActor
{
	GENERATED_BODY()

public:
	AShipActor();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Mesh")
	USkeletalMeshComponent* ExteriorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Mesh")
	UStaticMeshComponent* InteriorMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ship|Terminals")
	TArray<ABaseTerminal*> Terminals;

	// ── ALARM ────────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	FName AlarmLightTag = FName("AlarmLight");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float AlarmIntensity = 8000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float PulseOnTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float PulseOffTime = 0.45f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alarm|State")
	bool bAlarmActive = false;

	UFUNCTION(BlueprintCallable, Category = "Alarm")
	void StartAlarm();

	UFUNCTION(BlueprintCallable, Category = "Alarm")
	void StopAlarm();

	// ── ANIMATION ─────────────────────────────────────────────────────────────

	/**
	 * Montage that plays when the ship OPENS.
	 * REQUIREMENT: ExteriorMesh AnimBP must have a DefaultSlot node in AnimGraph.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ship|Animation")
	UAnimMontage* OpenMontage = nullptr;

	/** Montage that plays when the ship CLOSES. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ship|Animation")
	UAnimMontage* CloseMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Animation|State")
	bool bShouldBeOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Animation|State")
	bool bCurrentlyOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Animation|State")
	bool bIsAnimationPlaying = false;

	UFUNCTION(BlueprintCallable, Category = "Ship|Animation")
	void OnPlayerEnteredCamp();

	UFUNCTION(BlueprintCallable, Category = "Ship|Animation")
	void OnPlayerExitedCamp();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Ship|Events")
	void BP_RegisterWithGameMode();

	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm|Events")
	void BP_OnAlarmStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm|Events")
	void BP_OnAlarmStopped();

	/**
	 * Ship finished opening. Use for:
	 *   - Enabling ramp collision component
	 *   - Playing door-open SFX
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ship|Animation|Events")
	void BP_OnShipOpened();

	/**
	 * Ship finished closing. Use for:
	 *   - Disabling ramp collision component
	 *   - Playing door-close SFX
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ship|Animation|Events")
	void BP_OnShipClosed();

private:
	// Alarm
	FTimerHandle             PulseTimerHandle;
	TArray<ULightComponent*> AlarmLights;
	TArray<float>            OriginalIntensities;
	void CollectAlarmLightsByTag();
	void CacheOriginalIntensities();
	void PulseOn();
	void PulseOff();
	void SetAllLightsIntensity(float Intensity);

	// Animation
	void TryUpdateAnimation();
	void PlayMontage(UAnimMontage* Montage);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Camp
	UPROPERTY()
	ABaseCamp* CachedBaseCamp = nullptr;
	void FindAndBindBaseCamp();

	UFUNCTION() void HandlePlayerEnteredCamp();
	UFUNCTION() void HandlePlayerExitedCamp();
};