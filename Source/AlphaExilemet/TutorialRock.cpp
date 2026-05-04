#include "TutorialRock.h"
#include "AlphaExilemet/Resources/ResourceBase.h"

void ATutorialRock::DepleteResource()
{
	// Call AResourceBase::DepleteResource() directly.
	// This hides the actor, disables collision, and starts the regen timer —
	// exactly what we want for a tutorial rock.
	//
	// We intentionally SKIP ASolidResource::DepleteResource() because that
	// function reads from the DataTable and spawns ADroppedSolidResource actors.
	// Tutorial rocks must not drop ore.
	AResourceBase::DepleteResource();

	// Notify BP_TutorialDirector that this rock has been destroyed.
	// The Director listens to this to count destroyed rocks and trigger
	// the skull-reveal sequence when all rocks are gone.
	OnTutorialRockDestroyed.Broadcast(this);
}
