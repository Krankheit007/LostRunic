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
  3. 打开 `ULRGameContentSet` 资产，填写对白/阅读 DataTable、物品/收藏品/关卡事件定义和地图注册信息。Guard/NPC 不在 Content Set 注册。DataTable 行名必须与 `DialogueId`/`ReadingId` 一致，定义资产 ID 必须唯一，地图 `MapId` 必须唯一且有 World 引用。
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

- **代码入口**：`ALRGuardCharacter`、`ALRGuardAIController`、`ULRAlertComponent`、`ULRGuardKnowledgeComponent`、`ALRRoomVolume`。
- **蓝图/资产**：`BP_Guard`、`BP_LRGuardController`、`ST_Guard`、`WBP_GuardAlertBar` 和 `ALRRoomVolume`。Guard Definition、独立 Guard Tuning DataAsset 和 Search 行为均不再存在。
- **配置步骤**：
  1. `ALRGuardAIController` 的 C++ 构造函数创建唯一继承组件 `AIPerception` 与 `StateTreeAI`；蓝图只能配置继承组件，Controller/Pawn 都禁止手工 Add Component。
  2. 打开 `/Game/LostRunic/Blueprints/Guard/BP_LRGuardController`，确认父类为 `ALRGuardAIController`。在继承的 **AIPerception** 中配置一份 Sight、一份 Hearing 和 Dominant Sense；在继承的 **StateTreeAI** 中指定 `/Game/LostRunic/Blueprints/Guard/ST_Guard`。
  3. 在 **Class Defaults > Guard|调优** 配置 `FLRGuardTuningSettings`。所有编辑器字段显示中文名；不同敌人通过派生 Controller Blueprint 覆盖。
  4. 打开 `BP_Guard`，设置 **AI Controller Class = BP_LRGuardController**、**Auto Possess AI = Placed in World or Spawned**；组件树不得含 AIPerception、StateTree 或 StateTreeAI。
  5. `ST_Guard` 使用 `StateTreeAIComponentSchema`，保留 `Root` 下五个平级状态：`IdlePatrol / Suspicious / Investigate / Chase / Stunned`。每个状态的 `FLRGuardStateCondition` 和 `FLRGuardBehaviorTask` 使用同名行为枚举；`AI.Event.BehaviorChanged` 从 Root 重新选择并使用 `ForceChanged`。
  6. UI 只绑定 `OnAlertSnapshotChanged`/`HandleAlertSnapshotChanged` 表现契约，不从 Widget 反向修改 Alert 或 Knowledge。
- **验收**：在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 验证五种行为、同状态 Investigate 重定位、Grace 内抵达/失败、Hard Hidden 与真实 Sight Lost 的区别、Room Run 连续增长和 `11→10` 追逐丢失流程；StateTree Debugger 应只出现五个 Guard 行为状态。

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

### 交互提示、描边与 NPC 对话（2026-08-18）

- 状态：`代码、资产配置、定向自动化测试与 PIE 定向验收已完成`
- 代码入口：`ULRInteractionComponent`、`ULRHUDWidgetController`、`ULRHUDScreenWidget`、`ALRNPCCharacter`
- 受影响蓝图：
  - `/Game/LostRunic/UI/WBP_HUD`：父类必须为 `ULRHUDScreenWidget`。
  - `/Game/LostRunic/Blueprints/Interaction/BP_LRHomePickup`、`BP_LRHomeDoor`：保留现有 `InteractionCollision`、`Presentation` 和网格上的 `InteractionOutline` 标签。
  - `/Game/LostRunic/Blueprints/Character/BP_NPC1`：父类为 `ALRNPCCharacter`；交互碰撞由 C++ 自动创建，不要在蓝图重复添加第二个交互组件。

#### `WBP_HUD` 配置步骤

1. 编译 `LostRunicEditor` 后打开 `/Game/LostRunic/UI/WBP_HUD`，在 **Class Settings → Parent Class** 确认父类为 `LRHUDScreenWidget`，然后执行 **Compile** 和 **Save**。
2. 在 Designer 中确认最外层 `Background` 为 CanvasPanel，并将 `/Game/LostRunic/UI/WBP_Interaction` 的实例命名为 `InteractionWidget`，作为 `Background` 的直接子项；不要对该实例调用 `AddToViewport`。
3. `ULRHUDScreenWidget` 通过 `BindWidget` 显式绑定 `InteractionWidget`，并在运行时订阅 Controller 的 `OnInteractionPromptChanged`。不要在 Event Graph 中扫描 WidgetTree、扫描世界或每帧重新判断交互规则。
4. `InteractionWidget` 的 Canvas Slot 使用固定左上锚点 `(0, 0)`、`Size To Content = true` 和底部居中 Alignment `(0.5, 1.0)`；位置由 `ULRHUDScreenWidget` 每帧从 `PromptAnchor` 投影到 `Background` Canvas 本地坐标。不要把该 Slot 保留为中心锚点，否则动态位置会叠加中心锚点而跑出窗口。投影失败、相机背后或完整 Widget 越界时隐藏，不执行屏幕边缘夹取。

#### `WBP_Interaction` 配置步骤

1. 在 **Class Settings → Parent Class** 确认父类为 `ULRInteractionWidget`，然后 Compile/Save。
2. 根层级必须包含两个 `TextBlock`，名称和类型固定为 `InteractionKey : TextBlock`、`InteractionInfo : TextBlock`。它们只负责布局、字体和样式，不绑定输入事件或世界对象。
3. 非活动状态由 C++ 设置为 `Collapsed`；活动状态由 C++ 设置为 `HitTestInvisible`。不要在蓝图 Tick 中控制可见性。
4. 按键文本由 `ULRHUDWidgetController` 查询当前 Local Player 的 active Enhanced Input mappings；键鼠显示实际绑定键（例如 `E`），所有 Gamepad 当前使用 Xbox-style label，`Gamepad_FaceButton_Left` 显示为 `X`，不包含括号。

#### 描边与交互 Actor 配置步骤

1. 分别打开 `/Game/LostRunic/Materials/PostProcess/M_PP_LR_StyleOutline` 与 `/Game/LostRunic/Materials/M_PP_InteractionOutline`，在 **Material Details** 确认两者均为 `Material Domain = Post Process`、`Blendable Location = After DOF`；艺术描边 `Blendable Priority = 0`，交互描边 `Blendable Priority = 10`。当前 A–F 交互参数为 `InteractionOutlineWidthPx = 1.0`、`InteractionDiagonalScale = 0.70710678`、`InteractionDepthBiasCm = 1.0`、白色 `InteractionOutlineTint`。
2. 全局 Gameplay 后处理统一使用 `LR_PostProcess`：在 **Details → Rendering Features → Post Process Materials → Weighted Blendables** 按艺术描边在前、`MI_PP_LR_InteractionOutline_Diag0707` 在后的顺序放置两项，权重均为 `1.0`；该体积固定 `Priority = 0`、`Infinite/Unbound = On`。测试只使用 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`。局部 `ArtBench_PPV` 只保留 Review Lighting/颜色覆盖，必须清空重复 Blendable，不得再挂载旧的 `M_PP_InteractionOutline` 或第二份艺术描边。
3. 打开交互 Actor 蓝图，在 **Components** 选择需要描边的 `StaticMeshComponent` 或 `SkeletalMeshComponent`，于 **Details → Tags → Component Tags** 添加 `InteractionOutline`。不要把 Actor Tag 当成 Component Tag，也不要手工常开 Render CustomDepth。
4. `ULRInteractionPresentationComponent` 在 BeginPlay 扫描上述组件，写入 `LRCustomStencil::InteractionSelected = 1`，并按 `NearOutline/Focused` 状态开启 Render CustomDepth；切到其他目标或 None 时自动清理。项目配置 `Config/DefaultEngine.ini` 必须保持 `r.CustomDepth=3`。
5. `BP_LRHomePickup` 与 `BP_LRHomeDoor` 的可交互碰撞必须为 Query Only，并将 Object Type 设为项目 `Interaction` 通道；其余交互规则、距离和执行合法性由 C++ 处理。
6. `MI_PP_LR_InteractionOutline_Diag100` 与 `MI_PP_LR_InteractionOutline_Diag0707` 位于 `/Game/LostRunic/Materials/Benchmark/Instances/`，只用于 A/B；正式默认使用 0.707，不按资产另建交互描边实例。

#### Normal Material Coverage G–L 资产配置

1. 环境 Opaque 打开 `/Game/LostRunic/Materials/Master/M_LR_StylizedOpaque`。所有 MI 都能看到 `SurfaceUTiling` / `SurfaceVTiling`，但只有 Wood、Wallpaper、Tile 等方向性/可平铺环境家族可按批准规则修改；Plaster、Metal、Cloth 保持 `1.0`。墙体长度必须由 Mesh UV/Trim UV 维持物理尺度，不按墙长派生 MI。
2. Foliage 使用 `/Game/LostRunic/Materials/Master/M_LR_StylizedFoliageMasked`；Benchmark 实例为 `/Game/LostRunic/Materials/Benchmark/Instances/MI_LR_Benchmark_Foliage`。正式 Alpha 先以大叶簇为单位，检查卡片正反面与 Quad Overdraw 后再扩展密集资产。
3. Ruth 打开 `/Game/LostRunic/Blueprints/Character/BP_Ruth`，选择 `CharacterMesh0`，确认 Material Override 使用 `/Game/LostRunic/Materials/Instances/Characters/MI_LR_Ruth_Child`，并关闭 **Rendering → Receives Decals**。Character MI 不提供 Surface U/V 调整；Painterly seam 问题先降低 `PainterlyStrength`。
4. 单层玻璃使用 `/Game/LostRunic/Materials/Master/M_LR_StylizedGlass`；Benchmark 实例为 `/Game/LostRunic/Materials/Benchmark/Instances/MI_LR_Benchmark_Glass`。Material Details 必须保持 `Thin Translucent`、`Surface ForwardShading`、无 Refraction；不要让 Wash 驱动 Opacity。
5. 纯颜色裂纹使用 `/Game/LostRunic/Materials/Decals/M_D_LR_StylizedColor`；Benchmark 实例为 `/Game/LostRunic/Materials/Benchmark/Instances/MI_D_LR_Benchmark_WallCrack`。只允许 Opaque Receiver 使用 `Decal Response = Color`；Foliage、Character、Glass 为 None。Decal Actor 的 Bounds 尊重源 Mask 比例，不用极端非均匀缩放覆盖整墙。
6. 打开 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`，在 Outliner 的 `ArtBenchmark/Normal` 检查 `ArtBench_FoliageCard_A/B`、`ArtBench_Ruth`、`ArtBench_Glass`、`ArtBench_WallCrackDecal` 与 `ArtBench_Door`。Combined Gate 中 Door 的目标网格必须写 Stencil 1；生产运行时仍由 `ULRInteractionPresentationComponent` 按状态开关 CustomDepth，不手工常开。

#### Camera Cutaway 配置步骤（2026-09-01）

本轮参数权威约定：`/Game/LostRunic/Materials/Parameters/MPC_LR_CutawayView` 中的 `CutawayTransitionRefPx`（默认 `8`）是唯一编辑器调优来源。`M_LR_StylizedOpaque`、`M_LR_StylizedFoliageMasked` 与 PP 描边材质均通过 Collection Parameter 读取该值；C++ 初始化只读取并校验运行时 MPC 值，不写回或覆盖默认值。工具脚本不得创建同名本地 Scalar Parameter。

1. `ALRCharacter` 已由 C++ 创建 `CameraRig` 与 `CameraCutaway`，并固定 `CameraBoom.bDoCollisionTest = false`。普通流程保持 700cm；仅特殊演出调用 `SetSpecialCameraDistance(300–1400cm, BlendSeconds)`，结束后调用 `RestoreDefaultCameraDistance`。负 Blend 使用 0.25s 默认值，0 为立即切换，非法/非有限距离拒绝且不改变当前距离。
2. `BP_Ruth.CharacterMesh0` 会自动写 `LRCustomStencil::PlayerOccluded = 2`。以后新增 Hair、Dress、Equipment 或剧情附件时，在对应 **Primitive Component → Details → Tags → Component Tags** 添加 `CharacterVisual`；不要在各蓝图自行分配 Stencil。`DA_LRPresentationTuning.PlayerOcclusionPostProcessMaterial` 已指向 `/Game/LostRunic/Materials/PostProcess/M_PP_LR_PlayerOcclusion`，角色相机会自动添加 After DOF、Priority 20 的白色填充与 `1px @ 1080p Reference` 外轮廓。
3. 在 `/Game/LostRunic/Data/Tuning/DA_LRPresentationTuning` 的 **Presentation → Cutaway → Materials** 分类，将硬引用 `CutawayViewParameterCollection` 指向 `/Game/LostRunic/Materials/Parameters/MPC_LR_CutawayView`，然后 Compile/Save DataAsset。该 MPC 的 `LocalCutawayState0..3` 为 `RGBA = CenterUV.X, CenterUV.Y, RadiusRefPx, Amount`，`LocalCutawayActiveCount` 为连续活动槽数（0–4），`CutawayTransitionRefPx` 固定默认 `8`；引用缺失时仅禁用艺术描边抑制并输出一次 Warning，不影响 Local Cutaway。
4. 为墙、屋顶、树木等遮挡 Actor 添加 `LRCutawayTargetComponent`。在 **Cutaway → Primitives** 显式填写 `AffectedPrimitiveReferences`；若留空，只回退到该 Actor 自身 Mesh Components。需要独立简单碰撞时填写 `DetectionPrimitiveReferences`。所有检测 Primitive 必须 `Collision Enabled = Query Only`、`CameraCutaway = Overlap`，不得设为 Block；其余通道按原玩法碰撞配置。C++ 不会自动开启 Query（避免同时激活该 Primitive 的 Pawn/Visibility 等旧响应）；配置为 `NoCollision/PhysicsOnly` 会输出 `LogLostRunicCutaway Warning` 并无法参与检测。
5. Target 的 `Local / Group / Foreground` 三个开关分别授权圆孔、房间整体消失、近镜头 PixelDepth 消失。`Foreground` 在 BeginPlay 以 Target 自身为 requester 直接初始化为 1；远于 300cm 时 Shader 输出无视觉影响，不播放开场 Fade。潜行进入 Hard Hidden 时角色组件自动清除 Local 请求，但仍保留全局白色剪影定位。
6. Opaque 遮挡物从 `/Game/LostRunic/Materials/Instances/Cutaway/` 选择模板：墙 `MI_LR_CutawayWall_Template`、屋顶 `MI_LR_CutawayRoof_Template`、树干 `MI_LR_CutawayTrunk_Template`。三者最终 Blend Mode 均为 Masked，父级保持 `M_LR_StylizedOpaque`；普通 Opaque MI 不受零值 CPD 影响。叶簇继续使用真实资产 `M_LR_StylizedFoliageMasked`，不创建或引用旧名 `M_LR_StylizedMasked`。
7. Root Preserve 是 MI 静态美术配置（Scalar 0/1，不是 Static Switch）：墙/树干模板为 `Enabled=1, Height=10cm, Feather=5cm`；屋顶与叶簇默认 0。特殊实例可在 Target 勾选 `bOverrideRootPreserve` 并填写 Height/Feather；显式 0 合法。Override 的单位是世界厘米，C++ 使用 Primitive 的 Local Bounds 高度乘 `abs(Component World Scale Z)` 换算 `RootHeight01 / RootFeather01`，因此父级或 Actor 的 Z 缩放也会计入。不要给每个 foliage cluster 套根部保留。
8. 永不消失的槽（门牌、交互附件等）加入 `PersistentMaterialSlots`，并使用不读取 Cutaway CPD 的材质。Target 在 BeginPlay 只校验一次其余槽：最终 Blend Mode 必须是 Masked，Base Material 必须属于 `DA_LRPresentationTuning.ApprovedCutawayMasterMaterials`；该列表当前只有真实的 Opaque 与 Foliage Master。
9. 房间整体切换时在关卡放置 `LRCutawayRegion`，调整 `RegionBounds`，并在 **Cutaway → Targets** 显式引用本房间需要隐藏的 Target。多个 Region 各自作为 requester，重叠时取最大请求；退出/EndPlay 只清理自身请求。不要复用 `LRRoomVolume` 或在 Level Blueprint 手动改透明度。
10. CPD 0–8 固定为 `LocalAmount, GroupAmount, ForegroundAmount, CenterUV.X, CenterUV.Y, RadiusRefPx, RootOverrideAmount, RootHeight01, RootFeather01`。逐帧只写活动/恢复中 Target 的 CenterUV；Radius 仅激活或配置变化时写，Root 仅配置变化时写，Amount 仅过渡时写。`M_LR_StylizedOpaque` 与 `M_LR_StylizedFoliageMasked` 的 LocalAmount 必须来自共享 `/Game/LostRunic/Materials/Functions/MF_LR_CutawayRadialShape` 的 `CutawayCoverage`；函数使用 ViewportUV/CenterUV/RadiusRefPx/Amount/ViewSize/Transition，内部 reference-space painter hash 仅保留在该函数内。描边材质使用四个 MPC 状态的 `SignedBoundaryDistanceRefPx`，在 `[-10,+2]`（Transition 8 + Margin 2）范围内做约 1px 软抑制并取四槽最大值；不在 PP 中重新计算半径或 Dither。
11. PIE 只使用 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`。至少验证普通移动、反复跨 Region 边缘、Local 恢复途中重新遮挡、Hard Hidden、墙/屋顶/树干/叶簇 Root 语义，以及相机贴外墙、狭窄走廊、楼梯下、高墙角、屋顶附近和 300/1400cm 特殊距离；检查无回缩、穿模背面、Near Clip 与异常 Cutaway。另用一个本地高度 300cm、Component/父级累积 Z Scale=2 的 Target 验证 `RootHeight=40cm` 最终仍只保留约 40cm，而不是 80cm。
12. Art Outline suppression 的 sticky state 是固定四槽视觉缓存，不是 Cutaway gameplay authority。必须分别压力验证 `1 / 2 / 4 / 5 / 6` 个同时 Local Occluder：前四个应连续占槽且恢复值降至 `<=0.02` 后释放；第五个及以后仍通过自身 CPD 正常挖洞，但当前不会进入描边 MPC，存在洞口人工黑边风险。若从 5 个开始出现黑边，再单独评估提高槽数或更换 suppression 机制，不在本轮静默扩大容量。

本次定向自动化 `LostRunic.Cutaway` 为 6/6 通过，覆盖过渡中途反转、常量契约、Root Override 世界缩放换算、有限 suppression ring、请求 max 聚合与零值清理、sticky 槽优先级；`LostRunicEditor Win64 Development` 构建通过。完整 GPU/PIE 画面验收仍以测试关卡、Lumen、VSM、Nanite 和 1080p/1440p/4K 实机观察为权威。

#### M1 Cluster-Based Foliage Prototype 配置

1. 树与草网格位于 `/Game/LostRunic/ArtBenchmark/M1Foliage/Meshes/`；共享纹理位于 `/Game/LostRunic/Materials/Benchmark/M1Foliage/Textures/`；材质实例位于 `/Game/LostRunic/Materials/Benchmark/M1Foliage/Instances/`。确定性源文件由 `Tools/ArtBenchmark/generate_m1_foliage_meshes.py` 与 `generate_m1_foliage_textures.py` 生成，禁止手工改导入资产后不回写源规则。
2. 在 Static Mesh Editor 检查树资产 `SM_LR_M1_Tree_Medium_A`：Material Slot 0 为 Trunk、Slot 1 为 Canopy；Build/Import 必须保留 Imported Custom Normals，`Recompute Normals = Off`、`Recompute Tangents = On`、`Use MikkTSpace = On`，并关闭 weighted normals。`generate_m1_foliage_meshes.py` 导出前逐渲染面检查 `Dot(FaceGeometricNormal, Normalize(Average(VertexCustomNormals))) > 0.5`；平均 authored normal 近零直接失败，错误必须带对象名、材质区、面索引和实际 dot，同时打印各材质区最小/平均值。打开 World Normal 可视化确认每个 Lobe 呈连续径向/椭球分布。树当前双 Section 只用于 M1；未来 LOD 必须复核槽位一致性、Section 数与实例批处理成本。
3. DCC 中所有 Vertex Color RGB 在艺术涂色前显式初始化为 `0.5/0.5/0.5`。`M_LR_StylizedFoliageMasked` 的 `FoliageMassTintRange` 候选为 `0.2`，Master 的 `FoliageMassTintStrength` 保持 `0`，M1 Tree/Grass/Hero 实例设置为 `1`。A 通道仅保留为 `FutureBendWeight`，本材质不得读取。
4. 打开 `T_LR_M1_Foliage_BC`，确认 **Texture → sRGB = On**、**Level of Detail → Texture Group = World**、生成 Mips、**Texture → Do Scale Mips for Alpha Coverage = On**，并只将 **Alpha Coverage Thresholds → A = 0.4**；R/G/B Threshold 保持 0。当前项目没有独立 Foliage Texture Group。SMK 使用 Linear/Masks，Normal 使用 Normalmap。
5. 树冠使用 `MI_LR_M1_Foliage_Tree_Normal0` 与 `MI_LR_M1_Foliage_Tree_Normal025` 做 NormalStrength 0/0.25 A/B；草与 Hero 分别使用 `MI_LR_M1_Foliage_Grass`、`MI_LR_M1_Foliage_HeroTuft`。Cluster Alpha 以连续簇外轮廓为主，内部叶片身份来自 BaseColor/SMK，只保留少量大透空。
6. 打开 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`，在 Outliner 的 `ArtBenchmark/Normal/FoliageM1` 检查 Ground Base、两棵视觉 A/B Tree、Grass Band、Hero Tuft、Tree Density Cell 与 Dense Grass Cell；`ArtBenchmark/Normal/FoliageM1/ReviewLighting` 子文件夹下的 Neutral Key/Fill/PPV 只用于固定植被评审。BaseColor Diagnostic 使用 Unlit/BaseColor 视图，不通过关闭主要灯光模拟。
7. 当前 Density Cell 是固定屏幕覆盖与重叠压力 Proxy，由多个 StaticMeshActor 组成；它不证明 Static Mesh Foliage/HISM 的 Draw Submission、Cluster Culling 或实例化成本。正式性能检查须改用 Foliage Mode/ISM，并固定 Camera、1920×1080、SP100 与 ROI，记录 No Foliage、Single Tree、Tree Density、Grass、Dense Grass、Combined 的 Base Pass/VSM/Quad Overdraw 趋势。
8. 按两阶段顺序验收：Stage 1 只使用 `NormalStrength = 0 + Default Lit + Neutral Lighting + Painterly Off + Art Outline Off`，用 WorldNormal/Lit 确认四个 Lobe 是连续大块受光；失败时只排查 authored normal、绕序、导入和 tangent。Stage 1 通过后，Stage 2 仅切 `NormalStrength = 0.25`；若贴图重新打碎体积，成品候选保持 `0`，0.25 仅作失败对照。随后才在 `NormalStrength = 0`、Outline Off 的 Gameplay Camera 25% 缩略图执行 Cluster Silhouette Gate：若细长条、edge-on Card、大空洞或纸片轮廓主导，归类 `Cluster Geometry/Orientation/Alpha Density`，本轮停止且不修改 Cluster 数量/分布/尺寸/朝向策略/Alpha 密度，也不继续改 Shader。最后才用 `ArtEdgeDebugView 1/3/4` 检查 Depth/Normal/Final；目标为树干清晰、树冠弱外轮廓、草极弱或不强调。
9. Near/Mid/Far 只验证现有机制：同一树屏幕高度约 `35% / 15% / 7%`，检查真实 mip 稳定性与 `1200–4500 cm` 内部 Normal Edge 淡出，远景保留 Depth Silhouette；不新增 Shader 分档。`ArtBench_M1_NeutralPPV` 必须覆盖整个 `ArtBenchmark/Normal/FoliageM1` 区域（当前测试体积 bounds 约 `X 4450–8100 / Y -1300–1300 / Z -102–660 cm`）。

#### NPC 对话配置步骤

1. 打开 `/Game/LostRunic/Blueprints/Character/BP_NPC1`，确认组件树中存在 C++ 自动创建的 `InteractionCollision`，其为 Query Only、Object Type 为 `Interaction`，并只对 `Interaction` 通道生成 Overlap。
2. 确认 `DialogueComponent.ScriptId` 仍为 `HomeSisterHide`，并按 SUDS 对话登记为该 ID 提供脚本注册表；不要在 NPC 蓝图中另建 E 输入或自行判断距离。
3. NPC 是否可执行由 `ALRNPCCharacter::CanInteract_Implementation` 和 `TryInteract_Implementation` 维护；HUD 会以同一套 `OnInteractionPromptChanged` 显示 `E` 与 `对话`。NPC 的默认 Prompt Anchor 为 `InteractionCollision`；可在其 `Presentation` 组件中用 `PromptAnchorOverride` 指定其它 SceneComponent。
4. 全局默认提示高度来自 `/Game/LostRunic/Data/Tuning/DA_LRInteractionTuning` 的 `InteractionPromptZOffset`，默认 `40 cm`。若单个实例需要调整，在其可选 `Presentation` 组件中启用 `bOverridePromptZOffset` 并填写 `PromptZOffsetOverride`。

#### 验收

- 编译：`LostRunicEditor Win64 Development` 已通过；`WBP_HUD` 已通过 UMG Compile，父类为 `LRHUDScreenWidget`。
- 自动化：`LostRunic.Interaction` 定向测试通过（3/3）；`LostRunic.UI.InteractionWidgetBlueprintContract` 通过（1/1）。
- PIE 地图：`/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`；已确认有效焦点下提示出现在拾取物上方，按 E 成功拾取后提示消失。门与 `BP_NPC1` 的完整交互流程仍按同一地图继续验收。
- 描边：靠近拾取物和门时确认 `CustomDepth` 与后处理轮廓同时可见；离开范围后提示、轮廓和焦点状态清除。
- 描边自动化：运行 `LostRunic.Interaction.OutlineStencilClearsAcrossTargets`，确认 A 选中、A→B、B→None 均写入/清理 Stencil=1 且无残留。本次 2026-08-30 运行通过，0 Warning / 0 Error。
- 描边 PIE：在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 验证 1px 外轮廓覆盖艺术黑线、内部不漂白；用非 CustomDepth Primitive 部分遮挡目标时，遮挡部分不得透出白线。
- Output Log：记录本次 PIE 中项目级 Warning/Error；引擎本机 Turnkey/缓存权限提示不作为项目交互失败判定。

## SUDS 对话与本地化生产链（2026-08-17）

### 运行时资产配置

本批次已将 SUDS 固定为对白结构引擎；Reading DataTable 和 `NarrativeScreenClass` 仍属于阅读系统，不要把它们替换为 Dialogue Widget。

1. 将 `Plugins/SUDS` 保持为 Git submodule，当前已固定到 commit `3b3145d727b2e140bb3f37c155a651013eae8af5`。升级 SUDS 时先运行 Editor build 和 Contract Test，再修改唯一的 `LRSUDSLocalizationParser` 适配层。
2. 在 `/Game/LostRunic/Dialogue/Data/` 创建 `DA_DialogueScriptRegistry`，类型为 `ULRDialogueScriptRegistry`。每个条目填写唯一 `ScriptId` 和对应的 `USUDSScript`；同一 Script 不得绑定到两个 ID。第一版 NPC 使用硬引用，保证同步 `TryStartDialogue` 可用。
3. 打开 `BP_NPC1`（父类 `ALRNPCCharacter`），选择 C++ 自动创建的 `DialogueComponent`，填写 `ScriptRegistry`、`ScriptId`、可选 `StartLabel` 和可选 `CompletionStoryTag`。对白身份只保留在该组件中。
4. 在 `/Game/LostRunic/Dialogue/Data/` 创建 `ST_DialogueSpeakers`，添加 `Adele`、`Butler`、`Narrator`（必要时添加 `Player`）条目；创建 `DA_DialogueSpeakers`（类型 `ULRDialogueSpeakerRegistry`），使每个 `DisplayName` 直接引用该 String Table entry。普通手写 FText 会被 Registry 校验拒绝。
   同时在 `DA_LRGameContentSet` 的 `Content|Localization|DialogueSpeakerRegistry` 指定该资产，运行时从中读取本地化 Speaker Name 和硬引用 Portrait。
5. 在项目的 SUDS Editor Settings 中确认 `AlwaysGenerateSpeakerLinesFromChoices=False`。`.sud` Fixture 也必须保持 `GenerateSpeakerLinesFromChoices false`；LostRunic UI 由 `OnSpeakerLine` 读取 Choice，不依赖 `OnChoice` 生成选项。
6. 打开 HUD 蓝图的类默认值，配置独立的 `DialogueScreenClass` 为 `ULRDialogueWidget` 派生 Widget；`NarrativeScreenClass` 继续指向 Reading Widget。Dialogue Widget 的文本、Speaker Name、Portrait 和 Choice 只消费 C++ Presentation，不在蓝图重判分支。

### 运行时语义验收

- `FLRDialogueStartRequest` 必须同时带 `ScriptId` 与 `USUDSScript*`；两者必须等于 Registry 当前映射。运行时不得反查 ScriptId。
- `OnSpeakerLine` 后读取 `GetText`、`GetSpeakerDisplayName`、`IsSimpleContinue`、`GetNumberOfChoices` 和 `GetChoiceText` 刷新 UI；`IsSimpleContinue=true` 时 Choice 数为 0，否则至少为 1。`OnChoice` 只用于 telemetry、音效或日志。
- `Story.*` 只写入 `ULRStoryStateSubsystem`，`Save.*` 只提出 Save Request；StoryFlag 必须可由 SUDS Boolean variable 读取。不可重复的世界副作用不要放在 Story Event 中。
- `CompletionStoryTag` 只在 `ELRDialogueEndReason::CompletedNaturally` 时提交。强制 End、取消、Owner 销毁和 Level Travel 即使收到 SUDS `OnFinished` 也不得提交完成标签。
- Dialogue Owner 销毁或 Level Travel 时必须清理 SUDS、Input Layer 和两个 Narrative Screen；不保存 SUDS 当前播放位置。

### 本地化生产命令

配置文件必须提交到 `Config/Localization/`；生成的 PO/Archive/LOCRES 属于 UE Localization 资产，Manifest、Import JSON 和 Python venv 属于 `Intermediate/`，人工 XLSX 默认保存到 `Saved/DialogueLocalization/Workbooks/`，不由清理工具删除。

在 Developer Command Prompt 中从项目根目录执行：

```powershell
# 首次机器/CI 初始化；默认要求系统 Python 3.14，也可显式传入 Python 3.14
.\Tools\DialogueLocalization\bootstrap.ps1
# 非标准安装位置：
# .\Tools\DialogueLocalization\bootstrap.ps1 -BasePython C:\Python314\python.exe

# 校验 Fixture/所有脚本的 @hex@、ChoicePath、Metadata 和生成 Speaker Line 设置
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\LostRunic.uproject '-run=LRDialogueLocalization' '-Mode=ValidateSUDS' '-ScriptId="Home.Butler.Introduction"' '-Script="Content/LostRunic/Dialogue/Source/Fixture.sud"' '-unattended' '-nop4'

# 先完成 Reimport SUDS asset，再执行 UE Gather/Export；-PO 必须是本次 Export 产生的当前 PO
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\LostRunic.uproject '-run=LRDialogueLocalization' '-Mode=PrepareXLSX' '-Registry=/Game/LostRunic/Dialogue/Data/DA_DialogueScriptRegistry' '-SpeakerRegistry=/Game/LostRunic/Dialogue/Data/DA_DialogueSpeakers' '-PO=Content/Localization/LostRunic/en/LostRunic.po' '-Culture=en' '-unattended' '-nop4'

# 翻译员编辑 Saved/DialogueLocalization/Workbooks/Dialogue_en.xlsx 后
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\LostRunic.uproject '-run=LRDialogueLocalization' '-Mode=ImportXLSX' '-XLSX="Saved/DialogueLocalization/Workbooks/Dialogue_en.xlsx"' '-Culture=en' '-unattended' '-nop4'
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\LostRunic.uproject '-run=LRDialogueLocalization' '-Mode=ApplyPO' '-PO="Content/Localization/LostRunic/en/LostRunic.po"' '-Import="Intermediate/DialogueLocalization/DialogueLocalizationImport.json"' '-unattended' '-nop4'

# ApplyPO 后必须 Import，再 Compile；PO 不是 Compile 的直接权威输入
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\LostRunic.uproject -run=LRDialogueLocalization -Mode=ImportLocalization -unattended -nop4
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\LostRunic.uproject -run=LRDialogueLocalization -Mode=CompileLocalization -unattended -nop4

# 等价的一键顺序：ApplyPO -> ImportLocalization -> CompileLocalization
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\LostRunic.uproject '-run=LRDialogueLocalization' '-Mode=ApplyAndCompileLocalization' '-PO="Content/Localization/LostRunic/en/LostRunic.po"' '-Import="Intermediate/DialogueLocalization/DialogueLocalizationImport.json"' '-unattended' '-nop4'
```

`Entries` 与 `Speakers` 工作表都通过当前 PO Join；Speakers 表的 Translation 同样会进入 Import JSON。普通导入在 `manifestHash` 或 `poRevision` 变化时只给 Warning，并逐行依据 `ScriptId + StringKey`、SpeakerId、SourceHash、String Table Identity、Namespace + Key、MsgCtxt + MsgId 和 FormatArgs 决定是否接受；stale 行被拒绝而其他行继续。CI 加 `-StrictManifest`，要求 Manifest、PO revision、Workbook fingerprint 和完整行集一致。`ApplyPO` 只写 PO `msgstr`，下一步由 UE Localization Import 更新 `.archive`，最后 Compile 生成 `.locres`。

### Python 环境与验收

默认 Python 位于 `Intermediate/DialogueLocalization/PythonEnv/`，由 `requirements.lock.txt` 固定 `openpyxl==3.1.5`，`.ready` 是 Editor 调用 XLSX 工具的前置条件。特殊机器可在 **Project Settings > Plugins/Developer Settings > LostRunic Dialogue Localization > Python** 填 `Python Executable Override`，或在 Commandlet 传 `-PythonExecutable=`。

Editor Contract Test 名称：

- `LostRunicEditor.DialogueLocalization.EntryIdentitySurvivesReorder`
- `LostRunicEditor.DialogueLocalization.SudsAdapterDoesNotLeakTypes`
- `LostRunicEditor.DialogueLocalization.GenerateSpeakerLinesFromChoicesDisabled`
- `LostRunicEditor.DialogueLocalization.ApplyImportCompileProducesLocres`

最终在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 验收普通对白、Choice、Speaker Name、Portrait、Story 条件、自然完成/强制结束，以及 Dialogue/Reading 屏幕隔离。检查 `LogLostRunicDialogueLocalization`、`LogLostRunicNarrative`、`LogLostRunicState` 无项目级 Warning/Error；端到端验收还必须确认 PO 只有预期 `msgstr` 变化、Archive 已更新、LOCRES 可读且 PIE 显示新译文。

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

### 敌人警戒（2026-08-27 重构基线）

- 旧版 `VisibilityScore → EffectiveExposureSeconds → DetectionStage → AlertFloor` 已移除；本系统不再积分 Exposure，也不使用光照/姿态可见度系数。详细架构边界见 `Docs/Technical/08_ArchitectureBoundaries.md`。
- Guard 行为固定为：`0=IdlePatrol`、`1-5=Suspicious`、`6-10=Investigate`；`11` 只有同时存在匹配的 `ConfirmedThreat`、`VisualCandidate` 和当前有效视觉时才进入 `Chase`，否则回退到 `Investigate`；另有 `Stunned` 覆盖。有效 Sight 从 `0-5` 直接到 `6`，从 `6-10` 直接到 `11`；Noise/吸引注意只能把 Alert 推到 10。
- `SightToChaseGraceSeconds` 是低警戒首次 Sight `→6` 的一次性 Grace。Grace 内 Alert 冻结；Noise 只记录 Disturbance。调查抵达或导航失败都要等 Grace 结束后处理：仍可见则确认，已抵达或导航失败则开始 RedObserve；失败状态保持 Failed，不伪装成已抵达，也不自动重试。
- Hard Hidden 不等于 UE Raw Sight Lost：Raw Contact 存在时持续 `SightTrackingIntervalSeconds` 检查，有效视觉暂时为 false；真正 Sight Lost 才停止跟踪。`11→10` 保留 ConfirmedThreat 和最后可见位置；Alert 回到 0 才清空本轮记忆。
- UI 四档由 `FLRAlertSnapshot` 驱动：0 隐藏；1-5 使用 `Alert_Bar_White` 且百分比 `Level/5`；6-10 使用 `Alert_Bar_Red` 且百分比 `(Level-5)/5`；11 红条满并播放 `Alert_Full_Red`。C++ 不 Bind 两个 ProgressBar，WBP 负责最终布局、颜色和动画表现。
- Room Run 当前房间按“低于 Floor 先到 `RoomRunAlertLevel`，达到 Floor 后按 `AttractAlertAmount` 继续 +1”；相邻房间按 `AdjacentRoomRunAlertAmount` +1；所有噪声最高到 10。声音步态在产生时快照，首次刺激 CD 再按结果所属白/红档和步态倍率计算。

### 通用 NPC

- **`BP_LRNPCController`**：继承 `ALRNPCController`；只配置 C++ 继承的 AIPerception（Hearing）和 StateTreeAI（`ST_NPC_Stand`），禁止手工添加同类组件；Class Defaults 配置 `FLRNPCTuningSettings` 和 `DefaultBehavior=Idle`。
- **`BP_NPC1`**：继承 `ALRNPCCharacter`，只配置网格、动画、对白和 **AI Controller Class = BP_LRNPCController**；不得含 AIPerception、StateTree 或 StateTreeAI。
- **`ST_NPC_Stand`**：状态 `Idle / Patrol / ReactToNoise / Conversation`；条件 `FLRNPCStateCondition` 比较控制器 `GetActiveBehavior()`，任务 `FLRNPCBehaviorTask`；Idle 附加 `FLRNPCLookAtPlayerTask`，ReactToNoise 附加 `FLRNPCReactToNoiseTask`。树由 NPC 行为事件驱动。
- **对话**：交互选项 `Interaction.Action.Talk`（Normal 状态）经 NPC 上的 `ULRDialogueComponent::TryStartDialogue` 启动 SUDS；Conversation 高优先级，普通噪声不打断（只触发 `OnNoiseHeard` 表现钩子）。对白脚本由 `ULRDialogueScriptRegistry` 通过 `ScriptId` 唯一解析，巡逻点按实例配置。
- **调优**：在 `BP_LRNPCController > Class Defaults > NPC|Tuning` 配置 `LookAtPlayerRadiusCm`、`LookAtIntervalSeconds`、`NoiseReactionDurationSeconds`、`PatrolSpeedCm`。
- **预留**：`OnNoiseHeard`（BlueprintImplementableEvent）与 `OnNPCAttentionChanged` 委托为未来告警/逃离扩展钩子，本批次不实现告警逻辑。

### 四状态美术表现预留（不实现视觉效果）

- **表现事件（已具备）**：`ULRStateComponent` 的 `OnStateChanging(PreviousMode, NextMode, Reason)` / `OnStateChanged`；`ULRStatePresentationComponent` 转发 `OnStatePresentationRequested` + `PresentStateChange`（表现锁由 `CompleteStatePresentation` 释放）。未来接入 Normal/Perception/Courage/Memory 四套视觉/后处理/音频只需订阅既有事件。
- **表现调优接入点（新增 getter）**：`GetPerceptionRevealRadius()`(4.5m)、`GetNoiseRevealRadius()`(2m)、`GetNoiseRevealDurationSeconds()`(5s)、`GetPerceptionBlendWeight()`、`GetCourageBlendWeight()`——值来自 `ULRPresentationTuning`（`DA_LRGameTuningSet.Presentation`）。
- **其他钩子**：`ULRNoiseEmitterComponent::OnNoiseEmitted`（声源显现，房间传播路径也已广播）；`ELRScreenType::StateOverlay`（屏幕层）；`LRHUDWidgetController::OnPerceptionModeChanged`（已有）。

### 输入与调优变更

- `SneakAction` 已废弃；潜行切换继续使用 `ToggleCrouchAction`（C / B 切换）。
- Guard 的唯一运行时调优是 `FLRGuardTuningSettings`，位于 `BP_LRGuardController > Class Defaults > Guard|调优`。已移除 `VisibilityScore`、Exposure 积分、DetectionStage、光照/姿态系数和 Search 时长；Sight 半径、Lose Sight 半径、半角和 LOS 只在继承的 AIPerception Sense Config 中配置。
- 当前可调 Guard 字段为：警戒增加量、可疑观察时间、调查观察时间、视觉追逐确认宽限、白色/红色刺激冷却、警戒衰减量/间隔、当前房奔跑警戒下限、相邻房奔跑警戒增加量、视觉跟踪间隔、奔跑/走路/潜行首次刺激冷却倍率、巡逻/调查/追逐速度、调查到达误差、调查重定向距离、捕获半径。所有字段元数据使用中文 `DisplayName`/`ToolTip`。
- `Config/DefaultEngine.ini` 只保留必要的旧属性重定向；重命名后的字段必须在蓝图中重新保存，并通过 `LR.Debug.Tuning` 或自动化测试确认实际来源。不要恢复旧 Detection/Exposure 字段。

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

## 主菜单、暂停、存档选择与本地化最终契约（2026-08-16）

- 状态：`C++ 契约与资产装配已实现；PIE 验收结果见本节末尾`
- 测试地图：通用 UI/输入冒烟仍使用 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`；主菜单地图为 `/Game/LostRunic/Levels/Menu/L_MainMenu`。
- 主菜单框架：`ALRMainMenuGameMode` + `ALRMainMenuHUD`，`DefaultPawnClass=nullptr`，复用 `ALRPlayerController`；菜单 Host 不创建玩法 Pawn。

### Widget 父类与 BindWidget 名称

| 蓝图用途 | C++ 父类 | 必须存在的控件名 |
| --- | --- | --- |
| 主菜单 | `ULRMainMenuWidget` | `NewGameButton`、`ContinueButton`、`LoadButton`、`OptionsButton`、`ExitButton` |
| 暂停菜单 | `ULRPauseWidget` | `Resume`、`SaveGame`、`Options`、`MainMenu` |
| 存档选择页 | `ULRSaveSelectionWidget` | `SlotListPanel`、`BackButton`、`TitleText`、`StatusText` |
| 存档槽 | `ULRSaveSlotWidget` | `SlotButton`、`SlotNameText`、`MapNameText`、`SavedAtText`、`PlayTimeText`、`CollectibleCountText`、`HealthText`、`BackgroundImage` |
| 创建槽 | `ULRCreateSaveSlotWidget` | `CreateButton`、`Slot_Index`、`CreateLabelText` |
| 覆盖/删除确认 | `ULRSaveConfirmDialogWidget` | 面板 `Cover`、`Delete`；按钮 `Cover_Confirm`、`Cover_Cancel`、`Delete_Confirm`、`Delete_Cancel`；文字 `MessageText`、`Cover_Confirm_T`、`Cover_Cancel_T`、`Delete_T_1`、`Delete_T`、`Delete_Confirm_T`、`Delete_Cancel_T` |

现有 Widget Blueprint 是绑定契约的事实来源。上述名称来自资产现有 Designer 结构；C++ 只适配这些既有名称，并用 `FLRSaveConfirmViewModel` 下发 StringTable 文案，不重命名或重建控件。`Cover` 与 `Delete` 共用一套运行时 ViewModel，但根据请求类型互斥显示。

### 资产路径、父类与 Class Defaults

1. `/Game/LostRunic/UI/Save/WBP_MainMenu`：父类 `ULRMainMenuWidget`。
2. `/Game/LostRunic/UI/Save/WBP_Pause`：父类 `ULRPauseWidget`；`Options` 由 C++ 保持禁用。
3. `/Game/LostRunic/UI/Save/WBP_SaveSelection`：父类 `ULRSaveSelectionWidget`。在 **Class Defaults > Save UI > Widget Classes** 分别设置 `Save Slot Widget Class = WBP_SaveSlot`、`Create Save Slot Widget Class = WBP_CreateSaveSlot`、`Confirm Dialog Widget Class = WBP_SaveConfirmDialog`。**Save UI > Layout > Slot Row Height** 保持 `200`，与现有 `WBP_SaveSlot`、`WBP_CreateSaveSlot` 的 Designer 行高一致；C++ 会用 `SizeBox` 包装动态行，避免 CanvasPanel 根控件直接加入 ScrollBox 后期望高度变为 0。
4. `/Game/LostRunic/UI/Save/WBP_SaveSlot`：父类 `ULRSaveSlotWidget`。整栏只保留 `SlotButton`；鼠标点击和 Primary 都执行模式相关主操作，Delete 输入只作用于当前焦点手动槽。`WBP_SaveSelection` 收到快照后在 C++ 中创建该行，并通过 `ApplyView` 写入所有 TextBlock；蓝图 EventGraph 不负责生成文字。
5. `/Game/LostRunic/UI/Save/WBP_CreateSaveSlot`：父类 `ULRCreateSaveSlotWidget`；创建入口只存在于此资产的 `CreateButton`，选择页不再绑定页面级 `CreateSlotButton`。该行同样由 `WBP_SaveSelection` 在 C++ 中创建；`Slot_Index` 由 `ApplyView` 写入纯数字，`CreateLabelText` 由 StringTable 快照写入。
6. `/Game/LostRunic/UI/Save/WBP_SaveConfirmDialog`：父类 `ULRSaveConfirmDialogWidget`；Overwrite 只显示 `Cover`，Delete 只显示 `Delete`，关闭时两者均 Collapsed。
7. `/Game/LostRunic/Blueprints/UI/BP_LRMainMenuHUD`：父类 `ALRMainMenuHUD`；Class Defaults 设置 `Main Menu Screen Class = WBP_MainMenu`、`Save Slots Screen Class = WBP_SaveSelection`。
8. `/Game/LostRunic/Blueprints/UI/BP_LRMainMenuGameMode`：父类 `ALRMainMenuGameMode`；`HUD Class = BP_LRMainMenuHUD`、`Default Pawn Class = None`。`L_MainMenu` 的 **World Settings > GameMode Override** 指向该类。
9. `/Game/LostRunic/Blueprints/Character/BP_LRGameMode`：`Default Pawn Class = /Game/LostRunic/Blueprints/Character/BP_Ruth.BP_Ruth_C`，供 `L_Home` 生成可见主角。
10. `/Game/LostRunic/Data/DA_LRGameContentSet`：`MainMenuMapId = Menu`；`Maps` 中必须已有 `Menu` 注册。Pause 返回主菜单只调用 GameFlow 注册旅行，不在 UI C++ 中写地图路径。

`ALRHUD` 是 `ULRSaveWidgetController` 的唯一宿主。Widget 只在 `SetSaveWidgetController` 时绑定 `OnSnapshotChanged`，在 `NativeDestruct` 时解除绑定，不调用 `Initialize/Deinitialize`，也不保存玩法状态。

存档目录非 `Ready` 时页面只能显示加载/阻塞状态，不得把空列表解释为“没有存档”；正式列表来自 `FLRSaveCatalogSnapshot`。`WBP_SaveSelection` 根据快照动态创建槽位：Save 模式容量允许时追加 `WBP_CreateSaveSlot`，Load 模式不创建该栏。确认页消费 `FLRSaveConfirmViewModel`，焦点消费 `FLRSaveFocusTarget`：已有槽按 `SlotId` 恢复，创建槽按 `CreateDisplayIndex` 恢复；刷新、取消确认与删除后必须落到仍有效的目标。

本阶段批准的表现例外：不新增可见错误 `TextBlock`；阻塞与操作失败沿用现有 `StatusText`/确认层表现，具体错误原因保留在 C++ 日志与结果码中。

### 输入与本地化

1. 在 `DA_LRInputConfig` 新增 `UIDeleteAction`，资产路径约定为 `/Game/LostRunic/Input/Actions/IA_LRUIDelete`；`IMC_LRMenu` 映射键盘 `Delete`、手柄 `Gamepad_FaceButton_Top`（Xbox X 语义）到该动作。代码已接通；资产未配置时保留兼容并不绑定该动作。
2. `/Game/LostRunic/Input/Actions/IA_LRCancel` 必须勾选 **Trigger When Paused**。Pause 打开的 Save 页面保持 World Paused；未勾选时 Escape/手柄 Cancel 不会进入统一 UI Cancel 路由，也就无法返回 Pause。
3. `ULRGameContentSet.UIStringTable` 指向 `/Game/LostRunic/Localization/ST_LRUI`。地图显示名、存档页标题、创建入口、自动槽名称、健康状态、错误提示、确认文案和确认/取消按钮都由 `ULRGameContentSet::ResolveUIText` 解析后写入只读 UI 快照；Widget Blueprint 不再保存这些文字的运行时权威值。`WBP_SaveSelection` 收到 `OnSnapshotChanged` 时只把动态委托当作刷新通知，实际显示数据统一从 `ULRSaveWidgetController.GetSnapshot()` 读取，避免在反射委托参数中复制嵌套 `FText`。
4. `FLRSaveSlotMetadata.SavedAtUtc` 只保存 UTC；槽位 Widget 使用 `LRSaveFormatting::FormatSavedAtLocal` 转为当前本地时区和 Culture。时长使用 `HH:MM:SS`，不按 24 小时取模；收藏进度使用 `{Count}/{Total}`。
5. `FLRSaveSlotMetadata.CollectedCount` 追加在现有 SaveGame 字段末尾；旧 V1 Catalog 不含该字段时按 `0` 迁移。冻结兼容夹具为 `Source/LostRunic/Tests/Fixtures/CatalogV1_NoCollectedCount.bin`，由 `LostRunic.Save.Catalog.LoadsFrozenV1Fixture` 回归验证。

### `ST_LRUI` 的实际配置步骤

`ST_LRUI` 不是 DataTable，也不是把 `Namespace.Key` 写进文本的表。每行只有两个关键字段：`Key` 和源语言 `Source String`。Key 必须与 `DisplayNameTextKey` 完全一致，区分大小写，不要填写 `/Game/...` 路径或命名空间前缀。

1. 在 Content Browser 打开 `/Game/LostRunic/Localization/ST_LRUI`，确认它是 **String Table**，不是 DataTable。当前工程的 Table ID 应为 `/Game/LostRunic/Localization/ST_LRUI.ST_LRUI`；不要手动修改它。
2. 在表中添加地图显示名。例如：

   | Key | Source String（`zh-Hans`） | 对应设置 |
   | --- | --- | --- |
   | `MapHome` | `家` | Content Set 的 Maps 行：`MapId=Home`、`DisplayNameTextKey=MapHome` |
   | `MapMenu` | `主菜单` | Content Set 的 Maps 行：`MapId=Menu`、`DisplayNameTextKey=MapMenu` |

   存档界面使用以下稳定 Key，统一在此 StringTable 修改源语言，不在各 Widget Blueprint 中分别维护文案：

   | Key | Source String（`zh-Hans`） |
   | --- | --- |
   | `SaveTitle` | `存档` |
   | `SaveCreateNew` | `新建存档` |
   | `SaveAuto` | `自动存档` |
   | `SaveHealthHealthy` / `SaveHealthMissing` / `SaveHealthCorrupt` | `存档正常` / `存档文件缺失` / `存档损坏` |
   | `SaveHealthUnsupported` / `SaveHealthMismatch` | `存档版本不支持` / `存档数据不匹配` |
   | `SaveHealthUnknown` / `SaveHealthInvalid` | `未知存档类型` / `无效存档` |
   | `SaveCatalogBlocked` / `SaveCapacityFull` / `SaveOperationFailed` | 存档页状态错误文案 |
   | `SaveConfirmOverwrite` / `SaveConfirmDelete` | 覆盖、删除确认文案 |
   | `SaveDeleteWarning` | `删除后无法恢复` |
   | `UIConfirm` / `UICancel` | `确认` / `取消` |

3. 打开 `/Game/LostRunic/Data/DA_LRGameContentSet`，在 `Content → Localization → UI String Table` 选择 `ST_LRUI`。这个引用当前已经存在，但仍要确认没有被改空。
4. 在同一个资产的 `Maps` 数组中编辑对应地图行：

   - `MapId`：稳定 ID，例如 `Home`。
   - `World`：该地图的软引用。
   - `DisplayNameTextKey`：填写 `MapHome`，必须与 StringTable 的 Key 完全相同。
   - 不再填写 `DisplayName`：该字段已删除，地图名称只由 `DisplayNameTextKey` 提供。

5. 保存资产并执行 Data Validation。存档槽显示地图名时，代码会按 `MapId` 找到该行，再从 `DisplayNameTextKey` 解析 StringTable；因此只在表里新增 Key、但不填写地图注册行，不会产生任何可见变化。

### 如何判断配置是否生效

- 显示 `家`：`UIStringTable`、Maps 行和 Key 都正确。
- 显示 `MapHome`：Key 不存在、拼写/大小写不一致，或 `UIStringTable` 没有赋值。
- 显示 `Home`：Maps 行的 `DisplayNameTextKey` 为空，或地图注册行不存在，代码只能回退到 `MapId`。

### 英文配置

StringTable 的 `Source String` 只填写源语言（本项目约定为 `zh-Hans`），不要在同一 Key 下再复制一行英文。需要英文时，在 Localization Dashboard 创建/打开项目 Target，将 Native Culture 设为 `zh-Hans`，Supported Cultures 加入 `en`；Gather 后导出 PO，在英文翻译列填写 `Home`，再 Import 并 Compile。运行时切换 `culture=zh-Hans` 与 `culture=en` 验收同一个 Key 的两种文本。

> 数字、UTC 转本地时间、累计时长和收藏计数不走文案 Key；其余存档页可见文字必须由上述稳定 Key 解析。Key 缺失时界面会显示 Key 本身，便于直接诊断配置遗漏。

### 存档页按钮语义

- 主菜单：`NewGame`、`Continue`、`Load`、`Exit` 可用；`Options` 置灰。
- Pause：`Resume` 关闭暂停；`SaveGame` 打开 Save 模式并在 Back 时返回 Pause；`MainMenu` 解除暂停并前往注册的 `Menu` 地图；`Options` 置灰。
- Save 页：自动槽的 Primary 禁用，且不可覆盖/删除；手动槽覆盖必须确认；删除必须确认。Load 能力只在 Load 模式暴露，Overwrite 能力只在 Save 模式暴露；列表未 `Ready` 时 Create/Overwrite/Load/Delete/Continue/NewGame 均由存档子系统返回 `RejectedBusy`。
- Load 页：只有 `Healthy` 槽可加载；`Continue` 与 `RequestContinue` 共享同一“最新健康槽”解析规则。

### 本节 PIE 验收

1. 在 `L_PIE_Test` 验证目录 Loading/Recovering/Ready/Blocked 四种表现，确认 Loading 不显示伪造空列表。
2. 在 Save 页键鼠验证 Create/Overwrite/Delete/Confirm/Cancel；Delete 动作完成后焦点回到同一槽或 Root。
3. 验证 `Delete` 与 Xbox X 只进入删除确认，不直接删除；自动槽无删除入口。
4. 切换 zh-Hans/English，确认主菜单、地图名、健康状态、时间、时长和收藏计数均来自 StringTable/格式化函数。
5. 主菜单从 `L_MainMenu` 点击 NewGame 后进入 `Home`；不生成 Pawn 的主菜单不会出现 Character/Save WorldReady 相关错误。

### 验证记录（2026-08-16）

- `LostRunicEditor Win64 Development` 构建通过。
- 定向自动化测试 7/7 通过、0 Warning/Error：主菜单无 Pawn、PIE 内容契约、Designer Widget 契约、刷新后焦点恢复、焦点目标规则、Primary/Delete 动作规则和存档快照规则。
- `L_MainMenu` 已确认菜单显示、专用 GameMode/HUD 生效、无 Pawn，且 `LogGameMode`、`LogLostRunicUI`、`LogBlueprint` 无项目级 Warning/Error。
- `L_Home` 已确认运行时生成 `BP_Ruth_C` 且角色模型可见。Pause → Save、确认弹窗及返回链路的最终手动 PIE 由项目负责人继续验收。

## 架构边界与 GameFlow/StoryState 配置登记

### 交互提示

- 交互领域只生成 FLRInteractionFocusSnapshot：Target、Prompt、ActionTag、PromptAnchor、PromptWorldOffset。
- 不要在 ULRInteractionComponent 或交互 Actor 中配置 InputAction、InputConfig、InputKeyText 或 Glyph。
- 打开 HUD 的 Widget Controller，确认其拥有当前 ALRPlayerController，并通过 PlayerController 的 InputConfig 解析语义 ActionTag 对应的 InputAction、当前设备和 Enhanced Input Mapping 显示文本。
- 现有 HUD Prompt Widget 的控件名和 BindWidget 绑定保持不变；Widget 只消费 FLRInteractionPromptView。
- 交互查询使用项目命名通道 Interaction（底层仍为 ECC_GameTraceChannel1），不要在蓝图中新增匿名 GameTraceChannel。

### StoryState 与对话事件

- StoryFlags、CompletedEventIds、MemoryEventIds 只能由 ULRStoryStateSubsystem 持有；Dialogue Widget 不保存这些集合。
- Event Definition 资产位于 /Game/LostRunic/Data/，在 Event 分类填写稳定 EventId、条件标签、SavePolicy；需要事件完成时追加 CompletionStoryFlag。StoryState.CommitEvent 会在广播前同时写入完成事件和 StoryFlag。
- 正常 Load/New Game 使用 ReplacePersistentState；Memory 返回使用 ReplacePersistentState(HomeSnapshot.Story) 后再 ApplyPersistentDelta(DurableNarrativeDelta)。不要用增量 API 替代完整 Load。
- Save V2 的 FLRSaveStoryChunk 不在 Narrative 类型或 StoryState 蓝图接口中出现；转换由 Save 层 LRStorySaveAdapter 完成。

### GameFlow、Save 与 Transition UI

- 主菜单 New Game/Continue/Load 的按钮 Controller 调用 ULRGameFlowSubsystem，不直接控制地图旅行或 Transition Widget。
- GameFlow 为 Load/New Game/Memory 创建 GameFlowTransactionId；Save 为实际队列操作创建 SaveOperationId。蓝图事件必须同时传递并匹配两个 ID，不得用“下一个完成事件”关联流程。
- ULRGameFlowSubsystem 只广播 OnFlowPhaseChanged(TransactionId, OperationId, Phase, MapId)。ULRPlayerUIComponent 订阅它并控制 Transition Layer；GameFlow 不查找 HUD、PlayerController 或 Widget。
- Save 的 Started、PhaseChanged、LoadRequested、NewGameRequested、Completed 事件只作为状态/请求通知。不要在 Widget 中消费 WritingPayload、LoadingPayload 等 Save 内部状态。
- Memory Critical Save 的数据来源固定为 HomeSnapshot + DurableNarrativeDelta。不要从 Memory 当前世界重新 Capture。HomeSnapshot 只有对应 Return SaveOperationId 成功后才能清空。

### Variant/旧模板清理

- Variant Runtime C++ 已从 Source/LostRunic/Variant_Strategy 和 Source/LostRunic/Variant_TwinStick 移除；不要在新蓝图中选择这些类。
- Content/TopDown 当前仍被 BP_LRPlayerController 的输入/光标资产引用，不能整体删除。任何进一步迁移必须先运行 Asset Registry Referencers、Blueprint ParentClass、Soft/Class Path 和 L_PIE_Test 包加载检查。
- 旧 TopDown class redirect 保留，目标为 LRCharacter、LRGameMode、LRPlayerController；本次不删除 Redirect。

正式边界文档：Docs/Technical/08_ArchitectureBoundaries.md。
## Guard / NPC Controller Blueprint 唯一配置（2026-08-27）

本节是当前 Guard/NPC 蓝图装配的唯一权威路径，覆盖旧版连续 Exposure、VisibilityScore、DetectionStage 和 Search 配置。Guard 的 C++ 规则、参数和职责边界以 `Docs/Technical/08_ArchitectureBoundaries.md` 为准。

### 组件所有权和运行生命周期

- `ALRGuardAIController` 与 `ALRNPCController` 在 C++ 构造函数中各创建且只创建一个 `AIPerception` 和一个 `StateTreeAI`；两者默认不自动激活/启动。
- 派生 Controller Blueprint 只配置继承组件；Controller Blueprint 的 SimpleConstructionScript、`BP_Guard` 和 `BP_NPC1` 均禁止手工添加 AIPerception、StateTree 或 StateTreeAI。
- `BeginPlay`/`OnPossess` 通过 `TryInitializeRuntime()` 启动一次“Alert runtime → Perception delegate → Perception Activate → StateTree StartLogic”；`UnPossess` 对称移除委托、停用感知、停止 StateTree、导航、Focus 和计时器。
- C++ 运行时只用 `GetSenseConfig<T>()` 读取蓝图 Sense；不得在运行时 `ConfigureSense()`、`SetStateTree()` 或反射 StateTree 资产。

### BP_LRGuardController

路径：`/Game/LostRunic/Blueprints/Guard/BP_LRGuardController`，父类必须为 `ALRGuardAIController`。

1. 在 Components 选择继承的 **AIPerception**。在 **AI Perception > Senses Config** 只保留一份 Sight 和一份 Hearing，Dominant Sense 设为 Sight。当前基线资源为 Sight Radius `500 cm`、Lose Sight Radius `600 cm`、Peripheral Vision Half Angle 由 Blueprint 资产配置、Max Age `0`；UE 的半角配置可按 Guard 类型调整，C++ 不固定 60°，也不重复计算距离、扇形或 LOS。Hearing Range 为 `5000 cm`、Max Age `0`。三种 Affiliation 按项目敌对关系开启。
2. 选择继承的 **StateTreeAI**，设置 State Tree 为 `/Game/LostRunic/Blueprints/Guard/ST_Guard`；Auto Start 由 C++ 关闭。
3. 在 **Class Defaults > Guard|调优** 配置下面的 Inline `FLRGuardTuningSettings`。字段在编辑器中显示中文 `DisplayName`，不要在蓝图或关卡另存一份规则：

| 分类 | C++ 字段 | 编辑器中文名 | 基线 |
| --- | --- | --- | ---: |
| 警戒 | `AttractAlertAmount` | 警戒增加量 | 1 |
| 警戒 | `SuspiciousObserveSeconds` | 可疑观察时间 | 3 s |
| 警戒 | `InvestigateObserveSeconds` | 调查观察时间 | 3 s |
| 警戒 | `SightToChaseGraceSeconds` | 视觉追逐确认宽限 | 0.5 s |
| 警戒 | `SuspiciousStimulusCooldownSeconds` | 白色刺激冷却 | 0.5 s |
| 警戒 | `InvestigateStimulusCooldownSeconds` | 红色刺激冷却 | 0.2 s |
| 警戒 | `AlertDecayAmount` | 警戒衰减量 | 1 |
| 警戒 | `AlertDecayIntervalSeconds` | 警戒衰减间隔 | 0.5 s |
| 警戒 | `RoomRunAlertLevel` | 当前房奔跑警戒下限 | 5 |
| 警戒 | `AdjacentRoomRunAlertAmount` | 相邻房奔跑警戒增加量 | 1 |
| 视觉 | `SightTrackingIntervalSeconds` | 视觉跟踪间隔 | 0.1 s |
| 警戒 | `FirstAttractRunCooldownMultiplier` | 奔跑首次刺激冷却倍率 | 0.6 |
| 警戒 | `FirstAttractWalkCooldownMultiplier` | 走路首次刺激冷却倍率 | 1.0 |
| 警戒 | `FirstAttractSneakCooldownMultiplier` | 潜行首次刺激冷却倍率 | 1.6 |
| 移动 | `PatrolSpeed` | 巡逻速度 | 120 cm/s |
| 移动 | `InvestigateSpeed` | 调查速度 | 170 cm/s |
| 移动 | `ChaseSpeed` | 追逐速度 | 300 cm/s |
| 移动 | `MoveAcceptanceRadius` | 调查到达误差 | 50 cm |
| 移动 | `InvestigateMoveRetargetDistanceCm` | 调查重定向距离 | 75 cm |
| 捕获 | `CaptureRadius` | 捕获半径 | 75 cm |

### Guard 行为和感知验收规则

- `Alert=0` 隐藏警戒条并 Idle/Patrol；异常刺激到 1，有效 Sight 直接到 6。
- `Alert=1-5` 显示白条、面向异常；从 0 进入观察 3 秒，接受 Noise 后 +1 并刷新观察；自然衰减每 0.5 秒 -1。
- `Alert=6-10` 显示红条，前往 `LatestInvestigationLocation`，速度默认 170 cm/s；抵达后才开始红色观察，观察结束后衰减；Noise 最多到 10，Sight 直接到 11。
- `Alert=11` 显示满红条和额外红色动画；只有当前有效可见的匹配 `ConfirmedThreat` 才以默认 300 cm/s 持续追逐并在 `CaptureRadius` 执行死亡占位；真实 Sight Lost 后 11→10 并调查最后可见位置。
- `SightToChaseGraceSeconds` 只在 `Alert<=5` 首次有效 Sight `→6` 时启动一次。Grace 内 Alert 冻结；Noise 只记录异常位置，不改 Alert、不启动刺激 CD、不刷新观察、不抢调查目标。Grace 内抵达或导航失败也不启动 RedObserve；Grace 结束后仍可见→11，已抵达或导航失败→观察；失败状态保持 Failed，不伪装成已抵达，也不自动重试。
- Hard Hidden 不等于 Raw Sight Lost：Raw Contact 仍存在时持续视觉跟踪；真正的 UE Sight Lost 才停止跟踪。Grace 已消费后，红色周期内再次 Sight 立即 11；进入白色（含 6→5）或回到 0 时重置该标记。
- Room Run 当前房间低于 Floor 时先到 5，达到 Floor 后继续按 `AttractAlertAmount` +1；相邻房间按 `AdjacentRoomRunAlertAmount` +1；Noise 不能到 11。首次刺激 CD 按事件结果所属白/红档选择 0.5/0.2 基准，再乘声音产生时快照的步态倍率；非玩家/无步态事件使用 1.0。Sight 不受 Noise CD 阻挡。

### ST_Guard 配置步骤

1. 打开 `/Game/LostRunic/Blueprints/Guard/ST_Guard`，Schema 使用 `StateTreeAIComponentSchema`，Context Actor 为 `ALRGuardCharacter`，AI Controller 为 `ALRGuardAIController`。
2. Root 下只保留五个平级行为状态：`IdlePatrol`、`Suspicious`、`Investigate`、`Chase`、`Stunned`。每个状态的 `FLRGuardStateCondition` 与 `FLRGuardBehaviorTask` 使用对应枚举。
3. 每个行为状态配置 `AI.Event.BehaviorChanged → Goto Root`，事件消费并将 Reactivate Target State 设为 `ForceChanged`，保证从 Root 重新按 Alert 选择；运行时不再创建 Search 子状态。
4. `FLRGuardBehaviorTask` 在有效 Controller 上保持 `Running`；Investigate 同状态重定位由 Controller 直接更新导航，不通过第二次 StateTree 重选。

### BP_Guard、警戒条与 Room Volume

1. 打开 `/Game/LostRunic/Blueprints/Guard/BP_Guard`，设置 **AI Controller Class = BP_LRGuardController**、**Auto Possess AI = Placed in World or Spawned**。Pawn 组件树不得出现 AIPerception、StateTree 或 StateTreeAI；Alert、Knowledge、AlertWidget 由 C++ 创建。
2. `AlertWidget.Widget Class` 设置为 `/Game/LostRunic/UI/WBP_GuardAlertBar`。保留现有控件名 `Alert_Bar_White`、`Alert_Bar_Red`，在 WBP 中创建/保留名为 `Alert_Full_Red` 的 Widget Animation，并让动画绑定 `Alert_Bar_Red`。在 WBP 事件图实现继承事件 `HandleAlertSnapshotChanged`，由该事件控制两个进度条的 Percent/Visibility 和满红动画；C++ 只转发快照，不查找或控制具体控件。
3. 快照表现必须为：Alert 0 隐藏；1-5 只显示白条，Percent=`Level/5`；6-10 只显示红条，Percent=`(Level-5)/5`；11 红条 100% 并循环播放 `Alert_Full_Red`，离开 11 时停止动画。
4. 在关卡摆放 `ALRRoomVolume`，填写稳定 `RoomId` 并配置 `AdjacentRooms`。当前房奔跑和相邻房传播由 Room Volume 接线触发；无房间时使用普通 Hearing fallback。Room Volume 必须早于 Guard 生成。

### BP_LRNPCController 与 BP_NPC1

1. `/Game/LostRunic/Blueprints/Character/BP_LRNPCController` 继承 `ALRNPCController`，只在继承的 AIPerception 配置 Hearing，并指定 `/Game/LostRunic/Blueprints/Guard/ST_NPC_Stand`；不要配置 Sight 或重复添加组件。
2. `/Game/LostRunic/Blueprints/Character/BP_NPC1` 设置 **AI Controller Class = BP_LRNPCController**；Pawn 不添加 AIPerception、StateTree 或 StateTreeAI。

### 编译与 PIE 验收

- 编译：`LostRunicEditor Win64 Development`。
- 测试地图：`/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`。
- 必测序列：普通 Noise `0→1`、白色观察/衰减、`0/1/5 + Sight→6`、Grace 内 Noise 保持 6、Grace 内抵达/失败、Grace 结束仍可见→11、Grace 丢失→10 级调查、`6→5` 后 Sight 重新获得 Grace、红色 Noise `6→7→10`、Room Run `0→5→6→7→10`、Hard Hidden 保留 Raw Contact、真实 Sight Lost 停止跟踪、`11→10` 保留 ConfirmedThreat、Alert 归零清空记忆。
- 检查 StateTree Debugger 只出现五个 Guard 状态，检查导航移动时身体转向而非横向平移，检查 Output Log 不新增 Guard/AIPerception/StateTree/Navigation/UI Warning 或 Error。

## Perception Rendering Runtime（2026-09-04）

本节是闭眼显影第一版的运行时装配契约。代码实现位于 `ULRPerceptionPresentationComponent`、`ULRPerceptionEventSubsystem`、`ULRPerceptionSoundSourceComponent` 和 `ULRPerceptionAccentComponent`；这些类型只维护表现状态，不改变 `ULRStateComponent` 的合法性或解锁协议。

### 资产与参数绑定

1. 打开 `/Game/LostRunic/Data/DA_LRGameContentSet`（若 Content Browser 显示的同名资产路径不同，以当前资产实际路径为准），在 **Content|Presentation → Default Visual Style** 指向项目默认 `ULRVisualStyleDefinition`。在每个 `FLRMapRegistration` 的 **Map|Presentation → Visual Style Override** 填写可选章节覆盖；未填写时回退到 Default Visual Style。
2. 打开 `/Game/LostRunic/Data/Tuning/DA_LRPresentationTuning`，在 **Presentation|Perception|Assets** 绑定 `M_PP_LR_PerceptionComposite`（Before Tonemapping）、`MPC_LR_VisualStyle`、`MPC_LR_PerceptionRuntime` 和可选 `NS_LR_PerceptionPulse`。材质参数名必须与 `Source/LostRunic/Perception/LRPerceptionMaterialParameters.h` 保持一致。缺少 Niagara 只关闭装饰波壳，不关闭规则显影。
3. 在同一资产设置：`PerceptionRevealRadius=450 cm`、`PerceptionFullRevealRadius=400 cm`、`NoiseRevealRadius=200 cm`、`NoiseRevealDurationSeconds=5 s`、`EchoExpansionSeconds=0.75 s`、`EchoWetSeconds=0.20 s`、`EchoDryFadeDurationSeconds=1.30 s`、`EchoWaveWidthCm=12.5 cm`、`EchoRefreshMergeDistanceCm=25 cm`、`AccentDepthToleranceCm=3 cm`、进入/退出 PP Blend 为 `0.30/0.20 s`。不要添加或保留 `EchoDryFadeStartSeconds`；3.70 s 只由 `5.0-1.3` 派生。
4. `MPC_LR_VisualStyle` 只放全局状态参数：`LR_StateBlend`、`LR_PerceptionIntensity`、`LR_NormalOutlineGate`、`LR_InteractionPresentationGate`、`LR_PlayerOcclusionColor`。`MPC_LR_PerceptionRuntime` 只放 `LR_PlayerPosition` 以及 `LR_EchoCenterRadius0..7`、`LR_EchoTiming0..7`。Palette、HDR、Normal retention、Shape lift、Echo/Wet/Accent 外观及 Perception/Echo 半径时间参数由 PP MID 提供：`LR_PerceptionFullRevealRadius`、`LR_PerceptionRevealRadius`、`LR_EchoExpansionSeconds`、`LR_EchoWetSeconds`、`LR_EchoDryFadeDurationSeconds`、`LR_EchoWaveWidthCm`、`LR_PerceptionBoundaryNoiseCm`、`LR_AccentDepthToleranceCm`。

### 组件装配与事件语义

1. `ALRCharacter` 原生创建一个 `PerceptionPresentation`。它订阅 sibling `StatePresentation.OnStatePresentationRequested(previous,next,reason)` 启动进入/退出，而不是绕过表现边界订阅 `StateComponent.OnStateChanged`；`StateComponent` 只用于 BeginPlay 初始模式查询。组件无常驻 Tick；进入/退出 PP Blend 时才启用组件 Tick。玩家移动时由 `CharacterMovementUpdated` 更新 `PlayerPosition`，静止时不产生位置写入。
2. 固定 8 槽的 `EchoTimingN` 是 `(FirstStartTime, LastPulseTime, ExpireTime, Intensity)`，时间统一来自 `UWorld::GetTimeSeconds()`；材质使用与同一 View Game Time 对齐的 Time 节点（Ignore Pause=false、Override Period=false），不通过 MPC 传 `CurrentTime`。Residue Dry Fade 只使用 `ExpireTime-CurrentTime` 的最后 `EchoDryFadeDurationSeconds`。
3. `ULRPerceptionSoundSourceComponent` 的 `LoopIntervalSeconds=0` 使用 PresentationTuning 的 `DefaultLoopIntervalSeconds=3 s`；Radio 可保持 0，Wind 等来源显式填 `6 s`。正值是该实例的明确覆盖。视觉半径正值使用实例覆盖，否则使用 `NoiseRevealRadius=200 cm`，不会自动继承 AI Noise Radius。若 `bAlsoEmitToAI=true`，另填独立的 `AIHearingRadiusCm`；该字段直接送入 `UAISense_Hearing::ReportNoiseEvent`，不会改变视觉半径，也不依赖 `ULRNoiseEmitterComponent`。
4. 只有 `bRefreshExistingSource=true` 的 looping 来源才尝试合并；必须同一 SourceObject、原槽未过期且新位置与旧 Center 距离不超过 `25 cm`。OneShot/gameplay pulse 默认 false，因此移动 NPC 在 A/B 两点发声不会瞬移旧残影。
5. `ULRNoiseEmitterComponent` 在自身 BeginPlay/EndPlay 注册和注销 World Perception bridge；bridge 只接收白名单 Reason，并把原 AI Hearing Radius 丢弃为视觉默认半径，避免两套规则半径耦合。Narrative Accent 组件进入 Perception 时缓存 Primitive 的旧 CustomDepth/Stencil，写 `Stencil=3` 并打开 CustomDepth；退出时只在仍由它写入 `3` 的情况下恢复。Stencil 3 的材质可见性还必须通过 SceneDepth/CustomDepth 线性深度容差门，不能用 Stencil 单独显影。Interaction Presentation 在此期间被 `SetInteractionPresentationSuppressed(true)` 挂起，退出后按当前状态重新应用；Stencil 1/2/3 仍是互斥固定值。

### 状态表现协调与验收

1. `/Game/LostRunic/UI/WBP_LRStateEyeOverlay` 派生自 `ULRStateEyeOverlayWidget`。Widget Tree 使用全屏 `CanvasPanel` Root，并创建 `TopBlock`、`BottomBlock` 两个勾选 **Is Variable** 的 `Image`；上块锚点 `(0,0)-(1,0.5)`、下块锚点 `(0,0.5)-(1,1)`，Offsets 全为 0，Root 与 Image 均设为 **Hit Test Invisible**。运行时位移、Tint、Opacity 由 C++ 控制，不在 Widget Blueprint 重复制作动画。
2. 打开 `/Game/LostRunic/Blueprints/UI/BP_LRHUD`，在 Class Defaults 将 **State Overlay Screen Class** 指向 `WBP_LRStateEyeOverlay`。该层常驻 HUD 生命周期，但稳定状态会由 Widget 自身折叠。
3. `ULRStatePresentationComponent::CompleteStatePresentation()` 仍是唯一正常解锁入口。由于原生组件的 protected `PresentStateChange` 事件不会作为 Actor override 暴露给 `BP_Ruth`，当前 `/Game/LostRunic/Blueprints/Character/BP_Ruth` 在 BeginPlay 对原生 `StatePresentation` 组件执行 **Assign On State Presentation Requested**：委托事件按 `nextMode == Perception` 选择 `0.30 s`，其他状态选择 `0.20 s`，经唯一 Delay 后只调用一次该组件的 `CompleteStatePresentation()`。PP transition 由同一委托驱动，Eye Overlay 由 Hold/State UI 事件自行收尾，不向这条链回报完成；Renderer、HUD Controller、Eye Overlay 均不得直接释放状态锁。

在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 验收：

- Enter 后约 2 s 暂停游戏 5 s 再恢复，Echo 应继续约 2 s 年龄，不应立即过期；用 Time Dilation `0.5` 和 `2.0` 重复。
- 验证 1、8、9 个事件的空槽/过期槽/最早 ExpireTime 选择、同源近距刷新、远距空间拆槽和刷新后 `FirstStartTime` 保留。
- 验证 Player 400 cm 全显影、400–450 cm Wash 羽化、450 cm 外无 Player Reveal；Echo 不受 450 cm 玩家半径限制。
- 验证 Stencil 3 前景 Accent 可见、墙后 Accent 不泄漏，Stencil 2 Player Occlusion 仍按原规则工作；双角色 Interaction 在 Accent 生命周期内不恢复白色 Outline/FarHint，但 200 cm HUD 执行提示仍可读。
- Normal Stable 时 GPU Visualizer/DumpGPU 不应出现 Perception Composite Pass；Candidate Development Budget（1080p 1.5 ms、1440p 2.5 ms）只作为当前开发 GPU 警戒线，目标硬件锁定后重新基准化。
