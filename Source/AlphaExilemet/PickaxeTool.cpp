// Fill out your copyright notice in the Description page of Project Settings.

#include "PickaxeTool.h"
#include "ResourceBase.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

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
	StartMining();
}
void APickaxeTool::StopUsing_Implementation()
{
	StopMining();
}


/* ----------------------------- */
/*        MINING CONTROL         */
/* ----------------------------- */

void APickaxeTool::StartMining()
{
	GetWorldTimerManager().SetTimer(
		MiningTimer,
		this,
		&APickaxeTool::PerformMiningTrace,
		MiningInterval,
		true
	);
}
void APickaxeTool::StopMining()
{
	GetWorldTimerManager().ClearTimer(MiningTimer);
}



/* ----------------------------- */
/*         MINING TRACE          */
/* ----------------------------- */

void APickaxeTool::PerformMiningTrace()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
		return;
	
	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
		return;
	
	APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
	if (!CameraManager)
		return;
	
	
	
	FVector Start = CameraManager->GetCameraLocation();
	FVector Forward = CameraManager->GetCameraRotation().Vector();
	FVector End = Start + Forward * MiningRange;
	
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
	
	if (bHit)
	{
		AActor* HitActor = Hit.GetActor();

		if (HitActor)
		{
			ApplyMiningDamage(HitActor);
		}
	}
}



/* ----------------------------- */
/*         DAMAGE LOGIC          */
/* ----------------------------- */

void APickaxeTool::ApplyMiningDamage(AActor* Target)
{
	if (!Target)
		return;
	
	AResourceBase* Resource = Cast<AResourceBase>(Target); // Cast to ASolidResource when available
	if (!Resource)
		return;
	
	float Damage = BaseMiningDamage + STR * StrengthScaling;
	
	Resource->ApplyResourceDamage(Damage);
}