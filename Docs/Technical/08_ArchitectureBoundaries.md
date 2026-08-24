# LostRunic Architecture Boundaries

Status: implementation source of truth for compile dependencies and runtime event flow.

## Compile dependency contract

The project keeps one Runtime module. The intended direction is:

    Core / Data / Tuning
            |
    Interaction       StoryState
                            ^
                    Dialogue / World Events
            |
    Save Story Adapter
            |
    SaveSubsystem
            |
    GameFlow
            |
    PlayerUI / HUD

The arrows describe compile-time ownership, not delegate delivery order.

- Narrative headers define FLRNarrativePersistentState and FLRNarrativePersistentDelta. They do not include Save V2 types.
- StoryState owns persistent narrative facts and has no Dialogue, Save, or UI dependency.
- Dialogue and world events own narrative rules: definition lookup, condition evaluation, one-shot policy, and SavePolicy choice. They submit facts to StoryState.
- LRStorySaveAdapter is the only conversion boundary between narrative state and FLRSaveStoryChunk. Save V2 disk fields remain unchanged.
- SaveSubsystem owns the persistence queue, catalog, payload, provider capture/restore, and operation state. It does not include HUD, PlayerController, PlayerUI, or travel code.
- GameFlow owns map travel, world-ready coordination, restore sequencing, and Memory transactions. It publishes phases and IDs; it does not locate widgets.
- PlayerUIComponent subscribes to GameFlow. HUD and widgets render the resulting presentation state.

## Runtime event flow

Story event submission is ordered:

1. Narrative Rules accept an event and construct FLRStoryEventCommit.
2. StoryState.CommitEvent writes CompletedEventIds and the optional StoryFlag.
3. All StoryState queries can observe the new facts.
4. StoryState broadcasts OnStoryEventCommitted.
5. Save receives the event and performs the configured automatic or critical capture.

CommitEvent must not broadcast before the persistent facts are visible. Duplicate stable IDs are rejected. Memory events use CommitMemoryEvent, which returns an append-only FLRNarrativePersistentDelta to GameFlow; the delta is then accumulated and saved.

Save and travel use explicit correlation:

    Save request -> SaveOperationId
    GameFlow request -> GameFlowTransactionId + SaveOperationId
    travel -> world ready -> restore -> operation completed

Every flow callback must match both IDs. Queue order or the next completion event is never used as correlation.

Save emits Started, PhaseChanged, request, and Completed events with the relevant GameFlowTransactionId and SaveOperationId. Ordinary manual and automatic saves have no GameFlow transaction and carry an invalid transaction ID by design.

## StoryState ownership

ULRStoryStateSubsystem is the sole owner of:

- StoryFlags
- CompletedEventIds
- MemoryEventIds
- future persistent narrative variables

It exposes two intentionally different restore operations:

- ReplacePersistentState: full replacement for normal Load and New Game.
- ApplyPersistentDelta: append-only, idempotent merge for DurableNarrativeDelta and Memory return.

StoryState does not decide whether an event is allowed, whether a save is required, or how a definition is found. Those rules remain in Dialogue or the world event domain.

## Save adapter boundary

The domain state is represented by FLRNarrativePersistentState. Save-layer conversion is implemented by:

- LRStorySaveAdapter::ToSaveChunk
- LRStorySaveAdapter::ToPersistentState
- LRStorySaveAdapter::ApplyDeltaToSaveChunk
- LRStorySaveAdapter::ValidateDeltaAgainstState

ULRStoryStateSubsystem has no CaptureSaveChunk or RestoreSaveChunk API and never depends on FLRSaveStoryChunk.

## Memory Critical Save invariant

A Memory Critical Save contains exactly:

    HomeSnapshot + DurableNarrativeDelta

HomeSnapshot is captured once before entering Memory. Critical Save never captures the current Memory world. Durable narrative deltas originate from StoryState.CommitMemoryEvent and are merged into HomeSnapshot Story data by the Save Adapter.

Memory return is ordered:

1. Restore HomeSnapshot world/provider state.
2. StoryState.ReplacePersistentState(HomeSnapshot.Story).
3. StoryState.ApplyPersistentDelta(DurableNarrativeDelta).
4. Save HomeSnapshot plus the durable delta.
5. Clear HomeSnapshot only after the correlated return SaveOperationId succeeds.

Entry, event, and return failures preserve the HomeSnapshot, DurableNarrativeDelta, and GameFlowTransactionId so the transaction can retry. A Memory save must not be tied to a transient map-load callback.

## GameFlow phases

ELRGameFlowPhase is:

- Idle
- PreparingTravel
- Traveling
- WaitingForWorld
- Restoring
- EnteringMemory
- ReturningFromMemory

OnFlowPhaseChanged carries GameFlowTransactionId, SaveOperationId, phase, and map ID. PlayerUIComponent maps these phases to its Transition layer. GameFlow never calls HUD, PlayerController, or Widget methods.

## Interaction presentation boundary

FLRInteractionFocusSnapshot contains only:

- Target
- Prompt
- ActionTag
- PromptAnchor
- PromptWorldOffset

Interaction does not resolve InputConfig, InputAction, key text, glyphs, or device visibility. LRHUDWidgetController resolves those presentation fields from the current PlayerController InputConfig, device, and Enhanced Input mapping. Existing widget names and binding contracts remain unchanged.

The interaction overlap query uses the named project channel LR::CollisionChannels::Interaction. The serialized channel remains ECC_GameTraceChannel1 and the project configuration name is Interaction.

## Variant cleanup policy

Before deleting C++ classes, run Asset Registry checks for referencers, Blueprint ParentClass/NativeParentClass, config class paths, soft/class paths, external actors, and PIE package loading.

This pass removed the unreferenced Variant_Strategy and Variant_TwinStick Runtime source and the old root LostRunicCharacter, LostRunicGameMode, and LostRunicPlayerController classes. Build.cs no longer exposes their include paths. Core redirects are retained and now target LRCharacter, LRGameMode, and LRPlayerController.

Content/TopDown was not deleted wholesale. Asset Registry found current BP_LRPlayerController references to its input and cursor assets, and Lvl_TopDown has external actor/object references. Those assets require a separate migration and verification pass.

## Documentation ownership

- Docs/Technical is the human-maintained architecture source of truth.
- CLAUDE.md describes agent workflow and links here.
- .agents/ue-project-context.md is an agent context summary and should link here; it is not the source of architecture truth.