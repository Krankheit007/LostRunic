# Home Slice Implementation

Status: phases 1-7 complete. Phase 8 remains paused pending the Home assets and maps.

## Ownership

`ALRGameMode` assembles `ALRCharacter`, `ALRPlayerController`, `ALRGameState`, and `ALRHUD`.
`ALRCharacter` only composes movement, state, interaction, inventory, noise, hiding, and state presentation components; it does not tick.
`ULRGameInstanceSubsystem` owns validated content and tuning roots. `ULRDialogueSubsystem` owns DataTable traversal, condition checks, branches, and one-shot narrative events. Widgets receive immutable presentation data only.

`ULRPlayerUIComponent` coordinates narrative and menu requests. `ALRPlayerController` remains the only owner of Enhanced Input mapping contexts, input modes, viewport focus, and cursor state.

## Item System (2026-08-12)

The four-slot quick bar and the standalone weapon system were removed. The inventory now stores per-item stack entries (`FLRInventoryEntry`: `ItemId`, `Quantity`, `AcquisitionSequence`); `Quantity` is also the remaining use count for consumables, and infinite-use items are pinned to one. `AddItem` returns `ELRAddItemResult` (`Success`/`InventoryFull`/`InvalidDefinition`/`InvalidQuantity`) and refuses any add beyond `MaxStackSize`; world pickups hide only after a successful `AddItem`.

`ULRItemActionComponent` on `ALRCharacter` is the only item action entry: `RequestUseItem(ItemId, Target)` (target must implement `ILRItemUseTarget`) and `RequestAttack()` (target must implement `ILRAttackTarget`; no weapon resolves to an empty-handed attack). Both share one `ULRItemUseResolver` transaction: entry/request validation, ownership, action-tag declaration, attack-state rules, per-entry target checks, execution, post-success consumption, and structured rejection. `ULRInventoryComponent` is again pure inventory state plus explicit weapon selection (`SetSelectedWeapon`/`GetSelectedWeapon`); `GetEffectiveWeapon` lazily falls back to the earliest-acquired owned weapon and the selection clears immediately when the selected weapon is consumed. Attack distance, facing cone, and cooldown live in `ULRStateTuning` (`CourageAttackRangeCm`, `CourageAttackFacingDegrees`, `CourageAttackCooldownSeconds`).

The unified menu is one UMG asset with Inventory/Notes/Collectibles tabs; `FLRInventorySnapshot` carries per-item views and weapon IDs and contains no quick-slot data. `ALRNoteInteractableActor` records `ReadingId` as soon as the reading session opens; `ALRCollectiblePickupActor` hides only on a `Success` collectible record. `FLRSaveInventoryChunk.QuickSlots`/`SelectedQuickSlot` remain as legacy struct fields that are never populated, never restored, and never surfaced by UI.

## State And Input

| Input mode | Mapping context | Gameplay state blocker | Focus surface |
| --- | --- | --- | --- |
| Gameplay | `IMC_LRGameplay` | None | Game viewport |
| Dialogue | `IMC_LRDialogue` | `State.Blocker.Dialogue` | Narrative screen |
| Menu | `IMC_LRMenu` | `State.Blocker.Menu` | Current menu screen |
| Transition | `IMC_LRTransition` | `State.Blocker.Transition` | Transition screen |

Context replacement sets `bIgnoreAllPressedKeysUntilRelease`. Confirm uses `Started`: while typewriter text is incomplete it reveals the current page; the next confirm advances rules or opens legal choices. Reading uses the same session flow and has no speaker/portrait data.

## Content And IDs

Dialogue and reading row names must exactly match their stable `DialogueId` and `ReadingId`. Dialogue links and options are validated for missing targets and duplicate IDs. Level events use stable `FName EventId`; one-shot completion is retained in `ULRDialogueSubsystem` for later save serialization.

The phase-8 Home content uses these stable authored IDs: `Home_Dorothy_001`, `Home_Dorothy_002`, `Home_Dorothy_003`, `Home_Note_Mother`, and `Home_Doll`. The text intentionally does not establish a final identity for Adele.

Native narrative rejection tags are:

- `Narrative.Reject.NoSession`
- `Narrative.Reject.MissingContent`
- `Narrative.Reject.Conditions`
- `Narrative.Reject.InvalidChoice`
- `Narrative.Reject.AlreadyCompleted`

## Tuning

All runtime tuning is resolved by `ULRGameTuningSet`, loaded by `ULRGameInstanceSubsystem`. `ULRUITuning` is the only source for typewriter speed and refresh interval. `ULRDialogueWidgetController` falls back only to safe defaults when the configured root is unavailable and never encodes narrative text or branching rules.

## UI Assembly

`ALRHUD` has independent class slots for HUD, state overlay, narrative, journal, inventory, collectibles, pause, save slots, and transition screens. `ULRScreenWidget` supplies the common visibility, focusability, and presentation event boundary; Blueprint-derived layouts are created in phase 8.

Phase-8 follow-up (does not block the framework refactor): the current project contains `BP_Ruth` and only the `WBP_HUD` screen asset is wired by default. Confirm in the Home map whether the placed `BP_Ruth` or the GameMode `DefaultPawnClass` should be authoritative, then register the remaining screen classes in the `ALRHUD` slots as their Widget Blueprints are authored. This is intentionally tracked here instead of duplicating Pawn/UI ownership rules in GameMode code.

## Save Transactions

`ULRSaveSubsystem` is the sole runtime owner of persistence and lives on `UGameInstance`, so its working snapshot and transaction phase survive level travel. `ULRSaveGame` contains only `SaveGame` data: `FLRResumeAnchor`, inventory, narrative progress, completed memory events, death count, timestamps, and the v0 migration fields. Runtime actors and definitions are never serialized.

Slot IDs are stable and independent of display names: `LostRunic_Auto` is the single automatic slot and `LostRunic_Manual_01` through `LostRunic_Manual_10` are the bounded manual slots. `ULRSaveTuning` is the only source for the 7.5 second ordinary auto-save debounce, retry count, retry delay, and manual slot count. Every queued write duplicates the working save before entering the game-thread FIFO; asynchronous completion advances the queue and emits success or failure delegates.

Death recovery keeps the Home `FLRResumeAnchor` unchanged throughout Memory. Capture accepts Death, increments the death count, locks transition input, and travels to the registered `Memory` map. World-ready submits critical save A, Memory events are queued in order, and the return request travels to the saved anchor map. World-ready then restores inventory, narrative events, actor transform, and the Memory-to-Normal state request before submitting critical save B. Manual saves are rejected for every non-`None` transaction phase.

Loading a manual or latest save migrates v0 to v1 before applying it. If the current map differs from the saved anchor map, the subsystem travels there and defers application until `HandleWorldReady`; an unavailable registered map produces `RejectedUnavailableMap` and a user-facing load failure delegate. Invalid or corrupt saves produce `MissingOrCorrupt` with a diagnostic message.

## Evidence

Phase 6 verification on 2026-08-09:

- `LostRunicEditor Win64 Development` compiled successfully.
- `Automation RunTests LostRunic` completed with `GIsCriticalError=0` and exit code `0`.
- Data Validation completed with result `0`; 267 assets passed localization validation and 266 package-file validation.
- New tests cover narrative conditions, branching, one-shot events, reading reuse, typewriter two-step confirmation, menu screen legality, and UI tuning bounds.

Phase 7 verification on 2026-08-09:

- `LostRunicEditor Win64 Development` compiled successfully after the save implementation and resume-application fix; only existing UE engine deprecation warnings remain.
- `Automation RunTests LostRunic` discovered and completed 29 tests with `GIsCriticalError=0` and `EXIT CODE: 0`. The five save tests cover v0 migration, slot/manual rules, critical Memory A/B ordering, FIFO requests, immutable snapshots, and migrated anchor transform data.
- `DataValidationCommandlet` completed with result `0`; native validators reported 267 localization and 266 package-file checks with no project content-validation errors. The command host also reports a sandbox-only global Zen/EditorSettings write warning; `-DDC-ForceMemoryCache` keeps the project validation running without modifying project assets.
- `git diff --check` passed. Generated `Binaries/`, `Intermediate/`, `DerivedDataCache/`, and `Saved/` outputs remain outside the tracked source change.

Phase 8 item-system refactor verification on 2026-08-12:

- `LostRunicEditor Win64 Development` compiled successfully after the item-system refactor (quick slots removed, `AttackAction` added, resolver transaction reworked).
- `Automation RunTests LostRunic` completed 41/42 tests: all `LostRunic.Item.*` tests pass (infinite/consumable quantity rules, `InventoryFull` at `MaxStackSize`, definition/quantity validation, weapon selection and acquisition-order fallback, attack state/cooldown/immunity rejection, empty-handed attacks, `InvalidAttackItem` for misconfigured non-weapons, use/attack sharing one resolver, attack target selection rules, note/collectible stable IDs, and the quick-slot-free menu snapshot). The only failure is `LostRunic.Input.ProjectConfigIsComplete`, which requires assigning `IA_LRUseQuickSlot` to the new `AttackAction` slot inside `DA_LRInputConfig` in the editor.

## Outstanding Follow-Up Tasks

The following items remain intentionally unfinished and must be completed in Unreal Editor during the next Home-slice pass:

1. Author the `BP_LRMainMenu` widget Blueprint: bind `OnMenuTabChanged` to the tab switcher, consume `ULRMenuWidgetController.BuildInventorySnapshot` for the inventory page, wire "set as current weapon" to `ULRInventoryComponent.SetSelectedWeapon`, and in interaction-select mode call `ALRPlayerController.UseInventoryItemFromMenu` for the selected item (close the menu on success). Wire the deprecated `UseQuickSlotAction` input asset to the new `AttackAction` slot in `DA_LRInputConfig` and remove the old quick-slot mappings from `IMC_LRGameplay`.
2. In the HUD Widget Blueprint, implement `OnHUDWidgetControllerReady`; bind the injected controller's `OnPerceptionModeChanged` and `OnInteractionPromptChanged` events, then verify the state overlay and interaction prompt in Home PIE.
3. In the `StatePresentation` component on the player Blueprint, bind `OnStatePresentationRequested` to post-process/material/Niagara/animation presentation and call `CompleteStatePresentation` from the animation completion path.
4. Run Home and Memory PIE with keyboard/mouse and controller input. Confirm the project settings `ContentSet`, `TuningSet`, `InputConfig`, and `HUDScreenClass` resolve without `LogLostRunicTuning`, `LogLostRunicUI`, or `LogLostRunicSave` warnings/errors.
5. Complete the `ULRGameContentSet` asset: add valid dialogue/reading tables, unique item/collectible/guard/event definitions, and stable Home/Memory map registrations; run Data Validation after each content change.
6. Confirm whether the placed `BP_Ruth` or the GameMode `DefaultPawnClass` is authoritative for Home, then register the remaining `ALRHUD` screen class slots as their Widget Blueprints are authored.
