# Home Slice Implementation

Status: phases 1-7 complete. Phase 8 remains paused pending the Home assets and maps.

## Ownership

`ALRGameMode` assembles `ALRCharacter`, `ALRPlayerController`, `ALRGameState`, and `ALRHUD`.
`ALRCharacter` only composes movement, state, interaction, inventory, noise, hiding, and state presentation components; it does not tick.
`ULRGameInstanceSubsystem` owns validated content and tuning roots. `ULRDialogueSubsystem` owns DataTable traversal, condition checks, branches, and one-shot narrative events. Widgets receive immutable presentation data only.

`ULRPlayerUIComponent` coordinates narrative and menu requests. `ALRPlayerController` remains the only owner of Enhanced Input mapping contexts, input modes, viewport focus, and cursor state. Item selection in the menu invokes `ULRInventoryComponent::UseItemFromSelector`, which shares the same resolver transaction as quick slots.

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
