#include "TutorialDirector.h"
#include "AlphaExilemet/Tutorial/SkullProp.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/CinematicHandoffComponent.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/PlayerSpawnMarker.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create the handoff component so BP_TutorialDirector can configure it
	// via Class Defaults (CameraBlendTime, BlendFunction, BlendExponent).
	CinematicHandoff = CreateDefaultSubobject<UCinematicHandoffComponent>(
		TEXT("CinematicHandoff"));
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
		TEXT("ATutorialDirector::InitializeTutorial — SkullRef null."));

	// ── 3. HIDE MAIN HUD ─────────────────────────────────────────────────────
	BP_HideHUD();

	// ── 4. DISABLE SURVIVAL ──────────────────────────────────────────────────
	CachedPlayer->bIsSurvivalActive = false;

	// ── 5. VALIDATE SEQUENCE ─────────────────────────────────────────────────
	if (!IntroSequenceRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef is null. "
			     "Assign it in the placed instance Details panel → Tutorial|Config."));
		return;
	}

	IntroSequencePlayer = IntroSequenceRef->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef has no SequencePlayer."));
		return;
	}

	// ── 6. HIDE REAL PLAYER ───────────────────────────────────────────────────
	// The animator's Spawnable SK acts as the player during the cutscene.
	// The real pawn is hidden so it doesn't clip through the world or cast
	// an unexpected shadow.
	HidePlayerForCutscene();

	// ── 7. LOCK ALL INPUT ────────────────────────────────────────────────────
	LockPlayerInputFull();

	// ── 8. BIND CUTSCENE END ─────────────────────────────────────────────────
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	// ── 9. OPTIONAL: SNAP TO CINECAM BEFORE PLAY ─────────────────────────────
	// Prevents a 1-frame flash of the player's FP camera before the sequence
	// Camera Cut track takes over.
	if (TutorialCineCamRef)
	{
		CachedPC->SetViewTargetWithBlend(TutorialCineCamRef, 0.0f);
	}

	// ── 10. PLAY ─────────────────────────────────────────────────────────────
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
	{
		Mesh->SetVisibility(false, true);
	}

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Real player hidden for cutscene."));
}

// ─────────────────────────────────────────────────────────────────────────────
// FindCutsceneProxy — fallback when no PlayerSpawnMarker is assigned
// ─────────────────────────────────────────────────────────────────────────────

AActor* ATutorialDirector::FindCutsceneProxy() const
{
	if (ProxyCharacterTag.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::FindCutsceneProxy — ProxyCharacterTag is empty."));
		return nullptr;
	}

	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), ProxyCharacterTag, Tagged);

	if (Tagged.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::FindCutsceneProxy — No actor tagged '%s'."),
			*ProxyCharacterTag.ToString());
		return nullptr;
	}

	return Tagged[0];
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. DETERMINE PLAYER SPAWN TRANSFORM ───────────────────────────────────
	//
	// Priority:
	//   (a) TutorialPlayerSpawnMarker — placed manually at SK's feet on last frame.
	//       This is the correct production setup.
	//   (b) Proxy SK by tag — fallback if no marker (old behavior, prone to pivot error).
	//   (c) Player's current transform — last resort, will look wrong.
	FTransform SpawnTransform;

	if (TutorialPlayerSpawnMarker)
	{
		SpawnTransform = TutorialPlayerSpawnMarker->GetSpawnTransform();
		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector: Using PlayerSpawnMarker at %s."),
			*SpawnTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::OnIntroSequenceFinished — TutorialPlayerSpawnMarker is null. "
			     "Falling back to proxy tag '%s'. Place a BP_PlayerSpawnMarker in the level "
			     "for a correct result."),
			*ProxyCharacterTag.ToString());

		AActor* Proxy = FindCutsceneProxy();
		if (Proxy)
		{
			SpawnTransform = FTransform(Proxy->GetActorRotation(), Proxy->GetActorLocation());
			Proxy->SetActorHiddenInGame(true); // hide proxy before player appears
		}
		else
		{
			SpawnTransform = CachedPlayer->GetActorTransform();
			UE_LOG(LogTemp, Warning,
				TEXT("ATutorialDirector — No proxy found either. Player stays at spawn location."));
		}
	}

	// ── 2. BIND HANDOFF CALLBACK ──────────────────────────────────────────────
	//
	// OnCinematicHandoffComplete fires AFTER the blend finishes. Everything
	// that used to happen after ExecuteProxySwap() now goes in there.
	CinematicHandoff->OnHandoffComplete.AddUniqueDynamic(
		this, &ATutorialDirector::OnCinematicHandoffComplete);

	// ── 3. BEGIN HANDOFF ──────────────────────────────────────────────────────
	//
	// The component:
	//   (a) Spawns a ghost camera at LastTutorialCineCamera's world transform.
	//   (b) Snaps view to ghost (zero time — invisible).
	//   (c) Teleports player to SpawnTransform (still hidden).
	//   (d) Unhides player.
	//   (e) Blends view from ghost → player over CameraBlendTime seconds.
	//   (f) Destroys ghost. Fires OnHandoffComplete.
	CinematicHandoff->BeginHandoff(
		LastTutorialCineCamera,
		CachedPlayer,
		CachedPC,
		SpawnTransform);
}

// ─────────────────────────────────────────────────────────────────────────────
// OnCinematicHandoffComplete
// Called after the camera blend finishes — player has full camera control.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnCinematicHandoffComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. RESTORE MOVEMENT INPUT (soft) ──────────────────────────────────────
	// Player can now move and look around. Survival stays off.
	RestorePlayerMoveInput();

	// ── 2. SWITCH TO GAME INPUT MODE ──────────────────────────────────────────
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	// ── 3. SAVE CRATER TRANSFORM (position + rotation) ────────────────────────
	//
	// This is the transform the player starts gameplay with.
	// We save it HERE (after handoff) so it reflects the correct spawn position
	// from the marker, not the old PlayerStart position.
	// The OxygenSphere boundary guard uses this to teleport the player back
	// if they walk out of the tutorial area — restoring BOTH position AND
	// facing direction (fixes the "wrong rotation" teleport bug).
	CraterStartTransform = CachedPlayer->GetActorTransform();

	// ── 4. WIRE BOUNDARY GUARD ────────────────────────────────────────────────
	if (AActor* BaseCampActor = FindBaseCamp())
	{
		if (ABaseCamp* BaseCamp = Cast<ABaseCamp>(BaseCampActor))
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

	// ── 5. SPAWN TUTORIAL PICKAXE ─────────────────────────────────────────────
	if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

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
		TEXT("ATutorialDirector: Intro handoff complete. "
		     "CraterPos=%s CraterRot=%s."),
		*CraterStartTransform.GetLocation().ToString(),
		*CraterStartTransform.GetRotation().Rotator().ToString());

	// ── 6. NOTIFY BP (show TutorialOverlay widget) ────────────────────────────
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
	if (CraterStartTransform.GetLocation().IsZero()) return;
	ExecuteTeleportToCrater();
}

// ─────────────────────────────────────────────────────────────────────────────
// Teleport sequence (boundary violation)
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ExecuteTeleportToCrater()
{
	if (!CachedPlayer || !CachedPC) return;
	if (CraterStartTransform.GetLocation().IsZero()) return;

	SuppressPlayerMoveInput();

	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(0.f, 1.f, 1.0f, FLinearColor::Black, false, true);

	GetWorldTimerManager().SetTimer(TeleportFadeOutHandle,
		this, &ATutorialDirector::OnTeleportReadyToMove, 1.0f, false);
}

void ATutorialDirector::OnTeleportReadyToMove()
{
	if (!CachedPlayer || !CachedPC) return;

	// Restore BOTH position AND rotation — fixes the "wrong facing" bug.
	CachedPlayer->SetActorLocationAndRotation(
		CraterStartTransform.GetLocation(),
		CraterStartTransform.GetRotation().Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	// Also restore the control rotation so the first-person camera faces
	// the same direction as when gameplay began.
	CachedPC->SetControlRotation(CraterStartTransform.GetRotation().Rotator());

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
//   0.0s  flicker + spell SFX — player FREE
//   5.5s  skull StopAndReset
//   6.0s  input lock + black screen
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
		TEXT("ATutorialDirector::OnSkullInteracted — SkullRef null!"));

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
			SS->HandleTutorialCompletion();
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
// Misc helpers
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
