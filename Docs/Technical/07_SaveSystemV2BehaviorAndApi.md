# LostRunic Save System V2 Behavior and API

## Scope

The V2 save system has one persistence model and seven Blueprint entry points: `RequestCreateManualSave`, `RequestOverwriteSave`, `RequestAutoSave`, `RequestLoadSave`, `RequestDeleteSave`, `RequestContinue`, and `RequestNewGame`. `ULRSaveSubsystem` owns persistence and `ULRGameFlowSubsystem` owns map travel. The two communicate through requests and events; synchronous Save -> Travel -> Save recursion is forbidden. Legacy Blueprint nodes and legacy disk formats are intentionally unsupported and are not imported.

## System Invariants

1. Every catalog entry points to an existing payload whose identity validates, and `Catalog.PayloadKey == Payload.PayloadKey`.
2. A slot has one committed payload at a time. An old payload may remain orphaned during best-effort cleanup, but the catalog points to only one payload.
3. All persistent operations pass through the single `OperationQueue` and `ActiveOperation`. Memory Entry/Event/Return, health repair, save, load, and delete share this dispatcher and FIFO.
4. Map travel and provider initialization complete before the first map-ready automatic save. A partially initialized world is never captured.
5. Queue wait time does not consume `OperationTimeoutSeconds`. Payload callbacks are guarded by `OperationId`, and a late callback can never complete a later operation.

## Catalog and Health

Catalog A/B is a reliability layer, not a dynamic-slot mechanism. The catalog never scans payload files to infer slots. Orphan payloads are accepted and are only cleaned up best-effort; no background garbage collector is part of V2.

`LoadBestCatalog()` is a read-only bootstrap. If it finds `PendingOperation`, startup enqueues `RepairHealth` at the head; only that active operation may call `RecoverPendingOperation`, delete a pending payload, or commit health. Deterministic integrity failures (`CatalogMismatch`, `MissingPayload`, unsupported version, invalid identity) are also repaired through the same queue. A recovery failure blocks new persistence requests, returns `RejectedBusy`, and cancels queued operations.

A transient payload read or IO failure is reported for the current operation only and must not permanently mark a healthy slot as corrupt. `Continue` tries candidates in descending save time/sequence order and follows the same rule.

## Save and Load Lifecycle

Save requests enter `ULRSaveSubsystem` and complete through the single `OnSaveOperationCompleted` delegate. Ordinary auto-save captures only when debounce expires. Critical and New Game operations capture their immutable `FLRSaveDataV2` at the required boundary; Catalog sequence, payload key, and metadata are assigned on activation. Explicit payload failures retry the same payload according to `RetryCount`/`RetryDelaySeconds`.

Load reads and validates the payload before emitting `OnSaveLoadRequested`. `ULRGameFlowSubsystem` travels to the registered map, waits for `NotifyWorldReady`, then calls `NotifyLoadWorldReady`; Save restores non-player chunks first, then Player, and completes the operation. The overall timer starts on activation; the async watchdog starts only after `AsyncSaveGameToSlot`.

New Game uses `RequestNewGame` and `OnSaveNewGameRequested`. GameFlow travels to `ULRGameContentSet.NewGameMapId`, resolves the registered `DefaultStartAnchorId`, and notifies Save only after the world is ready. Save resets all providers, captures the new world, and commits a new automatic payload through the normal queue. The previous automatic payload remains catalog-referenced until this transaction succeeds.

## Pause and Manual Save

Pause uses `UGameplayStatics::SetGamePaused`. `IA_LRPause` triggers while paused so the same action can resume. Play time is active only in playable maps and excludes paused intervals. Manual save requires `MemoryPhase == None` and `World->IsPaused() == true`; UI state alone is not sufficient.

## UI Controller API

`ULRSaveWidgetController` exposes a read-only `FLRSaveUISnapshot` and `OnSnapshotChanged`. Widgets do not access catalog or payload objects. The controller supports `Open(Save|Load)`, `RequestCreateManualSave`, `RequestPrimarySlotAction`, `RequestDelete`, `ConfirmPendingAction`, `CancelPendingAction`, and `DismissError`.

Automatic slots cannot be overwritten or deleted. `RequestOverwriteSave(AutoSlot)` returns `RejectedProtectedSlot` before pause or eligibility checks. Damaged slots cannot be loaded. Overwrite and delete require confirmation. Busy states reject duplicate actions; failures enter `Error` until dismissed. Slot identity is `FLRSaveSlotId`, never a display index or widget name.

Memory captures the complete Home resume snapshot, including Home map, anchor, exact player transform, inventory, narrative, `MemoryEventIds`, and statistics. `RecordDeath()` owns `DeathCount`; `ULRDialogueSubsystem` owns `MemoryEventIds`. Entry, Event, and Return each enqueue a copied snapshot as `CriticalSave`. The Memory map does not need a `SaveAnchor`; restore validates the saved Home anchor only when returning Home.

## Crash Matrix

| Interrupt point | Recovery expectation |
| --- | --- |
| PendingWrite persisted | Startup queues `RepairHealth` first; the dispatcher validates and recovers it |
| Payload written | Finalize catalog on next transaction/recovery |
| Catalog finalize interrupted | Recover from the other A/B catalog |
| Old payload cleanup fails | Current catalog slot remains valid; orphan is tolerated |
| PendingDelete persisted | Startup queues `RepairHealth` first; delete recovery remains a queue operation |
| Delete completed before final catalog commit | Complete delete recovery without guessing slots |
| Overall timeout/watchdog | Broadcast `TimedOut`, clear timers, queue recovery when pending remains, and ignore late callbacks |

## Verification Evidence

- `LostRunicEditor Win64 Development` build: required before asset or PIE acceptance.
- Save automation: `LostRunic.Save` completed 8/8 focused V2 tests on 2026-08-15.
- UI automation: `LostRunic.UI` completed 11/11 tests; tuning validation completed 3/3 tests.
- The command-line automation runs used `-DDC-ForceMemoryCache` because the local Installed DDC graph had no writable node; this is an environment workaround and does not write project assets.
- Legacy API/type/queue/file scan: no matches; `git diff --check`: passed.
- Manual PIE map: `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` only.
