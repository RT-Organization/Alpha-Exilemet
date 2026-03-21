#include "PickaxeTool.h"
#include "ResourceBase.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

APickaxeTool::APickaxeTool()
{
	PrimaryActorTick.bCanEverTick = true;
	
	StrengthProgression.BaseValue = 8.0f;
	StrengthProgression.AdditivePerLevel = 4.0f;

	CapacityProgression.BaseValue = 1.0f;
	CapacityProgression.AdditivePerLevel = 1.0f;

	LuckProgression.BaseValue = 0.0f;
	LuckProgression.AdditivePerLevel = 5.0f;
}

void APickaxeTool::BeginPlay()
{
	Super::BeginPlay();
}

/* ----------------------------- */
/* INPUT               */
/* ----------------------------- */

void APickaxeTool::StartUsing_Implementation()
{
	StartMiningTimer();
}
void APickaxeTool::StopUsing_Implementation()
{
	StopMiningTimer();
}

/* ----------------------------- */
/* MINING CONTROL         */
/* ----------------------------- */

void APickaxeTool::StartMiningTimer()
{
	GetWorldTimerManager().SetTimer(
		MiningTimer,
		this,
		&APickaxeTool::PerformMiningTrace,
		MiningInterval,
		true
	);
}
void APickaxeTool::StopMiningTimer()
{
	GetWorldTimerManager().ClearTimer(MiningTimer);
}

void APickaxeTool::PerformMiningTrace()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (!OwnerCharacter) return;
	}
	
	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC) return;
	
	APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
	if (!CameraManager) return;
	
	FVector Start = CameraManager->GetCameraLocation();
	FVector Forward = CameraManager->GetCameraRotation().Vector();
	FVector End = Start + Forward * MiningRange;
	
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerCharacter);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	
	if (!bHit) return;
	
	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;
	
	ApplyMiningDamage(HitActor);
}

/* ----------------------------- */
/* DAMAGE LOGIC          */
/* ----------------------------- */

void APickaxeTool::ApplyMiningDamage(AActor* Target)
{
	if (!Target) return;
	
	AResourceBase* Resource = Cast<AResourceBase>(Target);
	if (!Resource) return;
	
	float Damage = GetMiningStrength(); 
	
	Resource->ApplyResourceDamage(Damage);
}

/* ----------------------------- */
/* STAT UPGRADES         */
/* ----------------------------- */

void APickaxeTool::UpgradeStat(FName StatName)
{
	Super::UpgradeStat(StatName);

	if (StatName == "Pickaxe_Strength") StrengthLevel++;
	else if (StatName == "Pickaxe_Capacity") CapacityLevel++;
	else if (StatName == "Pickaxe_Luck") LuckLevel++;
}

float APickaxeTool::GetMiningStrength() const
{
	return StrengthProgression.GetValueAtLevel(StrengthLevel);
}

float APickaxeTool::GetMiningLuck() const
{
	return LuckProgression.GetValueAtLevel(LuckLevel);
}

float APickaxeTool::GetMaxCapacity() const
{
	return CapacityProgression.GetValueAtLevel(CapacityLevel);
}