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

	// ── ALARM CONFIG ──────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	FName AlarmLightTag = FName("AlarmLight");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	float AlarmIntensity = 8000.f;

	UFUNCTION(BlueprintCallable, Category = "Alarm")
	void RecacheOriginalLightState() { CacheOriginalLightState(); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Alarm|Config")
	FLinearColor AlarmColor = FLinearColor(1.f, 0.f, 0.f, 1.f);

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

	// ── ANIMATION CONFIG ──────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ship|Animation")
	UAnimMontage* OpenMontage = nullptr;

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

	UFUNCTION(BlueprintImplementableEvent, Category = "Ship|Animation|Events")
	void BP_OnShipOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Ship|Animation|Events")
	void BP_OnShipClosed();

private:
	FTimerHandle             PulseTimerHandle;
	FTimerHandle             SpawnCheckTimerHandle;
	FTimerHandle             MontageEndTimerHandle;
	TArray<ULightComponent*> AlarmLights;
	TArray<float>            OriginalIntensities;
	TArray<FLinearColor>     OriginalColors;

	void CollectAlarmLightsByTag();
	void CacheOriginalLightState();
	void SetAllLightsState(float Intensity, FLinearColor Color);
	void SchedulePulseOff();
	void PulseOn();
	void PulseOff();

	void TryUpdateAnimation();
	void PlayMontage(UAnimMontage* Montage);

	bool IsOurMontage(UAnimMontage* Montage) const;

	// Fired by timer just as the montage reaches its last frame.
	// Updates state and fires BP events. No OnMontageEnded needed.
	void OnMontageReachedEnd();

	void CheckInitialOverlap();

	UPROPERTY()
	ABaseCamp* CachedBaseCamp = nullptr;
	void FindAndBindBaseCamp();

	UFUNCTION() void HandlePlayerEnteredCamp();
	UFUNCTION() void HandlePlayerExitedCamp();
};