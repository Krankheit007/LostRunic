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

### Controller、组件与配置所有权

- `ALRGuardAIController` 是 Guard 感知事件的唯一协调入口；它只消费派生 Controller Blueprint 已配置的 AIPerception 和 StateTreeAI，不在 C++ 重复配置 Sight 的距离、角度、LOS 或 StateTree 资产。
- `ALRGuardAIController`、`ULRGuardKnowledgeComponent`、`ULRAlertComponent` 的职责严格分开：Controller 负责感知适配、导航、朝向和执行阶段；Knowledge 只保存感知事实与位置记忆；AlertComponent 只维护 0-11 警戒条及观察/衰减/刺激 CD 计时。
- Guard 的所有手感参数来自派生 Controller Blueprint 的 Inline `FLRGuardTuningSettings`。C++ 默认值只是安全回退；编辑器展示名和 ToolTip 使用中文。
- Alert/Knowledge 的运行时 mutation 只由 Controller 调用；Blueprint、StateTree、UI 只能读取快照或消费事件。StateTree 只执行 Controller 已解析出的行为，不维护第二套警戒状态机。

运行时数据流：

    UE AIPerception Sight/Hearing + Room Noise
                         |
                         v
              ALRGuardAIController
                 |             |
                 v             v
          GuardKnowledge    AlertComponent
                 \             /
                  \           /
                   v         v
               FLRGuardAwarenessSnapshot
                         |
                         v
           ResolveTargetBehavior / StateTree

### Alert 与行为语义

Guard 行为由警戒值、Knowledge 证据和眩晕覆盖解析

```text
Stunned      -> Stunned
Alert 0      -> IdlePatrol
Alert 1..5   -> Suspicious
Alert 6..10  -> Investigate
Alert 11 + 当前可见确认目标        -> Chase
Alert 11 + 证据不完整             -> Investigate（安全回退）
```

| 警戒值 | UI | 行为与计时 |
|---:|---|---|
| `0` | 隐藏 | 原地或巡逻；异常刺激到 `1`，有效 Sight 到 `6` |
| `1-5` | 白色，`Level / 5` | 面向异常；从 `0` 进入时观察 `SuspiciousObserveSeconds`；接受异常 +1、最高 5、刷新白色观察；自然衰减每 `AlertDecayIntervalSeconds` 减 `AlertDecayAmount` |
| `6-10` | 红色，`(Level - 5) / 5` | 以 `InvestigateSpeed` 前往唯一的 `LatestInvestigationLocation`；抵达后才开始 `InvestigateObserveSeconds`，然后自然衰减；异常 +1、最高 10，发现敌对角色直接到 11 |
| `11` | 红色 100% + `Alert_Full_Red` | 只有 `ConfirmedThreat`、`VisualCandidate` 相同且当前有效可见时才以 `ChaseSpeed` 追逐；否则安全回退到 Investigate。真实 Sight Lost 后变为 10 并调查最后可见位置 |

AlertComponent 的内部枚举若存在，只表示计时模式：`None`、`WhiteObservation`、`RedObservation`、`Decay`。它不表示移动、导航成功/失败或 StateTree 行为。

- 白色接受异常使用 `SuspiciousStimulusCooldownSeconds`；红色接受异常使用 `InvestigateStimulusCooldownSeconds`。
- 首次刺激 CD（若启用动态步态倍率）按事件修改后的结果档位决定：`0→1/5` 使用白色基准，`5→6` 使用红色基准；事件的步态必须在声音产生时快照。Sight 不受刺激 CD 阻挡，也不走 `HandleAttractStimulus`。
- `6→5` 是衰减跨档，不启动白色观察、不启动 CD；下一次异常 `5→6` 直接按红色结果档处理。
- Room Run 的当前房间规则是：当前值低于 `RoomRunAlertLevel` 时设为该 Floor（默认 5），否则按 `AttractAlertAmount` 继续增加；相邻房间按 `AdjacentRoomRunAlertAmount` 增加；噪声不能超过 10。只有有效视觉确认能进入 11。

### Sight、Grace 与 Hard Hidden

- Sight 的距离、Lose Sight 距离、半角、Affiliation 和遮挡由 `BP_LRGuardController` 的 `UAISenseConfig_Sight` 唯一配置。`PeripheralVisionAngleDegrees` 是半角；C++ 不复制距离/扇形/LOS 计算。
- Controller 区分 **Raw Sight Contact** 与 **Effective Sight**。Hard Hidden 只会令 Effective Sight 暂时无效，保留 Raw Contact、VisualCandidate、LastKnownThreatLocation，并继续运行 `SightTrackingTimer`；只有真正收到 UE Sight Lost 才停止该 Timer 并清 Raw Contact。
- `Alert<=5` 的有效首次 Sight 直接 `→6`，启动一次 `SightToChaseGraceSeconds`。Grace 是最高优先级的警戒冻结：Noise 只记录 `LastDisturbanceLocation`，不改 Alert、不启动刺激 CD、不刷新观察、不抢 `LatestInvestigationLocation`。
- Grace 期间若导航已抵达或失败：停止移动，面向最后可见位置，但不启动 RedObserve；Grace 结束后仍可见才 `→11`，否则只有已抵达调查点才开始 RedObserve，未抵达则继续 Investigate。
- Grace 消费标记只对当前连续红色周期有效；进入白色（包括 `6→5`）或回到 `0` 时清除。Grace 已消费后，`Alert 6-10` 再次有效 Sight 立即 `→11`，不重新等待。
- `11` 丢失视线先 `→10`，保留 ConfirmedThreat 和 LastKnownThreatLocation，随后调查该位置。只有 Alert 回到 `0` 才执行本轮 Awareness Memory reset：清除候选目标、确认目标和全部调查位置。

### Noise、记忆与导航边界

- 固定入口只有三个：`HandleSightAcquiredOrTracked`、`HandleSightLost`、`HandleAttractStimulus`。Sight 入口负责 `6`、Grace、`11`、LastKnown 和追逐；Sight Lost 负责 `11→10`；Attract 入口负责 0-10 的异常增长、Room Run Floor、刺激 CD 与 Disturbance 记忆。
- Knowledge 对外保留 `VisualCandidate`、`ConfirmedThreat`、`LastKnownThreatLocation`、`LastDisturbanceLocation`、`LatestInvestigationLocation` 和 `bCurrentlyVisible`。可见目标与异常位置的更新集中在 `RecordVisibleThreat`、`RecordSightLost`、`RecordDisturbance`；当前有效视觉优先于 Noise，不允许旁侧 Noise 抢走调查目标。
- `LatestInvestigationLocation` 是 Controller 唯一的实际 MoveTo 目标。连续 Sight 用 `InvestigateMoveRetargetDistanceCm` 限制重复导航请求；离散 Noise 被接受后直接重定向，若已在接受半径内则重新开始 RedObserve。
- Controller 保存 `CurrentInvestigationMoveTarget`、请求 ID、移动状态和 Grace/追踪 Timer；Knowledge 不保存 Pending/Revision，AlertComponent 不知道导航阶段。导航失败只结束当前移动执行，不改变 Grace 的优先级规则。

### StateTree、UI 与验收边界

- `ST_Guard` 只保留 `IdlePatrol / Suspicious / Investigate / Chase / Stunned` 五个行为状态。`ELRGuardBehaviorState::Search_DEPRECATED` 仅保留原序列化槽位，运行时 Resolver 永远不返回 Search；编辑器资产不得再存在 Search 子状态。
- Controller Tick 保持启用以支持 Focus/ControlRotation；移动阶段使用 Orient Rotation to Movement，观察/追逐阶段使用 Controller Desired Rotation 和 Focus。移动动画消费 `Velocity.Size2D`，不把动画状态再写入 StateTree。
- `ULRWorldAlertBarWidgetBase::HandleAlertSnapshotChanged` 是 Blueprint 表现契约。现有 `WBP_GuardAlertBar` 使用 `Alert_Bar_White`、`Alert_Bar_Red` 和 `Alert_Full_Red`；C++ 只发布 `FLRAlertSnapshot`，不通过 `BindWidget` 接管具体样式。
- 关键回归测试必须覆盖：Grace 内 Noise 冻结、Grace 内抵达/失败、Hard Hidden 不等同 Sight Lost、`6→5` 后 Sight 重新获得 Grace、Room Run `0→5→6→7→10`、红白 UI 百分比和 `11→10` 记忆保留。
