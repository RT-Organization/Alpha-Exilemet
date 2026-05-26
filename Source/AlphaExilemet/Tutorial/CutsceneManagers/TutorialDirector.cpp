#include "TutorialDirector.h"
#include "AlphaExilemet/Tutorial/SkullProp.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/CinematicHandoffComponent.h"

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
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef is null."));
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
	HidePlayerForCutscene();

	// ── 7. LOCK ALL INPUT ────────────────────────────────────────────────────
	LockPlayerInputFull();

	// ── 8. BIND CUTSCENE END ─────────────────────────────────────────────────
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	// ── 9. SNAP TO FIRST CINECAM BEFORE PLAY ─────────────────────────────────
	// Prevents a 1-frame FP-camera flash before the Camera Cut track takes over.
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
		Mesh->SetVisibility(false, true);

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Real player hidden for cutscene."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
//
// This is where all the "figure out positions from the live sequence data"
// logic lives. We query bone positions directly from the Spawnable SK,
// then hand everything to CinematicHandoffComponent.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── A. FIND THE GHOST CAMERA START POSITION ──────────────────────────────
	//
	// Priority:
	//   (1) LastTutorialCineCamera — assigned in the instance Details panel.
	//   (2) PC current view target  — auto-detected at the moment OnStop fires.
	//       Reliable if the Camera Cut track has not yet released the camera.
	//   (3) Player actor location   — last resort, will look wrong.
	FVector  GhostStartLocation;
	FRotator GhostStartRotation;

	if (LastTutorialCineCamera)
	{
		GhostStartLocation = LastTutorialCineCamera->GetActorLocation();
		GhostStartRotation = LastTutorialCineCamera->GetActorRotation();
		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector: Ghost start from assigned LastTutorialCineCamera at %s."),
			*GhostStartLocation.ToString());
	}
	else if (AActor* ViewTarget = CachedPC->GetViewTarget())
	{
		GhostStartLocation = ViewTarget->GetActorLocation();
		GhostStartRotation = ViewTarget->GetActorRotation();
		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector: Ghost start auto-detected from PC ViewTarget (%s) at %s."),
			*ViewTarget->GetName(), *GhostStartLocation.ToString());
	}
	else
	{
		GhostStartLocation = CachedPlayer->GetActorLocation();
		GhostStartRotation = CachedPlayer->GetActorRotation();
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector: No CineCamera found. Ghost starts at player location. "
			     "Assign LastTutorialCineCamera in the instance Details panel for best results."));
	}

	// ── B. FIND THE SK_MANNY PROXY BY TAG ────────────────────────────────────
	//
	// The Spawnable SK must have:
	//   1. Actor Tag = ProxySkeletonTag (default "SKM_Manny")
	//   2. When Finished = Keep State
	// This keeps it alive after OnStop so we can query its bones.
	USkeletalMeshComponent* ProxyMesh   = nullptr;
	AActor*                 ProxyActor  = nullptr;

	if (!ProxySkeletonTag.IsNone())
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), ProxySkeletonTag, Tagged);

		if (Tagged.Num() > 0)
		{
			ProxyActor = Tagged[0];
			ProxyMesh  = ProxyActor->FindComponentByClass<USkeletalMeshComponent>();

			UE_LOG(LogTemp, Log,
				TEXT("ATutorialDirector: Found proxy SK '%s'."), *ProxyActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ATutorialDirector::OnIntroSequenceFinished — No actor tagged '%s'. "
				     "Camera will travel to its current position. "
				     "Verify the Spawnable SK_Manny track has this Actor Tag in Sequencer "
				     "AND that When Finished is set to Keep State."),
				*ProxySkeletonTag.ToString());
		}
	}

	// ── C. DETERMINE CAMERA TRAVEL TARGET (head bone) ────────────────────────
	//
	// The ghost camera will smoothly travel to this world position over
	// CameraBlendTime seconds. Using the head bone makes the transition
	// arrive at the character's eyes regardless of the SK's actor pivot.
	FVector CameraTarget;

	if (ProxyMesh && ProxyMesh->DoesSocketExist(HeadBoneName))
	{
		CameraTarget = ProxyMesh->GetBoneLocation(HeadBoneName);
		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector: Camera target = head bone '%s' at %s."),
			*HeadBoneName.ToString(), *CameraTarget.ToString());
	}
	else if (ProxyActor)
	{
		// SK has no readable head bone — fall back to the actor's eye height.
		CameraTarget = ProxyActor->GetActorLocation()
		             + FVector(0.f, 0.f, CachedPlayer->BaseEyeHeight);
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector: Head bone '%s' not found. "
			     "Using proxy actor location + EyeHeight as fallback."),
			*HeadBoneName.ToString());
	}
	else
	{
		// No proxy at all — camera arrives at its own current position (no travel).
		CameraTarget = GhostStartLocation;
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector: No proxy mesh found. Camera will not travel."));
	}

	// ── D. DETERMINE PLAYER SPAWN TRANSFORM (root bone) ──────────────────────
	//
	// The player pawn's actor origin sits at the capsule base (floor level).
	// The root bone of the Manny skeleton is also at floor level.
	// This gives us the correct foot position independent of the actor pivot.
	FTransform PlayerSpawnTransform;

	if (ProxyMesh && ProxyMesh->DoesSocketExist(RootBoneName))
	{
		FVector RootLoc = ProxyMesh->GetBoneLocation(RootBoneName);

		// Use the proxy actor's yaw for the player's facing direction.
		// Keep pitch and roll at 0 — the player should always start level.
		FRotator SpawnRot(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);

		PlayerSpawnTransform = FTransform(SpawnRot, RootLoc, FVector::OneVector);

		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector: Player spawn = root bone '%s' at %s, yaw %.1f."),
			*RootBoneName.ToString(), *RootLoc.ToString(), SpawnRot.Yaw);
	}
	else if (ProxyActor)
	{
		// No root bone — use the actor's location with its yaw.
		FRotator SpawnRot(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);
		PlayerSpawnTransform = FTransform(SpawnRot, ProxyActor->GetActorLocation());

		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector: Root bone '%s' not found. Using proxy actor location."),
			*RootBoneName.ToString());
	}
	else
	{
		// No proxy — player stays at their current (hidden) transform.
		PlayerSpawnTransform = CachedPlayer->GetActorTransform();
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector: No proxy found. Player will appear at PlayerStart."));
	}

	// ── E. REVEAL PERSISTENT SHIP ACTOR ──────────────────────────────────────
	//
	// The SHIP_1 Spawnable disappears when the sequence ends (Sequencer destroys it).
	// We copy its final world transform to PersistentShipActor and show it,
	// so the ship appears to stay in the world seamlessly.
	if (PersistentShipActor)
	{
		// Try to copy the Spawnable ship's transform if it is still alive.
		if (!CutsceneShipTag.IsNone())
		{
			TArray<AActor*> ShipActors;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), CutsceneShipTag, ShipActors);

			if (ShipActors.Num() > 0 && IsValid(ShipActors[0]))
			{
				PersistentShipActor->SetActorTransform(
					ShipActors[0]->GetActorTransform(),
					false, nullptr, ETeleportType::TeleportPhysics);

				UE_LOG(LogTemp, Log,
					TEXT("ATutorialDirector: PersistentShipActor placed at Spawnable ship transform."));
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("ATutorialDirector: No actor tagged '%s'. PersistentShipActor stays at its "
					     "level position. Place the ship manually or tag the Spawnable."),
					*CutsceneShipTag.ToString());
			}
		}

		PersistentShipActor->SetActorHiddenInGame(false);
		PersistentShipActor->SetActorEnableCollision(true);
		UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: PersistentShipActor revealed."));
	}

	// ── F. BIND HANDOFF CALLBACK ──────────────────────────────────────────────
	CinematicHandoff->OnHandoffComplete.AddUniqueDynamic(
		this, &ATutorialDirector::OnCinematicHandoffComplete);

	// ── G. BEGIN HANDOFF ──────────────────────────────────────────────────────
	//
	// The component will:
	//   1. Spawn a Ghost ACameraActor at GhostStartLocation (= last CineCamera).
	//   2. Snap PC view to ghost (zero time — image unchanged).
	//   3. Tick every frame: travel ghost from GhostStart → CameraTarget (head bone).
	//      Smooth-step easing, duration = CinematicHandoff->CameraBlendTime.
	//   4. On arrival: teleport player to PlayerSpawnTransform (still hidden).
	//      Destroy SK proxy. Unhide player. Snap PC view to player. Destroy ghost.
	//   5. Fire OnHandoffComplete → OnCinematicHandoffComplete().
	CinematicHandoff->BeginHandoff(
		CachedPlayer,
		CachedPC,
		GhostStartLocation,
		GhostStartRotation,
		CameraTarget,
		PlayerSpawnTransform,
		ProxyActor);       // proxy is destroyed when ghost arrives at target
}

// ─────────────────────────────────────────────────────────────────────────────
// OnCinematicHandoffComplete
// Ghost reached the head bone. Player has camera control. Safe to restore input.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnCinematicHandoffComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. RESTORE MOVEMENT INPUT ─────────────────────────────────────────────
	RestorePlayerMoveInput();

	// ── 2. GAME INPUT MODE ───────────────────────────────────────────────────
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	// ── 3. SAVE CRATER TRANSFORM (position + rotation) ────────────────────────
	// Player is now at the correct gameplay start position and facing direction.
	// Store both so the OxygenSphere boundary guard can fully restore the player
	// (position AND facing) if they wander outside the tutorial area.
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
		TEXT("ATutorialDirector: Intro handoff complete. Crater=%s Rot=%s."),
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

	// Restore BOTH position AND rotation. Fixes the old "wrong facing" bug.
	CachedPlayer->SetActorLocationAndRotation(
		CraterStartTransform.GetLocation(),
		CraterStartTransform.GetRotation().Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);

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

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}
