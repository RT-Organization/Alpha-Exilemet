#include "WakeUpDirector.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/CinematicHandoffComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

AWakeUpDirector::AWakeUpDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	CinematicHandoff = CreateDefaultSubobject<UCinematicHandoffComponent>(TEXT("CinematicHandoff"));
}

void AWakeUpDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
}

void AWakeUpDirector::InitializeWakeUp()
{
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC) return;

	if (!WakeUpSequenceRef)
	{
		CachedPlayer->bIsSurvivalActive = true;
		CachedPC->SetInputMode(FInputModeGameOnly());
		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		BP_OnWakeUpComplete();
		return;
	}

	WakeUpSequencePlayer = WakeUpSequenceRef->GetSequencePlayer();
	if (!WakeUpSequencePlayer) return;

	CachedPlayer->SetActorHiddenInGame(true);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(false, true);

	CachedPC->SetInputMode(FInputModeUIOnly());
	CachedPC->bShowMouseCursor = false;

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		if (Mv->MovementMode != MOVE_None)
			Mv->DisableMovement();

	if (WakeUpCineCamRef)
		CachedPC->SetViewTargetWithBlend(WakeUpCineCamRef, 0.f);

	WakeUpSequencePlayer->OnStop.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpSequenceFinished);

	WakeUpSequencePlayer->Play();

	// Remove the tutorial blackout — the cutscene owns visuals from here.
	if (UGameInstance* GI = GetGameInstance())
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->OnTutorialPlayerReady.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Wake-up cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// FindProxyMeshInSequence — TActorIterator + name hint (no GetBoundObjects)
// ─────────────────────────────────────────────────────────────────────────────

USkeletalMeshComponent* AWakeUpDirector::FindProxyMeshInSequence(
	AActor*& OutProxyActor) const
{
	OutProxyActor = nullptr;
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	const bool bUseHint = !ProxyMeshNameHint.IsEmpty();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || !IsValid(Actor) || Actor == CachedPlayer) continue;
		if (Actor->IsA<AAlphaExilemetCharacter>())              continue;

		if (bUseHint && !Actor->GetName().Contains(ProxyMeshNameHint, ESearchCase::IgnoreCase))
			continue;

		USkeletalMeshComponent* SkMesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
		if (!SkMesh) continue;

		OutProxyActor = Actor;
		UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Found proxy '%s'."), *Actor->GetName());
		return SkMesh;
	}

	if (bUseHint)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || !IsValid(Actor) || Actor == CachedPlayer) continue;
			if (Actor->IsA<AAlphaExilemetCharacter>())              continue;
			USkeletalMeshComponent* SkMesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
			if (!SkMesh) continue;
			OutProxyActor = Actor;
			UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Fallback proxy = '%s'."), *Actor->GetName());
			return SkMesh;
		}
	}

	return nullptr;
}

void AWakeUpDirector::OnWakeUpSequenceFinished()
{
	GetWorldTimerManager().SetTimerForNextTick(
		this, &AWakeUpDirector::OnWakeUpSequenceFinishedDeferred);
}

void AWakeUpDirector::OnWakeUpSequenceFinishedDeferred()
{
	if (!CachedPlayer || !CachedPC) return;

	FVector  GhostStartLoc;
	FRotator GhostStartRot;

	if (AActor* VT = CachedPC->GetViewTarget())
	{
		GhostStartLoc = VT->GetActorLocation();
		GhostStartRot = VT->GetActorRotation();
	}
	else
	{
		GhostStartLoc = CachedPlayer->GetActorLocation();
		GhostStartRot = CachedPlayer->GetActorRotation();
	}

	AActor*                 ProxyActor = nullptr;
	USkeletalMeshComponent* ProxyMesh  = FindProxyMeshInSequence(ProxyActor);

	FVector CameraTarget;
	if (ProxyMesh && ProxyMesh->DoesSocketExist(HeadBoneName))
		CameraTarget = ProxyMesh->GetBoneLocation(HeadBoneName);
	else if (ProxyActor)
		CameraTarget = ProxyActor->GetActorLocation() + FVector(0.f, 0.f, CachedPlayer->BaseEyeHeight);
	else
		CameraTarget = GhostStartLoc;

	FTransform PlayerSpawnTransform;
	if (ProxyMesh && ProxyMesh->DoesSocketExist(RootBoneName))
	{
		FVector  RootLoc  = ProxyMesh->GetBoneLocation(RootBoneName);
		FRotator SpawnRot = FRotator(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);
		PlayerSpawnTransform = FTransform(SpawnRot, RootLoc, FVector::OneVector);
	}
	else if (ProxyActor)
	{
		FRotator SpawnRot = FRotator(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);
		PlayerSpawnTransform = FTransform(SpawnRot, ProxyActor->GetActorLocation());
	}
	else
	{
		PlayerSpawnTransform = CachedPlayer->GetActorTransform();
	}

	CinematicHandoff->OnHandoffComplete.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpHandoffComplete);

	CinematicHandoff->BeginHandoff(
		CachedPlayer, CachedPC,
		GhostStartLoc, GhostStartRot,
		CameraTarget, PlayerSpawnTransform,
		ProxyActor);
}

void AWakeUpDirector::OnWakeUpHandoffComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	CachedPC->SetInputMode(FInputModeGameOnly());
	CachedPC->bShowMouseCursor = false;
	CachedPC->ResetIgnoreMoveInput();

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		Mv->SetMovementMode(MOVE_Walking);

	CachedPlayer->bIsSurvivalActive = true;

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Handoff complete. Survival ON."));
	BP_OnWakeUpComplete();
}
