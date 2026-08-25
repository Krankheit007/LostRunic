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
## Guard awareness boundary

Guard runtime state follows this one-way ownership flow:

    UE AI Perception / Room Noise
                 |
                 v
    ALRGuardAIController (adapter + coordinator)
          |                         |
          v                         v
    GuardKnowledge             AlertComponent
          \_________________________/
                       |
                       v
          FLRGuardAwarenessSnapshot
                       |
                       v
       LRAlertRules::ResolveTargetBehavior
                       |
                       v
                  StateTree

- ALRGuardAIController is the sole mutation entry. AI Hearing and room propagation both call ReceiveNoiseStimulus. Blueprint and external gameplay code may read snapshots and delegates but may not mutate Alert or Knowledge directly.
- GuardKnowledge owns visual candidate, confirmed threat, last-known threat location, last disturbance, effective exposure seconds, detection stage, pending investigation, and stimulus diagnostics. It performs no world, LOS, navigation, perception, or controller queries.
- AlertComponent owns only the 0-11 meter, tier/fraction, observing/decay, search flag, alert reason, and noise cooldown acceptance. It does not own target identity, sight state, or behavior resolution.
- A controller mutation is synchronous: evaluate raw stimulus, reject or accept it, update both peer components, resolve behavior from their snapshots, then publish one FLRGuardAwarenessSnapshot. Component delegates remain diagnostics; they do not drive controller control flow and UI/StateTree do not consume partial state.
- Raw noise metadata is not authoritative evidence. bRespond=false or cooldown rejection commits neither LastDisturbanceLocation nor LastKnownThreatLocation. Accepted threat-source noise may update LastKnownThreatLocation; other accepted noise may update LastDisturbanceLocation.
- Continuous visibility is sampled by the controller with actual elapsed time capped by tuning. Guard visibility rules are pure calculations. Hard target visibility, UE contact, range, cone and LOS are gates; distance and movement are multipliers. Knowledge integrates effective exposure and emits stage edges only.
- Alert decay asks the controller for permission. Active visual exposure prevents decay without copying a sight boolean back into Alert. Alert reaching zero and search reset are coordinated resets, so Alert and Knowledge remain peer components with no cyclic dependency.
- Behavior priority is Stunned; visible confirmed threat at chase threshold; zero alert patrol; pending reliable location at investigate floor; explicit red-band Search; max-alert Search; white-band Suspicious; remaining red-band Investigate.
- Invariant: AlertLevel == 11 does not imply ConfirmedThreat. Chase requires Knowledge to contain a confirmed, currently visible threat. PendingThreatInvestigation takes precedence over Search until its location is reached.
- BehaviorChanged is emitted only when the enum changes. Investigation location/revision changes use RefreshBehaviorContext and a retarget-distance gate, preserving StateTree state while updating navigation context.
- FLRAlertSnapshot.Behavior is presentation compatibility only and is filled by the controller when composing legacy UI data. FLRGuardAwarenessSnapshot.ResolvedBehavior is authoritative.

### Guard Awareness execution transactions (2026-08-25)

- **StateTree/Controller 执行权**：行为枚举变化时 Controller 只发送 `AI.Event.BehaviorChanged`，由 `FLRGuardBehaviorTask::EnterState` 首次调用 `EnterBehavior`；行为不变且 Investigate 上下文达到 retarget 阈值时，Controller 直接调用导航 Helper。两条路径共享幂等请求，不会对同一目标双发 Move。
- **事务提交**：`CommitAwareness` 是最终提交器，不执行 Move、Enter、Knowledge/Alert mutation 或递归提交。同步 `AlreadyAtGoal`/失败在当前事务中先收敛到 Search 再广播；已成功运行后的异步 Move completion/failure 是新的独立事务。
- **导航失败分类**：当前 RequestId 才能改变领域状态；`Blocked`/`OffPath`/当前请求的非 `NewRequest` Abort 调用 `MarkInvestigationUnreachable`，清 Pending、设置 Search flag 并清理目标，禁止 Detection sample 自动重试。不同 RequestId 的 stale callback 忽略；Retarget 产生的 `NewRequest` Abort 忽略；行为退出前清 ID，因此 Controller 主动 Abort 不污染 Knowledge。
- **瞬时视觉与历史记忆**：Sight Lost 清 Candidate/CurrentVisibility 但保留 Exposure、Stage 和 sample 时间，由单一 DetectionSampleTimer 做 inactive decay；Exposure 归零后 Stage=None 并停 Timer。UnPossess 额外立即清 Exposure/Stage，保留 ConfirmedThreat、位置记忆、Pending、Revision 和 Alert。
- **版本与广播**：`InvestigationContextRevision` 只描述已被导航接受的 Investigate 执行上下文；Suspicious SetFocalPoint 不递增。Knowledge 每个 sample 可更新，但 Awareness delegate 忽略单独 Exposure float 变化，只在 Alert/Stage/contact/threat/pending/revision/behavior 变化时广播。
