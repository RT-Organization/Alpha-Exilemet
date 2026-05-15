#include "TutorialRock.h"
#include "AlphaExilemet/Resources/ResourceBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

void ATutorialRock::DepleteResource()
{
	// 1. Spawn destruction VFX at this rock's world location (if assigned).
	//    UNiagaraFunctionLibrary::SpawnSystemAtLocation is fire-and-forget —
	//    the emitter manages its own lifetime, nothing to track.
	if (DestructionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DestructionVFX,
			GetActorLocation(),
			GetActorRotation(),
			FVector(DestructionVFXScale),
			true,   // bAutoDestroy
			true,   // bAutoActivate
			ENCPoolMethod::None);
	}

	// 2. Call AResourceBase::DepleteResource() directly.
	//    This hides the actor, disables collision, and starts the regen timer.
	//    We intentionally SKIP ASolidResource::DepleteResource() to avoid
	//    ore-drop spawning — tutorial rocks must not drop ore.
	AResourceBase::DepleteResource();

	// 3. Notify BP_TutorialDirector so it can count destroyed rocks and
	//    trigger the skull-reveal sequence when all rocks are gone.
	OnTutorialRockDestroyed.Broadcast(this);
}
