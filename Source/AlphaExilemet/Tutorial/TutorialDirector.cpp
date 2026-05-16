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
	// BP BeginPlay runs before C++ Super in a derived Blueprint class, so
	// SkullRef and rock bindings are already set by the time we arrive here.
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
// Called by GM_SimulatorGamemode via stored TutorialDirectorRef,
// guaranteed to run after the player pawn is spawned and possessed.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ── 1. CACHE PLAYER ──────────────────────────────────────────────────────
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — Player pawn not found."));
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
	if (SkullRef) SkullRef->SetDirector(this);
	else UE_LOG(LogTemp, Warning,
		TEXT("ATutorialDirector::InitializeTutorial — SkullRef null. "
		     "Did BP BeginPlay call GetActorOfClass(ASkullProp) → SET SkullRef?"));

	// ── 3. HIDE MAIN HUD ─────────────────────────────────────────────────────
	BP_HideHUD();

	// ── 4. DISABLE SURVIVAL ──────────────────────────────────────────────────
	CachedPlayer->bIsSurvivalActive = false;

	// ── 5. VALIDATE SEQUENCE REFERENCE ───────────────────────────────────────
	if (!IntroSequenceRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef is null. "
			     "Select BP_TutorialDirector in the level, Details "
			     "→ Tutorial|Config → Intro Sequence → assign Cutscene1."));
		return;
	}

	IntroSequencePlayer = IntroSequenceRef->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector — IntroSequenceRef has no SequencePlayer."));
		return;
	}

	// ── 6. BIND + LOCK + PLAY ────────────────────────────────────────────────
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	{
		FInputModeUIOnly UIMode;
		CachedPC->SetInputMode(UIMode);
		CachedPC->bShowMouseCursor = false;
	}

	if (TutorialCineCamRef)
	{
		CachedPC->SetViewTargetWithBlend(TutorialCineCamRef, 0.0f);
	}

	IntroSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	CraterStartPosition = CachedPlayer->GetActorLocation();

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

	if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
			TutorialPickaxeClass, CachedPlayer->GetActorLocation(),
			CachedPlayer->GetActorRotation(), Params);

		if (SpawnedTutorialPickaxe)
		{
			SpawnedTutorialPickaxe->SetActorEnableCollision(false);
			CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
			CachedPlayer->StartWieldTool(0);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Intro finished. CraterPos=%s."), *CraterStartPosition.ToString());

	BP_OnIntroFinished();
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
// Full timeline:
//   t + 0.0 s  OnSkullInteracted():      flicker starts, spell SFX. Player FREE.
//   t + 5.5 s  OnSpellDurationComplete(): skull StopAndReset — stands still. Player FREE.
//   t + 6.0 s  OnSkullPausedBeforeBlack(): input locked, black screen.
//   t + 7.0 s  OnPostBlackDelay():        explosion SFX.
//   t + 10.0 s OnLevelSwapReady():        Tutorial→Main.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;

	ClearTutorialPickaxe();

	if (SkullRef) SkullRef->StartFlicker();
	else UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::OnSkullInteracted — SkullRef null!"));

	BP_PlayMagicSpellSound();

	GetWorldTimerManager().SetTimer(SpellDurationHandle,
		this, &ATutorialDirector::OnSpellDurationComplete, 5.5f, false);

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Skull interaction started. Player free for 5.5s."));
}

void ATutorialDirector::OnSpellDurationComplete()
{
	// Stop the flicker — skull snaps to home position, fully visible.
	// Player can still move and look at the still skull for 0.5 s.
	if (SkullRef) SkullRef->StopAndReset();

	GetWorldTimerManager().SetTimer(SkullPauseHandle,
		this, &ATutorialDirector::OnSkullPausedBeforeBlack, 0.5f, false);

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Skull stopped. 0.5s pause before black screen."));
}

void ATutorialDirector::OnSkullPausedBeforeBlack()
{
	// Player has seen the skull standing still for 0.5 s.
	// Now lock input and show the black screen.
	LockPlayerInputFull();
	BP_ShowInstantBlack();

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
