# LostRunic Save System V2 Behavior and API

## Scope

The V2 save system has five user workflows: save selection, automatic save, manual save, automatic load, and manual load. `ULRSaveSubsystem` owns persistence and `ULRGameFlowSubsystem` owns map travel. The two communicate through requests and events; synchronous Save -> Travel -> Save recursion is forbidden.

## System Invariants

1. Every catalog entry points to an existing payload whose identity validates, and `Catalog.PayloadKey == Payload.PayloadKey`.
2. A slot has one committed payload at a time. An old payload may remain orphaned during best-effort cleanup, but the catalog points to only one payload.
3. All persistent operations pass through the single Save Operation Queue. One queue item may own at most one catalog transaction, and the catalog has at most one `PendingOperation`.
4. Map travel and provider initialization complete before the first map-ready automatic save. A partially initialized world is never captured.

## Catalog and Health

Catalog A/B is a reliability layer, not a dynamic-slot mechanism. The catalog never scans payload files to infer slots. Orphan payloads are accepted and are only cleaned up best-effort; no background garbage collector is part of V2.

Deterministic integrity failures (`CatalogMismatch`, `MissingPayload`, unsupported version, invalid identity) may be persisted as slot `Health`. A transient payload read or IO failure is reported for the current operation only and must not permanently mark a healthy slot as corrupt. `Continue` tries candidates in descending save time/sequence order and follows the same rule.

## Save and Load Lifecycle

Save requests enter `ULRSaveSubsystem` and complete through `OnSaveOperationCompleted`. Load reads and validates the payload before emitting `OnSaveLoadRequested`. `ULRGameFlowSubsystem` travels to the registered map, waits for `NotifyWorldReady`, then calls `NotifyLoadWorldReady`; Save restores non-player chunks first, then Player, and completes the operation.

New Game uses `RequestNewGame` and `OnSaveNewGameRequested`. GameFlow travels to `ULRGameContentSet.NewGameMapId`, resolves the registered `DefaultStartAnchorId`, and notifies Save only after the world is ready. Save resets all providers, captures the new world, and commits a new automatic payload through the normal queue. The previous automatic payload remains catalog-referenced until this transaction succeeds.

## Pause and Manual Save

Pause uses `UGameplayStatics::SetGamePaused`. `IA_LRPause` triggers while paused so the same action can resume. Play time is active only in playable maps and excludes paused intervals. Manual save requires `MemoryPhase == None` and `World->IsPaused() == true`; UI state alone is not sufficient.

## UI Controller API

`ULRSaveWidgetController` exposes a read-only `FLRSaveUISnapshot` and `OnSnapshotChanged`. Widgets do not access catalog or payload objects. The controller supports `Open(Save|Load)`, `RequestCreateManualSave`, `RequestPrimarySlotAction`, `RequestDelete`, `ConfirmPendingAction`, `CancelPendingAction`, and `DismissError`.

Automatic slots cannot be overwritten or deleted. Damaged slots cannot be loaded. Overwrite and delete require confirmation. Busy states reject duplicate actions; failures enter `Error` until dismissed. Slot identity is `FLRSaveSlotId`, never a display index or widget name.

## Crash Matrix

| Interrupt point | Recovery expectation |
| --- | --- |
| PendingWrite persisted | Recover pending catalog transaction on next startup |
| Payload written | Finalize catalog on next transaction/recovery |
| Catalog finalize interrupted | Recover from the other A/B catalog |
| Old payload cleanup fails | Current catalog slot remains valid; orphan is tolerated |
| PendingDelete persisted | Continue delete recovery on startup |
| Delete completed before final catalog commit | Complete delete recovery without guessing slots |

## Verification Evidence

- `LostRunicEditor` build: required before asset or PIE acceptance.
- Save automation: `LostRunic.Save`.
- UI automation: `LostRunic.UI`.
- Manual PIE map: `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` only.
