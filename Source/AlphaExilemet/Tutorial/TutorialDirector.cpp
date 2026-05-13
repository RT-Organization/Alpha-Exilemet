#include "TutorialDirector.h"
#include "SkullProp.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	// Intentionally empty.
	// CachedPlayer and CachedPC are set in InitializeTutorial(), which is
	// called by GM_SimulatorGamemode AFTER the player is spawned and possessed.
	//
	// DO NOT cache the player here — BeginPlay fires before the pawn exists.
	//
	// BP BeginPlay is responsible for:
	//   GetAllActorsOfClass(TutorialRock) → bind delegates
	//   GetActorOfClass(ASkullProp)       → SET SkullRef
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ── 1. CACHE PLAYER REFERENCES ───────────────────────────────────────────
	// This is the ONLY correct place to cache the player.
	// GetPlayerCharacter returns null during BeginPlay because the pawn has
	// not been possessed yet at that stage.
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — Player not found. "
			     "Ensure GM_SimulatorGamemode calls this after SpawnDefaultPawn."));
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — PlayerController not found."));
		return;
	}

	// ── 2. WIRE SKULL DIRECTOR REFERENCE ─────────────────────────────────────
	// BP BeginPlay sets SkullRef via GetActorOfClass, which fires before
	// InitializeTutorial(), so SkullRef is valid by now.
	if (SkullRef)
	{
		SkullRef->SetDirector(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::InitializeTutorial — SkullRef is null. "
			     "Did BP BeginPlay assign it via GetActorOfClass(ASkullProp)?"));
	}

	// ── 3. HIDE HUD ──────────────────────────────────────────────────────────
	// BP implementation: hide main HUD widget using CachedPlayer.
	BP_HideHUD();

	// ── 4. SURVIVAL OFF DURING TUTORIAL ──────────────────────────────────────
	CachedPlayer->bIsSurvivalActive = false;

	// ── 5. FIND INTRO SEQUENCE ───────────────────────────────────────────────
	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(World, IntroSequenceTag, TaggedActors);
	if (TaggedActors.Num() == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — No actor tagged '%s'."),
			*IntroSequenceTag.ToString());
		return;
	}

	ALevelSequenceActor* SeqActor = Cast<ALevelSequenceActor>(TaggedActors[0]);
	if (!SeqActor)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector — Tagged actor is not a LevelSequenceActor."));
		return;
	}

	IntroSequencePlayer = SeqActor->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector — LevelSequenceActor has no Sequence Player."));
		return;
	}

	// ── 6. BIND CUTSCENE END ─────────────────────────────────────────────────
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	// ── 7. LOCK INPUT FOR CUTSCENE ───────────────────────────────────────────
	{
		FInputModeUIOnly UIMode;
		CachedPC->SetInputMode(UIMode);
		CachedPC->bShowMouseCursor = false;
	}

	// ── 8. SET CINECAM BEFORE PLAY (avoids 1-frame first-person flash) ───────
	TArray<AActor*> CamActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("TutorialCineCam"), CamActors);
	if (CamActors.Num() > 0)
	{
		CachedPC->SetViewTargetWithBlend(CamActors[0], 0.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATutorialDirector — No actor tagged 'TutorialCineCam'."));
	}

	// ── 9. PLAY CUTSCENE ─────────────────────────────────────────────────────
	IntroSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: InitializeTutorial complete. Cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// Return camera to player
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);

	// Re-enable game input
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	// Capture crater position (used by ExecuteTeleportToCrater)
	CraterStartPosition = CachedPlayer->GetActorLocation();

	// Bind OxygenSphere EndOverlap — safe here, CraterStartPosition is set above
	if (AActor* BaseCampActor = FindBaseCamp())
	{
		if (ABaseCamp* BaseCamp = Cast<ABaseCamp>(BaseCampActor))
		{
			if (BaseCamp->OxygenSphere)
			{
				BaseCamp->OxygenSphere->OnComponentEndOverlap.AddUniqueDynamic(
					this, &ATutorialDirector::OnOxygenSphereEndOverlap);
				UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: OxygenSphere boundary guard active."));
			}
		}
	}

	// Spawn and equip tutorial pickaxe
	if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
			TutorialPickaxeClass,
			CachedPlayer->GetActorLocation(),
			CachedPlayer->GetActorRotation(),
			SpawnParams);

		if (SpawnedTutorialPickaxe)
		{
			SpawnedTutorialPickaxe->SetActorEnableCollision(false);
			CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
			CachedPlayer->StartWieldTool(0);
			UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Tutorial pickaxe equipped."));
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Intro finished. CraterPos=%s."),
		*CraterStartPosition.ToString());

	// Notify BP: create WBP_TutorialOverlay
	BP_OnIntroFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// OnOxygenSphereEndOverlap
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor*              OtherActor,
	UPrimitiveComponent* OtherComp,
	int32                OtherBodyIndex)
{
	if (!Cast<AAlphaExilemetCharacter>(OtherActor)) return;

	// Do not teleport if the skull interaction sequence is already running —
	// the player is committed to the end-of-tutorial flow.
	if (bSkullInteractionActive) return;

	if (CraterStartPosition.IsZero())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector: Boundary teleport skipped — CraterStartPosition is zero."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Player left boundary. Starting teleport."));
	ExecuteTeleportToCrater();
}

// ─────────────────────────────────────────────────────────────────────────────
// Teleport Sequence  (replaces BP_OnPlayerLeftBase entirely)
//
// Stage 1 — ExecuteTeleportToCrater:
//   Soft-locks move input. Fades camera to black over 1 second.
//   Sets TeleportFadeOutHandle (1.0s) → OnTeleportReadyToMove.
//
// Stage 2 — OnTeleportReadyToMove (fires at fade-out completion):
//   Teleports the player to CraterStartPosition.
//   Starts fade-in back to normal. Sets TeleportFadeInHandle (1.0s).
//
// Stage 3 — OnTeleportComplete (fires at fade-in completion):
//   Restores move input. Ensures movement mode is Walking.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ExecuteTeleportToCrater()
{
	if (!CachedPlayer || !CachedPC || CraterStartPosition.IsZero()) return;

	SuppressPlayerMoveInput();

	if (APlayerCameraManager* CamMgr = CachedPC->PlayerCameraManager)
	{
		// Fade to solid black over 1 second, hold when finished.
		CamMgr->StartCameraFade(0.f, 1.f, 1.0f, FLinearColor::Black, false, true);
	}

	GetWorldTimerManager().SetTimer(
		TeleportFadeOutHandle,
		this,
		&ATutorialDirector::OnTeleportReadyToMove,
		1.0f, false);
}

void ATutorialDirector::OnTeleportReadyToMove()
{
	if (!CachedPlayer || !CachedPC) return;

	// Teleport — TeleportPhysics avoids sweep errors at the new location.
	CachedPlayer->SetActorLocation(
		CraterStartPosition, false, nullptr, ETeleportType::TeleportPhysics);

	if (APlayerCameraManager* CamMgr = CachedPC->PlayerCameraManager)
	{
		// Fade back in from solid black over 1 second, do not hold.
		CamMgr->StartCameraFade(1.f, 0.f, 1.0f, FLinearColor::Black, false, false);
	}

	GetWorldTimerManager().SetTimer(
		TeleportFadeInHandle,
		this,
		&ATutorialDirector::OnTeleportComplete,
		1.0f, false);
}

void ATutorialDirector::OnTeleportComplete()
{
	RestorePlayerMoveInput();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Teleport complete. Input restored."));
}

// ─────────────────────────────────────────────────────────────────────────────
// ClearTutorialPickaxe
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ClearTutorialPickaxe()
{
	if (!CachedPlayer || !SpawnedTutorialPickaxe) return;

	CachedPlayer->OwnedTools.Remove(SpawnedTutorialPickaxe);

	if (CachedPlayer->CurrentTool == SpawnedTutorialPickaxe)
	{
		CachedPlayer->CurrentTool     = nullptr;
		CachedPlayer->ActiveToolIndex = -1;
	}

	SpawnedTutorialPickaxe->Destroy();
	SpawnedTutorialPickaxe = nullptr;

	CachedPlayer->OnInventoryUpdated.Broadcast();
	CachedPlayer->OnToolWielded.Broadcast(-1);

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Tutorial pickaxe cleared."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnSkullInteracted
//
// Called by ASkullProp::HandleInteract().
//
// IMPORTANT: Player input is NOT locked here.
// The player is free to move during the entire 5.5s flicker sequence and can
// look at the skull from any angle.
// Input is locked in OnSpellDurationComplete() at the instant-black moment.
//
// Timeline:
//   0.0s  — pickaxe removed, skull flicker starts, spell SFX plays
//   5.5s  — OnSpellDurationComplete: input locked, screen black, skull resets
//   6.5s  — OnPostBlackDelay: explosion SFX plays
//   9.5s  — OnLevelSwapReady: Tutorial → Main transition
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;

	ClearTutorialPickaxe();

	// Start skull flicker directly — no BP event needed, SkullRef is typed.
	if (SkullRef)
	{
		SkullRef->StartFlicker();
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::OnSkullInteracted — SkullRef is null!"));
	}

	BP_PlayMagicSpellSound();

	// Player moves freely for 5.5s while watching the flicker.
	GetWorldTimerManager().SetTimer(
		SpellDurationHandle,
		this,
		&ATutorialDirector::OnSpellDurationComplete,
		5.5f, false);

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Skull interaction started. Player free to move for 5.5s."));
}

void ATutorialDirector::OnSpellDurationComplete()
{
	// ── LOCK INPUT (instant-black moment) ────────────────────────────────────
	// This is the exact frame the screen goes black. The player cannot see
	// anything happening after this point, so it is safe to freeze everything.
	LockPlayerInputFull();

	// ── BLACK SCREEN ─────────────────────────────────────────────────────────
	BP_ShowInstantBlack();

	// ── STOP SKULL (invisible under black) ───────────────────────────────────
	if (SkullRef)
	{
		SkullRef->StopAndReset();
	}

	GetWorldTimerManager().SetTimer(
		PostBlackSoundHandle,
		this,
		&ATutorialDirector::OnPostBlackDelay,
		1.0f, false);
}

void ATutorialDirector::OnPostBlackDelay()
{
	BP_PlayExplosionSequence();

	GetWorldTimerManager().SetTimer(
		LevelSwapHandle,
		this,
		&ATutorialDirector::OnLevelSwapReady,
		3.0f, false);
}

void ATutorialDirector::OnLevelSwapReady()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* Streaming = GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			Streaming->HandleTutorialCompletion();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// INPUT HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::SuppressPlayerMoveInput()
{
	// Blocks movement input only. The movement component stays in Walking
	// mode — physics and gravity continue to work normally.
	if (CachedPC)
	{
		CachedPC->SetIgnoreMoveInput(true);
	}
}

void ATutorialDirector::RestorePlayerMoveInput()
{
	if (!CachedPC) return;

	CachedPC->SetIgnoreMoveInput(false);

	// Guarantee the movement component is in Walking mode.
	// If anything (e.g. a prior crash) left it in MOVE_None, this fixes it.
	if (CachedPlayer)
	{
		if (UCharacterMovementComponent* Movement = CachedPlayer->GetCharacterMovement())
		{
			if (Movement->MovementMode == MOVE_None)
			{
				Movement->SetMovementMode(MOVE_Walking);
				UE_LOG(LogTemp, Warning,
					TEXT("ATutorialDirector::RestorePlayerMoveInput — MovementMode was None; "
					     "forced back to Walking."));
			}
		}
	}
}

void ATutorialDirector::LockPlayerInputFull()
{
	if (!CachedPlayer || !CachedPC) return;

	// 1. Stop the character from moving at all.
	if (UCharacterMovementComponent* Movement = CachedPlayer->GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	// 2. Suppress move input so even if movement mode is restored
	//    externally, the player still cannot give move commands.
	CachedPC->SetIgnoreMoveInput(true);

	// 3. Switch to UI-only input — the blackout widget covers the screen and
	//    the player should not be able to fire weapons, jump, or look around.
	FInputModeUIOnly UIMode;
	CachedPC->SetInputMode(UIMode);
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

AAlphaExilemetCharacter* ATutorialDirector::GetTutorialPlayer() const
{
	return Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
}

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}
