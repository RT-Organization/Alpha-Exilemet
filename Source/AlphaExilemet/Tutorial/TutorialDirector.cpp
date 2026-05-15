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

// ─────────────────────────────────────────────────────────────────────────────
// BeginPlay
//
// Three responsibilities:
//   1. Let the BP complete its setup (rock binding + skull ref) — that runs
//      in the BP's own BeginPlay which fires first in derived classes.
//   2. Self-register with the GameMode so GM never needs to search for us.
//
// The player is NOT cached here — the pawn is not possessed yet.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();

	// BP BeginPlay runs before this (derived-before-base ordering in UE5 BP),
	// so SkullRef and the rock bindings are already set by the time we arrive.

	// Push self-reference to the GameMode.
	// This eliminates the GetAllActorsOfClass → GET[0] race condition:
	// the GM stores our pointer and uses it directly when the player is ready.
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
// Called by GM_SimulatorGamemode via its stored TutorialDirectorRef,
// after the player pawn is spawned and possessed.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ── 1. CACHE PLAYER REFERENCES ───────────────────────────────────────────
	// Safe here: the GM guarantees the pawn is possessed before calling us.
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — Player pawn not found. "
			     "GM must call this after the pawn is possessed."));
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — PlayerController not found."));
		return;
	}

	// ── 2. WIRE SKULL ────────────────────────────────────────────────────────
	// SkullRef is set in BP BeginPlay (which runs before InitializeTutorial).
	if (SkullRef)
	{
		SkullRef->SetDirector(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::InitializeTutorial — SkullRef is null. "
			     "BP BeginPlay must call GetActorOfClass(ASkullProp) → SET SkullRef."));
	}

	// ── 3. HIDE MAIN HUD ─────────────────────────────────────────────────────
	BP_HideHUD();

	// ── 4. DISABLE SURVIVAL ──────────────────────────────────────────────────
	CachedPlayer->bIsSurvivalActive = false;

	// ── 5. VALIDATE SEQUENCE REFERENCE ───────────────────────────────────────
	// IntroSequenceRef is EditInstanceOnly — assign it once by dragging
	// the LevelSequenceActor from the Outliner into the Details panel.
	// No tags, no search, no mismatch.
	if (!IntroSequenceRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef is null. "
			     "Select the placed BP_TutorialDirector in the Tutorial level, open "
			     "Details → Tutorial|Config → Intro Sequence, and assign Cutscene1."));
		return;
	}

	IntroSequencePlayer = IntroSequenceRef->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector — IntroSequenceRef has no SequencePlayer. "
			     "Verify the LevelSequenceActor has a valid LevelSequence asset."));
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

	// ── 8. VIEW TARGET (optional) ────────────────────────────────────────────
	// Skip if TutorialCineCamRef is null — the LevelSequence Camera Cut track
	// will handle the view target switch automatically in that case.
	if (TutorialCineCamRef)
	{
		CachedPC->SetViewTargetWithBlend(TutorialCineCamRef, 0.0f);
	}

	// ── 9. PLAY ──────────────────────────────────────────────────────────────
	IntroSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// Return camera + input to player.
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	// Save crater position — must be non-zero before the boundary guard fires.
	CraterStartPosition = CachedPlayer->GetActorLocation();

	// Wire boundary guard now that CraterStartPosition is valid.
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

	// Spawn tutorial pickaxe.
	if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
			TutorialPickaxeClass,
			CachedPlayer->GetActorLocation(),
			CachedPlayer->GetActorRotation(), Params);

		if (SpawnedTutorialPickaxe)
		{
			SpawnedTutorialPickaxe->SetActorEnableCollision(false);
			CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
			CachedPlayer->StartWieldTool(0);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Intro finished. CraterPos=%s."),
		*CraterStartPosition.ToString());

	BP_OnIntroFinished(); // BP creates WBP_TutorialOverlay
}

// ─────────────────────────────────────────────────────────────────────────────
// Boundary guard
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (!Cast<AAlphaExilemetCharacter>(OtherActor)) return;
	if (bSkullInteractionActive) return;
	if (CraterStartPosition.IsZero()) return;
	ExecuteTeleportToCrater();
}

// ─────────────────────────────────────────────────────────────────────────────
// Teleport sequence
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ExecuteTeleportToCrater()
{
	if (!CachedPlayer || !CachedPC || CraterStartPosition.IsZero()) return;
	SuppressPlayerMoveInput();
	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(0.f, 1.f, 1.0f, FLinearColor::Black, false, true);
	GetWorldTimerManager().SetTimer(TeleportFadeOutHandle,
		this, &ATutorialDirector::OnTeleportReadyToMove, 1.0f, false);
}

void ATutorialDirector::OnTeleportReadyToMove()
{
	if (!CachedPlayer || !CachedPC) return;
	CachedPlayer->SetActorLocation(CraterStartPosition, false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(1.f, 0.f, 1.0f, FLinearColor::Black, false, false);
	GetWorldTimerManager().SetTimer(TeleportFadeInHandle,
		this, &ATutorialDirector::OnTeleportComplete, 1.0f, false);
}

void ATutorialDirector::OnTeleportComplete()
{
	RestorePlayerMoveInput();
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
}

// ─────────────────────────────────────────────────────────────────────────────
// Skull interaction sequence
//
//   0.0 s  flicker starts, spell SFX plays — player FREE TO MOVE
//   5.5 s  input locked, black screen, skull resets
//   6.5 s  explosion SFX
//   9.5 s  Tutorial→Main via AlphaStreamingSubsystem
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;

	ClearTutorialPickaxe();
	if (SkullRef) SkullRef->StartFlicker();
	BP_PlayMagicSpellSound();

	GetWorldTimerManager().SetTimer(SpellDurationHandle,
		this, &ATutorialDirector::OnSpellDurationComplete, 5.5f, false);
}

void ATutorialDirector::OnSpellDurationComplete()
{
	LockPlayerInputFull();
	BP_ShowInstantBlack();
	if (SkullRef) SkullRef->StopAndReset();

	GetWorldTimerManager().SetTimer(PostBlackSoundHandle,
		this, &ATutorialDirector::OnPostBlackDelay, 1.0f, false);
}

void ATutorialDirector::OnPostBlackDelay()
{
	BP_PlayExplosionSequence();
	GetWorldTimerManager().SetTimer(LevelSwapHandle,
		this, &ATutorialDirector::OnLevelSwapReady, 3.0f, false);
}

void ATutorialDirector::OnLevelSwapReady()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			SS->HandleTutorialCompletion();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Input helpers
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::SuppressPlayerMoveInput()
{
	if (CachedPC) CachedPC->SetIgnoreMoveInput(true);
}

void ATutorialDirector::RestorePlayerMoveInput()
{
	if (!CachedPC) return;
	CachedPC->SetIgnoreMoveInput(false);
	if (CachedPlayer)
	{
		if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		{
			if (Mv->MovementMode == MOVE_None)
				Mv->SetMovementMode(MOVE_Walking);
		}
	}
}

void ATutorialDirector::LockPlayerInputFull()
{
	if (!CachedPlayer || !CachedPC) return;
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		Mv->DisableMovement();
	CachedPC->SetIgnoreMoveInput(true);
	FInputModeUIOnly UIMode;
	CachedPC->SetInputMode(UIMode);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

AAlphaExilemetCharacter* ATutorialDirector::GetTutorialPlayer() const
{
	return Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}