# 蓝图配置与代码功能登记

本文档是 C++ 功能交付给 Unreal Editor/蓝图使用时的操作手册和配置台账。代码功能完成后，如果需要创建蓝图派生类、添加组件、绑定事件、指定资产或填写调优参数，必须在本文档登记；代码接口或资产发生变化时，同步更新对应条目。

## 使用范围

- **C++ 核心规则**：状态合法性、交互筛选、物品结算、剧情条件、AI 决策和存档由 C++ 维护，蓝图只负责装配、表现和经过声明的配置。
- **蓝图配置**：记录可创建的蓝图类、必须添加的组件、必须引用的资产、事件绑定和实例覆盖。
- **数据资产**：记录 DataAsset、DataTable、Curve、Input Action/Mapping Context 的创建位置、字段填写规则和唯一权威来源。
- **验证记录**：每项功能必须有编译、PIE、输入设备（适用时）和 Output Log 验收结果。

## 标准配置流程

按以下顺序配置，完成一步再进行下一步：

1. **确认代码接口**：从对应 `.h` 文件确认 `UCLASS`、`BlueprintType`、`Blueprintable`、`BlueprintSpawnableComponent`、`UPROPERTY` 和 `UFUNCTION` 的可见性；不要依赖未暴露的运行时成员。
2. **编译模块**：编译 `LostRunicEditor`，确保反射信息、枚举、结构体和蓝图节点已经更新。编译失败时不要继续创建资产。
3. **创建或选择蓝图**：在代码指定的目录创建蓝图，选择文档条目要求的父类。父类不匹配时，组件和节点可能不会出现。
4. **添加组件并检查所有权**：只添加功能要求的组件；组件由 Actor 组合，跨 Actor 的长期状态使用 Subsystem。不要在蓝图中复制 C++ 规则或保存第二份状态。
5. **指定必需引用**：填写 Content/Data/Input 资产引用。启动必需资源可硬引用，非关键资源遵循代码中的软引用和异步加载约定；不要在蓝图节点中散落 `/Game/...` 字符串路径。
6. **填写参数**：先使用项目默认/聚合 Tuning Set，再按文档注明的覆盖层级填写关卡或实例参数。单位、范围、默认值和非法值处理以 C++ `UPROPERTY` 元数据及本文档为准。
7. **绑定事件与表现**：将 `BlueprintAssignable` 委托绑定到 UI、动画、音效、Niagara 或材质表现。事件只消费状态，不反向决定核心规则；禁止用 Tick 轮询可由事件驱动的状态。
8. **配置输入**：在 `ULRInputConfig` 或指定的 Input Action/Mapping Context 中绑定语义动作。蓝图配置动作名和上下文，不直接把规则绑定到某个物理按键。
9. **编译和保存资产**：编译蓝图，修复警告，保存蓝图及其引用的数据资产；检查引用是否指向稳定资产而非临时对象。
10. **PIE 验收并登记**：在受影响地图运行 PIE，按条目中的验收步骤检查键鼠/手柄、事件顺序、边界参数和失败提示；记录日期、结果和 Output Log 中的警告/错误。

## 参数填写规则

| 参数类别 | 填写位置 | 填写要求 |
| --- | --- | --- |
| 核心规则阈值 | `Data/Tuning/` 下的 Tuning DataAsset 或代码指定配置 | 只保留一个权威来源；填写单位、最小/最大值和安全回退原因。 |
| 内容 ID | DataTable 行名、定义资产 ID 或 `FName` | 使用稳定 ID，不使用显示名、数组序号或 Actor 临时名称；链接目标必须存在。 |
| 资源引用 | 蓝图默认值、DataAsset 或内容聚合资产 | 引用实际 Content 资产；非关键资源使用软引用并说明加载失败行为。 |
| 输入 | Input Action 与 Mapping Context | 填写语义动作、优先级、触发器、死区和键鼠/手柄映射；长按时间放入输入/调优资产。 |
| 表现参数 | Widget、动画、材质、Niagara 的蓝图默认值 | 只控制显示和反馈；不得用表现参数替代状态判定或保存规则。 |
| 关卡实例覆盖 | Actor 的 `EditInstanceOnly` 属性 | 仅填写本文档允许覆盖的字段，并记录关卡、实例和覆盖原因。 |

## 功能登记模板

新增功能时复制此模板，放入“功能登记”下并补全所有字段：

```md
### [功能名称]

- 状态：`待配置` / `已配置` / `已验证`
- 代码入口：`Source/LostRunic/.../*.h`（类型/函数）
- 蓝图资产：`/Game/LostRunic/.../BP_...`
- 蓝图父类或组件：`...`
- 数据/输入资产：`...`
- 依赖：`...`
- 参数来源与覆盖层级：项目默认 -> 关卡配置 -> 实例覆盖

#### 配置步骤

1. ...
2. ...

#### 参数填写

| 参数 | 位置 | 必填 | 默认/范围/单位 | 填写说明 |
| --- | --- | --- | --- | --- |
| ... | ... | 是/否 | ... | ... |

#### 事件与表现

- 绑定委托：`...`
- 绑定对象：`...`
- 禁止事项：`...`

#### 验收

- 编译：通过/失败（日期）
- PIE 地图：`...`
- 键鼠/手柄：通过/不适用
- 边界与失败路径：`...`
- Output Log：无项目级 Warning/Error，或记录具体原因
```

## 功能登记

以下条目作为已完成代码功能的首批登记；实现新功能时继续追加，不要删除历史配置说明。

### GameMode 与内容聚合

- **代码入口**：`ALRGameMode`、`ULRGameContentSet`、`ULRGameInstanceSubsystem`
- **蓝图资产**：项目使用的 GameMode 蓝图（当前默认资产为 `/Game/LostRunic/Blueprints/Character/BP_LRGameMode`）
- **配置步骤**：
  1. 创建 `ALRGameMode` 的蓝图派生类。
  2. 在 **Project Settings > Game > Lost Runic** 的 `Content Set` 中指定 `/Game/LostRunic/Data/DA_LRGameContentSet`，并同时确认 `Tuning Set`、`Input Config` 已指定。内容聚合资产不在 GameMode 默认值中重复配置；`ALRGameMode` 运行时通过 `ULRGameInstanceSubsystem` 读取这三个项目级权威来源。
  3. 打开 `ULRGameContentSet` 资产，填写对白/阅读 DataTable、物品/收藏品/守卫/关卡事件定义和地图注册信息。DataTable 行名必须与 `DialogueId`/`ReadingId` 一致，定义资产 ID 必须唯一，地图 `MapId` 必须唯一且有 World 引用。
  4. 在项目设置或关卡 World Settings 中指定该 GameMode；实例覆盖只用于关卡明确要求的差异。GameMode 蓝图可通过 `GetContentSet`、`GetTuningSet` 和 `HasValidConfiguration` 查询运行时结果，但不应建立第二份配置。
- **参数要求**：所有 DataTable 行 ID、定义资产 ID、Home/Memory 地图 ID 必须稳定且可解析；`ULRGameContentSet` 的 Data Validation 和 GameInstance 初始化会报告缺失、重复或错误类型引用。
- **验收**：进入 Home 和 Memory PIE，确认 GameMode、Pawn、Controller、HUD 和内容资产均已加载；在 Output Log 中确认没有 `LogLostRunicTuning` 或 `LogLostRunicSave` 的项目级 Warning/Error。

### 玩家角色与功能组件

- **代码入口**：`ALRCharacter` 及其移动、状态、交互、库存、噪声、掩体和状态表现组件。
- **蓝图资产**：项目使用的角色蓝图（由 `ALRCharacter` 派生）。
- **配置步骤**：
  1. 创建角色蓝图并确认继承 `ALRCharacter`。
  2. 检查 C++ 构造的组件是否存在；不要重复添加同职责组件。可通过 `GetStatePresentationComponent`、`GetNoiseEmitterComponent`、`GetCameraBoom` 和 `GetTopDownCamera` 获取对应运行时组件。
  3. 在蓝图中仅配置网格、动画、摄像机挂点和表现资源；状态、库存、交互合法性由组件处理。
  4. 在角色蓝图的 `StatePresentation` 组件上绑定 `OnStatePresentationRequested`，驱动后处理、Niagara、材质或动画表现；表现动画完成后调用该组件的 `CompleteStatePresentation`，不要用 Tick 轮询。
  5. 在 HUD Widget 的 `OnHUDWidgetControllerReady` 事件中接收注入的 `ULRHUDWidgetController`，绑定 `OnPerceptionModeChanged` 和 `OnInteractionPromptChanged`。不要在 `NativeOnInitialized` 中直接查询并缓存 Controller，也不要让 Widget 扫描世界或重新判断交互规则。
- **参数要求**：移动速度、状态长按阈值、交互距离、噪声和动画安全超时来自对应 Tuning 资产；蓝图不写入运行时状态。
- **验收**：PIE 中验证正常/感知/勇气状态、状态表现完成后可再次切换、交互提示、库存使用和掩体行为；适用时分别使用键鼠和手柄，并确认 Widget 不会因初始化时序缺失 Controller。

### 世界交互 Actor

- **代码入口**：`ALRWorldInteractionActor`、`ULRInteractionComponent`、`ILRInteractable`。
- **蓝图资产**：由 `ALRWorldInteractionActor` 派生的门、机关、可拾取物或可阅读物蓝图。
- **配置步骤**：
  1. 创建交互 Actor 蓝图并添加网格/碰撞/提示表现组件。
  2. 在 Interaction 配置中填写稳定对象 ID、交互选项、动作 Gameplay Tag、所需状态和所需物品标签。
  3. 若实现 `ILRInteractable` 的事件，保持事件只做表现或调用已声明的 C++ 动作。
  4. 将提示显示、成功、拒绝和完成事件绑定到 UI/动画/音效。
- **参数要求**：`MaxDistanceOverride` 为 `0` 时使用 `ULRInteractionTuning` 默认值；正值才覆盖默认距离。对象 ID 不得使用 Actor 名称。
- **验收**：验证距离、朝向、遮挡、状态和物品条件；快捷栏与背包选择器对同一目标必须得到相同结算。

### 守卫 AI 与 StateTree

- **代码入口**：`ALRGuardCharacter`、`ALRGuardAIController`（生命周期/感知/行为拆三个 cpp）、`ULRAlertComponent`、`ALRRoomVolume`。
- **蓝图/资产**：`BP_Guard`（守卫角色蓝图）、`DA_LRGuardDefinition`（定义 DataAsset，`Behavior` 为 **StateTree 硬引用**）、`ST_Guard`（StateTree 资产）、`WBP_GuardAlertBar`（世界警戒条）、`ALRRoomVolume`（室内奔跑房间体积）、关卡巡逻点。
- **配置步骤**：
  1. 创建 `ALRGuardCharacter` 派生蓝图，指定网格、动画和守卫定义；`AlertWidget`（WidgetComponent）的 WidgetClass 指定为 `WBP_GuardAlertBar`，绘制位置与样式在蓝图配置。
  2. 在实例上填写允许的巡逻点；移动速度、视野、听觉和警戒阈值填写到定义或 Tuning 资产。
  3. **StateTree 接线**：`ST_Guard` 使用 `StateTreeAIComponentSchema`，`ContextActorClass=ALRGuardCharacter`、`AIControllerClass=ALRGuardAIController`。树根必须是无任务的 `Root` Group，下面按 `IdlePatrol / Suspicious / Investigate / Search / Chase / Stunned` 顺序放六个平级行为状态；每个状态恰有一个 `FLRGuardStateCondition`、一个 `FLRGuardBehaviorTask`，两者的行为枚举一致，Controller 通过绑定的 `AIController` 上下文提供 `GetResolvedBehavior()`（StateTree 不自行推导警戒语义）。`DA_LRGuardDefinition.Behavior` 硬引用 `ST_Guard`，控制器 `OnPossess` 自动 `SetStateTree` + `StartLogic`（无需在 AIController 默认值手动指定）。
  4. **行为变化重选**：六个行为状态各添加 `On Event: AI.Event.BehaviorChanged -> Goto Root`，Normal 优先级、无延迟、消费事件，并将 `Reactivate Target State` 设为 `ForceChanged`。`ForceChanged` 是必需契约：事件必须离开当前行为状态、从 Root 重新按六个 `Guard Behavior Is` 条件顺序短路选择，再进入当前 `GetResolvedBehavior()` 唯一对应的子状态；仅配置普通 `Goto Root` 若 Root 被 Sustained、没有重新求值，不算完成。
  5. **持续任务与同状态 Investigate**：`FLRGuardBehaviorTask` 继承 `FStateTreeTaskCommonBase`，有效 Controller 的 `EnterState()` 必须返回 `Running`，`bShouldCallTick=false`；任务不能自行完成，只能由 `BehaviorChanged` 转换退出。新刺激若解析结果和 `ActiveBehavior` 都是 `Investigate`，Controller 直接再次 `EnterBehavior(Investigate)` 更新导航目标，不发送 `BehaviorChanged`，不触发 Root 重选，也不退出/重新进入 Investigate；只有解析后的行为枚举真正变化才发送事件。
  6. 绑定警戒快照（`OnAlertSnapshotChanged` / `GetAlertSnapshot`）到 UI、音效或动画表现。
- **参数要求**：警戒范围、导航速度、感知参数、吸引 CD（`AlertIncreaseCooldownSeconds`/`InvestigateIncreaseCooldownSeconds`）、房间警戒（`RoomRunAlertLevel`/`AdjacentRoomRunAlertAmount`）必须来自 Tuning/定义资产；蓝图不能通过 Tick 改写警戒值或直接决定状态转换。`AttractAlertAmount` 已重命名为语义值（资产中必须为 1），`SearchDurationSeconds` 已废弃（搜索由观察+自然衰减驱动）。
- **验收**：在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 用六种目标行为分别验证 Root 重选，不只确认最终状态，还要在 StateTree Debugger 看到当前状态离开、Root 重新求值并进入目标状态；在无行为事件下连续至少三个 StateTree 更新机会，确认同一行为仍 Active、组件仍 `Running`、没有自动完成/跳转。先进入 Investigate，再提交新异常位置且仍解析为 Investigate，确认导航目标更新而 Investigate 始终 Active、无额外 `BehaviorChanged`；随后切到 Search/Chase，确认事件触发 Root 重选；最后验证 Stunned 恢复。检查 `LR.Debug.Alert`、Visual Logger 与 Output Log。

### UI 屏幕与输入配置

- **代码入口**：`ALRPlayerController`、`ULRPlayerUIComponent`、`ULRScreenWidget` 及各屏幕 Widget Controller。
- **蓝图资产**：HUD、状态覆盖层、叙事/阅读、日志、库存、收藏品、暂停、存档和转场 Widget。
- **暂停输入资产**：`/Game/LostRunic/Input/Actions/IA_LRPause` 的 `Trigger When Paused` 必须启用，确保世界暂停后 `Esc`/`Start` 仍能关闭暂停层；不要在 Widget 蓝图中自行调用 `Set Game Paused`。
- **配置步骤**：
  1. 创建对应 `ULRScreenWidget` 的 Widget 蓝图，保持 BindWidget 名称与 C++ 声明一致。
  2. 只在 Widget 中配置布局、字体、动画、材质和图标；展示数据由 Controller/委托推送。
  3. 在 PlayerController 的 UI 类槽位指定各屏幕 Widget；不要让关卡蓝图创建第二套 UI。
  4. 在 `ULRInputConfig` 中指定 Gameplay、Dialogue、Menu、Transition 上下文和语义 Action。
- **参数要求**：输入上下文的优先级、焦点、鼠标光标和锁键行为由 PlayerController 管理；打字速度等表现参数来自 `ULRUITuning`。
- **验收**：验证打开/关闭、焦点切换、对话二段确认、菜单阻断 Gameplay 输入和转场期间的输入锁定。

## V2 存档 UI 基础资产（2026-08-14）

本次仅创建空的 Widget Blueprint 基础资产，暂不在资产内装配控件、事件图或存档规则。四个资产均继承 `ULRScreenWidget`，由项目负责人在 Unreal Editor 中完成 Designer 布局、绑定和导航配置。

| 资产 | 路径 | 父类 | 组装边界 |
| --- | --- | --- | --- |
| `WBP_MainMenu` | `/Game/LostRunic/UI/Save/WBP_MainMenu` | `ULRScreenWidget` | Continue / New Game / Load Game 入口与主菜单导航 |
| `WBP_SaveSelection` | `/Game/LostRunic/UI/Save/WBP_SaveSelection` | `ULRScreenWidget` | Save/Load 共用槽位列表、模式切换、空槽和损坏状态显示 |
| `WBP_SaveSlot` | `/Game/LostRunic/UI/Save/WBP_SaveSlot` | `ULRScreenWidget` | 单个槽位的显示编号、地图、时间、Health 和操作按钮 |
| `WBP_SaveConfirmDialog` | `/Game/LostRunic/UI/Save/WBP_SaveConfirmDialog` | `ULRScreenWidget` | 覆盖/删除确认、取消与不可用原因展示 |

### 负责人组装要求

1. 保持上述资产父类不变；展示数据从 Save UI Controller/委托推送，Widget 不直接读写 Catalog 或 Payload。
2. `WBP_SaveSelection` 必须同时支持 `ELRSaveSelectionMode::Save` 和 `Load`，并为保存、读取、删除、Continue 提供明确的焦点路径。
3. `WBP_SaveSlot` 的稳定身份使用 `FLRSaveSlotId`，显示编号只用于展示；不要用数组下标或 Widget 名称作为存档 ID。
4. `WBP_SaveConfirmDialog` 只负责确认表现和回调，不在蓝图中直接调用磁盘 API；操作统一转发到 `ULRSaveSubsystem` 的七个 V2 入口：`RequestCreateManualSave`、`RequestOverwriteSave`、`RequestAutoSave`、`RequestLoadSave`、`RequestDeleteSave`、`RequestContinue`、`RequestNewGame`。
5. 完成 Designer/绑定后，在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 验收主菜单 Continue、New Game、Load Game，以及 Save/Load 共用选择页。

### V2 存档 UI 控制器与新游戏 API

- **控制器所有者**：`ALRHUD` 创建唯一的 `ULRSaveWidgetController`，并将其注入 `SaveSlots` 页面。HUD 执行 `EndPlay` 时解除控制器绑定；关闭 `SaveSlots` 页面时调用 `Close()`。
- **只读视图模型**：槽位列表绑定 `GetSnapshot()` 和 `OnSnapshotChanged`。蓝图使用 `FLRSaveUISnapshot.Slots`，以及 `FLRSaveSlotView` 中的 `SlotId`、`DisplayIndex`、`MapDisplayName`、`Health`、`bCanLoad`、`bCanOverwrite` 和 `bCanDelete`。
- **保存模式**：调用 `Open(ELRSaveSelectionMode::Save)`；创建、主操作、删除、确认和取消分别调用 `RequestCreateManualSave()`、`RequestPrimarySlotAction(SlotId)`、`RequestDelete(SlotId)`、`ConfirmPendingAction()` 和 `CancelPendingAction()`。
- **读取模式**：主菜单调用 `Open(ELRSaveSelectionMode::Load)`，选择健康槽位后调用 `RequestPrimarySlotAction(SlotId)`。Widget Graph 不得直接访问 Catalog 或 Payload API。
- **自动槽保护**：`RequestOverwriteSave(AutoSlot)` 必须显示 `RejectedProtectedSlot`；自动槽只能由 `RequestAutoSave`、Memory/New Game 内部 Critical operation 写入，不能覆盖或删除。
- **统一完成事件**：所有保存、读取、删除、修复和 Memory critical operation 只监听 `OnSaveOperationCompleted`；Load/New Game 的地图切换仍由现有 request/notify 委托驱动。
- **Memory 装配**：死亡流程调用 SaveSubsystem 的 Memory 入口；不要在蓝图修改 `DeathCount` 或 `MemoryEventIds`。`ULRGameStatisticsSubsystem.RecordDeath()` 与 `ULRDialogueSubsystem` 分别维护这两类状态。
- **状态处理**：页面需要表现 `Idle`、`Confirming`、`Saving`、`Loading`、`Deleting` 和 `Error`。`bIsBusy=true` 时禁用重复操作；显示 `StatusMessage`，并为 `Error` 状态提供关闭错误提示的操作。
- **主菜单新游戏**：调用 `ULRSaveSubsystem.RequestNewGame()`。该流程异步执行；收到 `OnSaveOperationCompleted`，且 `Operation=NewGame`、`Code=Succeeded` 后，才能表现为已进入可玩世界。Widget 不得自行调用 `OpenLevel`。
- **新游戏数据配置**：填写 `ULRGameContentSet.NewGameMapId`；对应地图注册项的 `FLRMapRegistration.DefaultStartAnchorId` 必须能在目标地图中解析。新游戏先重置 Provider 状态，再替换自动槽；所有手动槽保持不变，启动失败时保留旧自动槽供 Continue 使用。

#### 存档 UI 与新游戏 Designer 检查表

1. 四个新资产统一放在 `/Game/LostRunic/UI/Save/`，并保持父类为 `ULRScreenWidget`。
2. 在 `WBP_SaveSelection` 中，将槽位列表绑定到快照变更事件，并把按钮操作转发到上述控制器函数。
3. 在 `WBP_SaveSlot` 中显示以稳定 `SlotId` 为身份的视图；`DisplayIndex` 仅用于显示文本，不参与槽位寻址。
4. 在 `WBP_SaveConfirmDialog` 中只调用确认或取消；不得直接写入 SaveGame 槽位。
5. 在 `WBP_MainMenu` 中，通过 SaveSubsystem API 和操作完成事件处理 Continue、Load 和 New Game。
6. 只在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 执行验收：暂停后打开 SaveSlots、未暂停时拒绝手动保存、自动槽覆盖返回 `RejectedProtectedSlot`、确认手动覆盖与删除、读取健康槽位，并验证 New Game 保留全部手动槽，且首次新自动存档成功前旧自动槽仍然有效。
7. Save tuning 在 `ULRSaveTuning` 的 `Save|Autosave`、`Save|Retry`、`Save|Reliability`、`Save|Slots` 分类中配置：`AutoSaveDebounceSeconds`、`RetryCount`、`RetryDelaySeconds`、`OperationTimeoutSeconds`、`AsyncWatchdogSeconds`、`MaxManualSaveSlots`。这些值来自 `ULRGameTuningSet`，缺失时只使用 C++ 安全回退并记录诊断。

## 核心玩法机制：四状态 + 潜行 + 敌人警戒 + NPC（2026-08-14）

本批次实现 4.1 四状态（睁眼/闭眼）与 4.2 潜行玩法（主角侧噪声、敌人警戒全量、掩体、通用 NPC），并为四状态美术风格差异预留接入点。**C++ 规则已完整实现**，以下为需要在蓝图中装配/配置的表面。

### 步态与噪声环境（主角侧）

- **步态权限**：`ULRLocomotionComponent` 提供 `RequestToggleSneak` / `RequestStartRun` / `RequestStopRun`（受状态规则验证，禁止时广播 `OnPaceRequestRejected` + 日志 `Movement.Reject.PaceForbidden`）；`ApplyPace`/`OverridePace`/`ClearPaceOverride` 为组件内部应用通道（掩体强制潜行使用 `Movement.Override.Hidden`）。进入状态自动应用默认步态（Perception 潜行 / Courage 走路 / Memory 走路）。
- **掩体**：`ALRHidePoint` 实例配置 `bAllowMovementWhileHidden`：桌下/草丛/管道（可移动）= true，柜/箱（固定）= false；进入掩体强制潜行，退出按当前状态重新求值。掩体容量与进出音效不在本批次。
- **噪声环境体积**：`ALRNoiseArea` 实例配置 `Environment`：`Indoor` / `Outdoor` / `OutdoorStealth`；重叠按 `Indoor > OutdoorStealth > Outdoor` 解析，无区域时默认 `Outdoor`。进入/退出都会重新求值。
- **脚步噪声**（纯规则，见 `LRMovementRules::ResolveFootstepNoise`）：潜行无声；走路 室内 400 / 室外潜行 250 / 室外非潜行 250+Faint；奔跑 室内房间传播（无房间回退 1200）/ 室外潜行 600 / 室外非潜行 250。调优字段已重命名：`OutdoorSneakGuardNoiseRadius`→`OutdoorStealthRunNoiseRadius`、`OutdoorAlertGuardNoiseRadius`→`OutdoorNoiseRadius`（PropertyRedirects 已迁移）。

### 敌人警戒（4.2.1 全量）

- **行为语义**（C++ 权威）：吸引噪声 +1（1-5 档 CD 0.5s；首次进入 6-10 档 0.5s、其后 0.2s；CD 内刺激完全忽略）；看见玩家 警戒<6→6、6-10→11、11 丢失→10；观察 3s（0→1 与抵达调查点）与衰减 0.5s/-1 由 `ULRAlertComponent` 计时器驱动；`ResolveTargetBehavior` 为行为唯一权威（眩晕优先）。警戒条 UI 只读 `FLRAlertSnapshot`（Level/Fraction/Tier/Behavior/bFullAlert）+ `OnAlertSnapshotChanged`，绑定后立即推送初值。
- **`WBP_GuardAlertBar`**：继承 `ULRWorldAlertBarWidgetBase`；覆盖 `HandleAlertSnapshotChanged` 只做表现：`Tier=Hidden` 隐藏、`White` 白色进度条、`Red` 红色、`Full` 满值+额外红色特效（样式需重新设计）。由 `BP_Guard` 的 `AlertWidget` 组件初始化，Widget 不自行猜测所属守卫。
- **`ALRRoomVolume` 摆放**：在关卡中摆放 Box 体积（Trigger profile），填写 `RoomId`（稳定 FName），`AdjacentRooms` 连线到相邻房间体积（门/窗拓扑）；守卫进入体积自动注册。**支持旋转与缩放**（包含判定用局部空间 + UnscaledBoxExtent，缩放由变换逆变换处理）。室内奔跑：当前房间守卫警戒至少提升到 `RoomRunAlertLevel`(5)、相邻房间 +1；同一守卫属多房间时取最大效果、不累加；无房间体积时回退 1200 半径听觉事件。房间体积必须早于守卫生成。
- **`ST_Guard` 资产（编辑器人工创建 + MCP 检查）**：见「守卫 AI 与 StateTree」小节接线要求；自动化契约覆盖 AI Schema、上下文类型、Root + 六状态、节点枚举和 `ForceChanged` 重选转换。

### 通用 NPC

- **`BP_NPC`**：继承 `ALRNPCCharacter`，配置网格、动画与 `DA_LRNPCDefinition`。
- **`DA_LRNPCDefinition`**：`NpcId`（稳定 FName）、`Behavior`（StateTree **硬引用** `ST_NPC`）、`DialogueRowId`（对话 DataTable 稳定行 ID）、`DefaultBehavior`（Idle/Patrol）。
- **`ST_NPC` 资产（编辑器人工创建 + MCP 检查）**：四个状态 `Idle / Patrol / ReactToNoise / Conversation`；条件 `FLRNPCStateCondition` 比较控制器 `GetActiveBehavior()`，任务 `FLRNPCBehaviorTask`；Idle 附加 `FLRNPCLookAtPlayerTask`（低频朝向检测），ReactToNoise 附加 `FLRNPCReactToNoiseTask`（限时反应，到时发 `AI.Event.NPCReactionEnded` 回默认行为）。树由 `AI.Event.NPCNoiseHeard` / `NPCDialogueStarted` / `NPCDialogueEnded` / `NPCReactionEnded` 驱动。
- **对话**：交互选项 `Interaction.Action.Talk`（Normal 状态）经 `ULRDialogueSubsystem::StartDialogue` 启动；Conversation 高优先级，普通噪声不打断（只触发 `OnNoiseHeard` 表现钩子）。巡逻点按实例配置。
- **调优**：`DA_LRNPCTuning`（登记进 `DA_LRGameTuningSet.NPC`）：`LookAtPlayerRadiusCm`、`LookAtIntervalSeconds`、`NoiseReactionDurationSeconds`、`PatrolSpeedCm`。
- **预留**：`OnNoiseHeard`（BlueprintImplementableEvent）与 `OnNPCAttentionChanged` 委托为未来告警/逃离扩展钩子，本批次不实现告警逻辑。

### 四状态美术表现预留（不实现视觉效果）

- **表现事件（已具备）**：`ULRStateComponent` 的 `OnStateChanging(PreviousMode, NextMode, Reason)` / `OnStateChanged`；`ULRStatePresentationComponent` 转发 `OnStatePresentationRequested` + `PresentStateChange`（表现锁由 `CompleteStatePresentation` 释放）。未来接入 Normal/Perception/Courage/Memory 四套视觉/后处理/音频只需订阅既有事件。
- **表现调优接入点（新增 getter）**：`GetPerceptionRevealRadius()`(4.5m)、`GetNoiseRevealRadius()`(2m)、`GetNoiseRevealDurationSeconds()`(5s)、`GetPerceptionBlendWeight()`、`GetCourageBlendWeight()`——值来自 `ULRPresentationTuning`（`DA_LRGameTuningSet.Presentation`）。
- **其他钩子**：`ULRNoiseEmitterComponent::OnNoiseEmitted`（声源显现，房间传播路径也已广播）；`ELRScreenType::StateOverlay`（屏幕层）；`LRHUDWidgetController::OnPerceptionModeChanged`（已有）。

### 输入与调优变更

- `SneakAction` 已废弃（`DeprecatedProperty`，移出 Validate 必填）；潜行切换继续使用 `ToggleCrouchAction`（C / B 切换）。
- 调优重命名（PropertyRedirects 已配置）：`HearingAlertAmount`→`AttractAlertAmount`（**资产值需改为 1**）、`SightAlertLevel`→`SightChaseLevel`(11)、移动噪声半径两项；`SearchDurationSeconds` 废弃。
- 新建资产清单（StateTree 需编辑器人工创建，其余可 MCP）：`ST_Guard`、`ST_NPC`、`DA_LRGuardDefinition`、`DA_LRNPCDefinition`、`DA_LRNPCTuning`、`BP_Guard`、`WBP_GuardAlertBar`、`BP_NPC`；`DA_LRGameTuningSet` 登记 `DA_LRNPCTuning`。
- **PIE 验收（`/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`，键鼠+手柄，Output Log 无项目级 Warning/Error）**：状态步态（Perception 强制潜行、Courage 拒潜行、Memory 仅走路）；掩体进入强制潜行/固定掩体不可移动/掩体内不可见/**掩体中发生状态变化（死亡或调试强制切换）仍保持潜行，退出后为当前状态默认步态**；噪声区域进入退出与重叠优先级、7 行步态×环境噪声、房间传播（本房→5、邻房+1、多房间取最大、无房间兜底、**缩放体积（Scale≠1）包含判定正确**）；完整警戒流程（0→吸引→1 观察 3s、CD 内忽略、看见→6 前往、抵达 Search 观察→衰减→0 巡逻、6-10 看见→11 追逐、丢失→10、追上死亡→Memory）；`ST_Guard` 六行为覆盖、持续 `Running`、Investigate 同状态重定位、Search/Chase 真实变化重选、Stunned 恢复；世界警戒条四档表现与首帧同步；击退眩晕 0.6s 恢复（`LR.Debug.Alert` 显示 Stunned）；NPC 巡逻/站立/对话开合/噪声限时反应/Idle 朝向玩家/对话结束回默认；`LR.Debug.Tuning` 确认重命名后来源。

## 更新记录

| 日期 | 变更 |
| --- | --- |
| 2026-08-11 | 新建蓝图配置与代码功能登记文档，加入标准流程、参数规则和现有 Home 切片功能条目。 |

## 交互系统重构（2026-08-11）

- 状态：`待项目负责人配置蓝图并在 L_Home 验收`
- 核心代码：`ULRInteractionComponent`、`ILRInteractable`、`ALRWorldInteractionActor`
- 表现代码：`ULRInteractionPresentationComponent`、`ULRHUDWidgetController`
- 简单交互：`ALRDoorInteractableActor`、`ALRPickupInteractableActor`
- 查询通道：项目对象通道 `Interaction`（C++ 对应 `ECC_GameTraceChannel1`）
- 权威参数：`ULRInteractionTuning`，默认执行 200 cm、描边 500 cm、远距提示上限 2000 cm、总朝向角 90 度、扫描间隔 0.1 秒
- 验证记录：2026-08-11 `LostRunicEditor Win64 Development` 编译、UHT 和链接通过；`LostRunic.Interaction` 自动化测试 2/2 通过且无警告/错误；`L_Home` PIE 仍需在蓝图装配后执行。

### 运行时数据流

1. `ULRInteractionComponent` 每次定时扫描只对 `Interaction` 对象通道做球形重叠查询。
2. 对候选生成临时 Evaluation，分别计算距离平方、模式/物品条件、朝向和表现状态。
3. 远距 Niagara 与 5 米内描边不做遮挡检测，也不依赖玩家朝向。
4. 只有 2 米执行距离内、左右各 45 度朝向内、条件满足的候选才进行 Visibility Line Trace。
5. 未遮挡候选按距离平方选择唯一 Focus Target，并向 HUD 发布弱目标引用、Prompt、ActionTag 和可见性。
6. 交互执行后立即刷新扫描，已完成的门或拾取物不会残留交互提示。

### BP_LRHomeDoor

- 推荐路径：`/Game/LostRunic/Blueprints/Interaction/BP_LRHomeDoor`
- 父类：`ALRDoorInteractableActor`
- C++ 已创建：`SceneRoot`、`DoorPivot`、`InteractionCollision`、`FarHintEffect`、`Presentation`

配置步骤：

1. 将现有门蓝图重设父类为 `ALRDoorInteractableActor`，不要在关卡蓝图中复制开门规则。
2. 把门网格挂在 `DoorPivot` 下，并在蓝图组件视图中把 `DoorPivot` 移到真实铰链位置。
3. 给需要白色描边的门网格组件添加 Component Tag：`InteractionOutline`。
4. 给 `FarHintEffect` 指定统一的 Niagara System；该组件由 Presentation 状态自动启停。
5. `InteractionCollision` 保持对象类型 `Interaction`、Query Only，并包住可代表该门的查询点；不要把门网格本身改成 Interaction 对象类型。
6. 在 `Interaction Options[0]` 中设置 `ActionTag=Interaction.Action.Interact`，Prompt 设置为“互动”。
7. `Open Yaw Degrees` 默认 90 度；若门应向另一侧开启，实例或蓝图默认值设置为 -90 度。

验收：玩家进入 2 米、位于总计 90 度朝向范围内且无遮挡时显示交互提示；按 Interact 后 `DoorPivot` 只旋转一次，重复输入不能再次开门。

### BP_LRHomePickup

- 推荐路径：`/Game/LostRunic/Blueprints/Interaction/BP_LRHomePickup`
- 父类：`ALRPickupInteractableActor`
- C++ 已创建：`SceneRoot`、`InteractionCollision`、`FarHintEffect`、`Presentation`

配置步骤：

1. 新建 `ALRPickupInteractableActor` 的蓝图派生类并添加 `PickupMesh`。
2. 给 `PickupMesh` 添加 Component Tag：`InteractionOutline`。
3. 给 `FarHintEffect` 指定与门一致的 Niagara System。
4. `InteractionCollision` 使用 `Interaction` 对象类型且保持 Query Only。
5. 在 `Interaction Options[0]` 中设置 `ActionTag=Interaction.Action.Pickup`，Prompt 设置为“拾取”。

验收：按 Interact 后 Actor 立即隐藏并关闭全部碰撞；本阶段不写入背包或快捷栏；重复输入不能再次拾取。

### HUD 交互提示

1. 在 HUD Widget 中取得 `ALRHUD.GetHUDWidgetController()`。
2. 绑定 `OnInteractionPromptChanged`，不要在 Widget 中扫描 Actor 或重新判断距离、朝向和遮挡。
3. `bVisible=false` 时隐藏提示；为 true 时显示 Prompt 和输入图标。
4. 不要把 `[E]` 拼进 Prompt。Widget 应从当前输入映射/当前设备取得 Interact Action 的按键或手柄图标，Prompt 只显示“互动”“拾取”等动作文本。
5. Prompt View 的目标引用为弱引用；UI 不得缓存强 Actor 引用。

### L_Home PIE 验收清单

1. 放置一个 `BP_LRHomeDoor` 和一个 `BP_LRHomePickup`，确认它们的查询碰撞对象类型都是 `Interaction`。
2. 大于 5 米且小于等于远距上限时只显示 Niagara；2-5 米显示 Niagara 与白色描边；2 米内的唯一 Focus 同时显示 HUD 提示。
3. 在 44.9 度、45.0 度、45.1 度左右边界检查 Focus；45.0 度包含在范围内。
4. 在玩家和目标之间放置阻挡 Visibility 的墙：远距 Niagara/描边保留，但 2 米内不能 Focus。
5. 同时放置多个 2 米内目标，确认仅选择朝向范围内最近且未遮挡者。
6. 验证门只旋转一次，拾取物隐藏并关闭碰撞；检查 Output Log 无 `LogLostRunicInteraction` Warning/Error。

## 物品系统重构（2026-08-12）

- 状态：`待项目负责人配置蓝图并在 L_Home 验收`
- 核心代码：`ULRItemActionComponent`、`ULRItemUseResolver`、`ULRInventoryComponent`、`ULRAttackTargetResolver`
- 目标接口：`ILRItemUseTarget`（交互选物）、`ILRAttackTarget`（攻击）
- 世界交互：`ALRPickupInteractableActor`、`ALRNoteInteractableActor`、`ALRCollectiblePickupActor`
- 统一菜单：单 UMG 资产 `BP_LRMainMenu`（背包/笔记/收集品 Tab）
- 权威参数：`ULRStateTuning`（Courage 攻击范围/朝向/冷却）、`ULRItemDefinition`（消费与堆叠规则）
- 验证记录：2026-08-12 `LostRunicEditor Win64 Development` 编译通过；`Automation RunTests LostRunic` 41/42 通过（覆盖库存堆叠、武器回退、攻击事务、笔记/收藏品和菜单快照）。唯一失败为 `LostRunic.Input.ProjectConfigIsComplete`：`DA_LRInputConfig` 资产的 `AttackAction` 槽位尚未在编辑器中配置（见下方 PIE 验收步骤 0），属于资产迁移工作。

### 物品定义规则（ULRItemDefinition）

- `bConsumable=false`：无限使用，成功使用不扣数量，UI 不显示数量；`MaxStackSize` 必须为 1（数据校验拒绝 `bConsumable=false && MaxStackSize>1`）。
- `bConsumable=true`：每次成功使用扣除一个，库存数量即剩余使用次数；`MaxStackSize` 允许大于 1。
- `ItemTags` 添加 `Item.Category.Weapon` 标识武器；武器只是普通物品的附加标签，可同时声明 `Interaction.Action.Use` 和 `Interaction.Action.Attack`。
- `AllowedActionTags` 只声明入口能力（Use/Attack）；不负责状态、距离、朝向、目标有效性或攻击结果判定。攻击入口除 Attack 外还必须带 `Item.Category.Weapon`，否则返回 `Item.Use.Reject.InvalidAttackItem`。
- ID 对齐：`ItemId`、`ReadingId`、`CollectibleId` 必须与定义资产/DataTable 行名一致，不得使用显示名或 Actor 名称。

### Attack Target 与 Item Use Target 的接口差异

- `ILRItemUseTarget`（`GetItemUseTargetTags`/`ApplyItemUse`）：交互选物目标，如门、机关。
- `ILRAttackTarget`（`GetAttackTargetTags`/`ApplyAttack`）：攻击目标，如守卫的 `ULRCourageResponseComponent`。
- 门、笔记、拾取物即使实现 `ILRItemUseTarget`，也永远不会成为攻击目标；攻击只接受 `ILRAttackTarget` 候选。
- 攻击距离（`CourageAttackRangeCm`）、朝向（`CourageAttackFacingDegrees`）和冷却（`CourageAttackCooldownSeconds`）全部来自 `ULRStateTuning`。

### 三类验收蓝图

#### BP_LRHomePickup（可使用物品拾取）

- 推荐路径：`/Game/LostRunic/Blueprints/Interaction/BP_LRHomePickup`
- 父类：`ALRPickupInteractableActor`
- 必填字段：`ItemDefinition`（引用 `ULRItemDefinition` 资产）、`PickupQuantity`（正数且不超过定义的 `MaxStackSize`）、`Interaction Options[0].ActionTag=Interaction.Action.Pickup`
- 规则：只有库存 `AddItem` 返回 `Success` 才隐藏 Actor、关闭碰撞并标记一次性完成；背包已满时 Actor 保持可见可交互，UI 显示“物品已满！”；蓝图不得直接销毁 Actor 或写库存。
- 验收：拾取后 Output Log 无 Warning；背包满时重复拾取，Actor 不消失且 HUD/菜单提示物品已满。

#### BP_LRHomeNote（可重复阅读笔记）

- 推荐路径：`/Game/LostRunic/Blueprints/Interaction/BP_LRHomeNote`
- 父类：`ALRNoteInteractableActor`
- 必填字段：`ReadingId`（与 `ReadingTable` 行名一致，如 `Home_Note_Mother`）、`Interaction Options[0].ActionTag=Interaction.Action.Read`
- 规则：阅读会话成功打开时立即记录笔记（`AddNoteId`），不等待玩家翻到末尾；重复打开仍可阅读但不产生重复记录；笔记非一次性，`bOneShot=false` 已由父类构造设置。
- 验收：首次阅读后菜单笔记页出现该条目；再次阅读不重复添加；阅读中途退出仍已记录。

#### BP_LRHomeCollectible（收藏品拾取）

- 推荐路径：`/Game/LostRunic/Blueprints/Interaction/BP_LRHomeCollectible`
- 父类：`ALRCollectiblePickupActor`
- 必填字段：`CollectibleDefinition`（引用 `ULRCollectibleDefinition` 资产，`CollectibleId` 如 `Home_Doll`）、`Interaction Options[0].ActionTag=Interaction.Action.Pickup`
- 规则：`AddCollectibleId` 返回 `Success` 才隐藏 Actor；重复拾取返回 `AlreadyOwned`，Actor 保持可见并记录 Warning 诊断；收藏品不可使用，不参与武器选择。

### 统一菜单 UI（BP_LRMainMenu）

- 一个 UMG 资产，通过 `OnMenuTabChanged(ELRScreenType)` 事件在背包/笔记/收集品 Tab 间切换；不显示快捷栏 HUD。
- 背包页：图标、名称、描述；一次性物品显示数量，无限物品不显示；武器标签；玩家显式选择的武器标记；武器条目提供“设为当前武器”操作（调用 `ULRInventoryComponent.SetSelectedWeapon`）。自动回退武器不写入 `SelectedWeaponItemId`，详情区可用快照的 `EffectiveWeaponItemId` 展示“攻击时将使用”。
- 交互选物模式：玩家与需要物品的目标交互后，`ULRPlayerUIComponent.OpenItemSelector` 打开统一菜单背包 Tab；快照（`ULRMenuWidgetController.BuildInventorySnapshot(inventory, target)`）只标记与目标兼容的物品，只允许提交兼容物品。
- 内部 `FailureReason` Gameplay Tag 只用于规则、日志和测试；`ULRPlayerUIComponent.DescribeItemUseFailure` 映射为友好提示，普通玩家看不到内部 Tag：

| 内部 Tag | 友好提示 |
| --- | --- |
| `Interaction.Reject.Item` / `Item.Use.Reject.InvalidAttackItem` | 无法在这里使用 |
| `Item.Use.Reject.InventoryFull` | 物品已满！ |
| `Item.Use.Reject.Target` | 目标已经失效 |
| `Item.Use.Reject.AttackState` / `State.Reject.Blocked` | 当前无法攻击 |
| `Collectible.Reject.AlreadyOwned` | 已拥有该收藏品 |
| 其他 | 物品无法使用 |

### 快捷栏废弃策略

- 输入：`AttackAction` 沿用原 `UseQuickSlotAction` 键位；`ULRInputConfig` 中 `UseQuickSlotAction` 保留为 `Deprecated` 字段仅作资产迁移回退；删除 1-4、上一栏、下一栏绑定与 `QuickSlotActions` 数组。
- UI：统一菜单无快捷栏 HUD；`FLRInventorySnapshot` 不再包含任何快捷栏字段。
- 存档：V2 使用 `FLRSaveInventoryChunkV2`（数量、获得序号、选中武器）以及独立的 `FLRSaveNotebookChunk`/`FLRSaveCollectibleChunk`；旧 quick-slot 和旧存档结构已删除。

### L_Home 物品 PIE 验收步骤

0. 编辑 `DA_LRInputConfig`：把 `IA_LRUseQuickSlot` 指到新的 `AttackAction` 槽位（键位沿用原 UseQuickSlot），从 `IMC_LRGameplay` 移除 1-4/上一栏/下一栏映射；`AttackAction` 配置完成后 `LostRunic.Input.ProjectConfigIsComplete` 自动恢复通过。
1. 放置 `BP_LRHomePickup`（可使用物品，如钥匙）、`BP_LRHomeNote`（`Home_Note_Mother`）、`BP_LRHomeCollectible`（`Home_Doll`）各一。
2. 键鼠与手柄分别验证：拾取物品、阅读笔记、拾取收藏品后 Output Log 无 Warning/Error。
3. 验证背包页数量显示：无限物品不显示数字，一次性物品显示剩余次数。
4. 验证交互选物：先用钥匙开 `BP_LRHomeDoor`（快捷路径 = 交互后从背包选钥匙），成功结算一致；错误物品被拒绝且显示“无法在这里使用”。
5. 验证武器：Courage 状态下攻击守卫触发击退，非 Courage 状态返回“当前无法攻击”；消耗最后一把一次性武器后 `SelectedWeaponItemId` 清空，攻击自动回退到最早获得的武器。
6. 验证背包已满：对 `MaxStackSize` 已满的物品重复拾取，Actor 不消失且显示“物品已满！”。
7. 验证菜单 Tab：背包/笔记/收集品切换正确，`OnMenuTabChanged` 事件生效，关闭后恢复 Gameplay 输入且无幽灵输入。

## 背包、笔记、收藏品 UI 与通用 UI 输入（2026-08-13）

- 状态：`C++ 已编译、自动化测试通过；蓝图主体已装配，剩余 3 项手工 Designer/编辑器步骤见下方`
- 核心代码：`ULRScreenWidget`、`ULRInventoryScreenWidget`、`ULRMenuWidgetController`、`ULRPlayerUIComponent`、`ALRPlayerController`、`ULRInputConfig`
- 统一菜单资产：`/Game/LostRunic/UI/WBP_Inventory`（**已重设父类为 `ULRInventoryScreenWidget`**）
- 输入资产：`IA_LRNavigate`（Axis2D）、`IA_LRPreviousTab`、`IA_LRNextTab`、`IA_LRUIPrimary`（Bool），映射写入 `IMC_LRMenu`；`DA_LRInputConfig` 已分配四个新槽位并修复 `AttackAction`（指向 `IA_LRUseQuickSlot`）
- 验证记录：2026-08-13 `LostRunicEditor Win64 Development` 编译通过；`Automation RunTests LostRunic` 除 `LostRunic.Input.ProjectConfigIsComplete` 外全部通过（该测试随 `DA_LRInputConfig` 配置完成已恢复）

### 输入链路与资产

1. 输入链路：Enhanced Input → `ALRPlayerController` → `ULRPlayerUIComponent` → `ALRHUD.GetFocusableScreen()` → `ULRScreenWidget` → 具体 Screen。Controller、PlayerUIComponent 和 Screen 基类不出现 Inventory 类型判断。
2. 输入层仲裁：`ULRPlayerUIComponent` 维护 Transition/Dialogue/Menu 三层开关，按 Transition > Dialogue > Menu > Gameplay 计算唯一有效模式；Save 子系统经 `SetTransitionLayer`、叙事经 `SetDialogueLayer`、菜单经 `SetMenuLayer` 请求，关闭高层后恢复仍然有效的下层。`ALRPlayerController.SetLRInputMode` 只执行仲裁结果（先清空 Mapping Context 再添加唯一目标 Context，并设置 `bIgnoreAllPressedKeysUntilRelease`）。
3. `IMC_LRMenu` 映射（已配置）：`IA_LRNavigate` ← W/A/S/D、方向键、DPad×4、`Gamepad_Left2D`；`IA_LRConfirm` ← E、`Gamepad_FaceButton_Bottom`；`IA_LRCancel` ← Escape、`Gamepad_FaceButton_Right`；`IA_LRPreviousTab`/`IA_LRNextTab` ← `Gamepad_Shoulder_Left`/`Right`；`IA_LRUIPrimary` ← `Gamepad_FaceButton_Top`（与 Gameplay 攻击键一致，菜单中 Y 只装备不攻击）。
4. `DA_LRInputConfig` 新增必填校验：`NavigateAction`、`PreviousTabAction`、`NextTabAction`、`UIPrimaryAction` 缺失时 `Validate` 失败。

### 统一菜单（WBP_Inventory）蓝图配置

1. **父类**：`ULRInventoryScreenWidget`（重设父类时 UMG 自动匹配 63 个 BindWidget，无需改名）。
2. **Tab 按钮**：`Bag`/`Note`/`Col` 的 `OnClicked` 已绑定 `SetActiveTab`（分别传 `Inventory`/`Journal`/`Collectibles` 枚举）；旧的 `Content.SetActiveWidgetIndex` 自定义事件实现已删除。不再使用基类 `OnMenuTabChanged`。
3. **详情文本**：`Bag_Item_Name`、`Bag_Item_Info`、`Note_Name_T`、`Note_Info_T`、`Col_Name_T`、`Col_Info_T` 由 C++ 在快照刷新/选择变化时直接 `SetText`（缓存 getter `GetCachedBagName` 等仍保留）。**手工步骤 A（必做）**：在 WBP_Inventory 编辑器中清除这 6 个 TextBlock 的 Text 绑定（Text 属性旁绑定图标 → 清除绑定），否则蓝图编译报 `TextDelegate 绑定 'None'` 错误；UMG 的绑定记录（`UWidgetBlueprint.Bindings`）无法经 MCP 脚本化修改。
4. **运行时槽位状态**：空槽清空 Brush 并 Disabled、武器标识（`Bag_Weapon_1~8`）只显示 `SelectedWeaponItemId` 槽位、`Choose_Weapon_BTN` 仅在选中武器时显示并启用——全部由 C++ `RefreshBag`/`UpdateEquipButton` 管理；Designer 默认状态（Collapsed/Disabled）为可选优化。
5. **手工步骤 B（必做）**：在 WBP_Inventory Designer 中配置三个固定布局的 UMG Navigation 元数据：背包 4×2（第 1/5 格边界、Wrap 或 Stop 按设计）、笔记 1×12（上下 Wrap，右方向 Explicit 到 `Note_Roll` 滚动区）、收藏品 4×3（Wrap）；`Note_Roll` 设 `Is Focusable=true`。C++ 侧 `ULRScreenWidget.HandleNavigate` 只调用 `FSlateApplication::NavigateFromWidget`，Designer 配置的 Stop/Wrap/Explicit/Custom/CustomBoundary 是唯一方向导航权威；C++ 不实现第二套几何寻路。
6. **手工步骤 C（推荐）**：在 `IMC_LRMenu` 编辑器中给 `Gamepad_Left2D` 映射添加 `InputModifierDeadZone` 修饰符（死区约 0.2）；MCP 无法实例化 IMC 内的 instanced 修饰符子对象。

### 快照与容量契约

- `FLRInventorySnapshot` 是只读 UI View Model：`Items`（含 Icon）、`Notes`（含 Locked 占位）、`Collectibles`（含 Locked 剪影占位）、`SelectedWeaponItemId`、`EffectiveWeaponItemId`、`bIsValid`。Widget 只消费快照，动作一律回到 `ULRInventoryComponent`。
- 排序固定：背包按 `AcquisitionSequence` 再按 `ItemId`；笔记按 `ReadingId` 字典序；收藏品按 `DisplayOrder` 再按 `CollectibleId`。
- 容量契约（开发期，非截断）：Bag 8 / Note 12 / Collectible 12。第九种独立物品由 `AddItem` 返回 `InventoryFull`；`ULRGameContentSet` 校验拒绝 13 条阅读行/13 件收藏品；快照越界 `ensureAlwaysMsgf` + `bIsValid=false` 让 UI fail closed。
- Locked 视图不泄露内容：笔记只暴露 `ReadingId`、`bUnlocked=false` 和“？？？”；收藏品只暴露稳定 ID、剪影图和解锁标记。剪影来源：`ULRCollectibleDefinition.LockedIcon`，缺失时回退 `ULRUITuning.LockedCollectibleIcon` 共享剪影。
- 领域事件：`OnInventoryChanged`、`OnNotesChanged`、`OnCollectiblesChanged`、`OnSelectedWeaponChanged`；`ULRMenuWidgetController` 菜单关闭期间只标记 dirty，打开或可见期间收到事件才重建并广播 `OnSnapshotChanged`。

### L_Home 统一菜单 PIE 验收步骤

0. 完成上述手工步骤 A/B/C，编译并保存 `WBP_Inventory`。
1. 验证打开/关闭：`Tab`（键盘）/DPad-Up（手柄）打开日志/背包页，`Escape`/B 关闭；关闭后清空各 Tab 焦点索引与选择 ID，恢复 Gameplay 输入且无幽灵输入。
2. 验证导航（键鼠 WASD/方向键、手柄 DPad/左摇杆）：背包 4×2 跨行/边界/Wrap；`Enabled -> Disabled -> Enabled` 由原生 UMG/Slate 正确跳过；笔记 1×12 上下滚动，右方向进入 `Note_Roll`，Up/Down 按 `ULRUITuning.NoteScrollStep` 步长滚动，Left 返回 `LastNoteIndex`；收藏品 4×3 与 Wrap。
3. 验证焦点恢复：Tab 内首次打开聚焦第一个 Enabled 条目；会话内切换 Tab 恢复上次索引；槽位被 Disabled/快照刷新后焦点不卡死；全 Disabled 时聚焦 Screen 自身。
4. 验证选择与详情：Confirm（E/A）或鼠标点击槽位更新右侧详情；仅移动焦点不改详情；选择武器显示 `Choose_Weapon_BTN`，选择非武器/切 Tab/关闭菜单隐藏。
5. 验证装备：`UIPrimaryAction`（Y）装备当前焦点武器；`Choose_Weapon_BTN` 装备已确认武器；装备后 `Bag_Weapon_N` 标识只出现在 `SelectedWeaponItemId` 槽位；菜单中 Y 不触发攻击，关闭菜单后攻击恢复。
6. 验证 Locked：未读笔记显示“？？？”且不可选；未收集收藏品显示剪影且不可选；已读/已收集后解锁显示真实内容。
7. 验证容量：背包满时拾取第 9 种物品显示“物品已满！”；ContentSet 超过 12 条笔记/收藏品时 Data Validation 报错。
8. 验证输入模式优先级：对话中打开菜单被拒绝；过场（Transition）期间菜单输入被屏蔽；Transition 结束后恢复对话/菜单层而不是无条件回 Gameplay。
9. Output Log 不允许出现 BindWidget、输入 Context、Focus、Navigation、容量或资源加载警告。

## 统一菜单打开语义与测试环境（2026-08-13 修订）

- 状态：`已验证（L_PIE_Test 冒烟）`
- 变更：打开统一菜单的语义从 `OpenJournalAction`（Tab 打开日志页、菜单中再按关闭）迁移为 `OpenInventoryAction`（固定打开背包页）。

### 输入语义（最终确定）

| 输入 | 行为 |
| --- | --- |
| `I`（键盘）/ `DPad-Up`（手柄） | 仅从 Gameplay 打开统一菜单背包页；菜单已打开或其他输入层激活时不处理（`I` 不加入 Menu Context，不能用于关闭菜单）。 |
| `Tab`（菜单内） | `NextTabAction` 循环切换下一页：背包 -> 笔记 -> 收藏品 -> 背包。 |
| `Escape` / `B` | 关闭菜单并恢复 Gameplay 输入。 |
| `LB` / `RB` | 上一页 / 下一页（与 Tab 同语义）。 |

### API 与资产迁移

1. `ULRInputConfig::OpenJournalAction` 迁移为 `OpenInventoryAction`；旧属性保留 `DeprecatedProperty` 标记，序列化经 `DefaultEngine.ini` 的 `[CoreRedirects] +PropertyRedirects` 自动迁移到新属性（实测加载后 `OpenInventoryAction` 自动指向 `IA_LROpenInventory`）。
2. 输入资产 `IA_LROpenJournal` 重命名为 `IA_LROpenInventory`（AssetTools 重命名自动更新引用），不新增第二套菜单打开动作。
3. `IMC_LRGameplay`：移除 `Tab -> OpenInventory`，新增 `I -> OpenInventory`，保留 `Gamepad_DPad_Up -> OpenInventory`；其余映射不变。
4. `IMC_LRMenu`：新增 `Tab -> NextTabAction`（LB/RB 保留）；`Gamepad_Left2D` 已带 `InputModifierDeadZone` 死区修饰符（约 0.2）。
5. 焦点守卫：`ULRInventoryScreenWidget::ApplyTabNavigationGuard` 在初始化时对所有可聚焦控件（Tab 按钮 3 + 背包 8 + 装备 1 + 笔记 12 + 收藏品 12）设置 Slate `Next/Previous` 导航为 `Stop`——Tab 键只由 `NextTabAction` 消费切页，不触发默认焦点遍历；方向键/WASD 的网格导航规则不变。

### 失败保护

- 菜单打开目标固定为 `ELRScreenType::Inventory`；`I` 在 Menu/Dialogue/Transition 层均不处理（只检查 Gameplay）。
- 菜单内 `Tab` 只切页；若 `NextTabAction` 未配置或绑定缺失，`HandleUICommand` 返回未处理，焦点保持不动（Slate Stop 规则兜底）。
- 序列化迁移失败（旧资产无引用）时 `ULRInputConfig::Validate` 报 `UI, gameplay, and attack actions are required`，控制器不绑定输入并 ensure 提示。

### 测试环境（AGENTS.md 同步）

- 所有通用手动 PIE、UI、输入和功能冒烟默认使用 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`；不得为测试打开/修改/摆放无关正式关卡。
- 测试分级：Bug 修复（测试关卡复现 + 已有精准测试）；新功能/共享框架/发布候选分别升级为定向、跨系统、全量测试。

### PIE 验收结果（2026-08-13，L_PIE_Test）

1. `I` 打开背包页并生成可见 `WBP_Inventory` 实例（Slate 文本检测确认）。
2. 菜单打开后 `I` 再按无动作（菜单保持）。
3. `Tab` 按顺序循环背包、笔记、收藏品，焦点不额外跳转（两次完整循环无 Focus/Navigation 警告）。
4. `Escape` 关闭后可用 `I` 重新打开（两次独立 PIE 循环均通过）。
5. Output Log 无菜单创建、Widget 编译、焦点或输入上下文警告。
6. 菜单打开后角色不能移动（Gameplay Context 移除 + Handler 防御）与鼠标可见（`ConfigureViewportInput(Menu)`）由架构保证，`LostRunic.UI.InputLayerPriorityAndRestore` 自动化测试覆盖层切换。
7. 手柄路径（`DPad-Up` 打开、`LB`/`RB` 切页）由 `LostRunic.Input.InventoryOpenMappings` 自动化测试断言映射，PIE 手柄实测留待发布验收。
