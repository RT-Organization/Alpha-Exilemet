// Fill out your copyright notice in the Description page of Project Settings.

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
}

void APickaxeTool::BeginPlay()
{
	Super::BeginPlay();
}



/* ----------------------------- */
/*           INPUT               */
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
/*        MINING CONTROL         */
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
// TODO: Here or in Blueprints, change timer to play animation, PerformMiningTrace on Animation Event
// (just override StartUsing/StopUsing)


/* ----------------------------- */
/*         MINING TRACE          */
/* ----------------------------- */

void APickaxeTool::PerformMiningTrace()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
		return;
	
	APlayerController* PlrCtrl = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PlrCtrl)
		return;
	
	APlayerCameraManager* CameraManager = PlrCtrl->PlayerCameraManager;
	if (!CameraManager)
		return;
	
	
	
	FVector Start = CameraManager->GetCameraLocation();
	FVector Forward = CameraManager->GetCameraRotation().Vector();
	FVector End = Start + Forward * MiningRange;
	
	// TODO: DELETE Debug
	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		FColor::Green,
		false,
		1.0f,
		0,
		2.0f
	);
	
	FHitResult Hit;
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerCharacter);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);
	
	if (!bHit)
		return;
	
	AActor* HitActor = Hit.GetActor();
	
	if (!HitActor)
		return;
	
	ApplyMiningDamage(HitActor);
}



/* ----------------------------- */
/*         DAMAGE LOGIC          */
/* ----------------------------- */

void APickaxeTool::ApplyMiningDamage(AActor* Target)
{
	if (!Target)
		return;
	
	AResourceBase* Resource = Cast<AResourceBase>(Target);
	
	if (!Resource)
		return;
	
	float Damage = BaseMiningDamage + STR * StrengthScaling;
	
	Resource->ApplyResourceDamage(Damage);
}

/* ----------------------------- */
/*         STAT UPGRADES         */
/* ----------------------------- */

void APickaxeTool::UpgradeStat(FName StatName)
{
	// 1. Call the parent function so ToolBase saves the level internally
	Super::UpgradeStat(StatName);

	// 2. Add your buffs here!
	if (StatName == "Pickaxe_Force")
	{
		STR += 1;
	}
	else if (StatName == "Pickaxe_Fortune")
	{
		LU += 1;
	}
	else if (StatName == "Pickaxe_Capacity")
	{
		CAP += 1;
	}
}