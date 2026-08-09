# 《不要忘记阿黛尔》技术设计

> 文档类型：绿地游戏架构与首个垂直切片方案
> 目标平台：Windows 单机；键鼠与手柄
> 代码策略：C++ 核心规则 + 蓝图派生/关卡装配/表现
> 当前状态：项目仍为 UE 5.8.1 初始模板，本文件不代表已实现代码。

## 1. 设计原则

1. **规则集中**：状态、警戒、交互筛选、存档和剧情条件由 C++ 维护单一真相。
2. **内容与数值数据化**：对白、物品、笔记、收集品、敌人参数、关卡事件和所有玩法调优值通过 DataTable/DataAsset、Input Action 或蓝图默认值配置。
3. **Actor 组合优先**：角色只负责移动和生命周期；状态、交互、物品、声音、掩体等使用组件。
4. **事件驱动**：UI、音效、保存和剧情响应事件，不依赖每帧轮询。
5. **蓝图负责表现与调参**：材质、Niagara、动画、widget 布局、关卡编排和局部实例参数可由蓝图实现，但不能在蓝图中复制核心规则或建立第二份权威数值。
6. **先切片后扩展**：先完成“家”章节的最小闭环，再增加多章内容和编辑器工具。

## 2. 模块与目录

前期保持单一 `LostRunic` Runtime 模块，按领域分目录。只有在内容验证器、表格导入工具或自定义编辑器稳定后，才考虑新增 `LostRunicEditor`。

```text
Source/LostRunic/
  Core/          日志、Gameplay Tags、ID、事件、通用类型
  Character/     角色、移动模式、摄像机、年龄表现接口
  State/         ELRPerceptionMode、状态规则、状态表现
  Interaction/   可交互接口、候选筛选、交互提示、交互执行
  Stealth/       噪声、掩体、感知、警戒值、死亡与逃脱
  AI/            Guard、AIController、StateTree 任务/条件/评估器
  Narrative/     对话、阅读、剧情事件、结局条件
  Items/         道具、笔记、收藏品、库存和快捷栏
  Save/          SaveGame、槽位、版本迁移、异步写盘
  UI/            HUD、对话、背包、存档、暂停和设置的数据接口
  Data/          DataAsset、DataTable Row、Tuning 配置、资产加载和校验
```

推荐依赖：`Core`、`CoreUObject`、`Engine`、`EnhancedInput`、`AIModule`、`NavigationSystem`、`StateTreeModule`、`GameplayStateTreeModule`、`GameplayTags`、`Niagara`、`UMG`、`Slate`。不引入 GAS、网络模块或 CommonUI。

## 3. 核心契约

### 3.1 状态

```cpp
UENUM(BlueprintType)
enum class ELRPerceptionMode : uint8
{
    Normal,
    Perception,
    Courage,
    Memory
};
```

`ULRStateComponent` 负责当前状态、请求切换、规则验证和事件广播。状态切换请求应携带来源、输入类型和目标状态，避免蓝图直接写入当前状态。

关键事件：

- `OnStateChanging(Previous, Next, Reason)`
- `OnStateChanged(Current)`
- `OnStateChangeRejected(Request, Reason)`

首版状态输入规则：

- 正常 -> 感知：闭眼输入持续 `0.8s`；感知 -> 正常：睁眼输入持续 `0.3s`。
- 正常 -> 勇气：睁眼输入持续 `0.8s`；勇气 -> 正常暂按闭眼输入持续 `0.3s` 实现，并在原型评审前确认。
- 首次进入动画约 `1s`；返回动画约 `0.5s`。阈值前释放会取消，达到阈值即提交状态；动画完成前拒绝新请求。
- 每次物理按下最多提交一次请求。动画完成后允许立刻开始相反输入，但不会因持续按住自动重复触发。
- 死亡可从任意可玩状态进入 Memory；非死亡的 Perception -> Memory 条件仍由剧情事件定义。

所有时间均来自 `ULRStateTuning` 或输入资产。`ULRStatePresentationComponent` 接收 UMG/动画完成事件后通知状态组件释放动画锁；配置中的安全超时只用于防止资源异常导致永久锁定，核心状态组件不直接依赖具体 Widget。规则还必须覆盖掩体、对话、菜单、过场、双键同时按下、死亡转场和拒绝反馈。

### 3.2 交互

```cpp
USTRUCT(BlueprintType)
struct FLRInteractionOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag ActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Prompt;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MaxDistanceOverride = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ELRPerceptionMode RequiredMode = ELRPerceptionMode::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagQuery RequiredItemTags;
};
```

`ILRInteractable` 提供候选选项、可用性查询和执行入口。`ULRInteractionComponent` 负责按距离、朝向 90°、遮挡和状态筛选唯一目标，并广播当前提示。`MaxDistanceOverride <= 0` 时使用 `ULRInteractionTuning` 默认执行距离，正值才作为该选项的显式覆盖。对象本身不应自行扫描玩家或直接创建 UI。

物品使用统一为 `FLRItemUseRequest`，至少携带物品 ID、来源栏位、当前交互目标、使用入口（快捷栏/交互选择器）和玩家状态。`ULRItemUseResolver`（或 Inventory 内等价职责）统一完成目标匹配、状态/标签检查、消耗、执行和结果反馈：

- 键鼠按 `1`-`4`，或手柄选中栏位后按 `Y`，向当前合法目标直接提交请求。
- 默认交互 `E/X` 遇到需要物品的门或机关时，打开背包的“选择使用道具”模式；选择结果提交同一种请求。
- 勇气武器仅在 Courage 状态下，对具有允许目标标签且通过距离/朝向筛选的敌人提交非致死击退；首版 CD 基线为 `1s`。
- 两条入口不得各自实现开门、消耗或剧情事件逻辑。无目标、错误物品、冷却中和免疫目标都返回结构化失败原因给 UI。

### 3.3 玩家组件

- `ULRStateComponent`：四状态与状态事件。
- `ULRInteractionComponent`：目标查询、提示和执行。
- `ULRInventoryComponent`：道具、快捷栏、笔记/收藏品 ID。
- `ULRNoiseEmitterComponent`：移动与交互产生的声源事件。
- `ULRHideComponent`：进入、退出、移动限制和可见性规则。
- `ULRStatePresentationComponent`：把状态事件映射到后处理、描边、Niagara 和 UI。

### 3.4 敌人与警戒

`ALRGuardCharacter` 只保存可配置参数和组件；`ALRGuardAIController` 负责感知与 StateTree；`ULRAlertComponent` 保存 0-11 警戒值、最后异常位置、观察计时和目标角色。

StateTree 初版状态：

```text
Idle/Patrol
  -> Suspicious       (警戒 1-5)
  -> Investigate      (警戒 6-10，前往异常位置)
  -> Search           (抵达后观察)
  -> Chase            (警戒 11 且视野内有玩家)
```

所有升降警戒的原因应记录为 Gameplay Tag，例如 `Noise.Footstep`、`Sight.Player`、`Search.Timeout`，便于日志和测试。

### 3.5 数据接口

- `FLRDialogueRow`：行 ID、说话者 ID、文本、立绘软引用、下一行 ID、选项组 ID、条件标签。
- `FLRReadingRow`：笔记 ID、标题、正文、章节标签。
- `ULRItemDefinition`：道具 ID、图标、显示名、可用动作和目标标签。
- `ULRCollectibleDefinition`：收藏品 ID、模型、说明、陈列顺序。
- `ULRGuardDefinition`：移动速度、视野角、视野半径、听觉倍率、警戒参数。
- `ULRLevelEventDefinition`：剧情事件 ID、触发条件、一次性标记和保存策略。
- `ULRStateTuning`：进入/返回长按时间、动画安全超时、勇气攻击 CD 与击退时间。
- `ULRInteractionTuning`：远距提示、描边、执行距离、朝向角和物品目标选择参数。
- `ULRMovementTuning`：潜行/走路/奔跑速度和不同环境的噪声参数。
- `ULRSaveTuning`：普通自动存档防抖、重试次数和保存策略默认值。

Excel 作为编剧输入，统一导出 UTF-8 CSV 后导入 DataTable。行 ID 不得依赖行号；文本使用 `FText` 和本地化键，不能在 C++ 中硬编码对白。

调优资产使用普通 `UDataAsset`，由一个 `ULRGameTuningSet` 聚合并在项目/模式默认值中指定。全局或原型级参数使用 `EditDefaultsOnly + BlueprintReadOnly`；确需逐关卡覆盖时才使用 `EditInstanceOnly`。属性应声明单位、Clamp 和合理 UI 范围。C++ 可提供安全回退默认值，但运行时规则函数不得散落数值字面量；缺少必需资产时应校验并明确报错。

## 4. 系统职责

### 4.1 Gameplay Framework

- `ALRGameMode`：单机规则、玩家生成和地图进入策略。
- `ALRPlayerController`：Enhanced Input、输入模式切换、暂停和 UI 层级。
- `ALRCharacter`：移动、朝向、摄像机挂点和组件组合。
- `ALRGameState`：只保存跨角色的世界事件或章节状态；单机不把所有系统塞进 GameState。
- `ULRGameInstanceSubsystem`：存档、全局剧情进度、槽位元数据和地图切换协调。

### 4.2 输入

使用 Enhanced Input，并至少拆分 `IMC_Gameplay`、`IMC_Dialogue`、`IMC_Menu` 和 `IMC_Transition`。高优先级上下文屏蔽低优先级冲突键，切换时忽略仍按住的按键直到释放；对话中的鼠标左键因此不会同时触发闭眼。

| Action 语义 | 键鼠默认映射 | Xbox 手柄默认映射 |
|---|---|---|
| Move | `WASD` | 左摇杆（Radial Dead Zone） |
| Interact | `E` | `X` |
| UseQuickSlot1-4 | `1`-`4` | 左/右方向键选择，`Y` 使用 |
| ToggleRun | `Shift` | `A` |
| ToggleCrouch | `C` | `B` |
| OpenJournal | `Tab` | 上方向键 |
| Pause | `Esc` | `Start` |
| CloseEyes | 鼠标左键 | `LT` |
| OpenEyes | 鼠标右键 | `RT` |
| DialogueConfirm | `Space`、鼠标左键 | `A` |

状态切换由 PlayerController 把输入边沿交给 `ULRStateComponent`；组件以计时器和 `ULRStateTuning` 判定当前状态所需的 `0.8s/0.3s`，不依赖 Tick。UI 接收按住开始、取消、阈值通过和动画结束事件。输入层只提出请求，StateComponent 决定是否接受；键鼠与手柄共享动作语义，不在角色类中写死按键或持续时间。

对话确认采用二段语义：打字机动画尚未结束时显示当前句全文；全文已显示时推进下一句。菜单打开、暂停、死亡遮罩和地图切换期间禁用 Gameplay 状态动作。

### 4.3 潜行

移动模式由 `ULRMovementModeComponent` 或角色移动状态驱动，速度和声音强度来自配置。声音通过 `UAISense_Hearing` 或项目统一噪声事件发布，敌人不直接读取角色速度。

视线检测必须使用带遮挡的 Sight Query。掩体通过接口或组件向警戒系统提供“不可见”结果，不修改敌人的基础视野。

### 4.4 叙事与 UI

`ULRDialogueSubsystem` 读取行 ID 并解析条件；`ULRDialogueWidgetController` 将当前行、选项、头像和“正在打字/已完整显示”状态推送给 UMG。打字机速度与动画表现由 Widget/调优资产控制，推进规则由控制器维护。阅读系统复用对话推进器，但隐藏说话者和立绘。

HUD 只订阅状态、交互目标、警戒提示和快捷栏事件。背包、笔记、收藏品、暂停和存档使用独立 widget/controller，避免一个根 Widget 包含全部逻辑。背包支持普通浏览和“为当前交互选择道具”两种模式，后者只展示或突出兼容物品，提交后返回交互结果。

### 4.5 状态表现

- 正常：关卡材质和基础后处理。
- 感知：角色周围持续显形，声源创建短时显形区域，配合声波 Niagara。
- 勇气：后处理材质参数控制边缘模糊、过曝和失真。
- 回忆：独立地图/关卡或独立场景层，收集品与提示 NPC 从存档进度生成。

状态表现只读取 `ELRPerceptionMode` 和参数，不参与状态判定。闭眼/睁眼 Widget 动画在取消时反向回退，在完成时通知状态组件解除动画锁；C++ 不重复写死约 `1s/0.5s` 的动画资源时长。

## 5. 存档方案

`ULRSaveGame` 使用 `SaveVersion` 和分块结构：恢复锚点、章节、状态、库存、笔记、收藏品、剧情事件、回忆事件、死亡次数、累计时间和时间戳。交互物和剧情事件使用稳定 `FName`/GUID，不使用 Actor 名称或数组顺序。

`FLRResumeAnchor` 单独保存可恢复地图 ID、稳定锚点 ID、位置和朝向；它与“当前正在显示的场景”分离。这样可以在回忆场景写入死亡次数和调查事件，而不把 Memory 地图覆盖为下次读档出生点。

`ULRSaveSubsystem` 负责：

1. 创建槽位元数据。
2. 自动存档防抖与安全点排队。
3. 手动槽覆盖确认。
4. 异步写入和失败重试。
5. 版本迁移与损坏存档提示。
6. 地图切换后恢复位置与事件状态。
7. 关键存档请求排队、顺序提交和跨地图中断恢复。

首版普通自动存档只由地图切换和 `SavePolicy.AutoOnComplete` 的重要剧情事件触发；普通请求使用 `ULRSaveTuning` 中默认 5-10 秒的可调防抖。事件数据显式声明保存策略，代码不按事件名称或交互类型猜测。

死亡回忆采用关键事务序列：

1. 死亡输入锁定，内存状态中的死亡次数 `+1`，缓存死亡前 `FLRResumeAnchor`。
2. 遮罩动画内异步切换 Memory 地图；世界就绪后提交关键自动存档 A，只更新死亡次数与持续进度，不更新恢复锚点。
3. 回忆调查事件持续记录；指定 NPC 触发离场后，切换到恢复锚点地图并在世界状态应用完毕后提交关键自动存档 B。
4. A、B 不参与普通防抖或简单“保留最后一次”合并。若已有写盘任务，Subsystem 按序排队并从不可变快照写入，完成/失败均产生事件和日志。

读档时无论存档是在死亡进入回忆后还是回忆调查中提交，都以 `FLRResumeAnchor` 为出生目标，并保留已成功提交的死亡次数/回忆事件。首版是否允许在 Memory 中手动存档仍为设计待决；实现前默认禁用并给出明确 UI 原因，避免生成语义不明的手动槽。

## 6. 资源目录建议

```text
Content/LostRunic/
  Characters/
  Levels/Home/
  Levels/Ship/
  Data/Items/
  Data/Dialogue/
  Data/Notes/
  Data/Collectibles/
  Data/Guards/
  Data/Tuning/
  Input/Actions/
  Input/Contexts/
  UI/
  Materials/States/
  FX/Perception/
  Audio/
```

## 7. 首个垂直切片：家

1. 角色移动、镜头、潜行/走路/奔跑。
2. 正常、感知、勇气、回忆状态的最小切换闭环，含键鼠/手柄长按、中断与动画锁。
3. 一个巡逻守卫，包含视线、脚步声、警戒 0-11 和追逐。
4. 一种可移动掩体和一种固定掩体。
5. 一个知识锁谜题：感知发现线索，勇气解锁原本被拒绝的交互。
6. 一段对话、一份可阅读内容和一个收藏品。
7. 角色死亡后进入回忆，调查后通过 NPC 离开并返回最近保存位置。
8. 地图/重要事件自动存档、死亡回忆双关键存档和手动存档，重启后恢复关键事件。
9. 同一扇门同时支持快捷栏钥匙和交互后背包选钥匙，两条路径结算一致。

## 8. 测试矩阵

### 自动化测试

- 状态转换合法/非法组合。
- `0.8s/0.3s` 阈值前取消、阈值通过、动画锁和一次按下只触发一次。
- Gameplay/Dialogue/Menu 输入上下文切换时不发生幽灵输入或闭眼/对话双触发。
- 0-11 警戒值的升降、冷却和观察计时。
- 视野遮挡、听觉半径和掩体不可见性。
- 交互候选按距离和朝向选择唯一目标。
- 道具条件、对话分支和剧情事件一次性标记。
- 快捷栏与交互选择器产生相同物品使用结果，勇气攻击遵守目标标签和 `1s` 可调 CD。
- 存档版本迁移、槽位覆盖、损坏数据、关键队列顺序和恢复锚点不被 Memory 地图覆盖。

### 功能测试与 PIE 冒烟

- “家”章节从开始到死亡、回忆调查、NPC 离场、返回保存位置和读档完整通过。
- 键鼠与手柄均可完成状态切换、交互、快捷栏和暂停。
- 在关键存档 A 后、B 前退出并重新进入，验证死亡次数/已提交回忆事件保留且出生点仍为原恢复位置。
- 用快捷栏和背包选择两种路径打开同一扇门，验证钥匙消耗、门状态、剧情事件与存档完全一致。
- 感知显形、勇气后处理、警戒条和声波特效正确显示。
- 关闭/重新打开 PIE 后，位置、事件、库存和笔记一致。

## 9. 调试入口

- `LR.Debug.State`：打印当前状态和最近一次切换原因。
- `LR.Debug.Alert`：绘制守卫视野、听觉半径和警戒阶段。
- `LR.Debug.Interaction`：显示交互候选及拒绝原因。
- `LR.Debug.Save`：输出槽位、版本和写盘结果。
- `LR.Debug.Tuning`：输出当前 Tuning Set、资产来源、覆盖层级和关键参数值。
- Visual Logger：记录守卫目标点、声源、路径和状态转换。

## 10. 风险与取舍

- 不使用 GAS：本项目没有复杂战斗属性，先用组件和 Gameplay Tags 降低架构成本。
- 不做多人：不设计复制、服务器权威和网络存档。
- 不做 CommonUI：首版用 UMG + 控制器接口，待平台数量增加后再评估。
- 不直接读取 Excel：通过 CSV/DataTable 稳定化内容管线，避免运行时依赖 Office 格式。
- 不把所有数值放进单一巨型蓝图：按 State、Interaction、Movement、Guard、Save 等领域建立配置资产，由聚合资产统一引用，保证可调同时保持所有权清晰。

## 更新记录

| 日期 | 变更 |
|---|---|
| 2026-08-09 | 纳入完整输入映射、状态长按、死亡回忆存档事务、双路径物品使用和编辑器可调参数架构。 |
