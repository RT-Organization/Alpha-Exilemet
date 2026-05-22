#include "CinematicHandoffComponent.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UCinematicHandoffComponent::UCinematicHandoffComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginHandoff
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::BeginHandoff(
	AActor*                  LastCineCamera,
	AAlphaExilemetCharacter* Player,
	APlayerController*       PC,
	FTransform               PlayerSpawnTransform)
{
	if (bHandoffInProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UCinematicHandoffComponent::BeginHandoff — already in progress. Ignoring call."));
		return;
	}

	if (!Player || !PC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UCinematicHandoffComponent::BeginHandoff — Player or PC is null. "
			     "Broadcasting OnHandoffComplete immediately as fallback."));
		OnHandoffComplete.Broadcast();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("UCinematicHandoffComponent::BeginHandoff — No World."));
		return;
	}

	bHandoffInProgress = true;
	CachedPC = PC;

	// ── STEP 1: Determine ghost camera transform ──────────────────────────────
	//
	// We want the ghost to sit EXACTLY where the sequence camera was on the
	// last frame. If the caller supplies LastCineCamera, we read its transform.
	// If not, we fall back to the player's FP camera (will cause a snap if the
	// sequence camera was elsewhere — always assign LastCineCamera in the editor).
	FTransform GhostTransform;

	if (LastCineCamera)
	{
		GhostTransform = LastCineCamera->GetActorTransform();
		UE_LOG(LogTemp, Log,
			TEXT("UCinematicHandoffComponent — Ghost at CineCamera pos: %s"),
			*GhostTransform.GetLocation().ToString());
	}
	else
	{
		if (Player->FirstPersonCameraComponent)
		{
			GhostTransform = Player->FirstPersonCameraComponent->GetComponentTransform();
		}
		else
		{
			GhostTransform = Player->GetActorTransform();
		}
		UE_LOG(LogTemp, Warning,
			TEXT("UCinematicHandoffComponent::BeginHandoff — LastCineCamera is null. "
			     "Ghost placed at player's FP camera position. "
			     "Assign LastTutorialCineCamera (or LastWakeUpCineCamera) in the Director "
			     "Details panel for a seamless transition."));
	}

	// ── STEP 2: Spawn the ghost camera ───────────────────────────────────────
	//
	// An ACameraActor placed at the sequence's final camera transform.
	// The PC view snaps to it with zero blend time — identical to what the
	// player was already seeing — so there is zero visual discontinuity.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Name = TEXT("CinematicHandoff_GhostCamera");

	GhostCamera = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(), GhostTransform, SpawnParams);

	if (!GhostCamera)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UCinematicHandoffComponent::BeginHandoff — Ghost camera spawn failed. "
			     "Skipping handoff; broadcasting OnHandoffComplete."));
		bHandoffInProgress = false;
		OnHandoffComplete.Broadcast();
		return;
	}

	// Snap view to ghost (zero blend time — no visible change).
	// This "freezes" the image at the sequence's last frame regardless of
	// what UE5 does internally when it releases the Camera Cut track.
	PC->SetViewTargetWithBlend(GhostCamera, 0.0f);

	// ── STEP 3: Teleport player to spawn transform ────────────────────────────
	//
	// Still hidden — nobody sees this happen. The ghost camera holds the view.
	Player->SetActorLocationAndRotation(
		PlayerSpawnTransform.GetLocation(),
		PlayerSpawnTransform.GetRotation().Rotator(),
		false,      // bSweep
		nullptr,    // OutSweepHitResult
		ETeleportType::TeleportPhysics);

	// Align control rotation so the FP camera faces the right direction
	// the moment the blend finishes and the player takes control.
	PC->SetControlRotation(PlayerSpawnTransform.GetRotation().Rotator());

	// Ensure movement mode is restored (it was disabled during the cutscene).
	if (UCharacterMovementComponent* Mv = Player->GetCharacterMovement())
	{
		if (Mv->MovementMode == MOVE_None)
		{
			Mv->SetMovementMode(MOVE_Walking);
		}
	}

	// ── STEP 4: Unhide player ─────────────────────────────────────────────────
	//
	// Player is now in the correct position. The camera is still pointing at
	// the ghost — so unhiding the player is invisible to the viewer.
	Player->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* Mesh = Player->GetMesh())
	{
		Mesh->SetVisibility(true, true); // true = propagate to child components
	}

	// ── STEP 5: Begin blend from ghost → player camera ────────────────────────
	//
	// The view smoothly transitions from the ghost (=sequence end position)
	// to the player's FirstPersonCamera. The ghost is invisible, so when the
	// blend arrives at the player camera, the view just settles into first-person.
	if (CameraBlendTime > 0.0f)
	{
		PC->SetViewTargetWithBlend(
			Player,
			CameraBlendTime,
			BlendFunction,
			BlendExponent,
			false); // bLockOutgoing = false

		GetWorld()->GetTimerManager().SetTimer(
			BlendCompleteHandle,
			this,
			&UCinematicHandoffComponent::OnBlendComplete,
			CameraBlendTime,
			false); // not looping
	}
	else
	{
		// Instant snap.
		PC->SetViewTargetWithBlend(Player, 0.0f);
		OnBlendComplete();
	}

	UE_LOG(LogTemp, Log,
		TEXT("UCinematicHandoffComponent::BeginHandoff — Ghost spawned, blend started (%.2fs). "
		     "Player teleported to %s."),
		CameraBlendTime,
		*PlayerSpawnTransform.GetLocation().ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// CancelHandoff
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::CancelHandoff()
{
	GetWorld()->GetTimerManager().ClearTimer(BlendCompleteHandle);

	if (GhostCamera)
	{
		GhostCamera->Destroy();
		GhostCamera = nullptr;
	}

	bHandoffInProgress = false;
	UE_LOG(LogTemp, Log, TEXT("UCinematicHandoffComponent::CancelHandoff — Handoff cancelled."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnBlendComplete (private)
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::OnBlendComplete()
{
	// Ghost camera is no longer needed. Destroy it before broadcasting so
	// any bound delegates that read the view target see the real player.
	if (GhostCamera)
	{
		GhostCamera->Destroy();
		GhostCamera = nullptr;
	}

	bHandoffInProgress = false;

	UE_LOG(LogTemp, Log,
		TEXT("UCinematicHandoffComponent::OnBlendComplete — Handoff finished. "
		     "Player has camera control."));

	// Notify the Director so it can restore input, show HUD, etc.
	OnHandoffComplete.Broadcast();
}
