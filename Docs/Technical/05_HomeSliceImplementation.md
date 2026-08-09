# Home Slice Implementation

Status: phases 1-6 complete. Phases 7-8 extend this document with save transactions and playable maps.

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

## Evidence

Phase 6 verification on 2026-08-09:

- `LostRunicEditor Win64 Development` compiled successfully.
- `Automation RunTests LostRunic` completed with `GIsCriticalError=0` and exit code `0`.
- Data Validation completed with result `0`; 267 assets passed localization validation and 266 package-file validation.
- New tests cover narrative conditions, branching, one-shot events, reading reuse, typewriter two-step confirmation, menu screen legality, and UI tuning bounds.
