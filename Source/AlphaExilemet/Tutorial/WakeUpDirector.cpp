#include "WakeUpDirector.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

AWakeUpDirector::AWakeUpDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWakeUpDirector::BeginPlay()
{
	Super::BeginPlay();
	// Push self-reference to GM so GM doesn't need GetAllActorsOfClass.
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeWakeUp
// Called by GM_SimulatorGamemode inside SpawnNewGamePlayer,
// only when transitioning from Tutorial (GamePhase == NewGame_Tutorial).
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
			TEXT("AWakeUpDirector::InitializeWakeUp — Player pawn not found."));
		// Still broadcast ready so the blackout widget doesn't get stuck.
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
			     "Skipping cutscene — enabling player directly."));

		// No cutscene assigned yet — enable everything immediately.
		CachedPlayer->bIsSurvivalActive = true;
		FInputModeGameOnly GameMode;
		CachedPC->SetInputMode(GameMode);

		// Signal blackout widget to remove itself.
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
		UE_LOG(LogTemp, Error, TEXT("AWakeUpDirector — WakeUpSequenceRef has no SequencePlayer."));
		return;
	}

	// ── 3. LOCK INPUT FOR CUTSCENE ───────────────────────────────────────────
	// Player arrives with UI-only input from the TutorialDirector — keep it
	// locked until the wake-up cutscene finishes.
	{
		FInputModeUIOnly UIMode;
		CachedPC->SetInputMode(UIMode);
		CachedPC->bShowMouseCursor = false;
	}

	// Make sure movement is still disabled (it was locked at black-screen time).
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
	{
		if (Mv->MovementMode != MOVE_None)
		{
			// In case something re-enabled it during the level swap, re-lock it.
			Mv->DisableMovement();
		}
	}

	// ── 4. OPTIONAL CINECAM ──────────────────────────────────────────────────
	if (WakeUpCineCamRef)
	{
		CachedPC->SetViewTargetWithBlend(WakeUpCineCamRef, 0.0f);
	}

	// ── 5. BIND + PLAY ───────────────────────────────────────────────────────
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

	// ── 1. RETURN CAMERA + INPUT TO PLAYER ───────────────────────────────────
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);

	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor   = false;
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
	// The widget never calls anything on the GM or the Director.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			SS->OnTutorialPlayerReady.Broadcast();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Wake-up complete. Player has full control."));

	// ── 5. OPTIONAL BP POLISH ────────────────────────────────────────────────
	// Show HUD, play ambient sounds, trigger ship terminal warning, etc.
	BP_OnWakeUpComplete();
}
