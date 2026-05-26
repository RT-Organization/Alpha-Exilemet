#include "CinematicHandoffComponent.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

UCinematicHandoffComponent::UCinematicHandoffComponent()
{
	// Tick is OFF by default. Enabled only while the ghost is travelling.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginHandoff
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::BeginHandoff(
	AAlphaExilemetCharacter* Player,
	APlayerController*       PC,
	FVector                  GhostStartLocation,
	FRotator                 GhostStartRotation,
	FVector                  CameraTargetLocation,
	FTransform               PlayerSpawnTransform,
	AActor*                  ProxyActorToDestroy)
{
	if (bHandoffInProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UCinematicHandoffComponent::BeginHandoff — already in progress. Ignoring."));
		return;
	}

	if (!Player || !PC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UCinematicHandoffComponent::BeginHandoff — Player or PC is null. "
			     "Firing OnHandoffComplete as immediate fallback."));
		OnHandoffComplete.Broadcast();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("UCinematicHandoffComponent::BeginHandoff — no World."));
		return;
	}

	// ── CACHE STATE ───────────────────────────────────────────────────────────
	bHandoffInProgress  = true;
	CachedPlayer        = Player;
	CachedPC            = PC;
	PendingPlayerSpawn  = PlayerSpawnTransform;
	PendingProxyToDestroy = ProxyActorToDestroy;

	// ── TRAVEL PARAMETERS ─────────────────────────────────────────────────────
	TravelStartPos  = GhostStartLocation;
	TravelEndPos    = CameraTargetLocation;
	TravelStartQuat = GhostStartRotation.Quaternion();
	TravelElapsed   = 0.0f;

	// The ghost's END rotation: look in the direction the player will face
	// (yaw of PlayerSpawnTransform), pitch = 0 so the player starts looking level.
	FRotator EndRot(0.0f, PlayerSpawnTransform.GetRotation().Rotator().Yaw, 0.0f);
	TravelEndQuat = EndRot.Quaternion();

	// ── SPAWN GHOST CAMERA AT LAST CINECAMERA POSITION ────────────────────────
	//
	// The ghost is placed at the exact world transform of the last-active
	// CineCamera. Snapping the PC view target to it (zero blend time) is
	// INVISIBLE because the image is identical to the sequence's last frame.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Name = TEXT("CinematicHandoff_Ghost");

	GhostCamera = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(),
		FTransform(GhostStartRotation, GhostStartLocation),
		SpawnParams);

	if (!GhostCamera)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UCinematicHandoffComponent::BeginHandoff — Ghost camera failed to spawn. "
			     "Firing OnHandoffComplete as fallback."));
		bHandoffInProgress = false;
		OnHandoffComplete.Broadcast();
		return;
	}

	// Freeze the view at the sequence's last frame.
	// Zero blend time = instant snap. Image does not change at all.
	PC->SetViewTargetWithBlend(GhostCamera, 0.0f);

	// ── ENABLE TICK → START TRAVEL ────────────────────────────────────────────
	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Log,
		TEXT("UCinematicHandoffComponent: Ghost spawned. "
		     "Travelling %.2fs from %s → %s (head bone)."),
		CameraBlendTime,
		*GhostStartLocation.ToString(),
		*CameraTargetLocation.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// TickComponent — moves the ghost toward the head bone every frame
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHandoffInProgress || !GhostCamera) return;

	// Advance time, clamp to total duration.
	TravelElapsed = FMath::Min(TravelElapsed + DeltaTime, CameraBlendTime);

	// Normalized alpha [0, 1].
	const float Alpha = (CameraBlendTime > KINDA_SMALL_NUMBER)
		? (TravelElapsed / CameraBlendTime)
		: 1.0f;

	// Smooth-step for a cinematic ease-in / ease-out feel.
	// SmoothStep: slow start, fast middle, slow arrival — matches how AAA games
	// approach camera-to-character transitions.
	const float Smooth = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	// Lerp position.
	const FVector NewPos = FMath::Lerp(TravelStartPos, TravelEndPos, Smooth);

	// Slerp rotation (shortest arc, no gimbal lock).
	const FQuat NewQuat = FQuat::Slerp(TravelStartQuat, TravelEndQuat, Smooth);

	GhostCamera->SetActorLocationAndRotation(
		NewPos, NewQuat.Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);

	// Arrival check.
	if (Alpha >= 1.0f)
	{
		SetComponentTickEnabled(false);
		OnArrivalAtTarget();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnArrivalAtTarget
// Ghost has reached the SK_Manny head bone position. Hand off to the player.
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::OnArrivalAtTarget()
{
	if (!CachedPlayer || !CachedPC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UCinematicHandoffComponent::OnArrivalAtTarget — CachedPlayer or CachedPC null."));
		if (GhostCamera) { GhostCamera->Destroy(); GhostCamera = nullptr; }
		bHandoffInProgress = false;
		OnHandoffComplete.Broadcast();
		return;
	}

	// ── 1. TELEPORT PLAYER TO ROOT BONE POSITION ──────────────────────────────
	//
	// Player is still hidden. The ghost camera is at the head bone.
	// We place the player at the root bone position with the correct yaw.
	// The FP camera (attached to the head socket) will end up at the head bone.
	CachedPlayer->SetActorLocationAndRotation(
		PendingPlayerSpawn.GetLocation(),
		PendingPlayerSpawn.GetRotation().Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);

	// Align control rotation so the camera faces the right direction
	// the moment the player takes over.
	CachedPC->SetControlRotation(PendingPlayerSpawn.GetRotation().Rotator());

	// ── 2. RESTORE MOVEMENT MODE ──────────────────────────────────────────────
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
	{
		if (Mv->MovementMode == MOVE_None)
			Mv->SetMovementMode(MOVE_Walking);
	}

	// ── 3. DESTROY SK PROXY ───────────────────────────────────────────────────
	//
	// Remove the proxy BEFORE unhiding the player so there is never a frame
	// where both meshes are visible at the same position.
	if (PendingProxyToDestroy && IsValid(PendingProxyToDestroy))
	{
		PendingProxyToDestroy->Destroy();
		PendingProxyToDestroy = nullptr;
	}

	// ── 4. UNHIDE PLAYER ──────────────────────────────────────────────────────
	//
	// Ghost is at the head → player FP camera is also at the head.
	// Unhiding the player is invisible because the view is still at the ghost.
	CachedPlayer->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
	{
		Mesh->SetVisibility(true, true); // propagate to child components
	}

	// ── 5. SNAP PC VIEW TO PLAYER ─────────────────────────────────────────────
	//
	// Ghost location == head bone == player FP camera location.
	// Zero blend time → true seamless swap.
	// FinalSnapBlendTime can be set to 0.1–0.2 in edge cases where a tiny
	// position discrepancy is visible.
	if (FinalSnapBlendTime > KINDA_SMALL_NUMBER)
	{
		CachedPC->SetViewTargetWithBlend(
			CachedPlayer, FinalSnapBlendTime,
			EViewTargetBlendFunction::VTBlend_EaseInOut, 2.0f);
	}
	else
	{
		CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);
	}

	// ── 6. DESTROY GHOST ──────────────────────────────────────────────────────
	if (GhostCamera)
	{
		GhostCamera->Destroy();
		GhostCamera = nullptr;
	}

	bHandoffInProgress = false;

	UE_LOG(LogTemp, Log,
		TEXT("UCinematicHandoffComponent: Arrived. Player placed at %s, facing %s. Handoff complete."),
		*PendingPlayerSpawn.GetLocation().ToString(),
		*PendingPlayerSpawn.GetRotation().Rotator().ToString());

	// Notify the Director: safe to restore input, show HUD, etc.
	OnHandoffComplete.Broadcast();
}

// ─────────────────────────────────────────────────────────────────────────────
// CancelHandoff
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::CancelHandoff()
{
	SetComponentTickEnabled(false);

	if (GhostCamera)
	{
		GhostCamera->Destroy();
		GhostCamera = nullptr;
	}

	bHandoffInProgress = false;
	UE_LOG(LogTemp, Log, TEXT("UCinematicHandoffComponent::CancelHandoff — Cancelled."));
}
