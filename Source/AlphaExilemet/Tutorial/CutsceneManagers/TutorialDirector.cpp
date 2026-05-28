#include "TutorialDirector.h"
#include "AlphaExilemet/Tutorial/SkullProp.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"

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
	BP_RegisterWithGameMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
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
			TEXT("ATutorialDirector::InitializeTutorial — Player not found."));
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

	// ── 3. HIDE HUD + DISABLE SURVIVAL ───────────────────────────────────────
	BP_HideHUD();
	CachedPlayer->bIsSurvivalActive = false;

	// ── 4. VALIDATE SEQUENCE ─────────────────────────────────────────────────
	if (!IntroSequenceRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef is null. "
			     "Drag LS_Tutorial from the Outliner into the placed BP_TutorialDirector."));
		return;
	}

	IntroSequencePlayer = IntroSequenceRef->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector — IntroSequenceRef has no SequencePlayer."));
		return;
	}

	// ── 5. HIDE PLAYER ────────────────────────────────────────────────────────
	// The sequence's animator SK handles all visuals.
	// The real player pawn is hidden and input-locked for the full duration.
	HidePlayerForCutscene();

	// ── 6. LOCK INPUT ────────────────────────────────────────────────────────
	LockPlayerInputFull();

	// ── 7. BIND SEQUENCE END ─────────────────────────────────────────────────
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	// ── 8. OPTIONAL: SNAP TO FIRST CINECAM ───────────────────────────────────
	// Prevents a 1-frame FP-camera flash before the Camera Cuts track takes over.
	if (TutorialCineCamRef)
		CachedPC->SetViewTargetWithBlend(TutorialCineCamRef, 0.f);

	// ── 9. PLAY ──────────────────────────────────────────────────────────────
	IntroSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Intro cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// HidePlayerForCutscene
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::HidePlayerForCutscene()
{
	if (!CachedPlayer) return;

	CachedPlayer->SetActorHiddenInGame(true);

	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(false, true);

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Player hidden for cutscene."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
//
// OnStop fires while Sequencer is still in the middle of its teardown
// (cinematic mode still active, movement component still locked by Sequencer).
// Deferring by one tick lets Sequencer finish before we do anything.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Sequence ended. Deferring one tick for Sequencer teardown."));

	GetWorldTimerManager().SetTimerForNextTick(
		this, &ATutorialDirector::OnIntroSequenceFinishedDeferred);
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinishedDeferred — one tick after OnStop
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinishedDeferred()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. REVEAL SHIP ────────────────────────────────────────────────────────
	RevealPersistentShip();

	// ── 2. UNHIDE PLAYER ──────────────────────────────────────────────────────
	// Player is still at their original spawn position (hidden throughout).
	// They need to be visible before the camera returns to them.
	CachedPlayer->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(true, true);

	// ── 3. CLEAR BLACK BARS ───────────────────────────────────────────────────
	// Sequencer's CineCamera may have set bConstrainAspectRatio on the
	// player's FP camera, causing black bars after the handoff.
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
	// The animator's sequence ends with the camera near the player head.
	// We blend back to the player's FP camera over CameraReturnBlendTime.
	// 0 = instant snap (use if animator already aligned camera perfectly).
	ReturnCameraToPlayer();
}

// ─────────────────────────────────────────────────────────────────────────────
// RevealPersistentShip
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::RevealPersistentShip()
{
	if (!PersistentShipActor)
	{
		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector: No PersistentShipActor assigned — skipping ship reveal."));
		return;
	}

	PersistentShipActor->SetActorHiddenInGame(false);
	PersistentShipActor->SetActorEnableCollision(true);

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: PersistentShipActor revealed."));
}

// ─────────────────────────────────────────────────────────────────────────────
// ReturnCameraToPlayer
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ReturnCameraToPlayer()
{
	if (!CachedPC || !CachedPlayer) return;

	CachedPC->SetViewTargetWithBlend(
		CachedPlayer,
		CameraReturnBlendTime,
		EViewTargetBlendFunction::VTBlend_EaseInOut,
		2.f,
		false);

	if (CameraReturnBlendTime > KINDA_SMALL_NUMBER)
	{
		// Wait for the blend to finish, then restore full input.
		GetWorldTimerManager().SetTimer(
			CameraReturnHandle,
			this,
			&ATutorialDirector::OnCameraReturnComplete,
			CameraReturnBlendTime,
			false);
	}
	else
	{
		// Instant snap — restore input immediately.
		OnCameraReturnComplete();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnCameraReturnComplete — camera blend finished, player has full control
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnCameraReturnComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. RESTORE INPUT ──────────────────────────────────────────────────────
	RestorePlayerMoveInput();

	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	// ── 2. SAVE CRATER TRANSFORM ─────────────────────────────────────────────
	// Save BOTH position AND rotation so the OxygenSphere boundary guard can
	// fully restore the player (position + facing) if they walk too far.
	CraterStartTransform = CachedPlayer->GetActorTransform();

	// ── 3. WIRE BOUNDARY GUARD ────────────────────────────────────────────────
	if (AActor* BC = FindBaseCamp())
	{
		if (ABaseCamp* BaseCamp = Cast<ABaseCamp>(BC))
		{
			if (BaseCamp->OxygenSphere)
			{
				BaseCamp->OxygenSphere->OnComponentEndOverlap.AddUniqueDynamic(
					this, &ATutorialDirector::OnOxygenSphereEndOverlap);
				UE_LOG(LogTemp, Log,
					TEXT("ATutorialDirector: OxygenSphere boundary guard active."));
			}
		}
	}

	// ── 4. SPAWN TUTORIAL PICKAXE ─────────────────────────────────────────────
	if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
			TutorialPickaxeClass,
			CachedPlayer->GetActorLocation(),
			CachedPlayer->GetActorRotation(),
			Params);

		if (SpawnedTutorialPickaxe)
		{
			SpawnedTutorialPickaxe->SetActorEnableCollision(false);
			CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
			CachedPlayer->StartWieldTool(0);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Intro complete. Player at %s facing %s."),
		*CraterStartTransform.GetLocation().ToString(),
		*CraterStartTransform.GetRotation().Rotator().ToString());

	// ── 5. NOTIFY BP ──────────────────────────────────────────────────────────
	BP_OnIntroFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// Boundary guard — OxygenSphere
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<AAlphaExilemetCharacter>(OtherActor)) return;
	if (bSkullInteractionActive)                    return;
	if (CraterStartTransform.GetLocation().IsZero()) return;

	ExecuteTeleportToCrater();
}

void ATutorialDirector::ExecuteTeleportToCrater()
{
	if (!CachedPlayer || !CachedPC) return;

	SuppressPlayerMoveInput();

	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(0.f, 1.f, 1.f, FLinearColor::Black, false, true);

	GetWorldTimerManager().SetTimer(TeleportFadeOutHandle,
		this, &ATutorialDirector::OnTeleportReadyToMove, 1.f, false);
}

void ATutorialDirector::OnTeleportReadyToMove()
{
	if (!CachedPlayer || !CachedPC) return;

	// Restore BOTH position AND rotation — fixes the old "wrong facing" bug.
	CachedPlayer->SetActorLocationAndRotation(
		CraterStartTransform.GetLocation(),
		CraterStartTransform.GetRotation().Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);

	CachedPC->SetControlRotation(CraterStartTransform.GetRotation().Rotator());

	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(1.f, 0.f, 1.f, FLinearColor::Black, false, false);

	GetWorldTimerManager().SetTimer(TeleportFadeInHandle,
		this, &ATutorialDirector::OnTeleportComplete, 1.f, false);
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
//   0.0s  flicker starts + spell SFX — player FREE to move
//   5.5s  skull StopAndReset
//   6.0s  input lock + instant black screen
//   7.0s  explosion SFX
//  10.0s  Tutorial → Main
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;

	ClearTutorialPickaxe();

	if (SkullRef) SkullRef->StartFlicker();
	else UE_LOG(LogTemp, Error,
		TEXT("ATutorialDirector::OnSkullInteracted — SkullRef is null!"));

	BP_PlayMagicSpellSound();

	GetWorldTimerManager().SetTimer(SpellDurationHandle,
		this, &ATutorialDirector::OnSpellDurationComplete, 5.5f, false);
}

void ATutorialDirector::OnSpellDurationComplete()
{
	if (SkullRef) SkullRef->StopAndReset();
	GetWorldTimerManager().SetTimer(SkullPauseHandle,
		this, &ATutorialDirector::OnSkullPausedBeforeBlack, 0.5f, false);
}

void ATutorialDirector::OnSkullPausedBeforeBlack()
{
	LockPlayerInputFull();
	BP_ShowInstantBlack();
	GetWorldTimerManager().SetTimer(PostBlackSoundHandle,
		this, &ATutorialDirector::OnPostBlackDelay, 1.f, false);
}

void ATutorialDirector::OnPostBlackDelay()
{
	BP_PlayExplosionSequence();
	GetWorldTimerManager().SetTimer(LevelSwapHandle,
		this, &ATutorialDirector::OnLevelSwapReady, 3.f, false);
}

void ATutorialDirector::OnLevelSwapReady()
{
	if (UGameInstance* GI = GetGameInstance())
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->HandleTutorialCompletion();
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
			if (Mv->MovementMode == MOVE_None)
				Mv->SetMovementMode(MOVE_Walking);
	}
}

void ATutorialDirector::LockPlayerInputFull()
{
	if (!CachedPlayer || !CachedPC) return;

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		Mv->DisableMovement();

	CachedPC->SetIgnoreMoveInput(true);
	CachedPC->SetInputMode(FInputModeUIOnly());
}

// ─────────────────────────────────────────────────────────────────────────────
// Misc
// ─────────────────────────────────────────────────────────────────────────────

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}