#include "WakeUpDirector.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

AWakeUpDirector::AWakeUpDirector()
{
	PrimaryActorTick.bCanEverTick          = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AWakeUpDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
}

void AWakeUpDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bTransitionActive) TickSmoothTransition(DeltaTime);
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeWakeUp
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::InitializeWakeUp()
{
	UWorld* World = GetWorld();
	if (!World) return;

	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("AWakeUpDirector::InitializeWakeUp — no player pawn."));
		// Safety broadcast so WB_TutorialBlackout doesn't get stuck.
		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error, TEXT("AWakeUpDirector::InitializeWakeUp — no PlayerController."));
		return;
	}

	// No sequence assigned — enable player directly.
	if (!WakeUpSequenceRef)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AWakeUpDirector::InitializeWakeUp — WakeUpSequenceRef null. Enabling player directly."));
		CachedPlayer->bIsSurvivalActive = true;
		FInputModeGameOnly GM; CachedPC->SetInputMode(GM);
		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		BP_OnWakeUpComplete();
		return;
	}

	WakeUpSequencePlayer = WakeUpSequenceRef->GetSequencePlayer();
	if (!WakeUpSequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("AWakeUpDirector — no SequencePlayer on WakeUpSequenceRef."));
		return;
	}

	// Lock input for cutscene duration.
	{ FInputModeUIOnly UIMode; CachedPC->SetInputMode(UIMode); CachedPC->bShowMouseCursor = false; }

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		if (Mv->MovementMode != MOVE_None) Mv->DisableMovement();

	if (WakeUpCineCamRef)
		CachedPC->SetViewTargetWithBlend(WakeUpCineCamRef, 0.0f);

	WakeUpSequencePlayer->OnStop.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpSequenceFinished);

	WakeUpSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: wake-up cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnWakeUpSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnWakeUpSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// Instant path.
	if (WakeUpTransitionBlendTime <= 0.0f)
	{
		OnTransitionComplete();
		return;
	}

	// Smooth path: read where the Sequencer left the camera.
	FVector  SequencerCamLoc = FVector::ZeroVector;
	FRotator SequencerCamRot = FRotator::ZeroRotator;

	if (APlayerCameraManager* CamMgr = CachedPC->PlayerCameraManager)
	{
		SequencerCamLoc = CamMgr->GetCameraLocation();
		SequencerCamRot = CamMgr->GetCameraRotation();
	}
	else if (CachedPlayer->FirstPersonCameraComponent)
	{
		SequencerCamLoc = CachedPlayer->FirstPersonCameraComponent->GetComponentLocation();
		SequencerCamRot = CachedPlayer->FirstPersonCameraComponent->GetComponentRotation();
	}

	// Spawn temp camera at Sequencer's final position.
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TempTransitionCamera = GetWorld()->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(), SequencerCamLoc, SequencerCamRot, P);

	if (!TempTransitionCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWakeUpDirector: failed to spawn TempTransitionCamera. Falling back."));
		OnTransitionComplete();
		return;
	}

	CachedPC->SetViewTargetWithBlend(TempTransitionCamera, 0.0f);

	TransitionStartLocation = SequencerCamLoc;
	TransitionStartRotation = SequencerCamRot;
	TransitionElapsed       = 0.0f;
	bTransitionActive       = true;

	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: smooth transition started. BlendTime=%.2f"),
		WakeUpTransitionBlendTime);
}

// ─────────────────────────────────────────────────────────────────────────────
// TickSmoothTransition
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::TickSmoothTransition(float DeltaTime)
{
	if (!TempTransitionCamera || !CachedPlayer) return;

	// Re-read target every tick.
	FVector  TargetLoc = TransitionStartLocation;
	FRotator TargetRot = TransitionStartRotation;
	if (CachedPlayer->FirstPersonCameraComponent)
	{
		TargetLoc = CachedPlayer->FirstPersonCameraComponent->GetComponentLocation();
		TargetRot = CachedPlayer->FirstPersonCameraComponent->GetComponentRotation();
	}

	TransitionElapsed += DeltaTime;

	float Alpha = (WakeUpTransitionBlendTime > 0.0f)
		? FMath::Clamp(TransitionElapsed / WakeUpTransitionBlendTime, 0.0f, 1.0f)
		: 1.0f;

	float SA = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	TempTransitionCamera->SetActorLocationAndRotation(
		FMath::Lerp(TransitionStartLocation, TargetLoc, SA),
		FMath::Lerp(TransitionStartRotation, TargetRot, SA));

	if (FVector::Dist(TempTransitionCamera->GetActorLocation(), TargetLoc) <= TransitionSnapDistance
		|| Alpha >= 1.0f)
	{
		bTransitionActive = false;
		SetActorTickEnabled(false);
		OnTransitionComplete();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnTransitionComplete
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnTransitionComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// Destroy the temp camera before switching view target.
	if (TempTransitionCamera)
	{
		TempTransitionCamera->Destroy();
		TempTransitionCamera = nullptr;
	}

	// Instant view switch — temp camera was already AT the player camera position.
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);

	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;
	CachedPC->ResetIgnoreMoveInput();

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		Mv->SetMovementMode(MOVE_Walking);

	CachedPlayer->bIsSurvivalActive = true;

	// Signal WB_TutorialBlackout to fade out and remove itself.
	if (UGameInstance* GI = GetGameInstance())
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->OnTutorialPlayerReady.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: wake-up complete. Player has full control."));

	BP_OnWakeUpComplete();
}
