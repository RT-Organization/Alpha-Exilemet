#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelStreamingManager.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// ENUMS
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EGameLevel : uint8
{
	None            UMETA(DisplayName = "None"),
	MainMenu        UMETA(DisplayName = "Main Menu"),
	Tutorial        UMETA(DisplayName = "Tutorial"),
	Main            UMETA(DisplayName = "Main"),
	Portal_Desert   UMETA(DisplayName = "Portal_Desert"),
	Portal_Canyon   UMETA(DisplayName = "Portal_Canyon"),
	Portal_Ice      UMETA(DisplayName = "Portal_Ice"),
	Portal_Lava     UMETA(DisplayName = "Portal_Lava"),
	Portal_Swamp    UMETA(DisplayName = "Portal_Swamp"),
};

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelTransitionComplete, EGameLevel, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelTransitionStarted, EGameLevel, FromLevel, EGameLevel, ToLevel);

// ─────────────────────────────────────────────────────────────────────────────
// ULevelStreamingManager
//
// SINGLE AUTHORITY for all level streaming.
//
// ══════════════════════════════════════════════════════════════════════════════
// GM BLUEPRINT WIRING  (read this before touching any Blueprint)
// ══════════════════════════════════════════════════════════════════════════════
//
// ── InitializeInstance ────────────────────────────────────────────────────────
//  1. GetGameInstance → Cast to AlphaExilemetGameInstance → SET Instance Ref
//  2. InitProgressionManager (target = Instance Ref)
//  3. LevelStreamingManager → Bind OnLevelTransitionStarted
//                              → Create Event (OnLevelTransitionStarted_Handler)
//  4. LevelStreamingManager → Bind OnLevelTransitionComplete
//                              → Create Event (OnLevelTransitionComplete_Handler)
//  5. GetCurrentLevelName → Branch (== "L_Persistent")
//       TRUE  → LevelStreamingManager → InitializeAtMainMenu (pass LoadingScreen class)
//               → AlphaStreamingSubsystem → CallOnTutorialPlayerReady
//       FALSE → LevelStreamingManager → InitializeForDirectPlay
//               (OnLevelTransitionComplete(Main) fires one frame later automatically)
//
// ── OnLevelTransitionStarted_Handler (FromLevel, ToLevel) ────────────────────
//  Branch: IsPortalLevel(ToLevel)?
//    TRUE  → do NOTHING.
//            The player pawn stays alive and travels with us conceptually.
//            C++ handles survival flag. DO NOT destroy the pawn here.
//    FALSE → GetPlayerCharacter → Cast → CleanupForLevelTransition → DestroyActor
//            SET TutorialDirectorRef = null
//            SET WakeUpDirectorRef   = null
//            SET PlayerRef           = null
//            SET ShowMouseCursor     = true
//            GetActorOfClass(DinoCamActor) → GetPlayerController
//                                         → SetViewTargetWithBlend (Time=0)
//
// ── OnLevelTransitionComplete_Handler (NewLevel) ──────────────────────────────
//  Switch on EGameLevel:
//
//    None        → (ignore)
//    MainMenu    → camera already forced by C++; show MainMenu UI if needed
//    Tutorial    → SpawnNewGamePlayer (custom event)
//    Main        → Branch: LevelStreamingManager.bIsPortalExit?
//                    TRUE  → [Portal Exit Path]
//                             GET PlayerRef from GameInstance
//                             SET PlayerRef on GM
//                             SetupPlayerGame (player already placed by C++)
//                             SET IsSurvivalActive = true
//                    FALSE → SpawnNewGamePlayer (custom event)
//    Portal_X    → [Portal Enter Path]
//                  LevelStreamingManager → TeleportPlayerToPortalStart
//                  SET IsSurvivalActive = false on PlayerRef
//                  SetupPlayerGame
//
// ── SpawnNewGamePlayer (custom event) ─────────────────────────────────────────
//  Delay(0.3) → InitializePlayer(self) → SetupPlayerGame → ...existing chain...
//
// ══════════════════════════════════════════════════════════════════════════════

UCLASS()
class ALPHAEXILEMET_API ULevelStreamingManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── CONFIGURATION ─────────────────────────────────────────────────────────

	/** Loading screen widget. Must implement StartFadeIn / StartFadeOut BP events. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LevelStreaming|Config")
	TSubclassOf<UUserWidget> LoadingScreenClass;

	/** Minimum seconds the loading screen stays visible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LevelStreaming|Config")
	float MinLoadingScreenTime = 2.0f;

	// ── STATE ─────────────────────────────────────────────────────────────────

	/** Currently active / target level. Always authoritative. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelStreaming|State")
	EGameLevel CurrentLevel = EGameLevel::None;

	/** True while any streaming transition is in progress. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelStreaming|State")
	bool bTransitionInProgress = false;

	/**
	 * True from the moment ExitPortal() begins until OnLevelTransitionComplete(Main) fires.
	 *
	 * READ THIS in the GM's OnLevelTransitionComplete_Handler on the Main pin:
	 *
	 *   Branch: LevelStreamingManager → bIsPortalExit ?
	 *     TRUE  → C++ already placed the player at PortalReturnTransform.
	 *             Just do: GET PlayerRef, SET on GM, SetupPlayerGame, SET IsSurvivalActive=true
	 *     FALSE → normal path (SpawnNewGamePlayer etc.)
	 *
	 * Automatically reset to false after OnLevelTransitionComplete fires.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelStreaming|State")
	bool bIsPortalExit = false;

	// ── DELEGATES ─────────────────────────────────────────────────────────────

	/** Fires when the new level is loaded AND MinLoadingScreenTime has elapsed. */
	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming")
	FOnLevelTransitionComplete OnLevelTransitionComplete;

	/** Fires at the START of a transition (before any I/O). Use for pawn cleanup. */
	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming")
	FOnLevelTransitionStarted OnLevelTransitionStarted;

	// ── PUBLIC TRANSITION FUNCTIONS ───────────────────────────────────────────

	/** Start a brand-new game. bSeamless=true skips the loading screen. */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void StartNewGame(const FString& SlotName, bool bSeamless = false);

	/** Load a saved game slot. Called by WB_LoadMenu. */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void LoadSavedGame(const FString& SlotName);

	/** Return to MainMenu from any level. */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void ReturnToMainMenu();

	/** Called at end of Tutorial skull sequence (via AlphaStreamingSubsystem). */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void CompleteTutorialAndLoadMain();

	/** Enter a portal challenge level. Called by PortalBase::EnterPortalLevel. */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void EnterPortal(EGameLevel PortalLevel, FTransform PlayerReturnTransform);

	/**
	 * Complete the portal challenge with NO delay.
	 * Broadcasts OnPortalExitStarted then immediately begins the exit transition.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void CompletePortalChallenge();

	/**
	 * Complete the portal challenge after ExploreDelay seconds.
	 * The loading screen appears only AFTER the delay — the player can still move.
	 * ExploreDelay = 0 behaves identically to CompletePortalChallenge().
	 *
	 * CALL THIS FROM BP_QuestManager (VerificaOrdine).
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void CompletePortalChallengeWithDelay(float ExploreDelay = 5.0f);

	/** Debug: force-exit portal immediately, cancelling any pending delay. */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming|Debug")
	void Debug_ForceExitPortal();

	// ── SETUP ─────────────────────────────────────────────────────────────────

	/**
	 * Call from GM InitializeInstance on the L_Persistent path.
	 * Sets CurrentLevel = MainMenu and stores the LoadingScreen class.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void InitializeAtMainMenu(TSubclassOf<UUserWidget> InLoadingScreenClass);

	/**
	 * Call from GM InitializeInstance on the direct-play path (any non-Persistent level).
	 *
	 * Sets CurrentLevel = Main and fires OnLevelTransitionComplete(Main) after one
	 * frame so all event bindings registered in InitializeInstance are live first.
	 *
	 * The GM's OnLevelTransitionComplete_Handler will then run the Main pin normally,
	 * detect bIsPortalExit=false, and call SpawnNewGamePlayer.
	 * Since CurrentPhase stays EGamePhase::None, survival is NOT activated.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming|Debug")
	void InitializeForDirectPlay();

	// ── PORTAL HELPERS ────────────────────────────────────────────────────────

	/**
	 * Teleports the player to the PlayerStart inside the active portal sublevel.
	 * Call this from GM's OnLevelTransitionComplete_Handler on every Portal_X pin.
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelStreaming")
	void TeleportPlayerToPortalStart();

	// ── UTILITY ───────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "LevelStreaming")
	static FName LevelToName(EGameLevel Level);

	UFUNCTION(BlueprintPure, Category = "LevelStreaming")
	static EGameLevel NameToLevel(FName Name);

	UFUNCTION(BlueprintPure, Category = "LevelStreaming")
	static bool IsPortalLevel(EGameLevel Level);

	// ── LEGACY DELEGATES (kept for existing BP bindings) ──────────────────────

	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming|Legacy")
	FOnLevelTransitionStarted OnPortalEnterComplete;

	UPROPERTY(BlueprintAssignable, Category = "LevelStreaming|Legacy")
	FOnLevelTransitionStarted OnPortalExitStarted;

private:
	// ── INTERNAL STATE ────────────────────────────────────────────────────────

	EGameLevel PendingLevel       = EGameLevel::None;

	/**
	 * Set to the level we were in when EnterPortal() was called.
	 * OnBothConditionsMet checks IsPortalLevel(PortalReturnFrom) to know whether
	 * to apply the deferred return teleport.
	 * Reset to None immediately after the teleport fires.
	 */
	EGameLevel PortalReturnFrom   = EGameLevel::None;

	/** Pre-portal player transform. Applied in OnBothConditionsMet after Main loads. */
	FTransform PortalReturnTransform;

	bool  bLevelLoaded        = false;
	bool  bMinTimeElapsed     = false;
	bool  bSeamlessTransition = false;

	int32 LoadLatentUUID      = 0;
	int32 UnloadLatentUUID    = 1000;

	TWeakObjectPtr<UUserWidget> ActiveLoadingScreen;

	// ── TRANSITION PIPELINE ───────────────────────────────────────────────────

	void BeginTransition(EGameLevel NewLevel, EGameLevel OldLevel, bool bSeamless = false);

	/**
	 * Internal portal exit. Never call from Blueprint.
	 * Use CompletePortalChallenge() or CompletePortalChallengeWithDelay() instead.
	 */
	void ExitPortal();

	void ShowLoadingScreen();
	void HideLoadingScreen();

	UFUNCTION() void OnLevelLoaded();
	UFUNCTION() void OnLevelUnloaded();
	void OnMinTimeElapsed();
	void OnBothConditionsMet();

	FTimerHandle MinTimeHandle;

	/**
	 * One-frame delay timer for InitializeForDirectPlay.
	 * Ensures OnLevelTransitionComplete fires AFTER InitializeInstance has
	 * finished registering all event bindings.
	 */
	FTimerHandle DirectPlayTimerHandle;
	UFUNCTION() void DirectPlayTimerCallback();

	/** Delay timer for CompletePortalChallengeWithDelay. */
	FTimerHandle PortalExitDelayHandle;
	UFUNCTION() void PortalExitDelayCallback();

	void DestroyPlayerPawn();
	void ForceCameraToMainMenuCineCamera();
	class AAlphaExilemetCharacter* GetLocalPlayer() const;
};