#include "WakeUpDirector.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/ShipActor.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

AWakeUpDirector::AWakeUpDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWakeUpDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeWakeUp
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::InitializeWakeUp()
{
	// ── 1. CACHE PLAYER ──────────────────────────────────────────────────────
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AWakeUpDirector::InitializeWakeUp — Player not found. "
			     "Broadcasting OnTutorialPlayerReady as fallback."));

		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AWakeUpDirector::InitializeWakeUp — PlayerController not found."));
		return;
	}

	// ── 2. VALIDATE SEQUENCE ─────────────────────────────────────────────────
	if (!WakeUpSequenceRef)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AWakeUpDirector::InitializeWakeUp — WakeUpSequenceRef is null. "
			     "Enabling player directly."));

		CachedPlayer->bIsSurvivalActive = true;
		CachedPC->SetInputMode(FInputModeGameOnly());

		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();

		BP_OnWakeUpComplete();
		return;
	}

	WakeUpSequencePlayer = WakeUpSequenceRef->GetSequencePlayer();
	if (!WakeUpSequencePlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AWakeUpDirector — WakeUpSequenceRef has no SequencePlayer."));
		return;
	}

	// ── 3. HIDE PLAYER FOR CUTSCENE ──────────────────────────────────────────
	CachedPlayer->SetActorHiddenInGame(true);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(false, true);

	// ── 4. LOCK INPUT ────────────────────────────────────────────────────────
	// Player arrives from Tutorial with UI-only input — keep it locked.
	{
		FInputModeUIOnly UIMode;
		CachedPC->SetInputMode(UIMode);
		CachedPC->bShowMouseCursor = false;
	}

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		if (Mv->MovementMode != MOVE_None)
			Mv->DisableMovement();

	// ── 5. OPTIONAL PRE-SEQUENCE CINECAM ─────────────────────────────────────
	if (WakeUpCineCamRef)
		CachedPC->SetViewTargetWithBlend(WakeUpCineCamRef, 0.f);

	// ── 6. BIND + PLAY ───────────────────────────────────────────────────────
	WakeUpSequencePlayer->OnStop.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpSequenceFinished);

	WakeUpSequencePlayer->Play();

	// Dismiss the WB_TutorialBlackout — the cutscene now owns the visuals.
	if (UGameInstance* GI = GetGameInstance())
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->OnTutorialPlayerReady.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Wake-up cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnWakeUpSequenceFinished — deferred by one tick
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnWakeUpSequenceFinished()
{
	UE_LOG(LogTemp, Log,
		TEXT("AWakeUpDirector: Sequence ended. Deferring one tick."));

	GetWorldTimerManager().SetTimerForNextTick(
		this, &AWakeUpDirector::OnWakeUpSequenceFinishedDeferred);
}

void AWakeUpDirector::OnWakeUpSequenceFinishedDeferred()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. REVEAL SHIP ────────────────────────────────────────────────────────
	RevealPersistentShip();

	// ── 2. UNHIDE PLAYER ──────────────────────────────────────────────────────
	CachedPlayer->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(true, true);

	// ── 3. CLEAR BLACK BARS ───────────────────────────────────────────────────
	if (UCameraComponent* FPCam = CachedPlayer->FindComponentByClass<UCameraComponent>())
	{
		FPCam->bConstrainAspectRatio = false;
		FPCam->PostProcessSettings.bOverride_VignetteIntensity = false;
	}

	// ── 4. RESTORE MOVEMENT ───────────────────────────────────────────────────
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		if (Mv->MovementMode == MOVE_None)
			Mv->SetMovementMode(MOVE_Walking);

	// ── 5. RETURN CAMERA TO PLAYER ───────────────────────────────────────────
	CachedPC->SetViewTargetWithBlend(
		CachedPlayer,
		CameraReturnBlendTime,
		EViewTargetBlendFunction::VTBlend_EaseInOut,
		2.f,
		false);

	if (CameraReturnBlendTime > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(
			CameraReturnHandle,
			this,
			&AWakeUpDirector::OnCameraReturnComplete,
			CameraReturnBlendTime,
			false);
	}
	else
	{
		OnCameraReturnComplete();
	}
}
// ─────────────────────────────────────────────────────────────────────────────
// OnCameraReturnComplete — blend finished, player has full control
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnCameraReturnComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// Restore full input.
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;
	CachedPC->ResetIgnoreMoveInput();

	// Re-enable movement (safety — should already be walking from step 3).
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		if (Mv->MovementMode == MOVE_None)
			Mv->SetMovementMode(MOVE_Walking);

	// Enable survival — oxygen starts draining from this moment.
	CachedPlayer->bIsSurvivalActive = true;

	UE_LOG(LogTemp, Log,
		TEXT("AWakeUpDirector: Wake-up complete. Player has full control. Survival ON."));

	// Show HUD, play ambient audio, trigger ship terminal warning, etc.
	BP_OnWakeUpComplete();
}

// ─────────────────────────────────────────────────────────────────────────────
// RevealPersistentShip
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::RevealPersistentShip()
{
	if (!PersistentShipActor) return;
	PersistentShipActor->SetActorHiddenInGame(false);
	PersistentShipActor->SetActorEnableCollision(true);

	// Re-cache light state NOW that the ship is visible
	if (AShipActor* Ship = Cast<AShipActor>(PersistentShipActor))
	{
		Ship->RecacheOriginalLightState(); // ← call a new public wrapper
	}
}