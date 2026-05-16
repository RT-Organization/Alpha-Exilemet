#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WakeUpDirector.generated.h"

class ULevelSequencePlayer;
class ALevelSequenceActor;
class AAlphaExilemetCharacter;
class APlayerController;

// ─────────────────────────────────────────────────────────────────────────────
// AWakeUpDirector  v1
//
// Placed in the Main level. Handles the "wake up from tutorial" cutscene that
// plays once — the first time the player arrives in Main from the Tutorial.
//
// WORKFLOW:
//   1. Placed BP_WakeUpDirector in the Main level.
//   2. Assign WakeUpSequenceRef in the instance Details panel (same as TutorialDirector).
//   3. GM_SimulatorGamemode stores a WakeUpDirectorRef (same pattern as TutorialDirectorRef).
//   4. After SpawnNewGamePlayer finishes setting up the player, GM checks:
//        if (WakeUpDirectorRef is valid && GamePhase was NewGame_Tutorial)
//            → WakeUpDirectorRef → InitializeWakeUp()
//      Otherwise skip (loaded game or already done).
//   5. After the wake-up cutscene ends, player has full control with no inventory.
//
// WHAT IT DOES:
//   - Plays the wake-up cutscene (your animator's sequence).
//   - During cutscene: HUD hidden, input locked to UI-only.
//   - After cutscene:
//       * Returns camera + input to player.
//       * Enables survival.
//       * Broadcasts OnTutorialPlayerReady so WB_TutorialBlackout removes itself.
//
// WHAT IT DOES NOT DO:
//   - It does NOT manage inventory (player has none — tutorial pickaxe was cleared).
//   - It does NOT save the game (first real save happens when player chooses to).
//   - It does NOT spawn the ship terminal warning (separate actor handles that).
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API AWakeUpDirector : public AActor
{
	GENERATED_BODY()

public:
	AWakeUpDirector();

protected:
	virtual void BeginPlay() override;

public:
	// ── LEVEL REFERENCE ───────────────────────────────────────────────────────

	/**
	 * The LevelSequenceActor for the wake-up cutscene in the Main level.
	 * Assign once: select placed BP_WakeUpDirector → Details
	 *   → WakeUp|Config → Wake Up Sequence → drag sequence from Outliner.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Wake Up Sequence"))
	ALevelSequenceActor* WakeUpSequenceRef = nullptr;

	/**
	 * Optional: camera actor for the cinematic view before sequence plays.
	 * Leave null if the LevelSequence Camera Cut track handles the view.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "WakeUp|Config",
		meta = (DisplayName = "Cinecam Actor"))
	AActor* WakeUpCineCamRef = nullptr;

	// ── RUNTIME STATE ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	ULevelSequencePlayer* WakeUpSequencePlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	AAlphaExilemetCharacter* CachedPlayer = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WakeUp|Runtime")
	APlayerController* CachedPC = nullptr;

	// ── PUBLIC INTERFACE ──────────────────────────────────────────────────────

	/**
	 * Called by GM_SimulatorGamemode inside SpawnNewGamePlayer,
	 * ONLY when transitioning from Tutorial (GamePhase == NewGame_Tutorial).
	 *
	 * GM BP check (two nodes after player setup):
	 *   [WakeUpDirectorRef → Is Valid]
	 *     True → [InitializeWakeUp  (Target = WakeUpDirectorRef)]
	 *   False → skip (loaded game)
	 */
	UFUNCTION(BlueprintCallable, Category = "WakeUp")
	void InitializeWakeUp();

protected:
	// ── BLUEPRINT IMPLEMENTABLE EVENTS ────────────────────────────────────────

	/**
	 * Called at the END of C++ BeginPlay.
	 * Push self-reference to the GM so GM never needs to search.
	 *
	 *   [Event BP_RegisterWithGameMode]
	 *     → [Get Game Mode → Cast To GM_SimulatorGamemode]
	 *     → [SET WakeUpDirectorRef  (Value = Self)]
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_RegisterWithGameMode();

	/**
	 * Called when the wake-up cutscene ends.
	 * Implement here: remove WB_TutorialBlackout reference if still present,
	 * show the main HUD (Player HUD Ref → Set Visibility Visible).
	 *
	 * The widget removes itself via OnTutorialPlayerReady delegate — this event
	 * is for any extra visual polish (fade-in of HUD, etc.).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WakeUp|Events")
	void BP_OnWakeUpComplete();

private:
	UFUNCTION()
	void OnWakeUpSequenceFinished();
};
