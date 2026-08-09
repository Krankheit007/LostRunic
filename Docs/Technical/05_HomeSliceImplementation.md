# Home Slice Implementation

Status: phases 1-8 implemented. `L_Home` is the project default map and `L_Memory` is the death-memory transaction world.

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

Phase 8 provides nine independent widget assets under `/Game/LostRunic/UI`: HUD, state overlay, narrative/reading, journal, inventory, collectibles, pause, save slots, and transition. Each derives from `ULRScreenWidget`; the native Slate fallback supplies a usable layout when a Blueprint has no authored WidgetTree. The Blueprint layer remains presentation-only.

## Phase 8 Assets And Flow

The default experience is `/Game/LostRunic/Levels/Home/L_Home` using `/Game/LostRunic/Blueprints/Framework/BP_LRGameMode`. The death transaction travels to `/Game/LostRunic/Levels/Memory/L_Memory`. Framework, character, guard, interaction, hiding, UI, definition, material, audio, StateTree, and Niagara assets live entirely under `/Game/LostRunic`. The TopDown, Strategy, and TwinStick template maps and their World Partition external packages were removed; their remaining non-map assets and source variants are retained.

Home is assembled from basic geometry and the existing Quinn/Manny mannequins. Its authored route is Dorothy dialogue, Perception key clue, shared-resolver key door, guard corridor with fixed and movable hide points, Courage token and blocked object, Mother's note, cloth doll, then an automatic overlap event with critical save policy. Memory contains the doll investigation, neutral NPC departure event, and return interaction. No Level Blueprint owns inventory, narrative, state, AI, or save rules.

Phase 8 stable IDs are:

- Content: `Home_Dorothy_001`, `Home_Dorothy_002`, `Home_Dorothy_003`, `Home_Note_Mother`, `Home_Doll`.
- Items and AI: `Home.Key`, `Home.CourageCharm`, `Home.Guard.Hall`.
- Home events: `Home.Event.DorothyTalked`, `Home.Event.DoorOpened`, `Home.Event.ObstacleMoved`, `Home.Event.NoteRead`, `Home.Event.DollCollected`, `Home.Event.Important`.
- Memory events: `Memory.Event.DollInvestigated`, `Memory.Event.NPCDeparted`.
- Maps and resume: `Home`, `Memory`, `Home.Hall`.

`ST_LRHomeGuard` is a LostRunic-owned StateTree containing `IdlePatrol`, `Suspicious`, `Investigate`, `Search`, and `Chase`. Runtime guard legality, alert changes, navigation speeds, capture, and fallback behavior remain in the C++ controller/components. The StateTree is a data-driven selection surface and is inspected through UE editor reflection after generation.

No callable UE MCP endpoint was available during asset authoring. Following repository policy, the implementation used the repository UE skills first, then a repeatable Unreal Editor Python script with UE asset factories, Blueprint compilation, StateTree editor-data inspection, and package saves. Generated scripts and temporary wave sources remain under ignored `Saved/` output.

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

Phase 8 verification on 2026-08-10:

- `LostRunicEditor Win64 Development` compiled successfully after final guard lifecycle, StateTree, and widget fallback changes.
- `Automation RunTests LostRunic` completed all 31 tests successfully with no LostRunic, StateTree, or asset-load errors.
- `DataValidationCommandlet` completed project content validation with result `0`: 306 localization checks and 305 package-file/reference checks passed. In the restricted command host, the process-level exit code can still be non-zero because the installed DDC graph has no writable local node; this occurs after successful project validation and does not indicate an asset failure.
- All generated Blueprint assets reported `BS_UP_TO_DATE`; `ST_LRHomeGuard` compiled with one root and the authored `IdlePatrol`, `Suspicious`, `Investigate`, `Search`, and `Chase` states.
- Home and Memory both entered Play World with `BP_LRGameMode_C`. The final Home smoke log contains no project-level StateTree or AI Perception lifecycle warnings.
- The TopDown, Strategy, and TwinStick template maps and their World Partition external packages were deleted at the user's request and are intentionally excluded from smoke coverage. Only `L_Home` and `L_Memory` remain as project maps.
- The phase adds 74 LostRunic assets and two LostRunic maps; no newly added LostRunic binary exceeds 100 MB. Git LFS owns all staged `.uasset` and `.umap` files.
- `LRGuardAIController.cpp` is 353 lines, above the 250-line review threshold but below the 400-line hard limit. It remains one guard-controller lifecycle/perception/navigation-state unit; phase 8 only adds definition-driven initialization, so an unrelated split was not introduced.
