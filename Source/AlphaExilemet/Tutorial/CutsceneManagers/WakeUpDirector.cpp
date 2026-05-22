#include "WakeUpDirector.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/CinematicHandoffComponent.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/PlayerSpawnMarker.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraActor.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

AWakeUpDirector::AWakeUpDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	CinematicHandoff = CreateDefaultSubobject<UCinematicHandoffComponent>(
		TEXT("CinematicHandoff"));
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
	UWorld* World = GetWorld();
	if (!World) return;

	// ── 1. CACHE PLAYER ──────────────────────────────────────────────────────
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AWakeUpDirector::InitializeWakeUp — Player pawn not found. "
			     "Broadcasting OnTutorialPlayerReady as fallback."));

		if (UGameInstance* GI = GetGameInstance())
		{
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		}
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
			     "Assign it in the placed BP_WakeUpDirector instance Details panel. "
			     "Enabling player directly (no cutscene)."));

		CachedPlayer->bIsSurvivalActive = true;
		FInputModeGameOnly GameMode;
		CachedPC->SetInputMode(GameMode);

		if (UGameInstance* GI = GetGameInstance())
		{
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		}

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
	// The animator's SK handles the visuals during the cutscene.
	CachedPlayer->SetActorHiddenInGame(true);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
	{
		Mesh->SetVisibility(false, true);
	}

	// ── 4. LOCK INPUT ────────────────────────────────────────────────────────
	// Player arrives with UI-only input from the TutorialDirector — keep locked.
	{
		FInputModeUIOnly UIMode;
		CachedPC->SetInputMode(UIMode);
		CachedPC->bShowMouseCursor = false;
	}

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
	{
		if (Mv->MovementMode != MOVE_None)
		{
			Mv->DisableMovement();
		}
	}

	// ── 5. OPTIONAL PRE-SEQUENCE CINECAM ─────────────────────────────────────
	if (WakeUpCineCamRef)
	{
		CachedPC->SetViewTargetWithBlend(WakeUpCineCamRef, 0.0f);
	}

	// ── 6. BIND + PLAY ───────────────────────────────────────────────────────
	WakeUpSequencePlayer->OnStop.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpSequenceFinished);

	WakeUpSequencePlayer->Play();

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Wake-up cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnWakeUpSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnWakeUpSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. DETERMINE PLAYER SPAWN TRANSFORM ───────────────────────────────────
	//
	// Priority:
	//   (a) WakeUpPlayerSpawnMarker — manually placed at SK's feet on last frame.
	//   (b) Player's current transform — fallback (will look wrong, spawn at PlayerStart).
	FTransform SpawnTransform;

	if (WakeUpPlayerSpawnMarker)
	{
		SpawnTransform = WakeUpPlayerSpawnMarker->GetSpawnTransform();
		UE_LOG(LogTemp, Log,
			TEXT("AWakeUpDirector: Using WakeUpPlayerSpawnMarker at %s."),
			*SpawnTransform.GetLocation().ToString());
	}
	else
	{
		SpawnTransform = CachedPlayer->GetActorTransform();
		UE_LOG(LogTemp, Warning,
			TEXT("AWakeUpDirector::OnWakeUpSequenceFinished — WakeUpPlayerSpawnMarker is null. "
			     "Player will appear at PlayerStart. Place a BP_PlayerSpawnMarker at the "
			     "SK's feet on the last frame of the wake-up cutscene and assign it here."));
	}

	// ── 2. BIND HANDOFF CALLBACK ──────────────────────────────────────────────
	CinematicHandoff->OnHandoffComplete.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpHandoffComplete);

	// ── 3. BEGIN HANDOFF ──────────────────────────────────────────────────────
	//
	// Ghost camera placed at LastWakeUpCineCamera's world transform → view freezes.
	// Player teleported to SpawnTransform (still hidden → unhide → blend → done.
	CinematicHandoff->BeginHandoff(
		LastWakeUpCineCamera,
		CachedPlayer,
		CachedPC,
		SpawnTransform);
}

// ─────────────────────────────────────────────────────────────────────────────
// OnWakeUpHandoffComplete
// Called after the camera blend finishes — player has full camera control.
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnWakeUpHandoffComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. RETURN INPUT TO PLAYER ─────────────────────────────────────────────
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor  = false;
	CachedPC->ResetIgnoreMoveInput();

	// ── 2. RE-ENABLE MOVEMENT ────────────────────────────────────────────────
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
	{
		Mv->SetMovementMode(MOVE_Walking);
	}

	// ── 3. ENABLE SURVIVAL ───────────────────────────────────────────────────
	CachedPlayer->bIsSurvivalActive = true;

	// ── 4. SIGNAL BLACKOUT WIDGET ────────────────────────────────────────────
	// WB_TutorialBlackout binds to OnTutorialPlayerReady in Event Construct.
	// When this fires, the widget plays its fade-out and removes itself.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			SS->OnTutorialPlayerReady.Broadcast();
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("AWakeUpDirector: Handoff complete. Player has full control. Survival ON."));

	// ── 5. OPTIONAL BP POLISH ────────────────────────────────────────────────
	// Show HUD, play ambient sounds, trigger ship terminal warning, etc.
	BP_OnWakeUpComplete();
}
