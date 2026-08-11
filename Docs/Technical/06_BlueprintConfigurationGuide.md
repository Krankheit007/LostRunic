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
- **蓝图资产**：项目使用的 GameMode 蓝图（当前 Home 体验为 `/Game/LostRunic/Blueprints/Framework/BP_LRGameMode`）
- **配置步骤**：
  1. 创建 `ALRGameMode` 的蓝图派生类。
  2. 在默认值中指定玩家 Pawn、PlayerController、HUD 和内容聚合资产。
  3. 在 `ULRGameContentSet` 中填写对白/阅读 DataTable、物品/收藏品/守卫定义和地图注册信息。
  4. 在项目设置或关卡 World Settings 中指定该 GameMode；实例覆盖只用于关卡明确要求的差异。
- **参数要求**：所有 DataTable 行 ID、定义资产 ID、Home/Memory 地图 ID 必须稳定且可解析；缺失引用应在校验或日志中暴露。
- **验收**：进入 Home 和 Memory PIE，确认 GameMode、Pawn、Controller、HUD 和内容资产均已加载，Output Log 无项目级加载错误。

### 玩家角色与功能组件

- **代码入口**：`ALRCharacter` 及其移动、状态、交互、库存、噪声、掩体和状态表现组件。
- **蓝图资产**：项目使用的角色蓝图（由 `ALRCharacter` 派生）。
- **配置步骤**：
  1. 创建角色蓝图并确认继承 `ALRCharacter`。
  2. 检查 C++ 构造的组件是否存在；不要重复添加同职责组件。
  3. 在蓝图中仅配置网格、动画、摄像机挂点和表现资源；状态、库存、交互合法性由组件处理。
  4. 将组件委托绑定到 HUD、状态提示、交互提示和特效表现。
- **参数要求**：移动速度、状态长按阈值、交互距离、噪声和动画安全超时来自对应 Tuning 资产；蓝图不写入运行时状态。
- **验收**：PIE 中验证正常/感知/勇气状态、交互提示、库存使用和掩体行为；适用时分别使用键鼠和手柄。

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
