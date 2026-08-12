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

- **代码入口**：`ALRGuardCharacter`、`ALRGuardAIController`、警戒组件和 `ST_LRHomeGuard`。
- **蓝图/资产**：守卫角色蓝图、守卫定义 DataAsset、StateTree 资产和关卡巡逻点。
- **配置步骤**：
  1. 创建 `ALRGuardCharacter` 派生蓝图，指定网格、动画和守卫定义。
  2. 在实例上填写允许的巡逻点/路径组件；移动速度、视野、听觉和警戒阈值填写到定义或 Tuning 资产。
  3. 在 AIController 默认值指定 StateTree；确认 StateTree 的状态名和 Gameplay Tag 与 C++ 规则一致。
  4. 绑定警戒变化、调查、追逐和捕获事件到 UI、音效或动画表现。
- **参数要求**：警戒范围、导航速度和感知参数必须来自定义/Tuning 资产；蓝图不能通过 Tick 改写警戒值或直接决定状态转换。
- **验收**：PIE 中验证 IdlePatrol、Suspicious、Investigate、Search、Chase 的进入/运行/退出/超时，并检查 Visual Logger/Output Log。

### UI 屏幕与输入配置

- **代码入口**：`ALRPlayerController`、`ULRPlayerUIComponent`、`ULRScreenWidget` 及各屏幕 Widget Controller。
- **蓝图资产**：HUD、状态覆盖层、叙事/阅读、日志、库存、收藏品、暂停、存档和转场 Widget。
- **配置步骤**：
  1. 创建对应 `ULRScreenWidget` 的 Widget 蓝图，保持 BindWidget 名称与 C++ 声明一致。
  2. 只在 Widget 中配置布局、字体、动画、材质和图标；展示数据由 Controller/委托推送。
  3. 在 PlayerController 的 UI 类槽位指定各屏幕 Widget；不要让关卡蓝图创建第二套 UI。
  4. 在 `ULRInputConfig` 中指定 Gameplay、Dialogue、Menu、Transition 上下文和语义 Action。
- **参数要求**：输入上下文的优先级、焦点、鼠标光标和锁键行为由 PlayerController 管理；打字速度等表现参数来自 `ULRUITuning`。
- **验收**：验证打开/关闭、焦点切换、对话二段确认、菜单阻断 Gameplay 输入和转场期间的输入锁定。

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
- 存档：`FLRSaveInventoryChunk.QuickSlots` 与 `SelectedQuickSlot` 暂时保留结构定义，捕获时不填充，恢复时完全忽略，不触发任何委托或选择恢复；后续存档重构再迁移物品获得顺序与选中武器。

### L_Home 物品 PIE 验收步骤

0. 编辑 `DA_LRInputConfig`：把 `IA_LRUseQuickSlot` 指到新的 `AttackAction` 槽位（键位沿用原 UseQuickSlot），从 `IMC_LRGameplay` 移除 1-4/上一栏/下一栏映射；`AttackAction` 配置完成后 `LostRunic.Input.ProjectConfigIsComplete` 自动恢复通过。
1. 放置 `BP_LRHomePickup`（可使用物品，如钥匙）、`BP_LRHomeNote`（`Home_Note_Mother`）、`BP_LRHomeCollectible`（`Home_Doll`）各一。
2. 键鼠与手柄分别验证：拾取物品、阅读笔记、拾取收藏品后 Output Log 无 Warning/Error。
3. 验证背包页数量显示：无限物品不显示数字，一次性物品显示剩余次数。
4. 验证交互选物：先用钥匙开 `BP_LRHomeDoor`（快捷路径 = 交互后从背包选钥匙），成功结算一致；错误物品被拒绝且显示“无法在这里使用”。
5. 验证武器：Courage 状态下攻击守卫触发击退，非 Courage 状态返回“当前无法攻击”；消耗最后一把一次性武器后 `SelectedWeaponItemId` 清空，攻击自动回退到最早获得的武器。
6. 验证背包已满：对 `MaxStackSize` 已满的物品重复拾取，Actor 不消失且显示“物品已满！”。
7. 验证菜单 Tab：背包/笔记/收集品切换正确，`OnMenuTabChanged` 事件生效，关闭后恢复 Gameplay 输入且无幽灵输入。
