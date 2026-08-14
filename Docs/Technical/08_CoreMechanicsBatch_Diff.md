# 核心玩法批次 Git Diff（2026-08-14）

提交信息：Implement core mechanics: four-state gating, stealth noise, guard alert, NPCs

提交范围：4.1 四状态 + 4.2 潜行核心机制（状态→步态权限、噪声语义、敌人警戒 4.2.1、房间传播、掩体、通用 NPC、警戒条数据层、表现预留）。

```diff
diff --git a/CLAUDE.md b/CLAUDE.md
index 6e799e4..aa0fb48 100644
--- a/CLAUDE.md
+++ b/CLAUDE.md
@@ -21,6 +21,7 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
 ```
 
 - 修改 C++ 后必须重新编译编辑器（或使用 Live Coding）再进 PIE；不要在未编译状态下创建/修改依赖反射的蓝图资产。
+- 仓库提供构建/测试包装脚本（避免 Git Bash→cmd 的引号问题）：`Scripts/BuildLostRunicEditor.bat`、`Scripts/RunLostRunicTests.bat`（引擎路径硬编码在脚本内，移动引擎时需同步修改）。
 - 自动化测试位于 `Source/LostRunic/Tests/LR*Tests.cpp`（`WITH_DEV_AUTOMATION_TESTS` 下编译），无独立测试套件；可经 Session Frontend 运行，也可命令行跑：
   `UnrealEditor.exe "$PWD\LostRunic.uproject" -unattended -nopause -ExecCmds="Automation RunTests <Test名或前缀>; Quit"`
 - 冒烟测试：在两个变体地图分别 PIE，检查 Output Log 警告/错误，AI/UI 改动同时验证键鼠与手柄。
@@ -36,11 +37,12 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
 | `Core/` | 日志、Gameplay Tags、通用类型、验证、调试命令 |
 | `Data/` | Tuning DataAsset、内容定义（物品/收藏品/守卫/关卡事件）、`ULRProjectSettings`、聚合资产 `ULRGameTuningSet` |
 | `Framework/` | `ALRGameMode`、`ALRPlayerController`、`ALRCharacter`、`ALRGameState`、`ULRGameInstanceSubsystem` |
-| `State/` | 四状态组件与规则（Normal/Perception/Courage/Memory）、状态表现组件 |
-| `AI/` | 守卫、警戒组件、感知规则、StateTree 节点（`LRGuardStateTreeNodes`） |
+| `State/` | 四状态组件与规则（Normal/Perception/Courage/Memory）、状态表现组件（含艺术表现预留 getter） |
+| `AI/` | 守卫（控制器按生命周期/感知/行为拆 3 cpp）、警戒组件（4.2.1 全量语义）、通用 NPC（`LRNPC*`，StateTree 驱动）、感知规则、StateTree 节点（`LRGuardStateTreeNodes`/`LRNPCStateTreeNodes`） |
 | `Interaction/` | `ILRInteractable` 接口、交互筛选组件、门/拾取/世界交互 Actor |
 | `Items/` | 背包、物品使用统一解析（`ULRItemUseResolver`，快捷栏与交互选择器双入口共用） |
-| `Stealth/` | 掩体（`ULRHideComponent`）、噪声发射（`ULRNoiseEmitterComponent`）、守卫可见性 |
+| `Stealth/` | 掩体（`ULRHideComponent`，进掩体强制潜行覆盖）、噪声发射（`ULRNoiseEmitterComponent`）、守卫可见性 |
+| `Gameplay/` | 移动（`ULRLocomotionComponent`：Request*/ApplyPace/OverridePace 权限拆分）、步态×噪声纯规则（`LRMovementRules`）、噪声区域（`ALRNoiseArea`）、室内奔跑房间体积（`ALRRoomVolume`） |
 | `Narrative/` | `ULRDialogueSubsystem`（DataTable 遍历、条件、分支、一次性剧情事件） |
 | `Save/` | SaveGame 分块结构、保存队列、`FLRResumeAnchor` 恢复锚点 |
 | `UI/` | HUD、Widget Controller（对话/菜单/过渡）、`ULRPlayerUIComponent` |
@@ -51,7 +53,10 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
 
 - **职责划分**：Actor 负责生命周期与组件组合，Component 负责独立能力，Subsystem 负责跨 Actor 长期状态。`ALRPlayerController` 是 Enhanced Input 上下文、输入模式、光标状态的唯一所有者；`ULRGameInstanceSubsystem` 持有已验证的内容与调优根；Widget 只接收不可变表现数据，不得反向决定规则。
 - **模板遗留**：模块根部的 `LostRunicCharacter`、`LostRunicGameMode`、`LostRunicPlayerController`（及 `Content/TopDown/`）是 UE 模板残留。新功能优先复用 `LR*` 类，避免形成第二套框架。
-- **调优与数据**：影响手感/平衡的值必须落在 `Content/LostRunic/Data/Tuning/` 的领域 DataAsset（`ULRStateTuning`、`ULRInteractionTuning`、`ULRMovementTuning`、`ULRGuardTuning`、`ULRSaveTuning`、`ULRUITuning` 等），由 `ULRGameTuningSet` 聚合，`DefaultGame.ini` 指定默认集。C++ 默认值只是安全回退；稳定 ID 用 `FName`/GUID；非关键资源用软引用异步加载；禁止散落硬编码 `/Game/...` 路径。
+- **调优与数据**：影响手感/平衡的值必须落在 `Content/LostRunic/Data/Tuning/` 的领域 DataAsset（`ULRStateTuning`、`ULRInteractionTuning`、`ULRMovementTuning`、`ULRGuardTuning`、`ULRSaveTuning`、`ULRUITuning`、`ULRNPCTuning` 等），由 `ULRGameTuningSet` 聚合，`DefaultGame.ini` 指定默认集。C++ 默认值只是安全回退；稳定 ID 用 `FName`/GUID；非关键资源用软引用异步加载；禁止散落硬编码 `/Game/...` 路径。**字段重命名必须同步 `Config/DefaultEngine.ini` 的 `+PropertyRedirects`（先例：`HearingAlertAmount`→`AttractAlertAmount` 等）**。
+- **输入**：Enhanced Input 语义动作，代码绑定动作不绑按键；长按阈值/死区/曲线放入输入与调优资产；上下文切换时 `bIgnoreAllPressedKeysUntilRelease`（`IMC_LRGameplay` / `IMC_LRDialogue` / `IMC_LRMenu` / `IMC_LRTransition`）。
+- **AI**：StateTree + 统一声源/视线事件，警戒 0–11 必须记录原因 Gameplay Tag（如 `Noise.Footstep`、`Sight.Player`）；禁止随机 Tick 分支替代状态图。守卫行为由 `ResolveTargetBehavior`（`LRAlertRules` 纯规则，眩晕优先）唯一权威解析，StateTree 只执行结果：树**仅由 `AI.Event.BehaviorChanged` 驱动**（`AI.Event.AlertChanged` 只表示数据变化）；`ST_Guard`/`ST_NPC` 资产需在编辑器人工创建（MCP 只能检查）并挂在 `DA_LRGuardDefinition`/`DA_LRNPCDefinition.Behavior`（**硬引用**）。
+- **潜行噪声语义（4.2）**：潜行完全无声；`ALRNoiseArea` 三环境（Indoor/Outdoor/OutdoorStealth，重叠按 Indoor>OutdoorStealth>Outdoor 解析，**无区域默认 Outdoor**）；室内奔跑走 `ALRRoomVolume` 房间传播（当前房警戒至少 5、相邻房 +1，多房间取最大，无房间回退 1200 半径听觉事件，传播路径绝不发 `ReportNoiseEvent` 防双计）；`Walk.Faint` 仅警戒 ≥6 守卫响应。
 - **输入**：Enhanced Input 语义动作，代码绑定动作不绑按键；长按阈值/死区/曲线放入输入与调优资产；上下文切换时 `bIgnoreAllPressedKeysUntilRelease`（`IMC_LRGameplay` / `IMC_LRDialogue` / `IMC_LRMenu` / `IMC_LRTransition`）。
 - **AI**：StateTree + 统一声源/视线事件，警戒 0–11 必须记录原因 Gameplay Tag（如 `Noise.Footstep`、`Sight.Player`）；禁止随机 Tick 分支替代状态图。
 - **默认禁止 Tick**：优先委托、计时器、StateTree、Gameplay Tags、事件队列。
@@ -62,5 +67,5 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
 
 - **蓝图工具**：编辑器中操作蓝图优先用 unreal-mcp（`http://127.0.0.1:8000/mcp`，需编辑器运行）或仓库内 `ue-*` skill（`.agents/skills/`、`~/.claude/skills/`）；无匹配 MCP 能力时才用 Unreal Editor Python 或人工流程并记录替代方案。实现游戏功能优先 C++ 而非蓝图。
 - **蓝图配置登记**：任何完成后需在蓝图中装配/派生/配置的 C++ 功能，必须同步登记到 `Docs/Technical/06_BlueprintConfigurationGuide.md`（含资产路径、配置步骤、参数来源、PIE 验收）；代码接口或资产变化也须在该文档同步维护。
-- **实现状态**：Home 垂直切片各阶段状态与稳定内容 ID（`Home_Dorothy_001`、`Home_Note_Mother`、`Home_Doll` 等）见 `Docs/Technical/05_HomeSliceImplementation.md`（阶段 8 因 Home 资产/地图未就绪暂停中）。
-- **提交**：仓库无 Git 历史与远程；如需要拉取 GitHub 内容，github.com 直连不可达，用 `https://ghfast.top/<原URL>` 镜像。
+- **实现状态**：Home 垂直切片各阶段状态与稳定内容 ID（`Home_Dorothy_001`、`Home_Note_Mother`、`Home_Doll` 等）见 `Docs/Technical/05_HomeSliceImplementation.md`（阶段 8 因 Home 资产/地图未就绪暂停中）。核心玩法批次（2026-08-14）：4.1 四状态 + 4.2 潜行（步态权限、噪声语义、敌人警戒 4.2.1 全量、房间传播、掩体、通用 NPC、警戒条数据层）**C++ 已实现**，`ST_Guard`/`ST_NPC`/`DA_LRGuardDefinition`/`DA_LRNPCDefinition`/`DA_LRNPCTuning`/`BP_Guard`/`BP_NPC`/`WBP_GuardAlertBar` 等资产待装配、`DA_LRGuardTuning.AttractAlertAmount` 待改为 1——详见 `Docs/Technical/06_BlueprintConfigurationGuide.md`「核心玩法机制」章节与 `.agents/ue-project-context.md`。
+- **提交**：仓库有 Git 历史（main 分支，无远程）；工作区可能混有负责人进行中的其他批次改动，提交前先确认范围。如需要拉取 GitHub 内容，github.com 直连不可达，用 `https://ghfast.top/<原URL>` 镜像。
diff --git a/Config/DefaultEngine.ini b/Config/DefaultEngine.ini
index d1e84b3..bb8ae32 100644
--- a/Config/DefaultEngine.ini
+++ b/Config/DefaultEngine.ini
@@ -139,3 +139,7 @@ ManualIPAddress=
 
 [CoreRedirects]
 +PropertyRedirects=(OldName="LRInputConfig.OpenJournalAction",NewName="OpenInventoryAction")
++PropertyRedirects=(OldName="LRMovementTuning.OutdoorSneakGuardNoiseRadius",NewName="OutdoorStealthRunNoiseRadius")
++PropertyRedirects=(OldName="LRMovementTuning.OutdoorAlertGuardNoiseRadius",NewName="OutdoorNoiseRadius")
++PropertyRedirects=(OldName="LRGuardTuning.HearingAlertAmount",NewName="AttractAlertAmount")
++PropertyRedirects=(OldName="LRGuardTuning.SightAlertLevel",NewName="SightChaseLevel")
diff --git a/Docs/Technical/06_BlueprintConfigurationGuide.md b/Docs/Technical/06_BlueprintConfigurationGuide.md
index 607bf17..763b458 100644
--- a/Docs/Technical/06_BlueprintConfigurationGuide.md
+++ b/Docs/Technical/06_BlueprintConfigurationGuide.md
@@ -119,20 +119,21 @@
 
 ### 守卫 AI 与 StateTree
 
-- **代码入口**：`ALRGuardCharacter`、`ALRGuardAIController`、警戒组件和 `ST_LRHomeGuard`。
-- **蓝图/资产**：守卫角色蓝图、守卫定义 DataAsset、StateTree 资产和关卡巡逻点。
+- **代码入口**：`ALRGuardCharacter`、`ALRGuardAIController`（生命周期/感知/行为拆三个 cpp）、`ULRAlertComponent`、`ALRRoomVolume`。
+- **蓝图/资产**：`BP_Guard`（守卫角色蓝图）、`DA_LRGuardDefinition`（定义 DataAsset，`Behavior` 为 **StateTree 硬引用**）、`ST_Guard`（StateTree 资产）、`WBP_GuardAlertBar`（世界警戒条）、`ALRRoomVolume`（室内奔跑房间体积）、关卡巡逻点。
 - **配置步骤**：
-  1. 创建 `ALRGuardCharacter` 派生蓝图，指定网格、动画和守卫定义。
-  2. 在实例上填写允许的巡逻点/路径组件；移动速度、视野、听觉和警戒阈值填写到定义或 Tuning 资产。
-  3. 在 AIController 默认值指定 StateTree；确认 StateTree 的状态名和 Gameplay Tag 与 C++ 规则一致。
-  4. 绑定警戒变化、调查、追逐和捕获事件到 UI、音效或动画表现。
-- **参数要求**：警戒范围、导航速度和感知参数必须来自定义/Tuning 资产；蓝图不能通过 Tick 改写警戒值或直接决定状态转换。
-- **验收**：PIE 中验证 IdlePatrol、Suspicious、Investigate、Search、Chase 的进入/运行/退出/超时，并检查 Visual Logger/Output Log。
+  1. 创建 `ALRGuardCharacter` 派生蓝图，指定网格、动画和守卫定义；`AlertWidget`（WidgetComponent）的 WidgetClass 指定为 `WBP_GuardAlertBar`，绘制位置与样式在蓝图配置。
+  2. 在实例上填写允许的巡逻点；移动速度、视野、听觉和警戒阈值填写到定义或 Tuning 资产。
+  3. **StateTree 接线**：`ST_Guard` 包含 `IdlePatrol / Suspicious / Investigate / Search / Chase / Stunned` 六个状态，条件节点 `FLRGuardStateCondition` 比较控制器的 `GetResolvedBehavior()`（StateTree 不自行推导警戒语义），任务节点 `FLRGuardBehaviorTask` 执行 `EnterBehavior`；树**仅由 `AI.Event.BehaviorChanged` 驱动**（`AI.Event.AlertChanged` 只表示数据变化，不驱动树）。`DA_LRGuardDefinition.Behavior` 硬引用 `ST_Guard`，控制器 `OnPossess` 自动 `SetStateTree` + `StartLogic`（无需在 AIController 默认值手动指定）。
+  4. 绑定警戒快照（`OnAlertSnapshotChanged` / `GetAlertSnapshot`）到 UI、音效或动画表现。
+- **参数要求**：警戒范围、导航速度、感知参数、吸引 CD（`AlertIncreaseCooldownSeconds`/`InvestigateIncreaseCooldownSeconds`）、房间警戒（`RoomRunAlertLevel`/`AdjacentRoomRunAlertAmount`）必须来自 Tuning/定义资产；蓝图不能通过 Tick 改写警戒值或直接决定状态转换。`AttractAlertAmount` 已重命名为语义值（资产中必须为 1），`SearchDurationSeconds` 已废弃（搜索由观察+自然衰减驱动）。
+- **验收**：PIE 中验证 IdlePatrol、Suspicious、Investigate、Search、Chase、Stunned 的进入/运行/退出，眩晕后按当前警戒恢复，`LR.Debug.Alert` 可诊断；检查 Visual Logger/Output Log。
 
 ### UI 屏幕与输入配置
 
 - **代码入口**：`ALRPlayerController`、`ULRPlayerUIComponent`、`ULRScreenWidget` 及各屏幕 Widget Controller。
 - **蓝图资产**：HUD、状态覆盖层、叙事/阅读、日志、库存、收藏品、暂停、存档和转场 Widget。
+- **暂停输入资产**：`/Game/LostRunic/Input/Actions/IA_LRPause` 的 `Trigger When Paused` 必须启用，确保世界暂停后 `Esc`/`Start` 仍能关闭暂停层；不要在 Widget 蓝图中自行调用 `Set Game Paused`。
 - **配置步骤**：
   1. 创建对应 `ULRScreenWidget` 的 Widget 蓝图，保持 BindWidget 名称与 C++ 声明一致。
   2. 只在 Widget 中配置布局、字体、动画、材质和图标；展示数据由 Controller/委托推送。
@@ -141,6 +142,84 @@
 - **参数要求**：输入上下文的优先级、焦点、鼠标光标和锁键行为由 PlayerController 管理；打字速度等表现参数来自 `ULRUITuning`。
 - **验收**：验证打开/关闭、焦点切换、对话二段确认、菜单阻断 Gameplay 输入和转场期间的输入锁定。
 
+## V2 存档 UI 基础资产（2026-08-14）
+
+本次仅创建空的 Widget Blueprint 基础资产，暂不在资产内装配控件、事件图或存档规则。四个资产均继承 `ULRScreenWidget`，由项目负责人在 Unreal Editor 中完成 Designer 布局、绑定和导航配置。
+
+| 资产 | 路径 | 父类 | 组装边界 |
+| --- | --- | --- | --- |
+| `WBP_MainMenu` | `/Game/LostRunic/UI/Save/WBP_MainMenu` | `ULRScreenWidget` | Continue / New Game / Load Game 入口与主菜单导航 |
+| `WBP_SaveSelection` | `/Game/LostRunic/UI/Save/WBP_SaveSelection` | `ULRScreenWidget` | Save/Load 共用槽位列表、模式切换、空槽和损坏状态显示 |
+| `WBP_SaveSlot` | `/Game/LostRunic/UI/Save/WBP_SaveSlot` | `ULRScreenWidget` | 单个槽位的显示编号、地图、时间、Health 和操作按钮 |
+| `WBP_SaveConfirmDialog` | `/Game/LostRunic/UI/Save/WBP_SaveConfirmDialog` | `ULRScreenWidget` | 覆盖/删除确认、取消与不可用原因展示 |
+
+### 负责人组装要求
+
+1. 保持上述资产父类不变；展示数据从 Save UI Controller/委托推送，Widget 不直接读写 Catalog 或 Payload。
+2. `WBP_SaveSelection` 必须同时支持 `ELRSaveSelectionMode::Save` 和 `Load`，并为保存、读取、删除、Continue 提供明确的焦点路径。
+3. `WBP_SaveSlot` 的稳定身份使用 `FLRSaveSlotId`，显示编号只用于展示；不要用数组下标或 Widget 名称作为存档 ID。
+4. `WBP_SaveConfirmDialog` 只负责确认表现和回调，不在蓝图中直接调用磁盘 API；操作统一转发到 `ULRSaveSubsystem` 的 V2 API。
+5. 完成 Designer/绑定后，在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 验收主菜单 Continue、New Game、Load Game，以及 Save/Load 共用选择页。
+
+### V2 存档 UI 控制器与新游戏 API
+
+- **控制器所有者**：`ALRHUD` 创建唯一的 `ULRSaveWidgetController`，并将其注入 `SaveSlots` 页面。HUD 执行 `EndPlay` 时解除控制器绑定；关闭 `SaveSlots` 页面时调用 `Close()`。
+- **只读视图模型**：槽位列表绑定 `GetSnapshot()` 和 `OnSnapshotChanged`。蓝图使用 `FLRSaveUISnapshot.Slots`，以及 `FLRSaveSlotView` 中的 `SlotId`、`DisplayIndex`、`MapDisplayName`、`Health`、`bCanLoad`、`bCanOverwrite` 和 `bCanDelete`。
+- **保存模式**：调用 `Open(ELRSaveSelectionMode::Save)`；创建、主操作、删除、确认和取消分别调用 `RequestCreateManualSave()`、`RequestPrimarySlotAction(SlotId)`、`RequestDelete(SlotId)`、`ConfirmPendingAction()` 和 `CancelPendingAction()`。
+- **读取模式**：主菜单调用 `Open(ELRSaveSelectionMode::Load)`，选择健康槽位后调用 `RequestPrimarySlotAction(SlotId)`。Widget Graph 不得直接访问 Catalog 或 Payload API。
+- **状态处理**：页面需要表现 `Idle`、`Confirming`、`Saving`、`Loading`、`Deleting` 和 `Error`。`bIsBusy=true` 时禁用重复操作；显示 `StatusMessage`，并为 `Error` 状态提供关闭错误提示的操作。
+- **主菜单新游戏**：调用 `ULRSaveSubsystem.RequestNewGame()`。该流程异步执行；收到 `OnSaveOperationCompleted`，且 `Operation=NewGame`、`Code=Succeeded` 后，才能表现为已进入可玩世界。Widget 不得自行调用 `OpenLevel`。
+- **新游戏数据配置**：填写 `ULRGameContentSet.NewGameMapId`；对应地图注册项的 `FLRMapRegistration.DefaultStartAnchorId` 必须能在目标地图中解析。新游戏先重置 Provider 状态，再替换自动槽；所有手动槽保持不变，启动失败时保留旧自动槽供 Continue 使用。
+
+#### 存档 UI 与新游戏 Designer 检查表
+
+1. 四个新资产统一放在 `/Game/LostRunic/UI/Save/`，并保持父类为 `ULRScreenWidget`。
+2. 在 `WBP_SaveSelection` 中，将槽位列表绑定到快照变更事件，并把按钮操作转发到上述控制器函数。
+3. 在 `WBP_SaveSlot` 中显示以稳定 `SlotId` 为身份的视图；`DisplayIndex` 仅用于显示文本，不参与槽位寻址。
+4. 在 `WBP_SaveConfirmDialog` 中只调用确认或取消；不得直接写入 SaveGame 槽位。
+5. 在 `WBP_MainMenu` 中，通过 SaveSubsystem API 和操作完成事件处理 Continue、Load 和 New Game。
+6. 只在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 执行验收：暂停后打开 SaveSlots、未暂停时拒绝手动保存、确认覆盖与删除、读取健康槽位，并验证 New Game 保留全部手动槽，且首次新自动存档成功前旧自动槽仍然有效。
+
+## 核心玩法机制：四状态 + 潜行 + 敌人警戒 + NPC（2026-08-14）
+
+本批次实现 4.1 四状态（睁眼/闭眼）与 4.2 潜行玩法（主角侧噪声、敌人警戒全量、掩体、通用 NPC），并为四状态美术风格差异预留接入点。**C++ 规则已完整实现**，以下为需要在蓝图中装配/配置的表面。
+
+### 步态与噪声环境（主角侧）
+
+- **步态权限**：`ULRLocomotionComponent` 提供 `RequestToggleSneak` / `RequestStartRun` / `RequestStopRun`（受状态规则验证，禁止时广播 `OnPaceRequestRejected` + 日志 `Movement.Reject.PaceForbidden`）；`ApplyPace`/`OverridePace`/`ClearPaceOverride` 为组件内部应用通道（掩体强制潜行使用 `Movement.Override.Hidden`）。进入状态自动应用默认步态（Perception 潜行 / Courage 走路 / Memory 走路）。
+- **掩体**：`ALRHidePoint` 实例配置 `bAllowMovementWhileHidden`：桌下/草丛/管道（可移动）= true，柜/箱（固定）= false；进入掩体强制潜行，退出按当前状态重新求值。掩体容量与进出音效不在本批次。
+- **噪声环境体积**：`ALRNoiseArea` 实例配置 `Environment`：`Indoor` / `Outdoor` / `OutdoorStealth`；重叠按 `Indoor > OutdoorStealth > Outdoor` 解析，无区域时默认 `Outdoor`。进入/退出都会重新求值。
+- **脚步噪声**（纯规则，见 `LRMovementRules::ResolveFootstepNoise`）：潜行无声；走路 室内 400 / 室外潜行 250 / 室外非潜行 250+Faint；奔跑 室内房间传播（无房间回退 1200）/ 室外潜行 600 / 室外非潜行 250。调优字段已重命名：`OutdoorSneakGuardNoiseRadius`→`OutdoorStealthRunNoiseRadius`、`OutdoorAlertGuardNoiseRadius`→`OutdoorNoiseRadius`（PropertyRedirects 已迁移）。
+
+### 敌人警戒（4.2.1 全量）
+
+- **行为语义**（C++ 权威）：吸引噪声 +1（1-5 档 CD 0.5s；首次进入 6-10 档 0.5s、其后 0.2s；CD 内刺激完全忽略）；看见玩家 警戒<6→6、6-10→11、11 丢失→10；观察 3s（0→1 与抵达调查点）与衰减 0.5s/-1 由 `ULRAlertComponent` 计时器驱动；`ResolveTargetBehavior` 为行为唯一权威（眩晕优先）。警戒条 UI 只读 `FLRAlertSnapshot`（Level/Fraction/Tier/Behavior/bFullAlert）+ `OnAlertSnapshotChanged`，绑定后立即推送初值。
+- **`WBP_GuardAlertBar`**：继承 `ULRWorldAlertBarWidgetBase`；覆盖 `HandleAlertSnapshotChanged` 只做表现：`Tier=Hidden` 隐藏、`White` 白色进度条、`Red` 红色、`Full` 满值+额外红色特效（样式需重新设计）。由 `BP_Guard` 的 `AlertWidget` 组件初始化，Widget 不自行猜测所属守卫。
+- **`ALRRoomVolume` 摆放**：在关卡中摆放 Box 体积（Trigger profile），填写 `RoomId`（稳定 FName），`AdjacentRooms` 连线到相邻房间体积（门/窗拓扑）；守卫进入体积自动注册。室内奔跑：当前房间守卫警戒至少提升到 `RoomRunAlertLevel`(5)、相邻房间 +1；同一守卫属多房间时取最大效果、不累加；无房间体积时回退 1200 半径听觉事件。房间体积必须早于守卫生成。
+- **`ST_Guard` 资产（编辑器人工创建 + MCP 检查）**：见「守卫 AI 与 StateTree」小节接线要求。
+
+### 通用 NPC
+
+- **`BP_NPC`**：继承 `ALRNPCCharacter`，配置网格、动画与 `DA_LRNPCDefinition`。
+- **`DA_LRNPCDefinition`**：`NpcId`（稳定 FName）、`Behavior`（StateTree **硬引用** `ST_NPC`）、`DialogueRowId`（对话 DataTable 稳定行 ID）、`DefaultBehavior`（Idle/Patrol）。
+- **`ST_NPC` 资产（编辑器人工创建 + MCP 检查）**：四个状态 `Idle / Patrol / ReactToNoise / Conversation`；条件 `FLRNPCStateCondition` 比较控制器 `GetActiveBehavior()`，任务 `FLRNPCBehaviorTask`；Idle 附加 `FLRNPCLookAtPlayerTask`（低频朝向检测），ReactToNoise 附加 `FLRNPCReactToNoiseTask`（限时反应，到时发 `AI.Event.NPCReactionEnded` 回默认行为）。树由 `AI.Event.NPCNoiseHeard` / `NPCDialogueStarted` / `NPCDialogueEnded` / `NPCReactionEnded` 驱动。
+- **对话**：交互选项 `Interaction.Action.Talk`（Normal 状态）经 `ULRDialogueSubsystem::StartDialogue` 启动；Conversation 高优先级，普通噪声不打断（只触发 `OnNoiseHeard` 表现钩子）。巡逻点按实例配置。
+- **调优**：`DA_LRNPCTuning`（登记进 `DA_LRGameTuningSet.NPC`）：`LookAtPlayerRadiusCm`、`LookAtIntervalSeconds`、`NoiseReactionDurationSeconds`、`PatrolSpeedCm`。
+- **预留**：`OnNoiseHeard`（BlueprintImplementableEvent）与 `OnNPCAttentionChanged` 委托为未来告警/逃离扩展钩子，本批次不实现告警逻辑。
+
+### 四状态美术表现预留（不实现视觉效果）
+
+- **表现事件（已具备）**：`ULRStateComponent` 的 `OnStateChanging(PreviousMode, NextMode, Reason)` / `OnStateChanged`；`ULRStatePresentationComponent` 转发 `OnStatePresentationRequested` + `PresentStateChange`（表现锁由 `CompleteStatePresentation` 释放）。未来接入 Normal/Perception/Courage/Memory 四套视觉/后处理/音频只需订阅既有事件。
+- **表现调优接入点（新增 getter）**：`GetPerceptionRevealRadius()`(4.5m)、`GetNoiseRevealRadius()`(2m)、`GetNoiseRevealDurationSeconds()`(5s)、`GetPerceptionBlendWeight()`、`GetCourageBlendWeight()`——值来自 `ULRPresentationTuning`（`DA_LRGameTuningSet.Presentation`）。
+- **其他钩子**：`ULRNoiseEmitterComponent::OnNoiseEmitted`（声源显现，房间传播路径也已广播）；`ELRScreenType::StateOverlay`（屏幕层）；`LRHUDWidgetController::OnPerceptionModeChanged`（已有）。
+
+### 输入与调优变更
+
+- `SneakAction` 已废弃（`DeprecatedProperty`，移出 Validate 必填）；潜行切换继续使用 `ToggleCrouchAction`（C / B 切换）。
+- 调优重命名（PropertyRedirects 已配置）：`HearingAlertAmount`→`AttractAlertAmount`（**资产值需改为 1**）、`SightAlertLevel`→`SightChaseLevel`(11)、移动噪声半径两项；`SearchDurationSeconds` 废弃。
+- 新建资产清单（StateTree 需编辑器人工创建，其余可 MCP）：`ST_Guard`、`ST_NPC`、`DA_LRGuardDefinition`、`DA_LRNPCDefinition`、`DA_LRNPCTuning`、`BP_Guard`、`WBP_GuardAlertBar`、`BP_NPC`；`DA_LRGameTuningSet` 登记 `DA_LRNPCTuning`。
+- **PIE 验收（`/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`，键鼠+手柄，Output Log 无项目级 Warning/Error）**：状态步态（Perception 强制潜行、Courage 拒潜行、Memory 仅走路）；掩体进入强制潜行/固定掩体不可移动/掩体内不可见；噪声区域进入退出与重叠优先级、7 行步态×环境噪声、房间传播（本房→5、邻房+1、多房间取最大、无房间兜底）；完整警戒流程（0→吸引→1 观察 3s、CD 内忽略、看见→6 前往、抵达 Search 观察→衰减→0 巡逻、6-10 看见→11 追逐、丢失→10、追上死亡→Memory）；世界警戒条四档表现与首帧同步；击退眩晕 0.6s 恢复（`LR.Debug.Alert` 显示 Stunned）；NPC 巡逻/站立/对话开合/噪声限时反应/Idle 朝向玩家/对话结束回默认；`LR.Debug.Tuning` 确认重命名后来源。
+
 ## 更新记录
 
 | 日期 | 变更 |
diff --git a/Docs/Technical/08_CoreMechanicsBatch_Diff.md b/Docs/Technical/08_CoreMechanicsBatch_Diff.md
new file mode 100644
index 0000000..27169a5
--- /dev/null
+++ b/Docs/Technical/08_CoreMechanicsBatch_Diff.md
@@ -0,0 +1,5438 @@
+# 核心玩法批次 Git Diff（2026-08-14）
+
+提交：d0b78fe（待 amend 后更新）
+
+提交范围：4.1 四状态 + 4.2 潜行核心机制（状态→步态权限、噪声语义、敌人警戒 4.2.1、房间传播、掩体、通用 NPC、警戒条数据层、表现预留）。
+
+```diff
+diff --git a/CLAUDE.md b/CLAUDE.md
+index 6e799e4..aa0fb48 100644
+--- a/CLAUDE.md
++++ b/CLAUDE.md
+@@ -21,6 +21,7 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
+ ```
+ 
+ - 修改 C++ 后必须重新编译编辑器（或使用 Live Coding）再进 PIE；不要在未编译状态下创建/修改依赖反射的蓝图资产。
++- 仓库提供构建/测试包装脚本（避免 Git Bash→cmd 的引号问题）：`Scripts/BuildLostRunicEditor.bat`、`Scripts/RunLostRunicTests.bat`（引擎路径硬编码在脚本内，移动引擎时需同步修改）。
+ - 自动化测试位于 `Source/LostRunic/Tests/LR*Tests.cpp`（`WITH_DEV_AUTOMATION_TESTS` 下编译），无独立测试套件；可经 Session Frontend 运行，也可命令行跑：
+   `UnrealEditor.exe "$PWD\LostRunic.uproject" -unattended -nopause -ExecCmds="Automation RunTests <Test名或前缀>; Quit"`
+ - 冒烟测试：在两个变体地图分别 PIE，检查 Output Log 警告/错误，AI/UI 改动同时验证键鼠与手柄。
+@@ -36,11 +37,12 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
+ | `Core/` | 日志、Gameplay Tags、通用类型、验证、调试命令 |
+ | `Data/` | Tuning DataAsset、内容定义（物品/收藏品/守卫/关卡事件）、`ULRProjectSettings`、聚合资产 `ULRGameTuningSet` |
+ | `Framework/` | `ALRGameMode`、`ALRPlayerController`、`ALRCharacter`、`ALRGameState`、`ULRGameInstanceSubsystem` |
+-| `State/` | 四状态组件与规则（Normal/Perception/Courage/Memory）、状态表现组件 |
+-| `AI/` | 守卫、警戒组件、感知规则、StateTree 节点（`LRGuardStateTreeNodes`） |
++| `State/` | 四状态组件与规则（Normal/Perception/Courage/Memory）、状态表现组件（含艺术表现预留 getter） |
++| `AI/` | 守卫（控制器按生命周期/感知/行为拆 3 cpp）、警戒组件（4.2.1 全量语义）、通用 NPC（`LRNPC*`，StateTree 驱动）、感知规则、StateTree 节点（`LRGuardStateTreeNodes`/`LRNPCStateTreeNodes`） |
+ | `Interaction/` | `ILRInteractable` 接口、交互筛选组件、门/拾取/世界交互 Actor |
+ | `Items/` | 背包、物品使用统一解析（`ULRItemUseResolver`，快捷栏与交互选择器双入口共用） |
+-| `Stealth/` | 掩体（`ULRHideComponent`）、噪声发射（`ULRNoiseEmitterComponent`）、守卫可见性 |
++| `Stealth/` | 掩体（`ULRHideComponent`，进掩体强制潜行覆盖）、噪声发射（`ULRNoiseEmitterComponent`）、守卫可见性 |
++| `Gameplay/` | 移动（`ULRLocomotionComponent`：Request*/ApplyPace/OverridePace 权限拆分）、步态×噪声纯规则（`LRMovementRules`）、噪声区域（`ALRNoiseArea`）、室内奔跑房间体积（`ALRRoomVolume`） |
+ | `Narrative/` | `ULRDialogueSubsystem`（DataTable 遍历、条件、分支、一次性剧情事件） |
+ | `Save/` | SaveGame 分块结构、保存队列、`FLRResumeAnchor` 恢复锚点 |
+ | `UI/` | HUD、Widget Controller（对话/菜单/过渡）、`ULRPlayerUIComponent` |
+@@ -51,7 +53,10 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
+ 
+ - **职责划分**：Actor 负责生命周期与组件组合，Component 负责独立能力，Subsystem 负责跨 Actor 长期状态。`ALRPlayerController` 是 Enhanced Input 上下文、输入模式、光标状态的唯一所有者；`ULRGameInstanceSubsystem` 持有已验证的内容与调优根；Widget 只接收不可变表现数据，不得反向决定规则。
+ - **模板遗留**：模块根部的 `LostRunicCharacter`、`LostRunicGameMode`、`LostRunicPlayerController`（及 `Content/TopDown/`）是 UE 模板残留。新功能优先复用 `LR*` 类，避免形成第二套框架。
+-- **调优与数据**：影响手感/平衡的值必须落在 `Content/LostRunic/Data/Tuning/` 的领域 DataAsset（`ULRStateTuning`、`ULRInteractionTuning`、`ULRMovementTuning`、`ULRGuardTuning`、`ULRSaveTuning`、`ULRUITuning` 等），由 `ULRGameTuningSet` 聚合，`DefaultGame.ini` 指定默认集。C++ 默认值只是安全回退；稳定 ID 用 `FName`/GUID；非关键资源用软引用异步加载；禁止散落硬编码 `/Game/...` 路径。
++- **调优与数据**：影响手感/平衡的值必须落在 `Content/LostRunic/Data/Tuning/` 的领域 DataAsset（`ULRStateTuning`、`ULRInteractionTuning`、`ULRMovementTuning`、`ULRGuardTuning`、`ULRSaveTuning`、`ULRUITuning`、`ULRNPCTuning` 等），由 `ULRGameTuningSet` 聚合，`DefaultGame.ini` 指定默认集。C++ 默认值只是安全回退；稳定 ID 用 `FName`/GUID；非关键资源用软引用异步加载；禁止散落硬编码 `/Game/...` 路径。**字段重命名必须同步 `Config/DefaultEngine.ini` 的 `+PropertyRedirects`（先例：`HearingAlertAmount`→`AttractAlertAmount` 等）**。
++- **输入**：Enhanced Input 语义动作，代码绑定动作不绑按键；长按阈值/死区/曲线放入输入与调优资产；上下文切换时 `bIgnoreAllPressedKeysUntilRelease`（`IMC_LRGameplay` / `IMC_LRDialogue` / `IMC_LRMenu` / `IMC_LRTransition`）。
++- **AI**：StateTree + 统一声源/视线事件，警戒 0–11 必须记录原因 Gameplay Tag（如 `Noise.Footstep`、`Sight.Player`）；禁止随机 Tick 分支替代状态图。守卫行为由 `ResolveTargetBehavior`（`LRAlertRules` 纯规则，眩晕优先）唯一权威解析，StateTree 只执行结果：树**仅由 `AI.Event.BehaviorChanged` 驱动**（`AI.Event.AlertChanged` 只表示数据变化）；`ST_Guard`/`ST_NPC` 资产需在编辑器人工创建（MCP 只能检查）并挂在 `DA_LRGuardDefinition`/`DA_LRNPCDefinition.Behavior`（**硬引用**）。
++- **潜行噪声语义（4.2）**：潜行完全无声；`ALRNoiseArea` 三环境（Indoor/Outdoor/OutdoorStealth，重叠按 Indoor>OutdoorStealth>Outdoor 解析，**无区域默认 Outdoor**）；室内奔跑走 `ALRRoomVolume` 房间传播（当前房警戒至少 5、相邻房 +1，多房间取最大，无房间回退 1200 半径听觉事件，传播路径绝不发 `ReportNoiseEvent` 防双计）；`Walk.Faint` 仅警戒 ≥6 守卫响应。
+ - **输入**：Enhanced Input 语义动作，代码绑定动作不绑按键；长按阈值/死区/曲线放入输入与调优资产；上下文切换时 `bIgnoreAllPressedKeysUntilRelease`（`IMC_LRGameplay` / `IMC_LRDialogue` / `IMC_LRMenu` / `IMC_LRTransition`）。
+ - **AI**：StateTree + 统一声源/视线事件，警戒 0–11 必须记录原因 Gameplay Tag（如 `Noise.Footstep`、`Sight.Player`）；禁止随机 Tick 分支替代状态图。
+ - **默认禁止 Tick**：优先委托、计时器、StateTree、Gameplay Tags、事件队列。
+@@ -62,5 +67,5 @@ LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的
+ 
+ - **蓝图工具**：编辑器中操作蓝图优先用 unreal-mcp（`http://127.0.0.1:8000/mcp`，需编辑器运行）或仓库内 `ue-*` skill（`.agents/skills/`、`~/.claude/skills/`）；无匹配 MCP 能力时才用 Unreal Editor Python 或人工流程并记录替代方案。实现游戏功能优先 C++ 而非蓝图。
+ - **蓝图配置登记**：任何完成后需在蓝图中装配/派生/配置的 C++ 功能，必须同步登记到 `Docs/Technical/06_BlueprintConfigurationGuide.md`（含资产路径、配置步骤、参数来源、PIE 验收）；代码接口或资产变化也须在该文档同步维护。
+-- **实现状态**：Home 垂直切片各阶段状态与稳定内容 ID（`Home_Dorothy_001`、`Home_Note_Mother`、`Home_Doll` 等）见 `Docs/Technical/05_HomeSliceImplementation.md`（阶段 8 因 Home 资产/地图未就绪暂停中）。
+-- **提交**：仓库无 Git 历史与远程；如需要拉取 GitHub 内容，github.com 直连不可达，用 `https://ghfast.top/<原URL>` 镜像。
++- **实现状态**：Home 垂直切片各阶段状态与稳定内容 ID（`Home_Dorothy_001`、`Home_Note_Mother`、`Home_Doll` 等）见 `Docs/Technical/05_HomeSliceImplementation.md`（阶段 8 因 Home 资产/地图未就绪暂停中）。核心玩法批次（2026-08-14）：4.1 四状态 + 4.2 潜行（步态权限、噪声语义、敌人警戒 4.2.1 全量、房间传播、掩体、通用 NPC、警戒条数据层）**C++ 已实现**，`ST_Guard`/`ST_NPC`/`DA_LRGuardDefinition`/`DA_LRNPCDefinition`/`DA_LRNPCTuning`/`BP_Guard`/`BP_NPC`/`WBP_GuardAlertBar` 等资产待装配、`DA_LRGuardTuning.AttractAlertAmount` 待改为 1——详见 `Docs/Technical/06_BlueprintConfigurationGuide.md`「核心玩法机制」章节与 `.agents/ue-project-context.md`。
++- **提交**：仓库有 Git 历史（main 分支，无远程）；工作区可能混有负责人进行中的其他批次改动，提交前先确认范围。如需要拉取 GitHub 内容，github.com 直连不可达，用 `https://ghfast.top/<原URL>` 镜像。
+diff --git a/Config/DefaultEngine.ini b/Config/DefaultEngine.ini
+index d1e84b3..bb8ae32 100644
+--- a/Config/DefaultEngine.ini
++++ b/Config/DefaultEngine.ini
+@@ -139,3 +139,7 @@ ManualIPAddress=
+ 
+ [CoreRedirects]
+ +PropertyRedirects=(OldName="LRInputConfig.OpenJournalAction",NewName="OpenInventoryAction")
+++PropertyRedirects=(OldName="LRMovementTuning.OutdoorSneakGuardNoiseRadius",NewName="OutdoorStealthRunNoiseRadius")
+++PropertyRedirects=(OldName="LRMovementTuning.OutdoorAlertGuardNoiseRadius",NewName="OutdoorNoiseRadius")
+++PropertyRedirects=(OldName="LRGuardTuning.HearingAlertAmount",NewName="AttractAlertAmount")
+++PropertyRedirects=(OldName="LRGuardTuning.SightAlertLevel",NewName="SightChaseLevel")
+diff --git a/Docs/Technical/06_BlueprintConfigurationGuide.md b/Docs/Technical/06_BlueprintConfigurationGuide.md
+index 607bf17..763b458 100644
+--- a/Docs/Technical/06_BlueprintConfigurationGuide.md
++++ b/Docs/Technical/06_BlueprintConfigurationGuide.md
+@@ -119,20 +119,21 @@
+ 
+ ### 守卫 AI 与 StateTree
+ 
+-- **代码入口**：`ALRGuardCharacter`、`ALRGuardAIController`、警戒组件和 `ST_LRHomeGuard`。
+-- **蓝图/资产**：守卫角色蓝图、守卫定义 DataAsset、StateTree 资产和关卡巡逻点。
++- **代码入口**：`ALRGuardCharacter`、`ALRGuardAIController`（生命周期/感知/行为拆三个 cpp）、`ULRAlertComponent`、`ALRRoomVolume`。
++- **蓝图/资产**：`BP_Guard`（守卫角色蓝图）、`DA_LRGuardDefinition`（定义 DataAsset，`Behavior` 为 **StateTree 硬引用**）、`ST_Guard`（StateTree 资产）、`WBP_GuardAlertBar`（世界警戒条）、`ALRRoomVolume`（室内奔跑房间体积）、关卡巡逻点。
+ - **配置步骤**：
+-  1. 创建 `ALRGuardCharacter` 派生蓝图，指定网格、动画和守卫定义。
+-  2. 在实例上填写允许的巡逻点/路径组件；移动速度、视野、听觉和警戒阈值填写到定义或 Tuning 资产。
+-  3. 在 AIController 默认值指定 StateTree；确认 StateTree 的状态名和 Gameplay Tag 与 C++ 规则一致。
+-  4. 绑定警戒变化、调查、追逐和捕获事件到 UI、音效或动画表现。
+-- **参数要求**：警戒范围、导航速度和感知参数必须来自定义/Tuning 资产；蓝图不能通过 Tick 改写警戒值或直接决定状态转换。
+-- **验收**：PIE 中验证 IdlePatrol、Suspicious、Investigate、Search、Chase 的进入/运行/退出/超时，并检查 Visual Logger/Output Log。
++  1. 创建 `ALRGuardCharacter` 派生蓝图，指定网格、动画和守卫定义；`AlertWidget`（WidgetComponent）的 WidgetClass 指定为 `WBP_GuardAlertBar`，绘制位置与样式在蓝图配置。
++  2. 在实例上填写允许的巡逻点；移动速度、视野、听觉和警戒阈值填写到定义或 Tuning 资产。
++  3. **StateTree 接线**：`ST_Guard` 包含 `IdlePatrol / Suspicious / Investigate / Search / Chase / Stunned` 六个状态，条件节点 `FLRGuardStateCondition` 比较控制器的 `GetResolvedBehavior()`（StateTree 不自行推导警戒语义），任务节点 `FLRGuardBehaviorTask` 执行 `EnterBehavior`；树**仅由 `AI.Event.BehaviorChanged` 驱动**（`AI.Event.AlertChanged` 只表示数据变化，不驱动树）。`DA_LRGuardDefinition.Behavior` 硬引用 `ST_Guard`，控制器 `OnPossess` 自动 `SetStateTree` + `StartLogic`（无需在 AIController 默认值手动指定）。
++  4. 绑定警戒快照（`OnAlertSnapshotChanged` / `GetAlertSnapshot`）到 UI、音效或动画表现。
++- **参数要求**：警戒范围、导航速度、感知参数、吸引 CD（`AlertIncreaseCooldownSeconds`/`InvestigateIncreaseCooldownSeconds`）、房间警戒（`RoomRunAlertLevel`/`AdjacentRoomRunAlertAmount`）必须来自 Tuning/定义资产；蓝图不能通过 Tick 改写警戒值或直接决定状态转换。`AttractAlertAmount` 已重命名为语义值（资产中必须为 1），`SearchDurationSeconds` 已废弃（搜索由观察+自然衰减驱动）。
++- **验收**：PIE 中验证 IdlePatrol、Suspicious、Investigate、Search、Chase、Stunned 的进入/运行/退出，眩晕后按当前警戒恢复，`LR.Debug.Alert` 可诊断；检查 Visual Logger/Output Log。
+ 
+ ### UI 屏幕与输入配置
+ 
+ - **代码入口**：`ALRPlayerController`、`ULRPlayerUIComponent`、`ULRScreenWidget` 及各屏幕 Widget Controller。
+ - **蓝图资产**：HUD、状态覆盖层、叙事/阅读、日志、库存、收藏品、暂停、存档和转场 Widget。
++- **暂停输入资产**：`/Game/LostRunic/Input/Actions/IA_LRPause` 的 `Trigger When Paused` 必须启用，确保世界暂停后 `Esc`/`Start` 仍能关闭暂停层；不要在 Widget 蓝图中自行调用 `Set Game Paused`。
+ - **配置步骤**：
+   1. 创建对应 `ULRScreenWidget` 的 Widget 蓝图，保持 BindWidget 名称与 C++ 声明一致。
+   2. 只在 Widget 中配置布局、字体、动画、材质和图标；展示数据由 Controller/委托推送。
+@@ -141,6 +142,84 @@
+ - **参数要求**：输入上下文的优先级、焦点、鼠标光标和锁键行为由 PlayerController 管理；打字速度等表现参数来自 `ULRUITuning`。
+ - **验收**：验证打开/关闭、焦点切换、对话二段确认、菜单阻断 Gameplay 输入和转场期间的输入锁定。
+ 
++## V2 存档 UI 基础资产（2026-08-14）
++
++本次仅创建空的 Widget Blueprint 基础资产，暂不在资产内装配控件、事件图或存档规则。四个资产均继承 `ULRScreenWidget`，由项目负责人在 Unreal Editor 中完成 Designer 布局、绑定和导航配置。
++
++| 资产 | 路径 | 父类 | 组装边界 |
++| --- | --- | --- | --- |
++| `WBP_MainMenu` | `/Game/LostRunic/UI/Save/WBP_MainMenu` | `ULRScreenWidget` | Continue / New Game / Load Game 入口与主菜单导航 |
++| `WBP_SaveSelection` | `/Game/LostRunic/UI/Save/WBP_SaveSelection` | `ULRScreenWidget` | Save/Load 共用槽位列表、模式切换、空槽和损坏状态显示 |
++| `WBP_SaveSlot` | `/Game/LostRunic/UI/Save/WBP_SaveSlot` | `ULRScreenWidget` | 单个槽位的显示编号、地图、时间、Health 和操作按钮 |
++| `WBP_SaveConfirmDialog` | `/Game/LostRunic/UI/Save/WBP_SaveConfirmDialog` | `ULRScreenWidget` | 覆盖/删除确认、取消与不可用原因展示 |
++
++### 负责人组装要求
++
++1. 保持上述资产父类不变；展示数据从 Save UI Controller/委托推送，Widget 不直接读写 Catalog 或 Payload。
++2. `WBP_SaveSelection` 必须同时支持 `ELRSaveSelectionMode::Save` 和 `Load`，并为保存、读取、删除、Continue 提供明确的焦点路径。
++3. `WBP_SaveSlot` 的稳定身份使用 `FLRSaveSlotId`，显示编号只用于展示；不要用数组下标或 Widget 名称作为存档 ID。
++4. `WBP_SaveConfirmDialog` 只负责确认表现和回调，不在蓝图中直接调用磁盘 API；操作统一转发到 `ULRSaveSubsystem` 的 V2 API。
++5. 完成 Designer/绑定后，在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 验收主菜单 Continue、New Game、Load Game，以及 Save/Load 共用选择页。
++
++### V2 存档 UI 控制器与新游戏 API
++
++- **控制器所有者**：`ALRHUD` 创建唯一的 `ULRSaveWidgetController`，并将其注入 `SaveSlots` 页面。HUD 执行 `EndPlay` 时解除控制器绑定；关闭 `SaveSlots` 页面时调用 `Close()`。
++- **只读视图模型**：槽位列表绑定 `GetSnapshot()` 和 `OnSnapshotChanged`。蓝图使用 `FLRSaveUISnapshot.Slots`，以及 `FLRSaveSlotView` 中的 `SlotId`、`DisplayIndex`、`MapDisplayName`、`Health`、`bCanLoad`、`bCanOverwrite` 和 `bCanDelete`。
++- **保存模式**：调用 `Open(ELRSaveSelectionMode::Save)`；创建、主操作、删除、确认和取消分别调用 `RequestCreateManualSave()`、`RequestPrimarySlotAction(SlotId)`、`RequestDelete(SlotId)`、`ConfirmPendingAction()` 和 `CancelPendingAction()`。
++- **读取模式**：主菜单调用 `Open(ELRSaveSelectionMode::Load)`，选择健康槽位后调用 `RequestPrimarySlotAction(SlotId)`。Widget Graph 不得直接访问 Catalog 或 Payload API。
++- **状态处理**：页面需要表现 `Idle`、`Confirming`、`Saving`、`Loading`、`Deleting` 和 `Error`。`bIsBusy=true` 时禁用重复操作；显示 `StatusMessage`，并为 `Error` 状态提供关闭错误提示的操作。
++- **主菜单新游戏**：调用 `ULRSaveSubsystem.RequestNewGame()`。该流程异步执行；收到 `OnSaveOperationCompleted`，且 `Operation=NewGame`、`Code=Succeeded` 后，才能表现为已进入可玩世界。Widget 不得自行调用 `OpenLevel`。
++- **新游戏数据配置**：填写 `ULRGameContentSet.NewGameMapId`；对应地图注册项的 `FLRMapRegistration.DefaultStartAnchorId` 必须能在目标地图中解析。新游戏先重置 Provider 状态，再替换自动槽；所有手动槽保持不变，启动失败时保留旧自动槽供 Continue 使用。
++
++#### 存档 UI 与新游戏 Designer 检查表
++
++1. 四个新资产统一放在 `/Game/LostRunic/UI/Save/`，并保持父类为 `ULRScreenWidget`。
++2. 在 `WBP_SaveSelection` 中，将槽位列表绑定到快照变更事件，并把按钮操作转发到上述控制器函数。
++3. 在 `WBP_SaveSlot` 中显示以稳定 `SlotId` 为身份的视图；`DisplayIndex` 仅用于显示文本，不参与槽位寻址。
++4. 在 `WBP_SaveConfirmDialog` 中只调用确认或取消；不得直接写入 SaveGame 槽位。
++5. 在 `WBP_MainMenu` 中，通过 SaveSubsystem API 和操作完成事件处理 Continue、Load 和 New Game。
++6. 只在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 执行验收：暂停后打开 SaveSlots、未暂停时拒绝手动保存、确认覆盖与删除、读取健康槽位，并验证 New Game 保留全部手动槽，且首次新自动存档成功前旧自动槽仍然有效。
++
++## 核心玩法机制：四状态 + 潜行 + 敌人警戒 + NPC（2026-08-14）
++
++本批次实现 4.1 四状态（睁眼/闭眼）与 4.2 潜行玩法（主角侧噪声、敌人警戒全量、掩体、通用 NPC），并为四状态美术风格差异预留接入点。**C++ 规则已完整实现**，以下为需要在蓝图中装配/配置的表面。
++
++### 步态与噪声环境（主角侧）
++
++- **步态权限**：`ULRLocomotionComponent` 提供 `RequestToggleSneak` / `RequestStartRun` / `RequestStopRun`（受状态规则验证，禁止时广播 `OnPaceRequestRejected` + 日志 `Movement.Reject.PaceForbidden`）；`ApplyPace`/`OverridePace`/`ClearPaceOverride` 为组件内部应用通道（掩体强制潜行使用 `Movement.Override.Hidden`）。进入状态自动应用默认步态（Perception 潜行 / Courage 走路 / Memory 走路）。
++- **掩体**：`ALRHidePoint` 实例配置 `bAllowMovementWhileHidden`：桌下/草丛/管道（可移动）= true，柜/箱（固定）= false；进入掩体强制潜行，退出按当前状态重新求值。掩体容量与进出音效不在本批次。
++- **噪声环境体积**：`ALRNoiseArea` 实例配置 `Environment`：`Indoor` / `Outdoor` / `OutdoorStealth`；重叠按 `Indoor > OutdoorStealth > Outdoor` 解析，无区域时默认 `Outdoor`。进入/退出都会重新求值。
++- **脚步噪声**（纯规则，见 `LRMovementRules::ResolveFootstepNoise`）：潜行无声；走路 室内 400 / 室外潜行 250 / 室外非潜行 250+Faint；奔跑 室内房间传播（无房间回退 1200）/ 室外潜行 600 / 室外非潜行 250。调优字段已重命名：`OutdoorSneakGuardNoiseRadius`→`OutdoorStealthRunNoiseRadius`、`OutdoorAlertGuardNoiseRadius`→`OutdoorNoiseRadius`（PropertyRedirects 已迁移）。
++
++### 敌人警戒（4.2.1 全量）
++
++- **行为语义**（C++ 权威）：吸引噪声 +1（1-5 档 CD 0.5s；首次进入 6-10 档 0.5s、其后 0.2s；CD 内刺激完全忽略）；看见玩家 警戒<6→6、6-10→11、11 丢失→10；观察 3s（0→1 与抵达调查点）与衰减 0.5s/-1 由 `ULRAlertComponent` 计时器驱动；`ResolveTargetBehavior` 为行为唯一权威（眩晕优先）。警戒条 UI 只读 `FLRAlertSnapshot`（Level/Fraction/Tier/Behavior/bFullAlert）+ `OnAlertSnapshotChanged`，绑定后立即推送初值。
++- **`WBP_GuardAlertBar`**：继承 `ULRWorldAlertBarWidgetBase`；覆盖 `HandleAlertSnapshotChanged` 只做表现：`Tier=Hidden` 隐藏、`White` 白色进度条、`Red` 红色、`Full` 满值+额外红色特效（样式需重新设计）。由 `BP_Guard` 的 `AlertWidget` 组件初始化，Widget 不自行猜测所属守卫。
++- **`ALRRoomVolume` 摆放**：在关卡中摆放 Box 体积（Trigger profile），填写 `RoomId`（稳定 FName），`AdjacentRooms` 连线到相邻房间体积（门/窗拓扑）；守卫进入体积自动注册。室内奔跑：当前房间守卫警戒至少提升到 `RoomRunAlertLevel`(5)、相邻房间 +1；同一守卫属多房间时取最大效果、不累加；无房间体积时回退 1200 半径听觉事件。房间体积必须早于守卫生成。
++- **`ST_Guard` 资产（编辑器人工创建 + MCP 检查）**：见「守卫 AI 与 StateTree」小节接线要求。
++
++### 通用 NPC
++
++- **`BP_NPC`**：继承 `ALRNPCCharacter`，配置网格、动画与 `DA_LRNPCDefinition`。
++- **`DA_LRNPCDefinition`**：`NpcId`（稳定 FName）、`Behavior`（StateTree **硬引用** `ST_NPC`）、`DialogueRowId`（对话 DataTable 稳定行 ID）、`DefaultBehavior`（Idle/Patrol）。
++- **`ST_NPC` 资产（编辑器人工创建 + MCP 检查）**：四个状态 `Idle / Patrol / ReactToNoise / Conversation`；条件 `FLRNPCStateCondition` 比较控制器 `GetActiveBehavior()`，任务 `FLRNPCBehaviorTask`；Idle 附加 `FLRNPCLookAtPlayerTask`（低频朝向检测），ReactToNoise 附加 `FLRNPCReactToNoiseTask`（限时反应，到时发 `AI.Event.NPCReactionEnded` 回默认行为）。树由 `AI.Event.NPCNoiseHeard` / `NPCDialogueStarted` / `NPCDialogueEnded` / `NPCReactionEnded` 驱动。
++- **对话**：交互选项 `Interaction.Action.Talk`（Normal 状态）经 `ULRDialogueSubsystem::StartDialogue` 启动；Conversation 高优先级，普通噪声不打断（只触发 `OnNoiseHeard` 表现钩子）。巡逻点按实例配置。
++- **调优**：`DA_LRNPCTuning`（登记进 `DA_LRGameTuningSet.NPC`）：`LookAtPlayerRadiusCm`、`LookAtIntervalSeconds`、`NoiseReactionDurationSeconds`、`PatrolSpeedCm`。
++- **预留**：`OnNoiseHeard`（BlueprintImplementableEvent）与 `OnNPCAttentionChanged` 委托为未来告警/逃离扩展钩子，本批次不实现告警逻辑。
++
++### 四状态美术表现预留（不实现视觉效果）
++
++- **表现事件（已具备）**：`ULRStateComponent` 的 `OnStateChanging(PreviousMode, NextMode, Reason)` / `OnStateChanged`；`ULRStatePresentationComponent` 转发 `OnStatePresentationRequested` + `PresentStateChange`（表现锁由 `CompleteStatePresentation` 释放）。未来接入 Normal/Perception/Courage/Memory 四套视觉/后处理/音频只需订阅既有事件。
++- **表现调优接入点（新增 getter）**：`GetPerceptionRevealRadius()`(4.5m)、`GetNoiseRevealRadius()`(2m)、`GetNoiseRevealDurationSeconds()`(5s)、`GetPerceptionBlendWeight()`、`GetCourageBlendWeight()`——值来自 `ULRPresentationTuning`（`DA_LRGameTuningSet.Presentation`）。
++- **其他钩子**：`ULRNoiseEmitterComponent::OnNoiseEmitted`（声源显现，房间传播路径也已广播）；`ELRScreenType::StateOverlay`（屏幕层）；`LRHUDWidgetController::OnPerceptionModeChanged`（已有）。
++
++### 输入与调优变更
++
++- `SneakAction` 已废弃（`DeprecatedProperty`，移出 Validate 必填）；潜行切换继续使用 `ToggleCrouchAction`（C / B 切换）。
++- 调优重命名（PropertyRedirects 已配置）：`HearingAlertAmount`→`AttractAlertAmount`（**资产值需改为 1**）、`SightAlertLevel`→`SightChaseLevel`(11)、移动噪声半径两项；`SearchDurationSeconds` 废弃。
++- 新建资产清单（StateTree 需编辑器人工创建，其余可 MCP）：`ST_Guard`、`ST_NPC`、`DA_LRGuardDefinition`、`DA_LRNPCDefinition`、`DA_LRNPCTuning`、`BP_Guard`、`WBP_GuardAlertBar`、`BP_NPC`；`DA_LRGameTuningSet` 登记 `DA_LRNPCTuning`。
++- **PIE 验收（`/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`，键鼠+手柄，Output Log 无项目级 Warning/Error）**：状态步态（Perception 强制潜行、Courage 拒潜行、Memory 仅走路）；掩体进入强制潜行/固定掩体不可移动/掩体内不可见；噪声区域进入退出与重叠优先级、7 行步态×环境噪声、房间传播（本房→5、邻房+1、多房间取最大、无房间兜底）；完整警戒流程（0→吸引→1 观察 3s、CD 内忽略、看见→6 前往、抵达 Search 观察→衰减→0 巡逻、6-10 看见→11 追逐、丢失→10、追上死亡→Memory）；世界警戒条四档表现与首帧同步；击退眩晕 0.6s 恢复（`LR.Debug.Alert` 显示 Stunned）；NPC 巡逻/站立/对话开合/噪声限时反应/Idle 朝向玩家/对话结束回默认；`LR.Debug.Tuning` 确认重命名后来源。
++
+ ## 更新记录
+ 
+ | 日期 | 变更 |
+diff --git a/Docs/Technical/08_CoreMechanicsBatch_Diff.md b/Docs/Technical/08_CoreMechanicsBatch_Diff.md
+new file mode 100644
+index 0000000..9d435d6
+--- /dev/null
++++ b/Docs/Technical/08_CoreMechanicsBatch_Diff.md
+@@ -0,0 +1,37 @@
++# 核心玩法批次 Git Diff（2026-08-14）
++
++提交范围：4.1 四状态 + 4.2 潜行核心机制（状态→步态权限、噪声语义、敌人警戒 4.2.1、房间传播、掩体、通用 NPC、警戒条数据层、表现预留）。
++
++```diff
++diff --git a/Source/LostRunic/Framework/LRPlayerController.cpp b/Source/LostRunic/Framework/LRPlayerController.cpp
++index eaf2f5a..7801a18 100644
++--- a/Source/LostRunic/Framework/LRPlayerController.cpp
+++++ b/Source/LostRunic/Framework/LRPlayerController.cpp
++@@ -163,7 +163,7 @@ void ALRPlayerController::HandleSneakToggle()
++ 	}
++ 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
++ 	{
++-		character->GetLocomotionComponent()->ToggleSneak();
+++		character->GetLocomotionComponent()->RequestToggleSneak();
++ 	}
++ }
++ 
++@@ -178,7 +178,7 @@ void ALRPlayerController::HandleRunStarted()
++ 	}
++ 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
++ 	{
++-		character->GetLocomotionComponent()->StartRun();
+++		character->GetLocomotionComponent()->RequestStartRun();
++ 	}
++ }
++ 
++@@ -193,7 +193,7 @@ void ALRPlayerController::HandleRunStopped()
++ 	}
++ 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
++ 	{
++-		character->GetLocomotionComponent()->StopRun();
+++		character->GetLocomotionComponent()->RequestStopRun();
++ 	}
++ }
++ 
++```
+diff --git a/Scripts/BuildLostRunicEditor.bat b/Scripts/BuildLostRunicEditor.bat
+new file mode 100644
+index 0000000..8316883
+--- /dev/null
++++ b/Scripts/BuildLostRunicEditor.bat
+@@ -0,0 +1,3 @@
++@echo off
++rem Build wrapper: avoids Git Bash / cmd quoting issues with paths containing spaces.
++call "D:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" LostRunicEditor Win64 Development -Project="D:\25DGame\LostRunic\LostRunic.uproject" %*
+diff --git a/Scripts/RunLostRunicTests.bat b/Scripts/RunLostRunicTests.bat
+new file mode 100644
+index 0000000..da414a0
+--- /dev/null
++++ b/Scripts/RunLostRunicTests.bat
+@@ -0,0 +1,3 @@
++@echo off
++rem Test runner: unattended automation run for all LostRunic tests, exits when done.
++"D:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "D:\25DGame\LostRunic\LostRunic.uproject" -unattended -nopause -ExecCmds="Automation RunTests LostRunic; Quit"
+diff --git a/Source/LostRunic/AI/LRAlertComponent.cpp b/Source/LostRunic/AI/LRAlertComponent.cpp
+index 26279f4..c623f0f 100644
+--- a/Source/LostRunic/AI/LRAlertComponent.cpp
++++ b/Source/LostRunic/AI/LRAlertComponent.cpp
+@@ -53,12 +53,13 @@ void ULRAlertComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
+ 	if (GetWorld())
+ 	{
+ 		GetWorld()->GetTimerManager().ClearTimer(DecayTimer);
++		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
+ 	}
+ 	Super::EndPlay(endPlayReason);
+ }
+ 
+ /**
+- * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化。
++ * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化；警戒归零时清理目标与观察状态。
+  * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
+  * @param location 世界空间位置，Unreal 单位为厘米。
+  * @param target 本次规则检查或操作的目标对象。
+@@ -80,11 +81,59 @@ void ULRAlertComponent::ApplyAlertDelta(const int32 delta, const FVector locatio
+ 		bSearching = false;
+ 	}
+ 	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
++	if (AlertLevel <= 0)
++	{
++		ClearWhenAlertZero();
++	}
++	BroadcastChange(previousLevel, previousState, reason);
++}
++
++/**
++ * @brief 吸引注意语义入口：按档位冷却门控（CD 内刺激完全忽略，不改变观察状态），每次 +AttractAlertAmount，并重置 3s 观察窗口。
++ * @param location 世界空间位置，Unreal 单位为厘米。
++ * @param target 本次规则检查或操作的目标对象。
++ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
++ */
++void ULRAlertComponent::ApplyAttract(const FVector location, AActor* target, const FGameplayTag reason)
++{
++	const ULRGuardTuning& tuning = GetEffectiveTuning();
++	const double now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
++	// 从 0 起的首次吸引立即生效（设计：0 -> 1）；之后的增加受档位冷却门控，CD 内完全忽略。
++	if (AlertLevel > 0
++		&& !LRAlertRules::IsIncreaseAllowed(now, LastIncreaseTimeSeconds,
++			LRAlertRules::ResolveAttractIncreaseCooldown(AlertLevel, bFirstIncreaseInBand, tuning)))
++	{
++		UE_LOG(LogLostRunicAI, Verbose, TEXT("Guard=%s attract ignored (cooldown) reason=%s"),
++			*GetNameSafe(GetOwner()), *reason.GetTagName().ToString());
++		return;
++	}
++
++	const int32 previousLevel = AlertLevel;
++	const ELRGuardBehaviorState previousState = GetBehaviorState();
++	const bool bCrossingIntoBand = AlertLevel < tuning.SightInvestigateLevel;
++	AlertLevel = LRAlertRules::ApplyDelta(AlertLevel, tuning.AttractAlertAmount);
++	LastDisturbanceLocation = location;
++	if (target)
++	{
++		TargetActor = target;
++	}
++	bSearching = false;
++	LastIncreaseTimeSeconds = now;
++	LastStimulusTimeSeconds = now;
++	if (bFirstIncreaseInBand)
++	{
++		bFirstIncreaseInBand = false;
++	}
++	if (bCrossingIntoBand && AlertLevel >= tuning.SightInvestigateLevel)
++	{
++		bFirstIncreaseInBand = true;
++	}
++	StartObservation();
+ 	BroadcastChange(previousLevel, previousState, reason);
+ }
+ 
+ /**
+- * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件。
++ * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件；按 4.2.1 分档：<6 看见升到 6，6-10 看见升到 11，11 丢失降回 10。
+  * @param target 本次规则检查或操作的目标对象。
+  * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+  * @param lastKnownLocation 空间值 `lastKnownLocation`；距离和位置使用 Unreal 厘米单位。
+@@ -96,10 +145,23 @@ void ULRAlertComponent::SetSightTarget(AActor* target, const bool bVisible, cons
+ 	LastDisturbanceLocation = lastKnownLocation;
+ 	TargetActor = target;
+ 	bHasConfirmedSight = bVisible;
+-	bSearching = !bVisible && AlertLevel > 0;
+ 	if (bVisible)
+ 	{
+-		AlertLevel = FMath::Max(AlertLevel, GetEffectiveTuning().SightAlertLevel);
++		const ULRGuardTuning& tuning = GetEffectiveTuning();
++		if (AlertLevel < tuning.SightInvestigateLevel)
++		{
++			AlertLevel = tuning.SightInvestigateLevel;
++			bFirstIncreaseInBand = true;
++		}
++		else if (AlertLevel < tuning.SightChaseLevel)
++		{
++			AlertLevel = tuning.SightChaseLevel;
++		}
++	}
++	else if (AlertLevel >= GetEffectiveTuning().SightChaseLevel)
++	{
++		// 11 丢失视线 -> 10，前往最后看见位置；不置 bSearching，使行为解析自然落到 Investigate。
++		AlertLevel = 10;
+ 	}
+ 	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
+ 	BroadcastChange(previousLevel, previousState,
+@@ -107,12 +169,13 @@ void ULRAlertComponent::SetSightTarget(AActor* target, const bool bVisible, cons
+ }
+ 
+ /**
+- * @brief 标记守卫已到达最后异常位置，使 StateTree 从 Investigate 转入 Search。
++ * @brief 标记守卫已到达最后异常位置，使 StateTree 从 Investigate 转入 Search，并开始 3s 抵达观察。
+  */
+ void ULRAlertComponent::MarkInvestigationReached()
+ {
+ 	const ELRGuardBehaviorState previousState = GetBehaviorState();
+ 	bSearching = AlertLevel > 0;
++	StartObservation();
+ 	BroadcastChange(AlertLevel, previousState, LRGameplayTags::SearchReached);
+ }
+ 
+@@ -126,7 +189,12 @@ void ULRAlertComponent::ResetAfterSearch()
+ 	AlertLevel = 0;
+ 	bSearching = false;
+ 	bHasConfirmedSight = false;
++	bObserving = false;
+ 	TargetActor.Reset();
++	if (GetWorld())
++	{
++		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
++	}
+ 	BroadcastChange(previousLevel, previousState, LRGameplayTags::SearchTimeout);
+ }
+ 
+@@ -148,14 +216,66 @@ void ULRAlertComponent::HandleDecayTimer()
+ 	{
+ 		return;
+ 	}
+-	const float elapsed = GetWorld()->GetTimeSeconds() - LastStimulusTimeSeconds;
+-	if (LRAlertRules::ShouldDecay(elapsed, GetEffectiveTuning().InitialObserveSeconds, bHasConfirmedSight))
++	if (LRAlertRules::ShouldDecay(bObserving, bHasConfirmedSight, GetBehaviorState()))
+ 	{
+ 		ApplyAlertDelta(-GetEffectiveTuning().AlertDecayAmount, LastDisturbanceLocation,
+ 			TargetActor.Get(), LRGameplayTags::SearchAlertDecay);
+ 	}
+ }
+ 
++/**
++ * @brief 处理 Handle Observation End 事件，将引擎回调转换为对应领域状态更新；观察结束前警戒维持不动。
++ */
++void ULRAlertComponent::HandleObservationEnd()
++{
++	bObserving = false;
++}
++
++/**
++ * @brief 开始 3 秒观察窗口；观察期间衰减被门控。
++ */
++void ULRAlertComponent::StartObservation()
++{
++	if (AlertLevel <= 0 || !GetWorld())
++	{
++		return;
++	}
++	bObserving = true;
++	GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
++	GetWorld()->GetTimerManager().SetTimer(ObservationTimer, this, &ULRAlertComponent::HandleObservationEnd,
++		GetEffectiveTuning().InitialObserveSeconds, false);
++}
++
++/**
++ * @brief 警戒归零时清理目标、搜索与观察状态；不额外广播（归零变化本身已广播）。
++ */
++void ULRAlertComponent::ClearWhenAlertZero()
++{
++	bSearching = false;
++	bHasConfirmedSight = false;
++	bObserving = false;
++	TargetActor.Reset();
++	if (GetWorld())
++	{
++		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
++	}
++}
++
++/**
++ * @brief 查询当前只读警戒快照（等级、归一化进度、显示档位、行为与满值标志）；UI 绑定 OnAlertSnapshotChanged 后应立即读取本值，避免首帧不同步。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++FLRAlertSnapshot ULRAlertComponent::GetAlertSnapshot() const
++{
++	FLRAlertSnapshot snapshot;
++	snapshot.Level = AlertLevel;
++	snapshot.Fraction = AlertLevel / static_cast<float>(LRAlertRules::MaxAlertLevel);
++	snapshot.Tier = LRAlertRules::ResolveAlertTier(AlertLevel);
++	snapshot.Behavior = GetBehaviorState();
++	snapshot.bFullAlert = AlertLevel >= LRAlertRules::MaxAlertLevel;
++	return snapshot;
++}
++
+ /**
+  * @brief 广播警戒旧值、新值和原因标签，供 StateTree、UI、日志与测试订阅。
+  * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
+@@ -169,8 +289,9 @@ void ULRAlertComponent::BroadcastChange(const int32 previousLevel, const ELRGuar
+ 	const ELRGuardBehaviorState currentState = GetBehaviorState();
+ 	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s alert %d -> %d state %d -> %d reason=%s location=%s"),
+ 		*GetNameSafe(GetOwner()), previousLevel, AlertLevel, static_cast<int32>(previousState),
+-		static_cast<int32>(currentState), *reason.ToString(), *LastDisturbanceLocation.ToCompactString());
++		static_cast<int32>(currentState), *reason.GetTagName().ToString(), *LastDisturbanceLocation.ToCompactString());
+ 	OnAlertChanged.Broadcast(previousLevel, AlertLevel, currentState, reason, LastDisturbanceLocation);
++	OnAlertSnapshotChanged.Broadcast(GetAlertSnapshot());
+ }
+ 
+ /**
+diff --git a/Source/LostRunic/AI/LRAlertComponent.h b/Source/LostRunic/AI/LRAlertComponent.h
+index 578d2a9..542f594 100644
+--- a/Source/LostRunic/AI/LRAlertComponent.h
++++ b/Source/LostRunic/AI/LRAlertComponent.h
+@@ -18,6 +18,7 @@ class ULRGuardTuning;
+ 
+ DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLRAlertChanged, int32, previousLevel, int32, currentLevel,
+ 	ELRGuardBehaviorState, currentState, FGameplayTag, reason, FVector, disturbanceLocation);
++DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRAlertSnapshotChanged, const FLRAlertSnapshot&, snapshot);
+ 
+ /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+ UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Alert"))
+@@ -42,7 +43,7 @@ public:
+ 	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
+ 
+ 	/**
+-	 * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化。
++	 * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化；警戒归零时清理目标与观察状态。
+ 	 * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
+ 	 * @param location 世界空间位置，Unreal 单位为厘米。
+ 	 * @param target 本次规则检查或操作的目标对象。
+@@ -51,6 +52,15 @@ public:
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
+ 	void ApplyAlertDelta(int32 delta, FVector location, AActor* target, FGameplayTag reason);
+ 
++	/**
++	 * @brief 吸引注意语义入口：按档位冷却门控（CD 内刺激完全忽略，不改变观察状态），每次 +AttractAlertAmount，并重置 3s 观察窗口。
++	 * @param location 世界空间位置，Unreal 单位为厘米。
++	 * @param target 本次规则检查或操作的目标对象。
++	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
++	 */
++	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
++	void ApplyAttract(FVector location, AActor* target, FGameplayTag reason);
++
+ 	/**
+ 	 * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件。
+ 	 * @param target 本次规则检查或操作的目标对象。
+@@ -114,15 +124,52 @@ public:
+ 	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
+ 	bool HasConfirmedSight() const { return bHasConfirmedSight; }
+ 
++	/**
++	 * @brief 判断 Is Searching 对应条件；不产生玩法副作用。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
++	bool IsSearching() const { return bSearching; }
++
++	/**
++	 * @brief 判断 Is Observing 对应条件；不产生玩法副作用。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
++	bool IsObserving() const { return bObserving; }
++
++	/**
++	 * @brief 查询当前只读警戒快照（等级、归一化进度、显示档位、行为与满值标志）；UI 绑定 OnAlertSnapshotChanged 后应立即读取本值，避免首帧不同步。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
++	FLRAlertSnapshot GetAlertSnapshot() const;
++
+ 	/** 当 Alert Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
+ 	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
+ 	FLRAlertChanged OnAlertChanged;
+ 
++	/** 当 Alert Snapshot Changed 发生时广播；世界警戒条 Widget 绑定该只读快照，蓝图只做表现。  */
++	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
++	FLRAlertSnapshotChanged OnAlertSnapshotChanged;
++
+ private:
+ 	/**
+ 	 * @brief 处理 Handle Decay Timer 事件，将引擎回调转换为对应领域状态更新。
+ 	 */
+ 	void HandleDecayTimer();
++	/**
++	 * @brief 处理 Handle Observation End 事件，将引擎回调转换为对应领域状态更新；观察结束前警戒维持不动。
++	 */
++	void HandleObservationEnd();
++	/**
++	 * @brief 开始 3 秒观察窗口；观察期间衰减被门控。
++	 */
++	void StartObservation();
++	/**
++	 * @brief 警戒归零时清理目标、搜索与观察状态；不额外广播（归零变化本身已广播）。
++	 */
++	void ClearWhenAlertZero();
+ 	/**
+ 	 * @brief 广播警戒旧值、新值和原因标签，供 StateTree、UI、日志与测试订阅。
+ 	 * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
+@@ -156,10 +203,18 @@ private:
+ 	FGameplayTag LastReason;
+ 	/** Last Stimulus Time Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+ 	double LastStimulusTimeSeconds = 0.0;
++	/** Last Increase Time Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	double LastIncreaseTimeSeconds = 0.0;
+ 	/** Has Confirmed Sight 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+ 	bool bHasConfirmedSight = false;
+ 	/** Searching 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+ 	bool bSearching = false;
++	/** Observing 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	bool bObserving = false;
++	/** First Increase In Band 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	bool bFirstIncreaseInBand = false;
+ 	/** Decay Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
+ 	FTimerHandle DecayTimer;
++	/** Observation Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
++	FTimerHandle ObservationTimer;
+ };
+diff --git a/Source/LostRunic/AI/LRAlertRules.cpp b/Source/LostRunic/AI/LRAlertRules.cpp
+index 56f1685..be0dae4 100644
+--- a/Source/LostRunic/AI/LRAlertRules.cpp
++++ b/Source/LostRunic/AI/LRAlertRules.cpp
+@@ -1,6 +1,6 @@
+ /**
+  * @file LRAlertRules.cpp
+- * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
++ * @brief 实现 0-11 警戒纯规则：边界钳制、行为档位解析（4.2.1）、衰减门控与吸引增加冷却。
+  *
+  * 关联文件：LRAlertRules.h；所属领域：AI。
+  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+@@ -8,10 +8,10 @@
+  */
+ #include "AI/LRAlertRules.h"
+ 
++#include "Data/LRGuardTuning.h"
++
+ namespace
+ {
+-	constexpr int32 MinAlertLevel = 0;
+-	constexpr int32 MaxAlertLevel = 11;
+ 	constexpr int32 SuspiciousMaxLevel = 5;
+ }
+ 
+@@ -27,7 +27,7 @@ int32 LRAlertRules::ApplyDelta(const int32 currentLevel, const int32 delta)
+ }
+ 
+ /**
+- * @brief 执行 Resolve State 的纯规则或事务判定，失败时提供结构化原因。
++ * @brief 按 4.2.1 档位解析行为：0 巡逻；11+视线 追逐；11 无视线 搜索兜底；搜索且 >=6 搜索；<=5 可疑；否则调查。
+  * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
+  * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+  * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+@@ -43,7 +43,11 @@ ELRGuardBehaviorState LRAlertRules::ResolveState(const int32 alertLevel, const b
+ 	{
+ 		return ELRGuardBehaviorState::Chase;
+ 	}
+-	if (bSearching || alertLevel >= MaxAlertLevel)
++	if (alertLevel >= MaxAlertLevel)
++	{
++		return ELRGuardBehaviorState::Search;
++	}
++	if (bSearching && alertLevel >= SuspiciousMaxLevel + 1)
+ 	{
+ 		return ELRGuardBehaviorState::Search;
+ 	}
+@@ -52,13 +56,79 @@ ELRGuardBehaviorState LRAlertRules::ResolveState(const int32 alertLevel, const b
+ }
+ 
+ /**
+- * @brief 判断 Should Decay 对应条件；不产生玩法副作用。
+- * @param secondsSinceStimulus 时间值 `secondsSinceStimulus`，单位为秒。
+- * @param observeSeconds 时间值 `observeSeconds`，单位为秒。
++ * @brief 守卫行为唯一权威解析：眩晕覆盖优先返回 Stunned，否则按警戒推导；StateTree 只执行本结果，不自行重新定义警戒语义。
++ * @param bStunned 布尔开关 `bStunned`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
+  * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRGuardBehaviorState LRAlertRules::ResolveTargetBehavior(const bool bStunned, const int32 alertLevel,
++	const bool bHasSight, const bool bSearching)
++{
++	return bStunned ? ELRGuardBehaviorState::Stunned : ResolveState(alertLevel, bHasSight, bSearching);
++}
++
++/**
++ * @brief 解析警戒显示档位：0 隐藏、1-5 白色、6-10 红色、11 满值。
++ * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRGuardAlertTier LRAlertRules::ResolveAlertTier(const int32 alertLevel)
++{
++	if (alertLevel <= MinAlertLevel)
++	{
++		return ELRGuardAlertTier::Hidden;
++	}
++	if (alertLevel >= MaxAlertLevel)
++	{
++		return ELRGuardAlertTier::Full;
++	}
++	return alertLevel <= SuspiciousMaxLevel ? ELRGuardAlertTier::White : ELRGuardAlertTier::Red;
++}
++
++/**
++ * @brief 判断 Should Decay 对应条件；观察中、追逐中或调查（前往）中不衰减，其余 0.5s/-1。
++ * @param bObserving 布尔开关 `bObserving`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @param bHasConfirmedSight 布尔开关 `bHasConfirmedSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++bool LRAlertRules::ShouldDecay(const bool bObserving, const bool bHasConfirmedSight,
++	const ELRGuardBehaviorState currentState)
++{
++	if (bObserving || bHasConfirmedSight)
++	{
++		return false;
++	}
++	return currentState != ELRGuardBehaviorState::Investigate && currentState != ELRGuardBehaviorState::Chase;
++}
++
++/**
++ * @brief 解析吸引增加的冷却时长：1-5 档与首次进入 6-10 档使用 AlertIncreaseCooldownSeconds，6-10 档后续使用 InvestigateIncreaseCooldownSeconds。
++ * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
++ * @param bFirstIncreaseInBand 布尔开关 `bFirstIncreaseInBand`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++float LRAlertRules::ResolveAttractIncreaseCooldown(const int32 currentAlert, const bool bFirstIncreaseInBand,
++	const ULRGuardTuning& tuning)
++{
++	if (currentAlert < tuning.SightInvestigateLevel || bFirstIncreaseInBand)
++	{
++		return tuning.AlertIncreaseCooldownSeconds;
++	}
++	return tuning.InvestigateIncreaseCooldownSeconds;
++}
++
++/**
++ * @brief 判断 Is Increase Allowed 对应条件；冷却拒绝的刺激被完全忽略，不改变观察状态。
++ * @param now 时间值 `now`，单位为秒。
++ * @param lastIncreaseTime 时间值 `lastIncreaseTime`，单位为秒。
++ * @param cooldownSeconds 时间值 `cooldownSeconds`，单位为秒。
+  * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+  */
+-bool LRAlertRules::ShouldDecay(const float secondsSinceStimulus, const float observeSeconds, const bool bHasSight)
++bool LRAlertRules::IsIncreaseAllowed(const double now, const double lastIncreaseTime, const float cooldownSeconds)
+ {
+-	return !bHasSight && secondsSinceStimulus >= observeSeconds;
++	return cooldownSeconds <= 0.0f || now - lastIncreaseTime >= cooldownSeconds;
+ }
+diff --git a/Source/LostRunic/AI/LRAlertRules.h b/Source/LostRunic/AI/LRAlertRules.h
+index bf2206c..b668f09 100644
+--- a/Source/LostRunic/AI/LRAlertRules.h
++++ b/Source/LostRunic/AI/LRAlertRules.h
+@@ -1,6 +1,6 @@
+ /**
+  * @file LRAlertRules.h
+- * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
++ * @brief 提供 0-11 警戒纯规则：边界钳制、行为档位解析（4.2.1 语义）、衰减门控与吸引增加冷却，供运行时组件与 LostRunic.AI 自动化测试共同调用。
+  *
+  * 关联文件：LRAlertRules.cpp；所属领域：AI。
+  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+@@ -10,8 +10,15 @@
+ 
+ #include "AI/LRGuardTypes.h"
+ 
++class ULRGuardTuning;
++
+ namespace LRAlertRules
+ {
++	/** 警戒上限；供 UI 快照与运行时组件共享。 */
++	inline constexpr int32 MaxAlertLevel = 11;
++	/** 警戒下限。 */
++	inline constexpr int32 MinAlertLevel = 0;
++
+ 	/**
+ 	 * @brief 按 0-11 边界应用警戒变化，并返回旧值、新值和原因。
+ 	 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
+@@ -20,7 +27,7 @@ namespace LRAlertRules
+ 	 */
+ 	LOSTRUNIC_API int32 ApplyDelta(int32 currentLevel, int32 delta);
+ 	/**
+-	 * @brief 执行 Resolve State 的纯规则或事务判定，失败时提供结构化原因。
++	 * @brief 按 4.2.1 档位解析行为：0 巡逻；11+视线 追逐；11 无视线 搜索兜底；搜索且 >=6 搜索；<=5 可疑；否则调查。
+ 	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
+ 	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ 	 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+@@ -28,11 +35,44 @@ namespace LRAlertRules
+ 	 */
+ 	LOSTRUNIC_API ELRGuardBehaviorState ResolveState(int32 alertLevel, bool bHasSight, bool bSearching);
+ 	/**
+-	 * @brief 判断 Should Decay 对应条件；不产生玩法副作用。
+-	 * @param secondsSinceStimulus 时间值 `secondsSinceStimulus`，单位为秒。
+-	 * @param observeSeconds 时间值 `observeSeconds`，单位为秒。
++	 * @brief 守卫行为唯一权威解析：眩晕覆盖优先返回 Stunned，否则按警戒推导；StateTree 只执行本结果，不自行重新定义警戒语义。
++	 * @param bStunned 布尔开关 `bStunned`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
+ 	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(bool bStunned, int32 alertLevel, bool bHasSight,
++		bool bSearching);
++	/**
++	 * @brief 解析警戒显示档位：0 隐藏、1-5 白色、6-10 红色、11 满值。
++	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API ELRGuardAlertTier ResolveAlertTier(int32 alertLevel);
++	/**
++	 * @brief 判断 Should Decay 对应条件；观察中、追逐中或调查（前往）中不衰减，其余 0.5s/-1。
++	 * @param bObserving 布尔开关 `bObserving`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @param bHasConfirmedSight 布尔开关 `bHasConfirmedSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API bool ShouldDecay(bool bObserving, bool bHasConfirmedSight, ELRGuardBehaviorState currentState);
++	/**
++	 * @brief 解析吸引增加的冷却时长：1-5 档与首次进入 6-10 档使用 AlertIncreaseCooldownSeconds，6-10 档后续使用 InvestigateIncreaseCooldownSeconds。
++	 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
++	 * @param bFirstIncreaseInBand 布尔开关 `bFirstIncreaseInBand`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API float ResolveAttractIncreaseCooldown(int32 currentAlert, bool bFirstIncreaseInBand,
++		const ULRGuardTuning& tuning);
++	/**
++	 * @brief 判断 Is Increase Allowed 对应条件；冷却拒绝的刺激被完全忽略，不改变观察状态。
++	 * @param now 时间值 `now`，单位为秒。
++	 * @param lastIncreaseTime 时间值 `lastIncreaseTime`，单位为秒。
++	 * @param cooldownSeconds 时间值 `cooldownSeconds`，单位为秒。
+ 	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ 	 */
+-	LOSTRUNIC_API bool ShouldDecay(float secondsSinceStimulus, float observeSeconds, bool bHasSight);
++	LOSTRUNIC_API bool IsIncreaseAllowed(double now, double lastIncreaseTime, float cooldownSeconds);
+ }
+diff --git a/Source/LostRunic/AI/LRGuardAIController.cpp b/Source/LostRunic/AI/LRGuardAIController.cpp
+index 1d5e54d..33e3871 100644
+--- a/Source/LostRunic/AI/LRGuardAIController.cpp
++++ b/Source/LostRunic/AI/LRGuardAIController.cpp
+@@ -1,6 +1,6 @@
+ /**
+  * @file LRGuardAIController.cpp
+- * @brief 把 AI Perception 的 Sight/Hearing 事件转换为警戒原因标签，并驱动 Idle、Suspicious、Investigate、Search、Chase 行为、导航速度和捕获检测。
++ * @brief 守卫控制器生命周期：构造、BeginPlay/EndPlay、OnPossess/OnUnPossess、调优解析与 StateTree 启动接线。感知与行为实现分别位于 LRGuardAIControllerPerception.cpp / LRGuardAIControllerBehavior.cpp。
+  *
+  * 关联文件：LRGuardAIController.h；所属领域：AI。
+  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+@@ -9,26 +9,22 @@
+ #include "AI/LRGuardAIController.h"
+ 
+ #include "AI/LRAlertComponent.h"
++#include "AI/LRAlertRules.h"
+ #include "AI/LRGuardCharacter.h"
+ #include "AI/LRGuardPerceptionRules.h"
+-#include "Components/ActorComponent.h"
+ #include "Components/StateTreeAIComponent.h"
+-#include "Core/LRGameplayTags.h"
+ #include "Core/LRLog.h"
+ #include "Data/LRGameTuningSet.h"
++#include "Data/LRGuardDefinition.h"
+ #include "Data/LRGuardTuning.h"
+-#include "DrawDebugHelpers.h"
++#include "Data/LRStateTuning.h"
+ #include "Engine/GameInstance.h"
+ #include "Engine/World.h"
+ #include "Framework/LRGameInstanceSubsystem.h"
+-#include "GameFramework/CharacterMovementComponent.h"
+-#include "Navigation/PathFollowingComponent.h"
++#include "Items/LRCourageResponseComponent.h"
+ #include "Perception/AIPerceptionComponent.h"
+-#include "Perception/AISense_Hearing.h"
+-#include "Perception/AISense_Sight.h"
+ #include "Perception/AISenseConfig_Hearing.h"
+ #include "Perception/AISenseConfig_Sight.h"
+-#include "Stealth/LRGuardVisibility.h"
+ #include "TimerManager.h"
+ 
+ /**
+@@ -41,6 +37,8 @@ ALRGuardAIController::ALRGuardAIController()
+ 	bStopAILogicOnUnposses = true;
+ 	bAttachToPawn = true;
+ 	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
++	// StateTree 由 OnPossess 依次完成定义解析、引用校验、SetStateTree、StartLogic，禁用自动启动。
++	StateTreeAI->SetStartLogicAutomatically(false);
+ 	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
+ 	SetPerceptionComponent(*AIPerception);
+ 	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
+@@ -55,8 +53,12 @@ void ALRGuardAIController::BeginPlay()
+ 	Super::BeginPlay();
+ 	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
+ 	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
+-	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
+-	if (!ensureMsgf(Tuning, TEXT("%s requires Guard tuning."), *GetNameSafe(this)))
++	if (subsystem && subsystem->GetTuningSet())
++	{
++		Tuning = subsystem->GetTuningSet()->Guard;
++		StateTuning = subsystem->GetTuningSet()->State;
++	}
++	if (!ensureMsgf(Tuning && StateTuning, TEXT("%s requires Guard and State tuning."), *GetNameSafe(this)))
+ 	{
+ 		return;
+ 	}
+@@ -79,13 +81,13 @@ void ALRGuardAIController::EndPlay(const EEndPlayReason::Type endPlayReason)
+ 	if (GetWorld())
+ 	{
+ 		GetWorld()->GetTimerManager().ClearTimer(CaptureTimer);
+-		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
++		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
+ 	}
+ 	Super::EndPlay(endPlayReason);
+ }
+ 
+ /**
+- * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
++ * @brief 处理 On Possess 事件：解析定义并校验引用、SetStateTree 后 StartLogic，绑定警戒与击退事件。
+  * @param inPawn Controller 新接管的 Pawn；期望为 ALRGuardCharacter。
+  */
+ void ALRGuardAIController::OnPossess(APawn* inPawn)
+@@ -97,10 +99,31 @@ void ALRGuardAIController::OnPossess(APawn* inPawn)
+ 	{
+ 		Alert->OnAlertChanged.AddDynamic(this, &ALRGuardAIController::HandleAlertChanged);
+ 	}
++	if (guard)
++	{
++		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
++		{
++			courage->OnKnockbackApplied.AddDynamic(this, &ALRGuardAIController::HandleKnockback);
++		}
++		ULRGuardDefinition* definition = guard->GetDefinition();
++		if (definition && definition->Behavior)
++		{
++			StateTreeAI->SetStateTree(definition->Behavior);
++			if (!StateTreeAI->IsRunning())
++			{
++				StateTreeAI->StartLogic();
++			}
++		}
++		else
++		{
++			UE_LOG(LogLostRunicAI, Warning, TEXT("Guard=%s definition or Behavior StateTree is missing; using controller fallback."),
++				*GetNameSafe(guard));
++		}
++	}
+ }
+ 
+ /**
+- * @brief 处理 On Un Possess 事件，将引擎回调转换为对应领域状态更新。
++ * @brief 处理 On Un Possess 事件：解绑警戒与击退委托，停止 StateTree 逻辑。
+  */
+ void ALRGuardAIController::OnUnPossess()
+ {
+@@ -108,268 +131,33 @@ void ALRGuardAIController::OnUnPossess()
+ 	{
+ 		Alert->OnAlertChanged.RemoveDynamic(this, &ALRGuardAIController::HandleAlertChanged);
+ 	}
+-	Alert.Reset();
+-	Super::OnUnPossess();
+-}
+-
+-/**
+- * @brief 进入指定守卫行为，设置移动速度、焦点、导航目标或搜索超时。
+- * @param behavior 要进入或退出的守卫 StateTree 行为状态。
+- */
+-void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
+-{
+-	ActiveBehavior = behavior;
+-	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
+-	if (!guard || !Alert.IsValid())
+-	{
+-		return;
+-	}
+-	GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
+-	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
+-	if (behavior == ELRGuardBehaviorState::Chase)
+-	{
+-		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
+-		SetFocus(Alert->GetTargetActor());
+-		MoveToActor(Alert->GetTargetActor(), GetEffectiveTuning().CaptureRadius);
+-	}
+-	else if (behavior == ELRGuardBehaviorState::Investigate)
+-	{
+-		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
+-		MoveToLocation(Alert->GetLastDisturbanceLocation(), GetEffectiveTuning().MoveAcceptanceRadius);
+-	}
+-	else if (behavior == ELRGuardBehaviorState::Search)
++	if (ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn()))
+ 	{
+-		StopMovement();
+-		SetFocalPoint(Alert->GetLastDisturbanceLocation());
+-		GetWorld()->GetTimerManager().SetTimer(SearchTimer, this, &ALRGuardAIController::HandleSearchTimeout,
+-			GetEffectiveTuning().SearchDurationSeconds, false);
+-	}
+-	else if (behavior == ELRGuardBehaviorState::Suspicious)
+-	{
+-		StopMovement();
+-		SetFocalPoint(Alert->GetLastDisturbanceLocation());
+-	}
+-	else
+-	{
+-		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
+-		StartPatrolMove();
+-	}
+-}
+-
+-/**
+- * @brief 退出指定守卫行为并清理该状态拥有的导航、焦点或计时器。
+- * @param behavior 要进入或退出的守卫 StateTree 行为状态。
+- */
+-void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
+-{
+-	if (GetWorld())
+-	{
+-		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
+-	}
+-	StopMovement();
+-	ClearFocus(EAIFocusPriority::Gameplay);
+-}
+-
+-/**
+- * @brief 处理 On Move Completed 事件，将引擎回调转换为对应领域状态更新。
+- * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
+- * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
+- */
+-void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
+-{
+-	Super::OnMoveCompleted(requestId, result);
+-	if (!result.IsSuccess() || !Alert.IsValid())
+-	{
+-		return;
+-	}
+-	if (ActiveBehavior == ELRGuardBehaviorState::Investigate)
+-	{
+-		Alert->MarkInvestigationReached();
+-	}
+-	else if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
+-	{
+-		++PatrolIndex;
+-		StartPatrolMove();
+-	}
+-}
+-
+-/**
+- * @brief 把 UE 感知刺激转换为可见/听见事件、异常位置和警戒原因标签。
+- * @param actor 本次查询、交互或事件涉及的 Actor。
+- * @param stimulus 时间值 `stimulus`，单位为秒。
+- */
+-void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
+-{
+-	if (!actor || !Alert.IsValid())
+-	{
+-		return;
+-	}
+-	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
+-	{
+-		const bool bVisible = stimulus.WasSuccessfullySensed() && CanConfirmSight(actor);
+-		Alert->SetSightTarget(actor, bVisible, stimulus.StimulusLocation);
+-	}
+-	else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
+-	{
+-		FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
+-		if (!reason.IsValid())
++		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
+ 		{
+-			reason = LRGameplayTags::NoiseInteraction;
++			courage->OnKnockbackApplied.RemoveDynamic(this, &ALRGuardAIController::HandleKnockback);
+ 		}
+-		Alert->ApplyAlertDelta(GetEffectiveTuning().HearingAlertAmount, stimulus.StimulusLocation, actor, reason);
+-	}
+-}
+-
+-/**
+- * @brief 处理 Handle Alert Changed 事件，将引擎回调转换为对应领域状态更新。
+- * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
+- * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
+- * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
+- * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+- * @param disturbanceLocation 空间值 `disturbanceLocation`；距离和位置使用 Unreal 厘米单位。
+- */
+-void ALRGuardAIController::HandleAlertChanged(const int32 previousLevel, const int32 currentLevel,
+-	const ELRGuardBehaviorState currentState, const FGameplayTag reason, const FVector disturbanceLocation)
+-{
+-	StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventAlertChanged, FConstStructView(), reason.GetTagName());
+-	if (!StateTreeAI->IsRunning())
+-	{
+-		EnterBehavior(currentState);
+ 	}
+-}
+-
+-/**
+- * @brief 用 Guard 调优资产配置 UE Sight/Hearing 感知，包括完整视野角换算、距离和阵营检测。
+- */
+-void ALRGuardAIController::ConfigurePerception()
+-{
+-	const ULRGuardTuning& tuning = GetEffectiveTuning();
+-	SightConfig->SightRadius = tuning.SightRadius;
+-	SightConfig->LoseSightRadius = tuning.LoseSightRadius;
+-	SightConfig->PeripheralVisionAngleDegrees = tuning.SightConeDegrees * 0.5f;
+-	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
+-	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
+-	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
+-	HearingConfig->HearingRange = tuning.MaxHearingRange * tuning.HearingRangeMultiplier;
+-	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
+-	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
+-	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
+-	AIPerception->ConfigureSense(*SightConfig);
+-	AIPerception->ConfigureSense(*HearingConfig);
+-	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
+-}
+-
+-/**
+- * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程。
+- */
+-void ALRGuardAIController::HandleCaptureTimer()
+-{
+-	if (!Alert.IsValid() || Alert->GetBehaviorState() != ELRGuardBehaviorState::Chase)
+-	{
+-		return;
+-	}
+-	AActor* target = Alert->GetTargetActor();
+-	if (!CanConfirmSight(target))
+-	{
+-		Alert->SetSightTarget(target, false, target ? target->GetActorLocation() : FVector::ZeroVector);
+-		return;
+-	}
+-	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
+-	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation()) <= GetEffectiveTuning().CaptureRadius)
+-	{
+-		guard->CaptureTarget(target);
+-	}
+-}
+-
+-/**
+- * @brief 处理 Handle Search Timeout 事件，将引擎回调转换为对应领域状态更新。
+- */
+-void ALRGuardAIController::HandleSearchTimeout()
+-{
+-	if (Alert.IsValid())
+-	{
+-		Alert->ResetAfterSearch();
+-	}
+-}
+-
+-/**
+- * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
+- */
+-void ALRGuardAIController::StartPatrolMove()
+-{
+-	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
+-	if (!guard || guard->GetPatrolPointCount() == 0)
++	if (GetWorld())
+ 	{
+-		StopMovement();
+-		return;
++		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
+ 	}
+-	PatrolIndex %= guard->GetPatrolPointCount();
+-	MoveToActor(guard->GetPatrolPoint(PatrolIndex), GetEffectiveTuning().MoveAcceptanceRadius);
+-}
+-
+-/**
+- * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
+- * @param actor 本次查询、交互或事件涉及的 Actor。
+- * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+- */
+-bool ALRGuardAIController::CanConfirmSight(AActor* actor) const
+-{
+-	const APawn* guardPawn = GetPawn();
+-	if (!actor || !guardPawn)
++	if (StateTreeAI->IsRunning())
+ 	{
+-		return false;
++		StateTreeAI->StopLogic(TEXT("OnUnPossess"));
+ 	}
+-	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
+-	const float distance = toTarget.Size2D();
+-	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
+-		toTarget.GetSafeNormal2D());
+-	return LRGuardPerceptionRules::CanConfirmSight(distance, forwardDot, !LineOfSightTo(actor),
+-		IsHiddenFromGuard(actor), GetEffectiveTuning());
++	Alert.Reset();
++	Super::OnUnPossess();
+ }
+ 
+ /**
+- * @brief 判断 Is Hidden From Guard 对应条件；不产生玩法副作用。
+- * @param actor 本次查询、交互或事件涉及的 Actor。
++ * @brief 查询 Resolved Behavior；行为状态唯一权威解析（眩晕优先，否则警戒推导），StateTree 只执行该结果。
+  * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+  */
+-bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
++ELRGuardBehaviorState ALRGuardAIController::GetResolvedBehavior() const
+ {
+-	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
+-	{
+-		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor, const_cast<ALRGuardAIController*>(this));
+-	}
+-	for (UActorComponent* component : actor->GetComponents())
+-	{
+-		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
+-			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component, const_cast<ALRGuardAIController*>(this)))
+-		{
+-			return true;
+-		}
+-	}
+-	return false;
+-}
+-
+-/**
+- * @brief 输出守卫行为、警戒值和最后异常点，并按调试开关绘制视野与听觉范围。
+- */
+-void ALRGuardAIController::LogAndDrawDiagnostics() const
+-{
+-	const APawn* guard = GetPawn();
+-	if (!guard || !Alert.IsValid())
+-	{
+-		return;
+-	}
+-	const ULRGuardTuning& tuning = GetEffectiveTuning();
+-	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s Alert=%d State=%d Target=%s Reason=%s Location=%s"),
+-		*GetNameSafe(guard), Alert->GetAlertLevel(), static_cast<int32>(Alert->GetBehaviorState()),
+-		*GetNameSafe(Alert->GetTargetActor()), *Alert->GetLastReason().ToString(),
+-		*Alert->GetLastDisturbanceLocation().ToCompactString());
+-	const FVector origin = guard->GetActorLocation();
+-	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
+-		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
+-		16, FColor::Yellow, false, 5.0f);
+-	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
+-	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
++	return LRAlertRules::ResolveTargetBehavior(bStunned, Alert.IsValid() ? Alert->GetAlertLevel() : 0,
++		Alert.IsValid() && Alert->HasConfirmedSight(), Alert.IsValid() && Alert->IsSearching());
+ }
+ 
+ /**
+diff --git a/Source/LostRunic/AI/LRGuardAIController.h b/Source/LostRunic/AI/LRGuardAIController.h
+index aa0a1e1..bee40bc 100644
+--- a/Source/LostRunic/AI/LRGuardAIController.h
++++ b/Source/LostRunic/AI/LRGuardAIController.h
+@@ -20,6 +20,7 @@ class UAISenseConfig_Hearing;
+ class UAISenseConfig_Sight;
+ class ULRAlertComponent;
+ class ULRGuardTuning;
++class ULRStateTuning;
+ class UStateTreeAIComponent;
+ 
+ /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+@@ -70,6 +71,13 @@ public:
+ 	 * @param behavior 要进入或退出的守卫 StateTree 行为状态。
+ 	 */
+ 	void ExitBehavior(ELRGuardBehaviorState behavior);
++	/**
++	 * @brief 查询 Resolved Behavior；行为状态唯一权威解析（眩晕优先，否则警戒推导），StateTree 只执行该结果。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
++	ELRGuardBehaviorState GetResolvedBehavior() const;
++
+ 	/**
+ 	 * @brief 输出守卫行为、警戒值和最后异常点，并按调试开关绘制视野与听觉范围。
+ 	 */
+@@ -109,13 +117,19 @@ private:
+ 	 */
+ 	void ConfigurePerception();
+ 	/**
+-	 * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程。
++	 * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程；眩晕期间跳过。
+ 	 */
+ 	void HandleCaptureTimer();
+ 	/**
+-	 * @brief 处理 Handle Search Timeout 事件，将引擎回调转换为对应领域状态更新。
++	 * @brief 处理 Handle Knockback 事件：进入 Stunned 覆盖（停止移动、清除焦点），按 Courage 击退时长计时恢复。
++	 * @param direction 击退方向 `direction`；仅用于诊断。
+ 	 */
+-	void HandleSearchTimeout();
++	UFUNCTION()
++	void HandleKnockback(FVector direction);
++	/**
++	 * @brief 眩晕计时结束后按当前警戒与视线重新解析行为并广播恢复事件。
++	 */
++	void HandleStunEnd();
+ 	/**
+ 	 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
+ 	 */
+@@ -158,6 +172,10 @@ private:
+ 	UPROPERTY(Transient)
+ 	TObjectPtr<ULRGuardTuning> Tuning;
+ 
++	/** State 调优缓存；眩晕时长与 Courage 击退根运动同源。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TObjectPtr<ULRStateTuning> StateTuning;
++
+ 	/** Alert 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+ 	UPROPERTY(Transient)
+ 	TWeakObjectPtr<ULRAlertComponent> Alert;
+@@ -166,8 +184,10 @@ private:
+ 	ELRGuardBehaviorState ActiveBehavior = ELRGuardBehaviorState::IdlePatrol;
+ 	/** Patrol Index 的内部运行时数据；不参与蓝图配置。 */
+ 	int32 PatrolIndex = 0;
++	/** Stunned 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	bool bStunned = false;
+ 	/** Capture Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
+ 	FTimerHandle CaptureTimer;
+-	/** Search Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
+-	FTimerHandle SearchTimer;
++	/** Stun Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
++	FTimerHandle StunTimer;
+ };
+diff --git a/Source/LostRunic/AI/LRGuardAIControllerBehavior.cpp b/Source/LostRunic/AI/LRGuardAIControllerBehavior.cpp
+new file mode 100644
+index 0000000..e350d74
+--- /dev/null
++++ b/Source/LostRunic/AI/LRGuardAIControllerBehavior.cpp
+@@ -0,0 +1,225 @@
++/**
++ * @file LRGuardAIControllerBehavior.cpp
++ * @brief 守卫控制器行为实现：行为进出与移动驱动、警戒数据变化到 BehaviorChanged 的分派（仅实际变化时广播）、击退晕眩覆盖、巡逻与诊断。
++ *
++ * 关联文件：LRGuardAIController.h；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "AI/LRGuardAIController.h"
++
++#include "AI/LRAlertComponent.h"
++#include "AI/LRGuardCharacter.h"
++#include "Components/StateTreeAIComponent.h"
++#include "Core/LRGameplayTags.h"
++#include "Core/LRLog.h"
++#include "Data/LRGuardTuning.h"
++#include "Data/LRStateTuning.h"
++#include "DrawDebugHelpers.h"
++#include "Engine/World.h"
++#include "GameFramework/CharacterMovementComponent.h"
++#include "Navigation/PathFollowingComponent.h"
++#include "TimerManager.h"
++
++/**
++ * @brief 进入指定守卫行为，设置移动速度、焦点、导航目标；眩晕中仅接受 Stunned。
++ * @param behavior 要进入或退出的守卫 StateTree 行为状态。
++ */
++void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
++{
++	if (bStunned && behavior != ELRGuardBehaviorState::Stunned)
++	{
++		ActiveBehavior = ELRGuardBehaviorState::Stunned;
++		return;
++	}
++	ActiveBehavior = behavior;
++	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
++	if (!guard || !Alert.IsValid())
++	{
++		return;
++	}
++	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
++	if (behavior == ELRGuardBehaviorState::Chase)
++	{
++		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
++		SetFocus(Alert->GetTargetActor());
++		MoveToActor(Alert->GetTargetActor(), GetEffectiveTuning().CaptureRadius);
++	}
++	else if (behavior == ELRGuardBehaviorState::Investigate)
++	{
++		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
++		MoveToLocation(Alert->GetLastDisturbanceLocation(), GetEffectiveTuning().MoveAcceptanceRadius);
++	}
++	else if (behavior == ELRGuardBehaviorState::Search)
++	{
++		// 抵达观察与自然衰减由 AlertComponent 的观察计时与衰减计时驱动，不再使用固定搜索时长。
++		StopMovement();
++		SetFocalPoint(Alert->GetLastDisturbanceLocation());
++	}
++	else if (behavior == ELRGuardBehaviorState::Suspicious)
++	{
++		StopMovement();
++		SetFocalPoint(Alert->GetLastDisturbanceLocation());
++	}
++	else if (behavior == ELRGuardBehaviorState::Stunned)
++	{
++		StopMovement();
++		ClearFocus(EAIFocusPriority::Gameplay);
++	}
++	else
++	{
++		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
++		StartPatrolMove();
++	}
++}
++
++/**
++ * @brief 退出指定守卫行为并清理该状态拥有的导航、焦点或计时器。
++ * @param behavior 要进入或退出的守卫 StateTree 行为状态。
++ */
++void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
++{
++	StopMovement();
++	ClearFocus(EAIFocusPriority::Gameplay);
++}
++
++/**
++ * @brief 处理 On Move Completed 事件：调查抵达转入 Search（开始观察），巡逻点到达续走下一段。
++ * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
++ * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
++ */
++void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
++{
++	Super::OnMoveCompleted(requestId, result);
++	if (!result.IsSuccess() || !Alert.IsValid())
++	{
++		return;
++	}
++	if (ActiveBehavior == ELRGuardBehaviorState::Investigate)
++	{
++		Alert->MarkInvestigationReached();
++	}
++	else if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
++	{
++		++PatrolIndex;
++		StartPatrolMove();
++	}
++}
++
++/**
++ * @brief 处理 Handle Alert Changed 事件：仅表示感知/警戒数据变化；只有当权威解析结果实际变化时才广播 BehaviorChanged，避免衰减计时等数值变化导致 StateTree 无意义重入。
++ * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
++ * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
++ * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
++ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
++ * @param disturbanceLocation 空间值 `disturbanceLocation`；距离和位置使用 Unreal 厘米单位。
++ */
++void ALRGuardAIController::HandleAlertChanged(const int32 previousLevel, const int32 currentLevel,
++	const ELRGuardBehaviorState currentState, const FGameplayTag reason, const FVector disturbanceLocation)
++{
++	const ELRGuardBehaviorState resolved = GetResolvedBehavior();
++	if (resolved == ActiveBehavior)
++	{
++		// 同状态 Investigate 的数据级重定位（新调查点），不发行为事件。
++		if (resolved == ELRGuardBehaviorState::Investigate)
++		{
++			EnterBehavior(ELRGuardBehaviorState::Investigate);
++		}
++		return;
++	}
++	if (StateTreeAI->IsRunning())
++	{
++		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
++	}
++	else
++	{
++		EnterBehavior(resolved);
++	}
++}
++
++/**
++ * @brief 处理 Handle Knockback 事件：进入 Stunned 覆盖（停止移动、清除焦点），按 Courage 击退时长计时恢复。
++ * @param direction 击退方向 `direction`；仅用于诊断。
++ */
++void ALRGuardAIController::HandleKnockback(const FVector direction)
++{
++	if (bStunned)
++	{
++		return;
++	}
++	bStunned = true;
++	StopMovement();
++	ClearFocus(EAIFocusPriority::Gameplay);
++	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s stunned for %.2fs direction=%s"), *GetNameSafe(GetPawn()),
++		StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, *direction.ToCompactString());
++	if (StateTreeAI->IsRunning())
++	{
++		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
++	}
++	else
++	{
++		EnterBehavior(ELRGuardBehaviorState::Stunned);
++	}
++	if (GetWorld())
++	{
++		GetWorld()->GetTimerManager().SetTimer(StunTimer, this, &ALRGuardAIController::HandleStunEnd,
++			StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, false);
++	}
++}
++
++/**
++ * @brief 眩晕计时结束后按当前警戒与视线重新解析行为并广播恢复事件；感知与警戒在眩晕期间持续运行。
++ */
++void ALRGuardAIController::HandleStunEnd()
++{
++	bStunned = false;
++	const ELRGuardBehaviorState resolved = GetResolvedBehavior();
++	if (StateTreeAI->IsRunning())
++	{
++		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
++	}
++	else
++	{
++		EnterBehavior(resolved);
++	}
++}
++
++/**
++ * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
++ */
++void ALRGuardAIController::StartPatrolMove()
++{
++	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
++	if (!guard || guard->GetPatrolPointCount() == 0)
++	{
++		StopMovement();
++		return;
++	}
++	PatrolIndex %= guard->GetPatrolPointCount();
++	MoveToActor(guard->GetPatrolPoint(PatrolIndex), GetEffectiveTuning().MoveAcceptanceRadius);
++}
++
++/**
++ * @brief 输出守卫行为、警戒值、观察/眩晕状态和最后异常点，并按调试开关绘制视野与听觉范围。
++ */
++void ALRGuardAIController::LogAndDrawDiagnostics() const
++{
++	const APawn* guard = GetPawn();
++	if (!guard || !Alert.IsValid())
++	{
++		return;
++	}
++	const ULRGuardTuning& tuning = GetEffectiveTuning();
++	UE_LOG(LogLostRunicAI, Display,
++		TEXT("Guard=%s Alert=%d ResolvedState=%d Observing=%d Stunned=%d Target=%s Reason=%s Location=%s"),
++		*GetNameSafe(guard), Alert->GetAlertLevel(), static_cast<int32>(GetResolvedBehavior()),
++		Alert->IsObserving() ? 1 : 0, bStunned ? 1 : 0,
++		*GetNameSafe(Alert->GetTargetActor()), *Alert->GetLastReason().GetTagName().ToString(),
++		*Alert->GetLastDisturbanceLocation().ToCompactString());
++	const FVector origin = guard->GetActorLocation();
++	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
++		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
++		16, FColor::Yellow, false, 5.0f);
++	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
++	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
++}
+diff --git a/Source/LostRunic/AI/LRGuardAIControllerPerception.cpp b/Source/LostRunic/AI/LRGuardAIControllerPerception.cpp
+new file mode 100644
+index 0000000..255508d
+--- /dev/null
++++ b/Source/LostRunic/AI/LRGuardAIControllerPerception.cpp
+@@ -0,0 +1,148 @@
++/**
++ * @file LRGuardAIControllerPerception.cpp
++ * @brief 守卫控制器感知实现：Sight/Hearing 刺激转换为警戒语义入口、感知配置、遮挡与隐藏判定、捕获计时（眩晕期间跳过）。
++ *
++ * 关联文件：LRGuardAIController.h；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "AI/LRGuardAIController.h"
++
++#include "AI/LRAlertComponent.h"
++#include "AI/LRGuardCharacter.h"
++#include "AI/LRGuardPerceptionRules.h"
++#include "Core/LRGameplayTags.h"
++#include "Data/LRGuardTuning.h"
++#include "Engine/World.h"
++#include "GameFramework/CharacterMovementComponent.h"
++#include "Perception/AIPerceptionComponent.h"
++#include "Perception/AISense_Hearing.h"
++#include "Perception/AISense_Sight.h"
++#include "Perception/AISenseConfig_Hearing.h"
++#include "Perception/AISenseConfig_Sight.h"
++#include "Stealth/LRGuardVisibility.h"
++
++/**
++ * @brief 把 UE 感知刺激转换为可见/听见事件、异常位置和警戒原因标签；听觉走 ResolveNoiseAlertDelta 语义（吸引/Set 分派）。
++ * @param actor 本次查询、交互或事件涉及的 Actor。
++ * @param stimulus 时间值 `stimulus`，单位为秒。
++ */
++void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
++{
++	if (!actor || !Alert.IsValid())
++	{
++		return;
++	}
++	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
++	{
++		const bool bVisible = stimulus.WasSuccessfullySensed() && CanConfirmSight(actor);
++		Alert->SetSightTarget(actor, bVisible, stimulus.StimulusLocation);
++	}
++	else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
++	{
++		FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
++		if (!reason.IsValid())
++		{
++			reason = LRGameplayTags::NoiseInteraction;
++		}
++		const FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
++			reason, Alert->GetAlertLevel(), GetEffectiveTuning());
++		if (!response.bRespond)
++		{
++			return;
++		}
++		if (response.bIsAttract)
++		{
++			Alert->ApplyAttract(stimulus.StimulusLocation, actor, reason);
++		}
++		else
++		{
++			Alert->ApplyAlertDelta(response.Delta, stimulus.StimulusLocation, actor, reason);
++		}
++	}
++}
++
++/**
++ * @brief 用 Guard 调优资产配置 UE Sight/Hearing 感知，包括完整视野角换算、距离和阵营检测。
++ */
++void ALRGuardAIController::ConfigurePerception()
++{
++	const ULRGuardTuning& tuning = GetEffectiveTuning();
++	SightConfig->SightRadius = tuning.SightRadius;
++	SightConfig->LoseSightRadius = tuning.LoseSightRadius;
++	SightConfig->PeripheralVisionAngleDegrees = tuning.SightConeDegrees * 0.5f;
++	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
++	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
++	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
++	HearingConfig->HearingRange = tuning.MaxHearingRange * tuning.HearingRangeMultiplier;
++	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
++	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
++	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
++	AIPerception->ConfigureSense(*SightConfig);
++	AIPerception->ConfigureSense(*HearingConfig);
++	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
++}
++
++/**
++ * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程；眩晕期间跳过。
++ */
++void ALRGuardAIController::HandleCaptureTimer()
++{
++	if (bStunned || !Alert.IsValid() || Alert->GetBehaviorState() != ELRGuardBehaviorState::Chase)
++	{
++		return;
++	}
++	AActor* target = Alert->GetTargetActor();
++	if (!CanConfirmSight(target))
++	{
++		Alert->SetSightTarget(target, false, target ? target->GetActorLocation() : FVector::ZeroVector);
++		return;
++	}
++	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
++	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation()) <= GetEffectiveTuning().CaptureRadius)
++	{
++		guard->CaptureTarget(target);
++	}
++}
++
++/**
++ * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
++ * @param actor 本次查询、交互或事件涉及的 Actor。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++bool ALRGuardAIController::CanConfirmSight(AActor* actor) const
++{
++	const APawn* guardPawn = GetPawn();
++	if (!actor || !guardPawn)
++	{
++		return false;
++	}
++	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
++	const float distance = toTarget.Size2D();
++	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
++		toTarget.GetSafeNormal2D());
++	return LRGuardPerceptionRules::CanConfirmSight(distance, forwardDot, !LineOfSightTo(actor),
++		IsHiddenFromGuard(actor), GetEffectiveTuning());
++}
++
++/**
++ * @brief 判断 Is Hidden From Guard 对应条件；不产生玩法副作用。
++ * @param actor 本次查询、交互或事件涉及的 Actor。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
++{
++	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
++	{
++		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor, const_cast<ALRGuardAIController*>(this));
++	}
++	for (UActorComponent* component : actor->GetComponents())
++	{
++		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
++			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component, const_cast<ALRGuardAIController*>(this)))
++		{
++			return true;
++		}
++	}
++	return false;
++}
+diff --git a/Source/LostRunic/AI/LRGuardCharacter.cpp b/Source/LostRunic/AI/LRGuardCharacter.cpp
+index 890a040..96fdcec 100644
+--- a/Source/LostRunic/AI/LRGuardCharacter.cpp
++++ b/Source/LostRunic/AI/LRGuardCharacter.cpp
+@@ -10,12 +10,14 @@
+ 
+ #include "AI/LRAlertComponent.h"
+ #include "AI/LRGuardAIController.h"
++#include "Components/WidgetComponent.h"
+ #include "Core/LRGameplayTags.h"
+ #include "Engine/GameInstance.h"
+ #include "Framework/LRCharacter.h"
+ #include "Items/LRCourageResponseComponent.h"
+ #include "Save/LRSaveSubsystem.h"
+ #include "State/LRStateComponent.h"
++#include "UI/LRWorldAlertBarWidgetBase.h"
+ 
+ /**
+  * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+@@ -27,6 +29,26 @@ ALRGuardCharacter::ALRGuardCharacter()
+ 	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
+ 	Alert = CreateDefaultSubobject<ULRAlertComponent>(TEXT("Alert"));
+ 	CourageResponse = CreateDefaultSubobject<ULRCourageResponseComponent>(TEXT("CourageResponse"));
++	AlertWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertWidget"));
++	AlertWidget->SetupAttachment(GetMesh());
++	AlertWidget->SetWidgetSpace(EWidgetSpace::Screen);
++	AlertWidget->SetDrawSize(FVector2D(120.0f, 24.0f));
++	AlertWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
++}
++
++/**
++ * @brief 在进入世界后解析运行时依赖：将世界警戒条 Widget 初始化到本守卫的警戒快照。
++ */
++void ALRGuardCharacter::BeginPlay()
++{
++	Super::BeginPlay();
++	if (UUserWidget* widget = AlertWidget->GetWidget())
++	{
++		if (ULRWorldAlertBarWidgetBase* alertBar = Cast<ULRWorldAlertBarWidgetBase>(widget))
++		{
++			alertBar->InitializeForGuard(this);
++		}
++	}
+ }
+ 
+ /**
+diff --git a/Source/LostRunic/AI/LRGuardCharacter.h b/Source/LostRunic/AI/LRGuardCharacter.h
+index 330bec0..c2b9dfc 100644
+--- a/Source/LostRunic/AI/LRGuardCharacter.h
++++ b/Source/LostRunic/AI/LRGuardCharacter.h
+@@ -17,6 +17,7 @@ class ALRGuardAIController;
+ class ULRCourageResponseComponent;
+ class ULRAlertComponent;
+ class ULRGuardDefinition;
++class UWidgetComponent;
+ 
+ DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRPlayerCaptured, AActor*, playerActor);
+ 
+@@ -32,6 +33,11 @@ public:
+ 	 */
+ 	ALRGuardCharacter();
+ 
++	/**
++	 * @brief 在进入世界后解析运行时依赖：将世界警戒条 Widget 初始化到本守卫的警戒快照。
++	 */
++	virtual void BeginPlay() override;
++
+ 	/**
+ 	 * @brief 查询 Alert Component；不修改领域状态。
+ 	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+@@ -47,6 +53,20 @@ public:
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI")
+ 	bool CaptureTarget(AActor* target);
+ 
++	/**
++	 * @brief 查询 Definition；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
++	ULRGuardDefinition* GetDefinition() const { return Definition; }
++
++	/**
++	 * @brief 查询 Courage Response Component；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
++	ULRCourageResponseComponent* GetCourageResponseComponent() const { return CourageResponse; }
++
+ 	/**
+ 	 * @brief 查询 Patrol Point；不修改领域状态。
+ 	 * @param index 目标元素索引，调用前必须满足对应容器边界。
+@@ -80,4 +100,8 @@ private:
+ 	/** Courage Response 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
+ 	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
+ 	TObjectPtr<ULRCourageResponseComponent> CourageResponse;
++
++	/** Alert Widget 的世界空间 WidgetComponent；WidgetClass 与样式由蓝图配置，C++ 只负责初始化绑定。 仅在蓝图或详情面板中查看，不可编辑。 */
++	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
++	TObjectPtr<UWidgetComponent> AlertWidget;
+ };
+diff --git a/Source/LostRunic/AI/LRGuardPerceptionRules.cpp b/Source/LostRunic/AI/LRGuardPerceptionRules.cpp
+index bb98311..f9bfdf6 100644
+--- a/Source/LostRunic/AI/LRGuardPerceptionRules.cpp
++++ b/Source/LostRunic/AI/LRGuardPerceptionRules.cpp
+@@ -8,6 +8,7 @@
+  */
+ #include "AI/LRGuardPerceptionRules.h"
+ 
++#include "Core/LRGameplayTags.h"
+ #include "Data/LRGuardTuning.h"
+ 
+ /**
+@@ -37,3 +38,36 @@ bool LRGuardPerceptionRules::CanHear(const float distance, const float sourceRad
+ {
+ 	return distance <= sourceRadius * tuning.HearingRangeMultiplier;
+ }
++
++/**
++ * @brief 按噪声原因标签解析守卫应做的警戒响应；CD 与观察时序由调用方组件执行，本函数只做语义映射。
++ * @param reason 噪声原因 Gameplay Tag，例如 Noise.Footstep.Walk 或 Noise.Footstep.Run.Indoor。
++ * @param currentAlert 守卫当前警戒值 0-11。
++ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++ * @return 结构化响应：是否响应、Delta 与是否走吸引语义（IsAttract 时调用方使用带 CD 门控的 ApplyAttract）。
++ */
++FLRNoiseResponse LRGuardPerceptionRules::ResolveNoiseAlertDelta(const FGameplayTag reason, const int32 currentAlert,
++	const ULRGuardTuning& tuning)
++{
++	FLRNoiseResponse response;
++	if (reason == LRGameplayTags::NoiseFootstepRunIndoor)
++	{
++		// 室内奔跑为「警戒至少提升到 RoomRunAlertLevel」的 Set 语义，不走吸引 CD。
++		response.bRespond = true;
++		response.Delta = FMath::Max(tuning.RoomRunAlertLevel - currentAlert, 0);
++		response.bIsAttract = false;
++		return response;
++	}
++	if (reason == LRGameplayTags::NoiseFootstepWalkFaint)
++	{
++		// 室外非潜行关走路：只有警戒 >=6 的守卫才会被吸引。
++		response.bIsAttract = true;
++		response.bRespond = currentAlert >= tuning.SightInvestigateLevel;
++		response.Delta = response.bRespond ? tuning.AttractAlertAmount : 0;
++		return response;
++	}
++	response.bRespond = true;
++	response.Delta = tuning.AttractAlertAmount;
++	response.bIsAttract = true;
++	return response;
++}
+diff --git a/Source/LostRunic/AI/LRGuardPerceptionRules.h b/Source/LostRunic/AI/LRGuardPerceptionRules.h
+index 95674cd..f9e65be 100644
+--- a/Source/LostRunic/AI/LRGuardPerceptionRules.h
++++ b/Source/LostRunic/AI/LRGuardPerceptionRules.h
+@@ -8,8 +8,21 @@
+  */
+ #pragma once
+ 
++#include "GameplayTagContainer.h"
++
+ class ULRGuardTuning;
+ 
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++struct LOSTRUNIC_API FLRNoiseResponse
++{
++	/** Respond 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 */
++	bool bRespond = false;
++	/** Delta 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 */
++	int32 Delta = 0;
++	/** Is Attract 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 */
++	bool bIsAttract = false;
++};
++
+ namespace LRGuardPerceptionRules
+ {
+ 	LOSTRUNIC_API bool CanConfirmSight(float distance, float forwardDot, bool bOccluded, bool bHidden,
+@@ -22,4 +35,13 @@ namespace LRGuardPerceptionRules
+ 	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ 	 */
+ 	LOSTRUNIC_API bool CanHear(float distance, float sourceRadius, const ULRGuardTuning& tuning);
++	/**
++	 * @brief 按噪声原因标签解析守卫应做的警戒响应；CD 与观察时序由调用方组件执行，本函数只做语义映射。
++	 * @param reason 噪声原因 Gameplay Tag，例如 Noise.Footstep.Walk 或 Noise.Footstep.Run.Indoor。
++	 * @param currentAlert 守卫当前警戒值 0-11。
++	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++	 * @return 结构化响应：是否响应、Delta 与是否走吸引语义（IsAttract 时调用方使用带 CD 门控的 ApplyAttract）。
++	 */
++	LOSTRUNIC_API FLRNoiseResponse ResolveNoiseAlertDelta(FGameplayTag reason, int32 currentAlert,
++		const ULRGuardTuning& tuning);
+ }
+diff --git a/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp b/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp
+index 5d07b0d..d1c17f7 100644
+--- a/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp
++++ b/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp
+@@ -61,6 +61,6 @@ void FLRGuardBehaviorTask::ExitState(FStateTreeExecutionContext& context,
+ bool FLRGuardStateCondition::TestCondition(FStateTreeExecutionContext& context) const
+ {
+ 	const FInstanceDataType& data = context.GetInstanceData(*this);
+-	const ULRAlertComponent* alert = data.AIController ? data.AIController->GetAlertComponent() : nullptr;
+-	return alert && alert->GetBehaviorState() == ExpectedBehavior;
++	// StateTree 只执行控制器解析的结果，不自行重新定义警戒语义。
++	return data.AIController && data.AIController->GetResolvedBehavior() == ExpectedBehavior;
+ }
+diff --git a/Source/LostRunic/AI/LRGuardTypes.h b/Source/LostRunic/AI/LRGuardTypes.h
+index 82c73e7..90c8bca 100644
+--- a/Source/LostRunic/AI/LRGuardTypes.h
++++ b/Source/LostRunic/AI/LRGuardTypes.h
+@@ -20,5 +20,43 @@ enum class ELRGuardBehaviorState : uint8
+ 	Suspicious UMETA(DisplayName = "Suspicious"),
+ 	Investigate UMETA(DisplayName = "Investigate"),
+ 	Search UMETA(DisplayName = "Search"),
+-	Chase UMETA(DisplayName = "Chase")
++	Chase UMETA(DisplayName = "Chase"),
++	Stunned UMETA(DisplayName = "Stunned")
++};
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Alert Tier"))
++enum class ELRGuardAlertTier : uint8
++{
++	Hidden UMETA(DisplayName = "Hidden"),
++	White UMETA(DisplayName = "White"),
++	Red UMETA(DisplayName = "Red"),
++	Full UMETA(DisplayName = "Full")
++};
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Alert Snapshot"))
++struct LOSTRUNIC_API FLRAlertSnapshot
++{
++	GENERATED_BODY()
++
++	/** Level 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 蓝图可读取但不可写入。 */
++	UPROPERTY(BlueprintReadOnly, Category = "Alert")
++	int32 Level = 0;
++
++	/** Fraction 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.0f`。 蓝图可读取但不可写入。 */
++	UPROPERTY(BlueprintReadOnly, Category = "Alert")
++	float Fraction = 0.0f;
++
++	/** Tier 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardAlertTier::Hidden`。 蓝图可读取但不可写入。 */
++	UPROPERTY(BlueprintReadOnly, Category = "Alert")
++	ELRGuardAlertTier Tier = ELRGuardAlertTier::Hidden;
++
++	/** Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardBehaviorState::IdlePatrol`。 蓝图可读取但不可写入。 */
++	UPROPERTY(BlueprintReadOnly, Category = "Alert")
++	ELRGuardBehaviorState Behavior = ELRGuardBehaviorState::IdlePatrol;
++
++	/** Full Alert 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
++	UPROPERTY(BlueprintReadOnly, Category = "Alert")
++	bool bFullAlert = false;
+ };
+diff --git a/Source/LostRunic/AI/LRNPCCharacter.cpp b/Source/LostRunic/AI/LRNPCCharacter.cpp
+new file mode 100644
+index 0000000..49da714
+--- /dev/null
++++ b/Source/LostRunic/AI/LRNPCCharacter.cpp
+@@ -0,0 +1,141 @@
++/**
++ * @file LRNPCCharacter.cpp
++ * @brief 通用非战斗 NPC 实现：对话交互（Talk 选项经 ULRDialogueSubsystem::StartDialogue）与噪声表现钩子；行为由 StateTree/控制器驱动。
++ *
++ * 关联文件：LRNPCCharacter.h；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "AI/LRNPCCharacter.h"
++
++#include "AI/LRNPCController.h"
++#include "Core/LRGameplayTags.h"
++#include "Core/LRLog.h"
++#include "Data/LRNPCDefinition.h"
++#include "Engine/GameInstance.h"
++#include "Narrative/LRDialogueSubsystem.h"
++
++/**
++ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++ */
++ALRNPCCharacter::ALRNPCCharacter()
++{
++	PrimaryActorTick.bCanEverTick = false;
++	AIControllerClass = ALRNPCController::StaticClass();
++	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
++}
++
++/**
++ * @brief 查询 Patrol Point；不修改领域状态。
++ * @param index 目标元素索引，调用前必须满足对应容器边界。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++AActor* ALRNPCCharacter::GetPatrolPoint(const int32 index) const
++{
++	return PatrolPoints.IsValidIndex(index) ? PatrolPoints[index].Get() : nullptr;
++}
++
++/**
++ * @brief 通知 NPC 听见噪声：触发表现钩子与预留委托；Conversation 高优先级时由控制器决定是否切换行为。
++ * @param location 世界空间位置，Unreal 单位为厘米。
++ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
++ */
++void ALRNPCCharacter::NotifyNoiseHeard(const FVector location, const FGameplayTag reason)
++{
++	OnNoiseHeard(location, reason);
++	OnNPCAttentionChanged.Broadcast(location, reason);
++}
++
++/**
++ * @brief 查询当前 NPC 行为（由控制器权威解析）；不修改领域状态。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRNPCBehaviorState ALRNPCCharacter::GetActiveBehavior() const
++{
++	const ALRNPCController* controller = Cast<ALRNPCController>(GetController());
++	return controller ? controller->GetActiveBehavior() : ELRNPCBehaviorState::Idle;
++}
++
++/**
++ * @brief 查询 Interaction Options：仅 Talk（对话），要求 Normal 状态。
++ * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++TArray<FLRInteractionOption> ALRNPCCharacter::GetInteractionOptions_Implementation(AActor* interactor)
++{
++	TArray<FLRInteractionOption> options;
++	if (Definition && !Definition->DialogueRowId.IsNone())
++	{
++		FLRInteractionOption option;
++		option.ActionTag = LRGameplayTags::InteractionActionTalk;
++		option.Prompt = NSLOCTEXT("LostRunic", "NPC.TalkPrompt", "对话");
++		option.RequiredMode = ELRPerceptionMode::Normal;
++		options.Add(option);
++	}
++	return options;
++}
++
++/**
++ * @brief 查询 Interaction Location；不修改领域状态。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++FVector ALRNPCCharacter::GetInteractionLocation_Implementation()
++{
++	return GetActorLocation();
++}
++
++/**
++ * @brief 实现 Execute Interaction 对应的领域步骤：Talk 经 ULRDialogueSubsystem::StartDialogue 启动对话并进入 Conversation；对话结束回到默认行为。
++ * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
++ * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++FLRInteractionResult ALRNPCCharacter::ExecuteInteraction_Implementation(AActor* interactor, const FGameplayTag actionTag)
++{
++	FLRInteractionResult result;
++	result.ActionTag = actionTag;
++	if (actionTag != LRGameplayTags::InteractionActionTalk)
++	{
++		result.FailureReason = LRGameplayTags::InteractionRejectState;
++		return result;
++	}
++	ULRDialogueSubsystem* dialogue = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
++	if (!dialogue || !Definition || Definition->DialogueRowId.IsNone())
++	{
++		result.FailureReason = LRGameplayTags::NarrativeRejectMissingContent;
++		return result;
++	}
++	const FLRNarrativeResult narrative = dialogue->StartDialogue(Definition->DialogueRowId);
++	if (!narrative.bSuccess)
++	{
++		result.FailureReason = narrative.FailureReason;
++		return result;
++	}
++	if (ALRNPCController* controller = Cast<ALRNPCController>(GetController()))
++	{
++		controller->NotifyDialogueStarted();
++	}
++	dialogue->OnSessionEnded.AddUniqueDynamic(this, &ALRNPCCharacter::HandleDialogueSessionEnded);
++	result.bSuccess = true;
++	return result;
++}
++
++/**
++ * @brief 处理 Handle Dialogue Session Ended 事件，将引擎回调转换为对应领域状态更新。
++ * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
++ * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
++ */
++void ALRNPCCharacter::HandleDialogueSessionEnded(const ELRNarrativeSessionType sessionType, const FName contentId)
++{
++	if (ALRNPCController* controller = Cast<ALRNPCController>(GetController()))
++	{
++		controller->NotifyDialogueEnded();
++	}
++	if (UGameInstance* gameInstance = GetGameInstance())
++	{
++		if (ULRDialogueSubsystem* dialogue = gameInstance->GetSubsystem<ULRDialogueSubsystem>())
++		{
++			dialogue->OnSessionEnded.RemoveDynamic(this, &ALRNPCCharacter::HandleDialogueSessionEnded);
++		}
++	}
++}
+diff --git a/Source/LostRunic/AI/LRNPCCharacter.h b/Source/LostRunic/AI/LRNPCCharacter.h
+new file mode 100644
+index 0000000..6188982
+--- /dev/null
++++ b/Source/LostRunic/AI/LRNPCCharacter.h
+@@ -0,0 +1,99 @@
++/**
++ * @file LRNPCCharacter.h
++ * @brief 通用非战斗 NPC：由 StateTree（Idle/Patrol/ReactToNoise/Conversation）驱动；实现对话交互（Talk 选项经 ULRDialogueSubsystem::StartDialogue）与噪声表现钩子（OnNoiseHeard / OnNPCAttentionChanged 预留未来告警/逃离）。
++ *
++ * 关联文件：LRNPCCharacter.cpp；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "AI/LRNPCTypes.h"
++#include "GameFramework/Character.h"
++#include "Interaction/LRInteractable.h"
++
++#include "LRNPCCharacter.generated.h"
++
++class ALRNPCController;
++class ULRNPCDefinition;
++class ULRDialogueSubsystem;
++
++DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNPCAttentionChanged, FVector, location, FGameplayTag, reason);
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Character"))
++class LOSTRUNIC_API ALRNPCCharacter : public ACharacter, public ILRInteractable
++{
++	GENERATED_BODY()
++
++public:
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 */
++	ALRNPCCharacter();
++
++	/**
++	 * @brief 查询 Definition；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|NPC")
++	ULRNPCDefinition* GetDefinition() const { return Definition; }
++
++	/**
++	 * @brief 查询 Patrol Point；不修改领域状态。
++	 * @param index 目标元素索引，调用前必须满足对应容器边界。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	AActor* GetPatrolPoint(int32 index) const;
++	/**
++	 * @brief 查询 Patrol Point Count；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	int32 GetPatrolPointCount() const { return PatrolPoints.Num(); }
++
++	/**
++	 * @brief 通知 NPC 听见噪声：触发表现钩子与预留委托；Conversation 高优先级时由控制器决定是否切换行为。
++	 * @param location 世界空间位置，Unreal 单位为厘米。
++	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
++	 */
++	void NotifyNoiseHeard(const FVector location, const FGameplayTag reason);
++
++	/**
++	 * @brief 查询当前 NPC 行为（由控制器权威解析）；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|NPC")
++	ELRNPCBehaviorState GetActiveBehavior() const;
++
++	//~ ILRInteractable
++	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;
++	virtual FVector GetInteractionLocation_Implementation() override;
++	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor, FGameplayTag actionTag) override;
++	//~ End ILRInteractable
++
++	/** 当 Noise Heard 发生时广播；蓝图可绑定该委托以更新表现（转向、表情等），不应在回调中改写核心规则。  */
++	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|NPC")
++	void OnNoiseHeard(const FVector location, const FGameplayTag reason);
++
++	/** 当 NPC Attention Changed 发生时广播；预留未来告警/逃离扩展钩子，本次不实现告警逻辑。  */
++	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|NPC")
++	FLRNPCAttentionChanged OnNPCAttentionChanged;
++
++protected:
++	/** Definition 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
++	TObjectPtr<ULRNPCDefinition> Definition;
++
++	/** Patrol Points 的领域数据，由所属类型负责维护和校验。 可在关卡中的蓝图实例详情面板配置。 */
++	UPROPERTY(EditInstanceOnly, Category = "NPC|Patrol")
++	TArray<TObjectPtr<AActor>> PatrolPoints;
++
++private:
++	/**
++	 * @brief 处理 Handle Dialogue Session Ended 事件，将引擎回调转换为对应领域状态更新。
++	 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
++	 * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
++	 */
++	UFUNCTION()
++	void HandleDialogueSessionEnded(ELRNarrativeSessionType sessionType, FName contentId);
++};
+diff --git a/Source/LostRunic/AI/LRNPCController.cpp b/Source/LostRunic/AI/LRNPCController.cpp
+new file mode 100644
+index 0000000..036702e
+--- /dev/null
++++ b/Source/LostRunic/AI/LRNPCController.cpp
+@@ -0,0 +1,375 @@
++/**
++ * @file LRNPCController.cpp
++ * @brief 通用 NPC 控制器实现：Hearing 感知、StateTree 生命周期、巡逻、低频玩家朝向与限时噪声反应。
++ *
++ * 关联文件：LRNPCController.h；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "AI/LRNPCController.h"
++
++#include "AI/LRNPCCharacter.h"
++#include "Components/StateTreeAIComponent.h"
++#include "Core/LRGameplayTags.h"
++#include "Core/LRLog.h"
++#include "Data/LRGameTuningSet.h"
++#include "Data/LRNPCDefinition.h"
++#include "Data/LRNPCTuning.h"
++#include "Engine/GameInstance.h"
++#include "Engine/World.h"
++#include "Framework/LRGameInstanceSubsystem.h"
++#include "GameFramework/CharacterMovementComponent.h"
++#include "Navigation/PathFollowingComponent.h"
++#include "Perception/AIPerceptionComponent.h"
++#include "Perception/AISense_Hearing.h"
++#include "Perception/AISenseConfig_Hearing.h"
++#include "TimerManager.h"
++
++/**
++ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++ */
++ALRNPCController::ALRNPCController()
++{
++	PrimaryActorTick.bCanEverTick = false;
++	bStartAILogicOnPossess = true;
++	bStopAILogicOnUnposses = true;
++	bAttachToPawn = true;
++	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
++	StateTreeAI->SetStartLogicAutomatically(false);
++	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
++	SetPerceptionComponent(*AIPerception);
++	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
++}
++
++/**
++ * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
++ */
++void ALRNPCController::BeginPlay()
++{
++	Super::BeginPlay();
++	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
++	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
++	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->NPC : nullptr;
++	if (!ensureMsgf(Tuning, TEXT("%s requires NPC tuning."), *GetNameSafe(this)))
++	{
++		return;
++	}
++	HearingConfig->HearingRange = 5000.0f;
++	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
++	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
++	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
++	AIPerception->ConfigureSense(*HearingConfig);
++	AIPerception->SetDominantSense(HearingConfig->GetSenseImplementation());
++	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRNPCController::HandlePerception);
++}
++
++/**
++ * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
++ * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
++ */
++void ALRNPCController::EndPlay(const EEndPlayReason::Type endPlayReason)
++{
++	if (AIPerception)
++	{
++		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRNPCController::HandlePerception);
++	}
++	if (GetWorld())
++	{
++		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
++		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
++	}
++	Super::EndPlay(endPlayReason);
++}
++
++/**
++ * @brief 处理 On Possess 事件：解析定义、SetStateTree 后 StartLogic，并绑定 Hearing 感知。
++ * @param inPawn Controller 新接管的 Pawn；期望为 ALRNPCCharacter。
++ */
++void ALRNPCController::OnPossess(APawn* inPawn)
++{
++	Super::OnPossess(inPawn);
++	Npc = Cast<ALRNPCCharacter>(inPawn);
++	Definition = Npc.IsValid() ? Npc->GetDefinition() : nullptr;
++	if (Definition.IsValid() && Definition->Behavior)
++	{
++		StateTreeAI->SetStateTree(Definition->Behavior);
++		if (!StateTreeAI->IsRunning())
++		{
++			StateTreeAI->StartLogic();
++		}
++	}
++	else
++	{
++		UE_LOG(LogLostRunicAI, Warning, TEXT("NPC=%s definition or Behavior StateTree is missing; using controller fallback."),
++			*GetNameSafe(inPawn));
++	}
++}
++
++/**
++ * @brief 处理 On Un Possess 事件：解绑感知并停止 StateTree 逻辑。
++ */
++void ALRNPCController::OnUnPossess()
++{
++	if (GetWorld())
++	{
++		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
++		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
++	}
++	if (StateTreeAI->IsRunning())
++	{
++		StateTreeAI->StopLogic(TEXT("OnUnPossess"));
++	}
++	Npc.Reset();
++	Definition.Reset();
++	Super::OnUnPossess();
++}
++
++/**
++ * @brief 处理 On Move Completed 事件：巡逻点到达续走下一段。
++ * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
++ * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
++ */
++void ALRNPCController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
++{
++	Super::OnMoveCompleted(requestId, result);
++	if (!result.IsSuccess())
++	{
++		return;
++	}
++	if (ActiveBehavior == ELRNPCBehaviorState::Patrol)
++	{
++		++PatrolIndex;
++		StartPatrolMove();
++	}
++}
++
++/**
++ * @brief 进入指定 NPC 行为：Idle 启动玩家朝向检测、Patrol 巡逻、ReactToNoise 转向声源限时反应、Conversation 停止一切反应。
++ * @param behavior 要进入或退出的 NPC StateTree 行为状态。
++ */
++void ALRNPCController::EnterBehavior(const ELRNPCBehaviorState behavior)
++{
++	ActiveBehavior = behavior;
++	ALRNPCCharacter* npc = Npc.Get();
++	if (!npc)
++	{
++		return;
++	}
++	switch (behavior)
++	{
++	case ELRNPCBehaviorState::Idle:
++		StopMovement();
++		ClearFocus(EAIFocusPriority::Gameplay);
++		StartLookAtTimer();
++		break;
++	case ELRNPCBehaviorState::Patrol:
++		StopLookAtTimer();
++		npc->GetCharacterMovement()->MaxWalkSpeed = GetEffectiveTuning().PatrolSpeedCm;
++		StartPatrolMove();
++		break;
++	case ELRNPCBehaviorState::ReactToNoise:
++		StopLookAtTimer();
++		StopMovement();
++		SetFocalPoint(LastNoiseLocation);
++		StartNoiseReaction(LastNoiseLocation);
++		break;
++	case ELRNPCBehaviorState::Conversation:
++		StopLookAtTimer();
++		StopNoiseReaction();
++		StopMovement();
++		ClearFocus(EAIFocusPriority::Gameplay);
++		break;
++	}
++}
++
++/**
++ * @brief 退出指定 NPC 行为并清理该状态拥有的导航、焦点或计时器。
++ * @param behavior 要进入或退出的 NPC StateTree 行为状态。
++ */
++void ALRNPCController::ExitBehavior(const ELRNPCBehaviorState behavior)
++{
++	StopLookAtTimer();
++	StopNoiseReaction();
++	StopMovement();
++	ClearFocus(EAIFocusPriority::Gameplay);
++}
++
++/**
++ * @brief 启动/停止 Idle 低频玩家朝向检测（任务节点调用）。
++ */
++void ALRNPCController::StartLookAtTimer()
++{
++	if (!GetWorld() || GetWorld()->GetTimerManager().IsTimerActive(LookAtTimer))
++	{
++		return;
++	}
++	GetWorld()->GetTimerManager().SetTimer(LookAtTimer, this, &ALRNPCController::HandleLookAtTimer,
++		GetEffectiveTuning().LookAtIntervalSeconds, true);
++}
++
++/**
++ * @brief 停止 Idle 低频玩家朝向检测（任务节点调用）。
++ */
++void ALRNPCController::StopLookAtTimer()
++{
++	if (GetWorld())
++	{
++		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
++	}
++}
++
++/**
++ * @brief 开始限时噪声反应：转向声源并按 NoiseReactionDurationSeconds 计时，结束后发送 NPCReactionEnded 事件。
++ * @param location 世界空间位置，Unreal 单位为厘米。
++ */
++void ALRNPCController::StartNoiseReaction(const FVector location)
++{
++	LastNoiseLocation = location;
++	if (!GetWorld())
++	{
++		return;
++	}
++	GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
++	GetWorld()->GetTimerManager().SetTimer(ReactionTimer, this, &ALRNPCController::HandleReactionTimeout,
++		GetEffectiveTuning().NoiseReactionDurationSeconds, false);
++}
++
++/**
++ * @brief 结束噪声反应计时（任务退出时调用）。
++ */
++void ALRNPCController::StopNoiseReaction()
++{
++	if (GetWorld())
++	{
++		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
++	}
++}
++
++/**
++ * @brief 对话开始：进入 Conversation（高优先级），停止一切反应。
++ */
++void ALRNPCController::NotifyDialogueStarted()
++{
++	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCDialogueStarted, ELRNPCBehaviorState::Conversation);
++}
++
++/**
++ * @brief 对话结束：回到配置的默认行为。
++ */
++void ALRNPCController::NotifyDialogueEnded()
++{
++	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCDialogueEnded, GetBaseBehavior());
++}
++
++/**
++ * @brief 把 UE 听觉刺激转换为噪声反应；Conversation 期间只触发表现钩子，不切换行为。
++ * @param actor 本次查询、交互或事件涉及的 Actor。
++ * @param stimulus 时间值 `stimulus`，单位为秒。
++ */
++void ALRNPCController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
++{
++	if (!actor || !Npc.IsValid() || stimulus.Type != UAISense::GetSenseID<UAISense_Hearing>()
++		|| !stimulus.WasSuccessfullySensed())
++	{
++		return;
++	}
++	FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
++	if (!reason.IsValid())
++	{
++		reason = LRGameplayTags::NoiseInteraction;
++	}
++	LastNoiseLocation = stimulus.StimulusLocation;
++	Npc->NotifyNoiseHeard(stimulus.StimulusLocation, reason);
++	// Conversation 为高优先级行为：普通噪声不打断对话（表现钩子已触发）。
++	if (ActiveBehavior == ELRNPCBehaviorState::Conversation)
++	{
++		return;
++	}
++	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCNoiseHeard, ELRNPCBehaviorState::ReactToNoise);
++}
++
++/**
++ * @brief 处理 Handle Look At Timer 事件：距离、视线与可达性满足时朝向玩家。
++ */
++void ALRNPCController::HandleLookAtTimer()
++{
++	ALRNPCCharacter* npc = Npc.Get();
++	if (!npc || !GetWorld())
++	{
++		return;
++	}
++	const APawn* playerPawn = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
++	if (!playerPawn)
++	{
++		return;
++	}
++	const float distance = FVector::Dist2D(npc->GetActorLocation(), playerPawn->GetActorLocation());
++	if (distance <= GetEffectiveTuning().LookAtPlayerRadiusCm && LineOfSightTo(playerPawn))
++	{
++		const FVector toPlayer = playerPawn->GetActorLocation() - npc->GetActorLocation();
++		npc->SetActorRotation(FRotator(0.0f, toPlayer.Rotation().Yaw, 0.0f));
++	}
++}
++
++/**
++ * @brief 处理 Handle Reaction Timeout 事件：噪声反应结束，发送 NPCReactionEnded。
++ */
++void ALRNPCController::HandleReactionTimeout()
++{
++	if (ActiveBehavior != ELRNPCBehaviorState::ReactToNoise)
++	{
++		return;
++	}
++	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCReactionEnded, GetBaseBehavior());
++}
++
++/**
++ * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
++ */
++void ALRNPCController::StartPatrolMove()
++{
++	ALRNPCCharacter* npc = Npc.Get();
++	if (!npc || npc->GetPatrolPointCount() == 0)
++	{
++		StopMovement();
++		return;
++	}
++	PatrolIndex %= npc->GetPatrolPointCount();
++	MoveToActor(npc->GetPatrolPoint(PatrolIndex), 50.0f);
++}
++
++/**
++ * @brief 解析配置的默认行为到运行态枚举。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRNPCBehaviorState ALRNPCController::GetBaseBehavior() const
++{
++	return Definition.IsValid() && Definition->DefaultBehavior == ENPCBaseBehavior::Patrol
++		? ELRNPCBehaviorState::Patrol : ELRNPCBehaviorState::Idle;
++}
++
++/**
++ * @brief 向 StateTree 发送行为事件；树未运行时直接进入行为。
++ * @param event 本次领域操作的结构化数据 `event`；字段语义由对应 USTRUCT 定义。
++ * @param behavior 要进入或退出的 NPC StateTree 行为状态。
++ */
++void ALRNPCController::DispatchBehaviorEvent(const FGameplayTag event, const ELRNPCBehaviorState behavior)
++{
++	if (StateTreeAI->IsRunning())
++	{
++		StateTreeAI->SendStateTreeEvent(event, FConstStructView(), FName());
++	}
++	else
++	{
++		EnterBehavior(behavior);
++	}
++}
++
++/**
++ * @brief 查询 Effective Tuning；不修改领域状态。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++const ULRNPCTuning& ALRNPCController::GetEffectiveTuning() const
++{
++	return Tuning ? *Tuning : *GetDefault<ULRNPCTuning>();
++}
+diff --git a/Source/LostRunic/AI/LRNPCController.h b/Source/LostRunic/AI/LRNPCController.h
+new file mode 100644
+index 0000000..b970b3a
+--- /dev/null
++++ b/Source/LostRunic/AI/LRNPCController.h
+@@ -0,0 +1,182 @@
++/**
++ * @file LRNPCController.h
++ * @brief 通用 NPC 控制器：Hearing 感知驱动噪声反应（Conversation 高优先级不被打断）、StateTree 生命周期（OnPossess 解析定义后启动）、巡逻与低频玩家朝向检测；不实现第二套计时器状态机。
++ *
++ * 关联文件：LRNPCController.cpp；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "AI/LRNPCTypes.h"
++#include "AIController.h"
++#include "GameplayTagContainer.h"
++#include "Perception/AIPerceptionTypes.h"
++
++#include "LRNPCController.generated.h"
++
++class ALRNPCCharacter;
++class UAIPerceptionComponent;
++class UAISenseConfig_Hearing;
++class ULRNPCDefinition;
++class ULRNPCTuning;
++class UStateTreeAIComponent;
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC AI Controller"))
++class LOSTRUNIC_API ALRNPCController : public AAIController
++{
++	GENERATED_BODY()
++
++public:
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 */
++	ALRNPCController();
++
++	/**
++	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
++	 */
++	virtual void BeginPlay() override;
++	/**
++	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
++	 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
++	 */
++	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
++	/**
++	 * @brief 处理 On Possess 事件：解析定义、SetStateTree 后 StartLogic，并绑定 Hearing 感知。
++	 * @param inPawn Controller 新接管的 Pawn；期望为 ALRNPCCharacter。
++	 */
++	virtual void OnPossess(APawn* inPawn) override;
++	/**
++	 * @brief 处理 On Un Possess 事件：解绑感知并停止 StateTree 逻辑。
++	 */
++	virtual void OnUnPossess() override;
++	/**
++	 * @brief 处理 On Move Completed 事件：巡逻点到达续走下一段。
++	 * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
++	 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
++	 */
++	virtual void OnMoveCompleted(FAIRequestID requestId, const FPathFollowingResult& result) override;
++
++	/**
++	 * @brief 进入指定 NPC 行为：Idle 启动玩家朝向检测、Patrol 巡逻、ReactToNoise 转向声源限时反应、Conversation 停止一切反应。
++	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
++	 */
++	void EnterBehavior(ELRNPCBehaviorState behavior);
++	/**
++	 * @brief 退出指定 NPC 行为并清理该状态拥有的导航、焦点或计时器。
++	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
++	 */
++	void ExitBehavior(ELRNPCBehaviorState behavior);
++	/**
++	 * @brief 查询 Active Behavior；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	ELRNPCBehaviorState GetActiveBehavior() const { return ActiveBehavior; }
++
++	/**
++	 * @brief 启动/停止 Idle 低频玩家朝向检测（任务节点调用）。
++	 */
++	void StartLookAtTimer();
++	/**
++	 * @brief 停止 Idle 低频玩家朝向检测（任务节点调用）。
++	 */
++	void StopLookAtTimer();
++	/**
++	 * @brief 开始限时噪声反应：转向声源并按 NoiseReactionDurationSeconds 计时，结束后发送 NPCReactionEnded 事件。
++	 * @param location 世界空间位置，Unreal 单位为厘米。
++	 */
++	void StartNoiseReaction(const FVector location);
++	/**
++	 * @brief 结束噪声反应计时（任务退出时调用）。
++	 */
++	void StopNoiseReaction();
++
++	/**
++	 * @brief 对话开始：进入 Conversation（高优先级），停止一切反应。
++	 */
++	void NotifyDialogueStarted();
++	/**
++	 * @brief 对话结束：回到配置的默认行为。
++	 */
++	void NotifyDialogueEnded();
++	/**
++	 * @brief 查询 Last Noise Location；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	FVector GetLastNoiseLocation() const { return LastNoiseLocation; }
++
++private:
++	/**
++	 * @brief 把 UE 听觉刺激转换为噪声反应；Conversation 期间只触发表现钩子，不切换行为。
++	 * @param actor 本次查询、交互或事件涉及的 Actor。
++	 * @param stimulus 时间值 `stimulus`，单位为秒。
++	 */
++	UFUNCTION()
++	void HandlePerception(AActor* actor, FAIStimulus stimulus);
++
++	/**
++	 * @brief 处理 Handle Look At Timer 事件：距离、视线与可达性满足时朝向玩家。
++	 */
++	void HandleLookAtTimer();
++	/**
++	 * @brief 处理 Handle Reaction Timeout 事件：噪声反应结束，发送 NPCReactionEnded。
++	 */
++	void HandleReactionTimeout();
++	/**
++	 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
++	 */
++	void StartPatrolMove();
++	/**
++	 * @brief 解析配置的默认行为到运行态枚举。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	ELRNPCBehaviorState GetBaseBehavior() const;
++	/**
++	 * @brief 向 StateTree 发送行为事件；树未运行时直接进入行为。
++	 * @param event 本次领域操作的结构化数据 `event`；字段语义由对应 USTRUCT 定义。
++	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
++	 */
++	void DispatchBehaviorEvent(const FGameplayTag event, ELRNPCBehaviorState behavior);
++	/**
++	 * @brief 查询 Effective Tuning；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	const ULRNPCTuning& GetEffectiveTuning() const;
++
++	/** State Tree AI 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
++	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
++	TObjectPtr<UStateTreeAIComponent> StateTreeAI;
++
++	/** AIPerception 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
++	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
++	TObjectPtr<UAIPerceptionComponent> AIPerception;
++
++	/** Hearing Config 的领域数据，由所属类型负责维护和校验。  */
++	UPROPERTY()
++	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
++
++	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TObjectPtr<ULRNPCTuning> Tuning;
++
++	/** Definition 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TWeakObjectPtr<ULRNPCDefinition> Definition;
++
++	/** Npc 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TWeakObjectPtr<ALRNPCCharacter> Npc;
++
++	/** Active Behavior 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	ELRNPCBehaviorState ActiveBehavior = ELRNPCBehaviorState::Idle;
++	/** Patrol Index 的内部运行时数据；不参与蓝图配置。 */
++	int32 PatrolIndex = 0;
++	/** Last Noise Location 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	FVector LastNoiseLocation = FVector::ZeroVector;
++	/** Look At Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
++	FTimerHandle LookAtTimer;
++	/** Reaction Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
++	FTimerHandle ReactionTimer;
++};
+diff --git a/Source/LostRunic/AI/LRNPCStateTreeNodes.cpp b/Source/LostRunic/AI/LRNPCStateTreeNodes.cpp
+new file mode 100644
+index 0000000..cc5f4a8
+--- /dev/null
++++ b/Source/LostRunic/AI/LRNPCStateTreeNodes.cpp
+@@ -0,0 +1,146 @@
++/**
++ * @file LRNPCStateTreeNodes.cpp
++ * @brief 通用 NPC 的 StateTree 节点实现：行为任务/条件、玩家朝向任务与限时噪声反应任务。
++ *
++ * 关联文件：LRNPCStateTreeNodes.h；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "AI/LRNPCStateTreeNodes.h"
++
++#include "AI/LRNPCController.h"
++#include "StateTreeExecutionContext.h"
++
++/**
++ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++ */
++FLRNPCBehaviorTask::FLRNPCBehaviorTask()
++{
++	bShouldCallTick = false;
++}
++
++/**
++ * @brief StateTree 进入节点时让 NPC 控制器进入配置的行为状态。
++ * @param context 当前 StateTree 执行上下文，用于读取实例数据。
++ * @param transition 触发本次进入的状态转换结果。
++ * @return 返回 Running，使行为持续到 StateTree 条件触发下一次转换。
++ */
++EStateTreeRunStatus FLRNPCBehaviorTask::EnterState(FStateTreeExecutionContext& context,
++	const FStateTreeTransitionResult& transition) const
++{
++	const FInstanceDataType& data = context.GetInstanceData(*this);
++	if (!data.AIController)
++	{
++		return EStateTreeRunStatus::Failed;
++	}
++	data.AIController->EnterBehavior(Behavior);
++	return EStateTreeRunStatus::Running;
++}
++
++/**
++ * @brief StateTree 离开节点时通知 NPC 控制器清理行为拥有的导航、焦点和计时器。
++ * @param context 当前 StateTree 执行上下文。
++ * @param transition 触发本次退出的状态转换结果。
++ */
++void FLRNPCBehaviorTask::ExitState(FStateTreeExecutionContext& context,
++	const FStateTreeTransitionResult& transition) const
++{
++	const FInstanceDataType& data = context.GetInstanceData(*this);
++	if (data.AIController)
++	{
++		data.AIController->ExitBehavior(Behavior);
++	}
++}
++
++/**
++ * @brief 比较当前 NPC 行为与 StateTree 条件配置，决定该分支是否可进入；只执行控制器解析结果。
++ * @param context 用于本次条件匹配的 `context` 标签或上下文。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++bool FLRNPCStateCondition::TestCondition(FStateTreeExecutionContext& context) const
++{
++	const FInstanceDataType& data = context.GetInstanceData(*this);
++	return data.AIController && data.AIController->GetActiveBehavior() == ExpectedBehavior;
++}
++
++/**
++ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++ */
++FLRNPCLookAtPlayerTask::FLRNPCLookAtPlayerTask()
++{
++	bShouldCallTick = false;
++}
++
++/**
++ * @brief 进入 Idle 时启动低频玩家朝向检测计时器。
++ * @param context 当前 StateTree 执行上下文。
++ * @param transition 触发本次进入的状态转换结果。
++ * @return 返回 Running，使检测持续到离开 Idle。
++ */
++EStateTreeRunStatus FLRNPCLookAtPlayerTask::EnterState(FStateTreeExecutionContext& context,
++	const FStateTreeTransitionResult& transition) const
++{
++	const FInstanceDataType& data = context.GetInstanceData(*this);
++	if (!data.AIController)
++	{
++		return EStateTreeRunStatus::Failed;
++	}
++	data.AIController->StartLookAtTimer();
++	return EStateTreeRunStatus::Running;
++}
++
++/**
++ * @brief 离开 Idle 时停止朝向检测计时器。
++ * @param context 当前 StateTree 执行上下文。
++ * @param transition 触发本次退出的状态转换结果。
++ */
++void FLRNPCLookAtPlayerTask::ExitState(FStateTreeExecutionContext& context,
++	const FStateTreeTransitionResult& transition) const
++{
++	const FInstanceDataType& data = context.GetInstanceData(*this);
++	if (data.AIController)
++	{
++		data.AIController->StopLookAtTimer();
++	}
++}
++
++/**
++ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++ */
++FLRNPCReactToNoiseTask::FLRNPCReactToNoiseTask()
++{
++	bShouldCallTick = false;
++}
++
++/**
++ * @brief 进入 ReactToNoise 时启动限时反应（转向声源，到时发送 NPCReactionEnded）。
++ * @param context 当前 StateTree 执行上下文。
++ * @param transition 触发本次进入的状态转换结果。
++ * @return 返回 Running，使反应持续到超时事件。
++ */
++EStateTreeRunStatus FLRNPCReactToNoiseTask::EnterState(FStateTreeExecutionContext& context,
++	const FStateTreeTransitionResult& transition) const
++{
++	const FInstanceDataType& data = context.GetInstanceData(*this);
++	if (!data.AIController)
++	{
++		return EStateTreeRunStatus::Failed;
++	}
++	data.AIController->StartNoiseReaction(data.AIController->GetLastNoiseLocation());
++	return EStateTreeRunStatus::Running;
++}
++
++/**
++ * @brief 离开 ReactToNoise 时停止反应计时器。
++ * @param context 当前 StateTree 执行上下文。
++ * @param transition 触发本次退出的状态转换结果。
++ */
++void FLRNPCReactToNoiseTask::ExitState(FStateTreeExecutionContext& context,
++	const FStateTreeTransitionResult& transition) const
++{
++	const FInstanceDataType& data = context.GetInstanceData(*this);
++	if (data.AIController)
++	{
++		data.AIController->StopNoiseReaction();
++	}
++}
+diff --git a/Source/LostRunic/AI/LRNPCStateTreeNodes.h b/Source/LostRunic/AI/LRNPCStateTreeNodes.h
+new file mode 100644
+index 0000000..45127c7
+--- /dev/null
++++ b/Source/LostRunic/AI/LRNPCStateTreeNodes.h
+@@ -0,0 +1,157 @@
++/**
++ * @file LRNPCStateTreeNodes.h
++ * @brief 通用 NPC 的 StateTree 节点：行为任务/条件（执行控制器解析结果）、Idle 玩家朝向任务与限时噪声反应任务；计时器由控制器持有，无 Tick。
++ *
++ * 关联文件：LRNPCStateTreeNodes.cpp；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "AI/LRNPCTypes.h"
++#include "StateTreeConditionBase.h"
++#include "StateTreeTaskBase.h"
++
++#include "LRNPCStateTreeNodes.generated.h"
++
++class ALRNPCController;
++
++USTRUCT()
++struct FLRNPCControllerInstanceData
++{
++	GENERATED_BODY()
++
++	/** AIController 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
++	UPROPERTY(EditAnywhere, Category = "Context")
++	TObjectPtr<ALRNPCController> AIController;
++};
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++USTRUCT(meta = (DisplayName = "Run NPC Behavior", Category = "Lost Runic|AI"))
++struct LOSTRUNIC_API FLRNPCBehaviorTask : public FStateTreeTaskCommonBase
++{
++	GENERATED_BODY()
++
++	using FInstanceDataType = FLRNPCControllerInstanceData;
++	/**
++	 * @brief 查询 Instance Data Type；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
++
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 */
++	FLRNPCBehaviorTask();
++	/**
++	 * @brief StateTree 进入节点时让 NPC 控制器进入配置的行为状态。
++	 * @param context 当前 StateTree 执行上下文，用于读取实例数据。
++	 * @param transition 触发本次进入的状态转换结果。
++	 * @return 返回 Running，使行为持续到 StateTree 条件触发下一次转换。
++	 */
++	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
++		const FStateTreeTransitionResult& transition) const override;
++	/**
++	 * @brief StateTree 离开节点时通知 NPC 控制器清理行为拥有的导航、焦点和计时器。
++	 * @param context 当前 StateTree 执行上下文。
++	 * @param transition 触发本次退出的状态转换结果。
++	 */
++	virtual void ExitState(FStateTreeExecutionContext& context,
++		const FStateTreeTransitionResult& transition) const override;
++
++	/** Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNPCBehaviorState::Idle`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
++	UPROPERTY(EditAnywhere, Category = "Behavior")
++	ELRNPCBehaviorState Behavior = ELRNPCBehaviorState::Idle;
++};
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++USTRUCT(meta = (DisplayName = "NPC Behavior Is", Category = "Lost Runic|AI"))
++struct LOSTRUNIC_API FLRNPCStateCondition : public FStateTreeConditionBase
++{
++	GENERATED_BODY()
++
++	using FInstanceDataType = FLRNPCControllerInstanceData;
++	/**
++	 * @brief 查询 Instance Data Type；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
++	/**
++	 * @brief 比较当前 NPC 行为与 StateTree 条件配置，决定该分支是否可进入；只执行控制器解析结果。
++	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
++
++	/** Expected Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNPCBehaviorState::Idle`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
++	UPROPERTY(EditAnywhere, Category = "Behavior")
++	ELRNPCBehaviorState ExpectedBehavior = ELRNPCBehaviorState::Idle;
++};
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++USTRUCT(meta = (DisplayName = "NPC Look At Player", Category = "Lost Runic|AI"))
++struct LOSTRUNIC_API FLRNPCLookAtPlayerTask : public FStateTreeTaskCommonBase
++{
++	GENERATED_BODY()
++
++	using FInstanceDataType = FLRNPCControllerInstanceData;
++	/**
++	 * @brief 查询 Instance Data Type；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
++
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 */
++	FLRNPCLookAtPlayerTask();
++	/**
++	 * @brief 进入 Idle 时启动低频玩家朝向检测计时器。
++	 * @param context 当前 StateTree 执行上下文。
++	 * @param transition 触发本次进入的状态转换结果。
++	 * @return 返回 Running，使检测持续到离开 Idle。
++	 */
++	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
++		const FStateTreeTransitionResult& transition) const override;
++	/**
++	 * @brief 离开 Idle 时停止朝向检测计时器。
++	 * @param context 当前 StateTree 执行上下文。
++	 * @param transition 触发本次退出的状态转换结果。
++	 */
++	virtual void ExitState(FStateTreeExecutionContext& context,
++		const FStateTreeTransitionResult& transition) const override;
++};
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++USTRUCT(meta = (DisplayName = "NPC React To Noise", Category = "Lost Runic|AI"))
++struct LOSTRUNIC_API FLRNPCReactToNoiseTask : public FStateTreeTaskCommonBase
++{
++	GENERATED_BODY()
++
++	using FInstanceDataType = FLRNPCControllerInstanceData;
++	/**
++	 * @brief 查询 Instance Data Type；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
++
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 */
++	FLRNPCReactToNoiseTask();
++	/**
++	 * @brief 进入 ReactToNoise 时启动限时反应（转向声源，到时发送 NPCReactionEnded）。
++	 * @param context 当前 StateTree 执行上下文。
++	 * @param transition 触发本次进入的状态转换结果。
++	 * @return 返回 Running，使反应持续到超时事件。
++	 */
++	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
++		const FStateTreeTransitionResult& transition) const override;
++	/**
++	 * @brief 离开 ReactToNoise 时停止反应计时器。
++	 * @param context 当前 StateTree 执行上下文。
++	 * @param transition 触发本次退出的状态转换结果。
++	 */
++	virtual void ExitState(FStateTreeExecutionContext& context,
++		const FStateTreeTransitionResult& transition) const override;
++};
+diff --git a/Source/LostRunic/AI/LRNPCTypes.h b/Source/LostRunic/AI/LRNPCTypes.h
+new file mode 100644
+index 0000000..be181c7
+--- /dev/null
++++ b/Source/LostRunic/AI/LRNPCTypes.h
+@@ -0,0 +1,31 @@
++/**
++ * @file LRNPCTypes.h
++ * @brief 声明通用 NPC 的行为状态与定义配置枚举，供 NPC 角色、控制器、StateTree 节点与内容定义共享。
++ *
++ * 关联文件：AI 目录内调用该公共契约的实现文件；所属领域：AI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "CoreMinimal.h"
++
++#include "LRNPCTypes.generated.h"
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UENUM(BlueprintType, meta = (DisplayName = "Lost Runic NPC Behavior"))
++enum class ELRNPCBehaviorState : uint8
++{
++	Idle UMETA(DisplayName = "Idle"),
++	Patrol UMETA(DisplayName = "Patrol"),
++	ReactToNoise UMETA(DisplayName = "React To Noise"),
++	Conversation UMETA(DisplayName = "Conversation")
++};
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UENUM(BlueprintType, meta = (DisplayName = "Lost Runic NPC Base Behavior"))
++enum class ENPCBaseBehavior : uint8
++{
++	Idle UMETA(DisplayName = "Idle"),
++	Patrol UMETA(DisplayName = "Patrol")
++};
+diff --git a/Source/LostRunic/Core/LRGameplayTags.cpp b/Source/LostRunic/Core/LRGameplayTags.cpp
+index f3a117d..91dae57 100644
+--- a/Source/LostRunic/Core/LRGameplayTags.cpp
++++ b/Source/LostRunic/Core/LRGameplayTags.cpp
+@@ -25,6 +25,8 @@ namespace LRGameplayTags
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectConcurrentInput, "State.Reject.ConcurrentInput", "Another eye input owns the current press.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectPresentationLocked, "State.Reject.PresentationLocked", "Presentation has not completed.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectAlreadyCurrent, "State.Reject.AlreadyCurrent", "The target mode is already active.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MovementRejectPaceForbidden, "Movement.Reject.PaceForbidden", "The requested pace is forbidden by the current state.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MovementOverrideHidden, "Movement.Override.Hidden", "Pace is temporarily forced to Sneak while hiding.");
+ 
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionInteract, "Interaction.Action.Interact", "Generic interaction.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionPickup, "Interaction.Action.Pickup", "Pickup interaction.");
+@@ -57,12 +59,20 @@ namespace LRGameplayTags
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepWalk, "Noise.Footstep.Walk", "Walking footstep stimulus.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepRun, "Noise.Footstep.Run", "Running footstep stimulus.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseInteraction, "Noise.Interaction", "Interaction-created noise stimulus.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepSneak, "Noise.Footstep.Sneak", "Silent sneak footstep; presentation/animation hook only, never emitted as hearing.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepWalkFaint, "Noise.Footstep.Walk.Faint", "Outdoor open-area walk; only guards with alert at least 6 respond.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepRunIndoor, "Noise.Footstep.Run.Indoor", "Indoor run; propagated through room volumes with alert floor semantics.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SightPlayer, "Sight.Player", "A guard saw the player.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SightPlayerLost, "Sight.Player.Lost", "A guard lost confirmed sight of the player.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchReached, "Search.Reached", "A guard reached the latest disturbance location.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchAlertDecay, "Search.AlertDecay", "Alert decayed after its observation delay.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchTimeout, "Search.Timeout", "A guard search timed out.");
+-	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventAlertChanged, "AI.Event.AlertChanged", "Alert state changed and StateTree should reselect.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventAlertChanged, "AI.Event.AlertChanged", "Alert or perception data changed; data-level event, does not drive StateTree selection.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventBehaviorChanged, "AI.Event.BehaviorChanged", "The resolved guard behavior changed; StateTree should reselect.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCNoiseHeard, "AI.Event.NPCNoiseHeard", "An NPC heard a noise and StateTree should react.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCDialogueStarted, "AI.Event.NPCDialogueStarted", "An NPC conversation started.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCDialogueEnded, "AI.Event.NPCDialogueEnded", "An NPC conversation ended.");
++	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCReactionEnded, "AI.Event.NPCReactionEnded", "An NPC noise reaction timed out; return to base behavior.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NarrativeEventCompleted, "Narrative.Event.Completed", "A stable narrative event completed.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NarrativeRejectNoSession, "Narrative.Reject.NoSession", "No narrative session can receive the request.");
+ 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NarrativeRejectMissingContent, "Narrative.Reject.MissingContent", "The requested stable content ID is not registered.");
+diff --git a/Source/LostRunic/Core/LRGameplayTags.h b/Source/LostRunic/Core/LRGameplayTags.h
+index 92dd09f..3221d22 100644
+--- a/Source/LostRunic/Core/LRGameplayTags.h
++++ b/Source/LostRunic/Core/LRGameplayTags.h
+@@ -87,6 +87,16 @@ namespace LRGameplayTags
+ 	 * @param StateRejectAlreadyCurrent 调用方提供的 `StateRejectAlreadyCurrent`，只在本次操作范围内使用。
+ 	 */
+ 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateRejectAlreadyCurrent);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param MovementRejectPaceForbidden 调用方提供的 `MovementRejectPaceForbidden`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(MovementRejectPaceForbidden);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param MovementOverrideHidden 调用方提供的 `MovementOverrideHidden`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(MovementOverrideHidden);
+ 
+ 	/**
+ 	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+@@ -239,6 +249,21 @@ namespace LRGameplayTags
+ 	 * @param NoiseInteraction 输入动作或数值 `NoiseInteraction`；不包含写死的具体键位。
+ 	 */
+ 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseInteraction);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param NoiseFootstepSneak 调用方提供的 `NoiseFootstepSneak`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepSneak);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param NoiseFootstepWalkFaint 调用方提供的 `NoiseFootstepWalkFaint`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepWalkFaint);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param NoiseFootstepRunIndoor 调用方提供的 `NoiseFootstepRunIndoor`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepRunIndoor);
+ 	/**
+ 	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ 	 * @param SightPlayer 调用方提供的 `SightPlayer`，只在本次操作范围内使用。
+@@ -269,6 +294,31 @@ namespace LRGameplayTags
+ 	 * @param AIEventAlertChanged 调用方提供的 `AIEventAlertChanged`，只在本次操作范围内使用。
+ 	 */
+ 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventAlertChanged);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param AIEventBehaviorChanged 调用方提供的 `AIEventBehaviorChanged`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventBehaviorChanged);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param AIEventNPCNoiseHeard 调用方提供的 `AIEventNPCNoiseHeard`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCNoiseHeard);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param AIEventNPCDialogueStarted 调用方提供的 `AIEventNPCDialogueStarted`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCDialogueStarted);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param AIEventNPCDialogueEnded 调用方提供的 `AIEventNPCDialogueEnded`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCDialogueEnded);
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 * @param AIEventNPCReactionEnded 调用方提供的 `AIEventNPCReactionEnded`，只在本次操作范围内使用。
++	 */
++	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCReactionEnded);
+ 	/**
+ 	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ 	 * @param NarrativeEventCompleted 调用方提供的 `NarrativeEventCompleted`，只在本次操作范围内使用。
+diff --git a/Source/LostRunic/Core/LRTypes.h b/Source/LostRunic/Core/LRTypes.h
+index f204d10..009ffdf 100644
+--- a/Source/LostRunic/Core/LRTypes.h
++++ b/Source/LostRunic/Core/LRTypes.h
+@@ -46,7 +46,8 @@ UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Noise Environment"))
+ enum class ELRNoiseEnvironment : uint8
+ {
+ 	Indoor UMETA(DisplayName = "Indoor"),
+-	Outdoor UMETA(DisplayName = "Outdoor")
++	Outdoor UMETA(DisplayName = "Outdoor"),
++	OutdoorStealth UMETA(DisplayName = "Outdoor Stealth")
+ };
+ 
+ /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+diff --git a/Source/LostRunic/Data/LRGameTuningSet.cpp b/Source/LostRunic/Data/LRGameTuningSet.cpp
+index cf6f4e7..7ec612f 100644
+--- a/Source/LostRunic/Data/LRGameTuningSet.cpp
++++ b/Source/LostRunic/Data/LRGameTuningSet.cpp
+@@ -12,6 +12,7 @@
+ #include "Data/LRGuardTuning.h"
+ #include "Data/LRInteractionTuning.h"
+ #include "Data/LRMovementTuning.h"
++#include "Data/LRNPCTuning.h"
+ #include "Data/LRPresentationTuning.h"
+ #include "Data/LRSaveTuning.h"
+ #include "Data/LRStateTuning.h"
+@@ -62,7 +63,8 @@ bool ULRGameTuningSet::Validate(FString& outError) const
+ 		&& ValidateEntry(TEXT("Guard"), Guard.Get(), outError)
+ 		&& ValidateEntry(TEXT("Save"), Save.Get(), outError)
+ 		&& ValidateEntry(TEXT("UI"), UI.Get(), outError)
+-		&& ValidateEntry(TEXT("Presentation"), Presentation.Get(), outError);
++		&& ValidateEntry(TEXT("Presentation"), Presentation.Get(), outError)
++		&& ValidateEntry(TEXT("NPC"), NPC.Get(), outError);
+ }
+ 
+ /**
+@@ -70,9 +72,9 @@ bool ULRGameTuningSet::Validate(FString& outError) const
+  */
+ void ULRGameTuningSet::LogSources() const
+ {
+-	UE_LOG(LogLostRunicTuning, Display, TEXT("TuningSet=%s State=%s Movement=%s Interaction=%s Guard=%s Save=%s UI=%s Presentation=%s"),
++	UE_LOG(LogLostRunicTuning, Display, TEXT("TuningSet=%s State=%s Movement=%s Interaction=%s Guard=%s Save=%s UI=%s Presentation=%s NPC=%s"),
+ 		*GetPathName(), *GetNameSafe(State), *GetNameSafe(Movement), *GetNameSafe(Interaction), *GetNameSafe(Guard),
+-		*GetNameSafe(Save), *GetNameSafe(UI), *GetNameSafe(Presentation));
++		*GetNameSafe(Save), *GetNameSafe(UI), *GetNameSafe(Presentation), *GetNameSafe(NPC));
+ }
+ 
+ #if WITH_EDITOR
+diff --git a/Source/LostRunic/Data/LRGameTuningSet.h b/Source/LostRunic/Data/LRGameTuningSet.h
+index f6d4e04..2c249d8 100644
+--- a/Source/LostRunic/Data/LRGameTuningSet.h
++++ b/Source/LostRunic/Data/LRGameTuningSet.h
+@@ -16,6 +16,7 @@
+ class ULRGuardTuning;
+ class ULRInteractionTuning;
+ class ULRMovementTuning;
++class ULRNPCTuning;
+ class ULRPresentationTuning;
+ class ULRSaveTuning;
+ class ULRStateTuning;
+@@ -56,6 +57,10 @@ public:
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
+ 	TObjectPtr<ULRPresentationTuning> Presentation;
+ 
++	/** NPC 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
++	TObjectPtr<ULRNPCTuning> NPC;
++
+ 	/**
+ 	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
+ 	 * @param outError 输出校验失败原因；成功时保持为空。
+diff --git a/Source/LostRunic/Data/LRGuardDefinition.h b/Source/LostRunic/Data/LRGuardDefinition.h
+index 357f320..106e5c4 100644
+--- a/Source/LostRunic/Data/LRGuardDefinition.h
++++ b/Source/LostRunic/Data/LRGuardDefinition.h
+@@ -31,9 +31,9 @@ public:
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
+ 	TObjectPtr<ULRGuardTuning> Tuning;
+ 
+-	/** Behavior 的开关；true 表示启用，false 表示禁用。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
++	/** Behavior 的 StateTree 硬引用；随定义资产同步加载，OnPossess 时确定启动顺序。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
+-	TSoftObjectPtr<UStateTree> Behavior;
++	TObjectPtr<UStateTree> Behavior;
+ 
+ 	/**
+ 	 * @brief 查询 Primary Asset Id；不修改领域状态。
+diff --git a/Source/LostRunic/Data/LRGuardTuning.cpp b/Source/LostRunic/Data/LRGuardTuning.cpp
+index d7c0090..0846296 100644
+--- a/Source/LostRunic/Data/LRGuardTuning.cpp
++++ b/Source/LostRunic/Data/LRGuardTuning.cpp
+@@ -34,12 +34,16 @@ bool ULRGuardTuning::Validate(FString& outError) const
+ 		&& LRValidation::RequireRange(TEXT("SightConeDegrees"), SightConeDegrees, 1.0f, 180.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("HearingRangeMultiplier"), HearingRangeMultiplier, 0.0f, 10.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("MaxHearingRange"), MaxHearingRange, 50.0f, 10000.0f, outError)
+-		&& LRValidation::RequireRange(TEXT("HearingAlertAmount"), HearingAlertAmount, 1, 11, outError)
+-		&& LRValidation::RequireRange(TEXT("SightAlertLevel"), SightAlertLevel, 1, 11, outError)
++		&& LRValidation::RequireRange(TEXT("AttractAlertAmount"), AttractAlertAmount, 1, 11, outError)
++		&& LRValidation::RequireRange(TEXT("SightInvestigateLevel"), SightInvestigateLevel, 1, 11, outError)
++		&& LRValidation::RequireRange(TEXT("SightChaseLevel"), SightChaseLevel, 1, 11, outError)
++		&& LRValidation::RequireRange(TEXT("AlertIncreaseCooldownSeconds"), AlertIncreaseCooldownSeconds, 0.0f, 10.0f, outError)
++		&& LRValidation::RequireRange(TEXT("InvestigateIncreaseCooldownSeconds"), InvestigateIncreaseCooldownSeconds, 0.0f, 10.0f, outError)
++		&& LRValidation::RequireRange(TEXT("RoomRunAlertLevel"), RoomRunAlertLevel, 0, 11, outError)
++		&& LRValidation::RequireRange(TEXT("AdjacentRoomRunAlertAmount"), AdjacentRoomRunAlertAmount, 1, 11, outError)
+ 		&& LRValidation::RequireRange(TEXT("AlertDecayAmount"), AlertDecayAmount, 1, 11, outError)
+ 		&& LRValidation::RequireRange(TEXT("InitialObserveSeconds"), InitialObserveSeconds, 0.1f, 30.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("AlertDecayIntervalSeconds"), AlertDecayIntervalSeconds, 0.05f, 10.0f, outError)
+-		&& LRValidation::RequireRange(TEXT("SearchDurationSeconds"), SearchDurationSeconds, 0.1f, 60.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("CaptureRadius"), CaptureRadius, 10.0f, 500.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("CaptureCheckIntervalSeconds"), CaptureCheckIntervalSeconds, 0.02f, 1.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("MoveAcceptanceRadius"), MoveAcceptanceRadius, 1.0f, 500.0f, outError)
+diff --git a/Source/LostRunic/Data/LRGuardTuning.h b/Source/LostRunic/Data/LRGuardTuning.h
+index e20932d..1654ab2 100644
+--- a/Source/LostRunic/Data/LRGuardTuning.h
++++ b/Source/LostRunic/Data/LRGuardTuning.h
+@@ -39,13 +39,33 @@ public:
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Hearing", meta = (ClampMin = "50.0", ClampMax = "10000.0", Units = "cm"))
+ 	float MaxHearingRange = 5000.0f;
+ 
+-	/** 有效听觉刺激首次增加的警戒量；默认 6。 C++ 安全默认值为 `6`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
++	/** 吸引注意噪声每次增加的警戒量；设计基线 1（0→1、档内 +1）。 C++ 安全默认值为 `1`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
+-	int32 HearingAlertAmount = 6;
++	int32 AttractAlertAmount = 1;
+ 
+-	/** 明确看见玩家后设置的警戒等级；默认 11，进入追逐。 C++ 安全默认值为 `11`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
++	/** 警戒低于 SightInvestigateLevel 时看见玩家设置的目标等级；默认 6，前往调查。 C++ 安全默认值为 `6`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
+-	int32 SightAlertLevel = 11;
++	int32 SightInvestigateLevel = 6;
++
++	/** 警戒处于 6-10 档时看见玩家设置的目标等级；默认 11，进入追逐。 C++ 安全默认值为 `11`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
++	int32 SightChaseLevel = 11;
++
++	/** 1-5 档吸引注意增加的冷却时间；默认 0.5 秒；从 0 首次进入 6-10 档的首个增量也使用该值。 C++ 安全默认值为 `0.5f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `10.0`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
++	float AlertIncreaseCooldownSeconds = 0.5f;
++
++	/** 6-10 档前往/观察中吸引注意增加的冷却时间；默认 0.2 秒。 C++ 安全默认值为 `0.2f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `10.0`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
++	float InvestigateIncreaseCooldownSeconds = 0.2f;
++
++	/** 室内奔跑对当前房间守卫的警戒下限；默认 5。 C++ 安全默认值为 `5`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `0`，最大值 `11`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0", ClampMax = "11"))
++	int32 RoomRunAlertLevel = 5;
++
++	/** 室内奔跑对相邻房间守卫的警戒增量；默认 1。 C++ 安全默认值为 `1`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
++	int32 AdjacentRoomRunAlertAmount = 1;
+ 
+ 	/** 每个衰减周期降低的警戒值；默认 1。 C++ 安全默认值为 `1`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
+@@ -67,8 +87,8 @@ public:
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.05", ClampMax = "10.0", Units = "s"))
+ 	float AlertDecayIntervalSeconds = 0.5f;
+ 
+-	/** 到达最后异常位置后的搜索持续时间；默认 5 秒。 C++ 安全默认值为 `5.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `60.0`。 */
+-	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.1", ClampMax = "60.0", Units = "s"))
++	/** 已废弃：旧固定搜索时长；搜索改为「抵达观察 3s → 自然衰减 → 归零清理」，保留字段仅用于资产序列化兼容，不再参与运行与校验。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert|Deprecated", meta = (DeprecatedProperty, ClampMin = "0.1", ClampMax = "60.0", Units = "s"))
+ 	float SearchDurationSeconds = 5.0f;
+ 
+ 	/** 追逐中判定捕获玩家的距离；默认 75 cm。 C++ 安全默认值为 `75.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `500.0`。 */
+diff --git a/Source/LostRunic/Data/LRMovementTuning.cpp b/Source/LostRunic/Data/LRMovementTuning.cpp
+index 81f8c33..00c5e17 100644
+--- a/Source/LostRunic/Data/LRMovementTuning.cpp
++++ b/Source/LostRunic/Data/LRMovementTuning.cpp
+@@ -35,7 +35,7 @@ bool ULRMovementTuning::Validate(FString& outError) const
+ 		&& LRValidation::RequireRange(TEXT("SampleIntervalSeconds"), SampleIntervalSeconds, 0.02f, 1.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("IndoorWalkNoiseRadius"), IndoorWalkNoiseRadius, 0.0f, 5000.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("IndoorRunNoiseRadius"), IndoorRunNoiseRadius, 0.0f, 5000.0f, outError)
+-		&& LRValidation::RequireRange(TEXT("OutdoorSneakGuardNoiseRadius"), OutdoorSneakGuardNoiseRadius, 0.0f, 5000.0f, outError)
+-		&& LRValidation::RequireRange(TEXT("OutdoorAlertGuardNoiseRadius"), OutdoorAlertGuardNoiseRadius, 0.0f, 5000.0f, outError)
++		&& LRValidation::RequireRange(TEXT("OutdoorStealthRunNoiseRadius"), OutdoorStealthRunNoiseRadius, 0.0f, 5000.0f, outError)
++		&& LRValidation::RequireRange(TEXT("OutdoorNoiseRadius"), OutdoorNoiseRadius, 0.0f, 5000.0f, outError)
+ 		&& LRValidation::RequireRange(TEXT("InteractionNoiseRadius"), InteractionNoiseRadius, 0.0f, 5000.0f, outError);
+ }
+diff --git a/Source/LostRunic/Data/LRMovementTuning.h b/Source/LostRunic/Data/LRMovementTuning.h
+index 07b5395..303c38c 100644
+--- a/Source/LostRunic/Data/LRMovementTuning.h
++++ b/Source/LostRunic/Data/LRMovementTuning.h
+@@ -51,13 +51,13 @@ public:
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
+ 	float IndoorRunNoiseRadius = 1200.0f;
+ 
+-	/** Outdoor Sneak Guard Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `600.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
++	/** Outdoor Stealth Run Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位；室外潜行关奔跑脚步噪声半径（设计 6m）。 C++ 安全默认值为 `600.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
+-	float OutdoorSneakGuardNoiseRadius = 600.0f;
++	float OutdoorStealthRunNoiseRadius = 600.0f;
+ 
+-	/** Outdoor Alert Guard Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `250.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
++	/** Outdoor Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位；室外潜行走路、室外非潜行走路与奔跑共用（设计 2.5m，走路的非潜行变体经 Faint 标签在守卫侧过滤）。 C++ 安全默认值为 `250.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
+-	float OutdoorAlertGuardNoiseRadius = 250.0f;
++	float OutdoorNoiseRadius = 250.0f;
+ 
+ 	/** Interaction Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `500.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
+diff --git a/Source/LostRunic/Data/LRNPCDefinition.cpp b/Source/LostRunic/Data/LRNPCDefinition.cpp
+new file mode 100644
+index 0000000..60074b1
+--- /dev/null
++++ b/Source/LostRunic/Data/LRNPCDefinition.cpp
+@@ -0,0 +1,40 @@
++/**
++ * @file LRNPCDefinition.cpp
++ * @brief 通用 NPC 内容定义 DataAsset 的主资产 ID 与编辑器数据校验。
++ *
++ * 关联文件：LRNPCDefinition.h；所属领域：Data。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "Data/LRNPCDefinition.h"
++
++#include "Misc/DataValidation.h"
++
++/**
++ * @brief 查询 Primary Asset Id；不修改领域状态。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++FPrimaryAssetId ULRNPCDefinition::GetPrimaryAssetId() const
++{
++	return FPrimaryAssetId(TEXT("LostRunicNPC"), NpcId);
++}
++
++#if WITH_EDITOR
++/**
++ * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
++ * @param context 用于本次条件匹配的 `context` 标签或上下文。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++EDataValidationResult ULRNPCDefinition::IsDataValid(FDataValidationContext& context) const
++{
++	if (NpcId.IsNone())
++	{
++		context.AddError(FText::FromString(TEXT("NpcId must not be empty.")));
++	}
++	if (!Behavior)
++	{
++		context.AddError(FText::FromString(TEXT("Behavior StateTree must be assigned.")));
++	}
++	return context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
++}
++#endif
+diff --git a/Source/LostRunic/Data/LRNPCDefinition.h b/Source/LostRunic/Data/LRNPCDefinition.h
+new file mode 100644
+index 0000000..3e0ad94
+--- /dev/null
++++ b/Source/LostRunic/Data/LRNPCDefinition.h
+@@ -0,0 +1,56 @@
++/**
++ * @file LRNPCDefinition.h
++ * @brief 通用 NPC 的内容定义 DataAsset：StateTree 硬引用、默认行为与对话行 ID；公共调优在 ULRNPCTuning，巡逻点按实例配置。
++ *
++ * 关联文件：LRNPCDefinition.cpp；所属领域：Data。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "AI/LRNPCTypes.h"
++#include "CoreMinimal.h"
++#include "Engine/DataAsset.h"
++
++#include "LRNPCDefinition.generated.h"
++
++class UStateTree;
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Definition"))
++class LOSTRUNIC_API ULRNPCDefinition : public UPrimaryDataAsset
++{
++	GENERATED_BODY()
++
++public:
++	/** Npc Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
++	FName NpcId = NAME_None;
++
++	/** Behavior 的 StateTree 硬引用；随定义资产同步加载，OnPossess 时确定启动顺序。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
++	TObjectPtr<UStateTree> Behavior;
++
++	/** Dialogue Row Id 的稳定 DataTable 行 ID，用于 ULRDialogueSubsystem::StartDialogue。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
++	FName DialogueRowId = NAME_None;
++
++	/** Default Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ENPCBaseBehavior::Idle`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
++	ENPCBaseBehavior DefaultBehavior = ENPCBaseBehavior::Idle;
++
++	/**
++	 * @brief 查询 Primary Asset Id；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
++
++#if WITH_EDITOR
++	/**
++	 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
++	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
++#endif
++};
+diff --git a/Source/LostRunic/Data/LRNPCTuning.cpp b/Source/LostRunic/Data/LRNPCTuning.cpp
+new file mode 100644
+index 0000000..5019e42
+--- /dev/null
++++ b/Source/LostRunic/Data/LRNPCTuning.cpp
+@@ -0,0 +1,24 @@
++/**
++ * @file LRNPCTuning.cpp
++ * @brief 通用 NPC 公共调优 DataAsset 的校验实现。
++ *
++ * 关联文件：LRNPCTuning.h；所属领域：Data。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "Data/LRNPCTuning.h"
++
++#include "Core/LRValidation.h"
++
++/**
++ * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
++ * @param outError 输出校验失败原因；成功时保持为空。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++bool ULRNPCTuning::Validate(FString& outError) const
++{
++	return LRValidation::RequireRange(TEXT("LookAtPlayerRadiusCm"), LookAtPlayerRadiusCm, 10.0f, 5000.0f, outError)
++		&& LRValidation::RequireRange(TEXT("LookAtIntervalSeconds"), LookAtIntervalSeconds, 0.05f, 2.0f, outError)
++		&& LRValidation::RequireRange(TEXT("NoiseReactionDurationSeconds"), NoiseReactionDurationSeconds, 0.1f, 30.0f, outError)
++		&& LRValidation::RequireRange(TEXT("PatrolSpeedCm"), PatrolSpeedCm, 1.0f, 1000.0f, outError);
++}
+diff --git a/Source/LostRunic/Data/LRNPCTuning.h b/Source/LostRunic/Data/LRNPCTuning.h
+new file mode 100644
+index 0000000..dd25d99
+--- /dev/null
++++ b/Source/LostRunic/Data/LRNPCTuning.h
+@@ -0,0 +1,44 @@
++/**
++ * @file LRNPCTuning.h
++ * @brief 通用 NPC 的公共调优 DataAsset：玩家朝向检测、噪声反应与巡逻参数；逐 NPC 内容配置在 ULRNPCDefinition，巡逻点按实例配置。
++ *
++ * 关联文件：LRNPCTuning.cpp；所属领域：Data。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "Data/LRTuningAsset.h"
++
++#include "LRNPCTuning.generated.h"
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Tuning"))
++class LOSTRUNIC_API ULRNPCTuning : public ULRTuningAsset
++{
++	GENERATED_BODY()
++
++public:
++	/** NPC 在 Idle 状态检测并朝向玩家的半径；默认 300 cm。 C++ 安全默认值为 `300.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `5000.0`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|LookAt", meta = (ClampMin = "10.0", ClampMax = "5000.0", Units = "cm"))
++	float LookAtPlayerRadiusCm = 300.0f;
++
++	/** Idle 状态玩家朝向检测的低频间隔；默认 0.25 秒，以计时器代替 Tick。 C++ 安全默认值为 `0.25f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.05`，最大值 `2.0`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|LookAt", meta = (ClampMin = "0.05", ClampMax = "2.0", Units = "s"))
++	float LookAtIntervalSeconds = 0.25f;
++
++	/** NPC 听见噪声后的限时反应时长；结束后回到配置的默认行为。 C++ 安全默认值为 `3.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `30.0`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Noise", meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
++	float NoiseReactionDurationSeconds = 3.0f;
++
++	/** NPC 巡逻移动速度；默认 150 cm/s。 C++ 安全默认值为 `150.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
++	float PatrolSpeedCm = 150.0f;
++
++	/**
++	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
++	 * @param outError 输出校验失败原因；成功时保持为空。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	virtual bool Validate(FString& outError) const override;
++};
+diff --git a/Source/LostRunic/Framework/LRPlayerController.cpp b/Source/LostRunic/Framework/LRPlayerController.cpp
+index eaf2f5a..7801a18 100644
+--- a/Source/LostRunic/Framework/LRPlayerController.cpp
++++ b/Source/LostRunic/Framework/LRPlayerController.cpp
+@@ -163,7 +163,7 @@ void ALRPlayerController::HandleSneakToggle()
+ 	}
+ 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
+ 	{
+-		character->GetLocomotionComponent()->ToggleSneak();
++		character->GetLocomotionComponent()->RequestToggleSneak();
+ 	}
+ }
+ 
+@@ -178,7 +178,7 @@ void ALRPlayerController::HandleRunStarted()
+ 	}
+ 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
+ 	{
+-		character->GetLocomotionComponent()->StartRun();
++		character->GetLocomotionComponent()->RequestStartRun();
+ 	}
+ }
+ 
+@@ -193,7 +193,7 @@ void ALRPlayerController::HandleRunStopped()
+ 	}
+ 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
+ 	{
+-		character->GetLocomotionComponent()->StopRun();
++		character->GetLocomotionComponent()->RequestStopRun();
+ 	}
+ }
+ 
+diff --git a/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp b/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp
+index 1cdc675..ad68967 100644
+--- a/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp
++++ b/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp
+@@ -1,6 +1,6 @@
+ /**
+  * @file LRLocomotionComponent.cpp
+- * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。
++ * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。玩家请求经状态步态规则验证，组件内部应用与掩体覆盖走独立通道。
+  *
+  * 关联文件：LRLocomotionComponent.h；所属领域：Gameplay。
+  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+@@ -14,9 +14,12 @@
+ #include "Data/LRMovementTuning.h"
+ #include "Engine/GameInstance.h"
+ #include "Engine/World.h"
++#include "Framework/LRCharacter.h"
+ #include "Framework/LRGameInstanceSubsystem.h"
+ #include "GameFramework/Character.h"
+ #include "GameFramework/CharacterMovementComponent.h"
++#include "Gameplay/LRMovementRules.h"
++#include "State/LRStateComponent.h"
+ #include "TimerManager.h"
+ 
+ /**
+@@ -37,13 +40,18 @@ void ULRLocomotionComponent::BeginPlay()
+ 	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
+ 	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
+ 	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
++	State = Cast<ALRCharacter>(GetOwner()) ? Cast<ALRCharacter>(GetOwner())->GetStateComponent() : nullptr;
+ 	if (!ensureMsgf(Character && Tuning, TEXT("%s requires an ACharacter owner and Movement tuning."), *GetNameSafe(this)))
+ 	{
+ 		return;
+ 	}
+ 
+ 	LastSampleLocation = Character->GetActorLocation();
+-	SetPace(Pace);
++	ApplyPace(Pace, FGameplayTag());
++	if (State.IsValid())
++	{
++		State->OnStateChanged.AddDynamic(this, &ULRLocomotionComponent::HandleStateChanged);
++	}
+ 	GetWorld()->GetTimerManager().SetTimer(SampleTimer, this, &ULRLocomotionComponent::SampleTravelDistance,
+ 		Tuning->SampleIntervalSeconds, true);
+ }
+@@ -54,6 +62,10 @@ void ULRLocomotionComponent::BeginPlay()
+  */
+ void ULRLocomotionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
+ {
++	if (State.IsValid())
++	{
++		State->OnStateChanged.RemoveDynamic(this, &ULRLocomotionComponent::HandleStateChanged);
++	}
+ 	if (GetWorld())
+ 	{
+ 		GetWorld()->GetTimerManager().ClearTimer(SampleTimer);
+@@ -62,69 +74,137 @@ void ULRLocomotionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
+ }
+ 
+ /**
+- * @brief 更新 Pace，并在需要时同步组件状态或广播变化事件。
+- * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
++ * @brief 请求切换潜行与走路；受当前状态步态规则验证，Perception 强制潜行，Courage 禁止潜行，Memory 仅走路。
+  */
+-void ULRLocomotionComponent::SetPace(const ELRMovementPace newPace)
++void ULRLocomotionComponent::RequestToggleSneak()
+ {
+-	Pace = newPace;
+-	if (!Character || !Tuning)
++	ELRMovementPace& targetPace = Pace == ELRMovementPace::Run ? PaceBeforeRun : Pace;
++	const ELRMovementPace candidate = targetPace == ELRMovementPace::Sneak ? ELRMovementPace::Walk : ELRMovementPace::Sneak;
++	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), candidate))
+ 	{
++		RejectPaceRequest(candidate);
+ 		return;
+ 	}
+-
+-	float speed = Tuning->WalkSpeed;
+-	if (Pace == ELRMovementPace::Sneak)
++	if (Pace != ELRMovementPace::Run)
+ 	{
+-		speed = Tuning->SneakSpeed;
++		ApplyPace(candidate, FGameplayTag());
+ 	}
+-	else if (Pace == ELRMovementPace::Run)
++	else
+ 	{
+-		speed = Tuning->RunSpeed;
++		targetPace = candidate;
+ 	}
+-	Character->GetCharacterMovement()->MaxWalkSpeed = speed;
+ }
+ 
+ /**
+- * @brief 在状态允许时切换潜行与走路；Perception 强制潜行，Courage 禁止潜行。
++ * @brief 请求开始奔跑；当前状态禁止奔跑时拒绝并广播拒绝原因。
+  */
+-void ULRLocomotionComponent::ToggleSneak()
++void ULRLocomotionComponent::RequestStartRun()
+ {
+-	ELRMovementPace& targetPace = Pace == ELRMovementPace::Run ? PaceBeforeRun : Pace;
+-	targetPace = targetPace == ELRMovementPace::Sneak ? ELRMovementPace::Walk : ELRMovementPace::Sneak;
+-	if (Pace != ELRMovementPace::Run)
++	if (Pace == ELRMovementPace::Run)
+ 	{
+-		SetPace(targetPace);
++		return;
+ 	}
++	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), ELRMovementPace::Run))
++	{
++		RejectPaceRequest(ELRMovementPace::Run);
++		return;
++	}
++
++	PaceBeforeRun = Pace;
++	ApplyPace(ELRMovementPace::Run, FGameplayTag());
+ }
+ 
+ /**
+- * @brief 开始 Start Run 流程，建立本次操作拥有的状态、委托或计时器。
++ * @brief 请求结束奔跑，恢复到奔跑前的步态。
+  */
+-void ULRLocomotionComponent::StartRun()
++void ULRLocomotionComponent::RequestStopRun()
+ {
+ 	if (Pace == ELRMovementPace::Run)
+ 	{
+-		return;
++		ApplyPace(PaceBeforeRun, FGameplayTag());
+ 	}
++}
+ 
+-	PaceBeforeRun = Pace;
+-	SetPace(ELRMovementPace::Run);
++/**
++ * @brief 组件内部应用步态（状态同步、掩体、调试）；玩家输入请走 Request* 入口。
++ * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
++ * @param source 来源 Gameplay Tag，用于日志与诊断；None 表示常规状态应用。
++ */
++void ULRLocomotionComponent::ApplyPace(const ELRMovementPace newPace, const FGameplayTag source)
++{
++	if (Pace != newPace)
++	{
++		UE_LOG(LogLostRunicState, Verbose, TEXT("Locomotion=%s pace %d -> %d source=%s"), *GetNameSafe(this),
++			static_cast<int32>(Pace), static_cast<int32>(newPace), *source.ToString());
++	}
++	Pace = newPace;
++	SyncMovementSpeed();
+ }
+ 
+ /**
+- * @brief 结束或取消 Stop Run 流程，并清理本次操作拥有的临时状态。
++ * @brief 带来源标识的临时步态覆盖（如掩体强制潜行）；清除时按当前状态重新求值合法步态。
++ * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
++ * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
+  */
+-void ULRLocomotionComponent::StopRun()
++void ULRLocomotionComponent::OverridePace(const ELRMovementPace newPace, const FGameplayTag source)
+ {
+-	if (Pace == ELRMovementPace::Run)
++	PaceOverride = newPace;
++	PaceOverrideSource = source;
++	SyncMovementSpeed();
++}
++
++/**
++ * @brief 清除指定来源的临时步态覆盖；覆盖期间的基础步态可能因状态规则过期，清除后重新求值。
++ * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
++ */
++void ULRLocomotionComponent::ClearPaceOverride(const FGameplayTag source)
++{
++	if (!PaceOverrideSource.IsValid() || PaceOverrideSource != source)
++	{
++		return;
++	}
++	PaceOverrideSource = FGameplayTag();
++	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), Pace))
++	{
++		Pace = LRMovementRules::GetDefaultPace(GetEffectiveMode());
++	}
++	SyncMovementSpeed();
++}
++
++/**
++ * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新；清空掩体覆盖并按状态默认步态应用。
++ * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
++ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
++ */
++void ULRLocomotionComponent::HandleStateChanged(const ELRPerceptionMode currentMode, const FGameplayTag reason)
++{
++	if (PaceOverrideSource.IsValid())
+ 	{
+-		SetPace(PaceBeforeRun);
++		ClearPaceOverride(PaceOverrideSource);
+ 	}
++	ApplyPace(LRMovementRules::GetDefaultPace(currentMode), reason);
++}
++
++/**
++ * @brief 查询 Effective Mode；State 组件缺失时回退 Normal（无 World 测试场景）。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRPerceptionMode ULRLocomotionComponent::GetEffectiveMode() const
++{
++	return State.IsValid() ? State->GetCurrentMode() : ELRPerceptionMode::Normal;
+ }
+ 
+ /**
+- * @brief 以低频计时器累计角色实际位移，达到步长后发布脚步而不使用 Tick。
++ * @brief 查询 Effective Pace；掩体等覆盖存在时返回覆盖值，否则返回基础步态。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRMovementPace ULRLocomotionComponent::GetEffectivePace() const
++{
++	return PaceOverrideSource.IsValid() ? PaceOverride : Pace;
++}
++
++/**
++ * @brief 以低频计时器累计角色实际位移，达到步长后按步态×环境解析并发布脚步而不使用 Tick。
+  */
+ void ULRLocomotionComponent::SampleTravelDistance()
+ {
+@@ -143,8 +223,8 @@ void ULRLocomotionComponent::SampleTravelDistance()
+ 	}
+ 
+ 	DistanceSinceFootstep = FMath::Fmod(DistanceSinceFootstep, stepDistance);
+-	const FGameplayTag noiseTag = Pace == ELRMovementPace::Run ? LRGameplayTags::NoiseFootstepRun : LRGameplayTags::NoiseFootstepWalk;
+-	OnFootstep.Broadcast(location, GetNoiseRadius(), noiseTag);
++	const FLRNoiseResolution resolution = LRMovementRules::ResolveFootstepNoise(GetEffectivePace(), NoiseEnvironment, *Tuning);
++	OnFootstep.Broadcast(location, resolution.Radius, resolution.Tag);
+ }
+ 
+ /**
+@@ -153,22 +233,40 @@ void ULRLocomotionComponent::SampleTravelDistance()
+  */
+ float ULRLocomotionComponent::GetStepDistance() const
+ {
+-	return Pace == ELRMovementPace::Run ? Tuning->RunStepDistance : Tuning->WalkStepDistance;
++	return GetEffectivePace() == ELRMovementPace::Run ? Tuning->RunStepDistance : Tuning->WalkStepDistance;
+ }
+ 
+ /**
+- * @brief 查询 Noise Radius；不修改领域状态。
+- * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ * @brief 按有效步态同步 CharacterMovement 的 MaxWalkSpeed。
+  */
+-float ULRLocomotionComponent::GetNoiseRadius() const
++void ULRLocomotionComponent::SyncMovementSpeed()
+ {
+-	if (Pace == ELRMovementPace::Sneak)
++	if (!Character || !Tuning)
+ 	{
+-		return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorSneakGuardNoiseRadius : 0.0f;
++		return;
+ 	}
+-	if (Pace == ELRMovementPace::Run)
++
++	float speed = Tuning->WalkSpeed;
++	const ELRMovementPace effectivePace = GetEffectivePace();
++	if (effectivePace == ELRMovementPace::Sneak)
+ 	{
+-		return Tuning->IndoorRunNoiseRadius;
++		speed = Tuning->SneakSpeed;
+ 	}
+-	return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorAlertGuardNoiseRadius : Tuning->IndoorWalkNoiseRadius;
++	else if (effectivePace == ELRMovementPace::Run)
++	{
++		speed = Tuning->RunSpeed;
++	}
++	Character->GetCharacterMovement()->MaxWalkSpeed = speed;
++}
++
++/**
++ * @brief 对禁止的步态请求统一记录日志并广播拒绝事件。
++ * @param requestedPace 本次操作使用的 `requestedPace` 枚举或模式值。
++ */
++void ULRLocomotionComponent::RejectPaceRequest(const ELRMovementPace requestedPace)
++{
++	UE_LOG(LogLostRunicState, Warning, TEXT("Locomotion=%s pace request %d rejected in mode %d reason=%s"),
++		*GetNameSafe(this), static_cast<int32>(requestedPace), static_cast<int32>(GetEffectiveMode()),
++		*LRGameplayTags::MovementRejectPaceForbidden.GetTag().ToString());
++	OnPaceRequestRejected.Broadcast(requestedPace, LRGameplayTags::MovementRejectPaceForbidden);
+ }
+diff --git a/Source/LostRunic/Gameplay/LRLocomotionComponent.h b/Source/LostRunic/Gameplay/LRLocomotionComponent.h
+index 45dde49..f264fcd 100644
+--- a/Source/LostRunic/Gameplay/LRLocomotionComponent.h
++++ b/Source/LostRunic/Gameplay/LRLocomotionComponent.h
+@@ -1,6 +1,6 @@
+ /**
+  * @file LRLocomotionComponent.h
+- * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。
++ * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。玩家请求经状态步态规则验证，组件内部应用与掩体覆盖走独立通道。
+  *
+  * 关联文件：LRLocomotionComponent.cpp；所属领域：Gameplay。
+  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+@@ -16,8 +16,10 @@
+ 
+ class ACharacter;
+ class ULRMovementTuning;
++class ULRStateComponent;
+ 
+ DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRFootstepEvent, FVector, location, float, radius, FGameplayTag, noiseTag);
++DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRPaceRequestRejected, ELRMovementPace, requestedPace, FGameplayTag, reason);
+ 
+ /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+ UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Locomotion"))
+@@ -42,39 +44,55 @@ public:
+ 	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
+ 
+ 	/**
+-	 * @brief 更新 Pace，并在需要时同步组件状态或广播变化事件。
+-	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
++	 * @brief 请求切换潜行与走路；受当前状态步态规则验证，Perception 强制潜行，Courage 禁止潜行，Memory 仅走路。
+ 	 */
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
+-	void SetPace(ELRMovementPace newPace);
++	void RequestToggleSneak();
+ 
+ 	/**
+-	 * @brief 在状态允许时切换潜行与走路；Perception 强制潜行，Courage 禁止潜行。
++	 * @brief 请求开始奔跑；当前状态禁止奔跑时拒绝并广播拒绝原因。
+ 	 */
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
+-	void ToggleSneak();
++	void RequestStartRun();
+ 
+ 	/**
+-	 * @brief 开始 Start Run 流程，建立本次操作拥有的状态、委托或计时器。
++	 * @brief 请求结束奔跑，恢复到奔跑前的步态。
+ 	 */
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
+-	void StartRun();
++	void RequestStopRun();
++
++	/**
++	 * @brief 查询有效步态（掩体覆盖优先）；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
++	ELRMovementPace GetPace() const { return GetEffectivePace(); }
+ 
+ 	/**
+-	 * @brief 结束或取消 Stop Run 流程，并清理本次操作拥有的临时状态。
++	 * @brief 组件内部应用步态（状态同步、掩体、调试）；玩家输入请走 Request* 入口。
++	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
++	 * @param source 来源 Gameplay Tag，用于日志与诊断；None 表示常规状态应用。
+ 	 */
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
+-	void StopRun();
++	void ApplyPace(ELRMovementPace newPace, FGameplayTag source = FGameplayTag());
+ 
+ 	/**
+-	 * @brief 查询 Pace；不修改领域状态。
+-	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 * @brief 带来源标识的临时步态覆盖（如掩体强制潜行）；清除时按当前状态重新求值合法步态。
++	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
++	 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
+ 	 */
+-	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
+-	ELRMovementPace GetPace() const { return Pace; }
++	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
++	void OverridePace(ELRMovementPace newPace, FGameplayTag source);
++
++	/**
++	 * @brief 清除指定来源的临时步态覆盖；覆盖期间的基础步态可能因状态规则过期，清除后重新求值。
++	 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
++	 */
++	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
++	void ClearPaceOverride(FGameplayTag source);
+ 
+ 	/**
+-	 * @brief 更新 Noise Environment，并在需要时同步组件状态或广播变化事件。
++	 * @brief 更新 Noise Environment，并在需要时同步组件状态或广播变化事件；仅由 ALRNoiseArea 调用。
+ 	 * @param newEnvironment 调用方提供的 `newEnvironment`，只在本次操作范围内使用。
+ 	 */
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
+@@ -84,9 +102,31 @@ public:
+ 	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
+ 	FLRFootstepEvent OnFootstep;
+ 
++	/** 当 Pace Request Rejected 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
++	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Movement")
++	FLRPaceRequestRejected OnPaceRequestRejected;
++
+ private:
+ 	/**
+-	 * @brief 以低频计时器累计角色实际位移，达到步长后发布脚步而不使用 Tick。
++	 * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新；清空掩体覆盖并按状态默认步态应用。
++	 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
++	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
++	 */
++	UFUNCTION()
++	void HandleStateChanged(ELRPerceptionMode currentMode, FGameplayTag reason);
++
++	/**
++	 * @brief 查询 Effective Mode；State 组件缺失时回退 Normal（无 World 测试场景）。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	ELRPerceptionMode GetEffectiveMode() const;
++	/**
++	 * @brief 查询 Effective Pace；掩体等覆盖存在时返回覆盖值，否则返回基础步态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	ELRMovementPace GetEffectivePace() const;
++	/**
++	 * @brief 以低频计时器累计角色实际位移，达到步长后按步态×环境解析并发布脚步而不使用 Tick。
+ 	 */
+ 	void SampleTravelDistance();
+ 	/**
+@@ -95,10 +135,14 @@ private:
+ 	 */
+ 	float GetStepDistance() const;
+ 	/**
+-	 * @brief 查询 Noise Radius；不修改领域状态。
+-	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 * @brief 按有效步态同步 CharacterMovement 的 MaxWalkSpeed。
+ 	 */
+-	float GetNoiseRadius() const;
++	void SyncMovementSpeed();
++	/**
++	 * @brief 对禁止的步态请求统一记录日志并广播拒绝事件。
++	 * @param requestedPace 本次操作使用的 `requestedPace` 枚举或模式值。
++	 */
++	void RejectPaceRequest(ELRMovementPace requestedPace);
+ 
+ 	/** Character 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+ 	UPROPERTY(Transient)
+@@ -108,12 +152,20 @@ private:
+ 	UPROPERTY(Transient)
+ 	TObjectPtr<ULRMovementTuning> Tuning;
+ 
++	/** State 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TWeakObjectPtr<ULRStateComponent> State;
++
+ 	/** Pace 的内部运行时数据；不参与蓝图配置。 */
+ 	ELRMovementPace Pace = ELRMovementPace::Walk;
+ 	/** Pace Before Run 的内部运行时数据；不参与蓝图配置。 */
+ 	ELRMovementPace PaceBeforeRun = ELRMovementPace::Walk;
+ 	/** Noise Environment 的内部运行时数据；不参与蓝图配置。 */
+-	ELRNoiseEnvironment NoiseEnvironment = ELRNoiseEnvironment::Indoor;
++	ELRNoiseEnvironment NoiseEnvironment = ELRNoiseEnvironment::Outdoor;
++	/** Pace Override 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	ELRMovementPace PaceOverride = ELRMovementPace::Walk;
++	/** Pace Override Source 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	FGameplayTag PaceOverrideSource;
+ 	/** Last Sample Location 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+ 	FVector LastSampleLocation = FVector::ZeroVector;
+ 	/** Distance Since Footstep 的内部运行时数据；不参与蓝图配置。 */
+diff --git a/Source/LostRunic/Gameplay/LRMovementRules.cpp b/Source/LostRunic/Gameplay/LRMovementRules.cpp
+new file mode 100644
+index 0000000..a100536
+--- /dev/null
++++ b/Source/LostRunic/Gameplay/LRMovementRules.cpp
+@@ -0,0 +1,123 @@
++/**
++ * @file LRMovementRules.cpp
++ * @brief 实现移动纯规则：状态×步态合法性矩阵、默认步态、步态×环境脚步噪声解析、噪声环境优先级与室内奔跑房间警戒目标值。
++ *
++ * 关联文件：LRMovementRules.h；所属领域：Gameplay。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "Gameplay/LRMovementRules.h"
++
++#include "Core/LRGameplayTags.h"
++#include "Data/LRGuardTuning.h"
++#include "Data/LRMovementTuning.h"
++
++/**
++ * @brief 判断 Is Pace Allowed 对应条件；Normal 全步态，Perception 仅潜行，Courage 走路+奔跑，Memory 仅走路。
++ * @param mode 本次操作使用的 `mode` 枚举或模式值。
++ * @param pace 本次操作使用的 `pace` 枚举或模式值。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++bool LRMovementRules::IsPaceAllowed(const ELRPerceptionMode mode, const ELRMovementPace pace)
++{
++	switch (mode)
++	{
++	case ELRPerceptionMode::Normal:
++		return true;
++	case ELRPerceptionMode::Perception:
++		return pace == ELRMovementPace::Sneak;
++	case ELRPerceptionMode::Courage:
++		return pace != ELRMovementPace::Sneak;
++	case ELRPerceptionMode::Memory:
++		return pace == ELRMovementPace::Walk;
++	}
++	return false;
++}
++
++/**
++ * @brief 查询 Get Default Pace 对应条件；进入状态时强制应用。
++ * @param mode 本次操作使用的 `mode` 枚举或模式值。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRMovementPace LRMovementRules::GetDefaultPace(const ELRPerceptionMode mode)
++{
++	return mode == ELRPerceptionMode::Perception ? ELRMovementPace::Sneak : ELRMovementPace::Walk;
++}
++
++/**
++ * @brief 按步态×环境解析脚步噪声；潜行无声（半径 0 + Sneak 标签，仅供动画/表现），室内奔跑返回 Run.Indoor 标签（房间传播在组件层处理，此半径仅作无房间兜底）。
++ * @param pace 本次操作使用的 `pace` 枚举或模式值。
++ * @param environment 本次操作使用的 `environment` 枚举或模式值。
++ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++FLRNoiseResolution LRMovementRules::ResolveFootstepNoise(const ELRMovementPace pace,
++	const ELRNoiseEnvironment environment, const ULRMovementTuning& tuning)
++{
++	FLRNoiseResolution resolution;
++	switch (pace)
++	{
++	case ELRMovementPace::Sneak:
++		resolution.Radius = 0.0f;
++		resolution.Tag = LRGameplayTags::NoiseFootstepSneak;
++		return resolution;
++	case ELRMovementPace::Run:
++		if (environment == ELRNoiseEnvironment::Indoor)
++		{
++			resolution.Radius = tuning.IndoorRunNoiseRadius;
++			resolution.Tag = LRGameplayTags::NoiseFootstepRunIndoor;
++			return resolution;
++		}
++		resolution.Radius = environment == ELRNoiseEnvironment::OutdoorStealth
++			? tuning.OutdoorStealthRunNoiseRadius : tuning.OutdoorNoiseRadius;
++		resolution.Tag = LRGameplayTags::NoiseFootstepRun;
++		return resolution;
++	case ELRMovementPace::Walk:
++		break;
++	}
++
++	resolution.Radius = environment == ELRNoiseEnvironment::Indoor
++		? tuning.IndoorWalkNoiseRadius : tuning.OutdoorNoiseRadius;
++	resolution.Tag = environment == ELRNoiseEnvironment::Outdoor
++		? LRGameplayTags::NoiseFootstepWalkFaint : LRGameplayTags::NoiseFootstepWalk;
++	return resolution;
++}
++
++/**
++ * @brief 按固定优先级从重叠集合解析环境：Indoor > OutdoorStealth > Outdoor；空集合默认 Outdoor。
++ * @param environments 调用方提供的 `environments`，只在本次操作范围内使用。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++ELRNoiseEnvironment LRMovementRules::ResolveEnvironmentFromSet(const TArray<ELRNoiseEnvironment>& environments)
++{
++	ELRNoiseEnvironment resolved = ELRNoiseEnvironment::Outdoor;
++	for (const ELRNoiseEnvironment environment : environments)
++	{
++		if (environment == ELRNoiseEnvironment::Indoor)
++		{
++			return ELRNoiseEnvironment::Indoor;
++		}
++		if (environment == ELRNoiseEnvironment::OutdoorStealth)
++		{
++			resolved = ELRNoiseEnvironment::OutdoorStealth;
++		}
++	}
++	return resolved;
++}
++
++/**
++ * @brief 解析室内奔跑的房间警戒目标值：当前房间 max(当前警戒, RoomRunAlertLevel)；相邻房间 max(当前警戒, 当前警戒+AdjacentRoomRunAlertAmount)。多房间候选取最大由调用方完成，不在此累加。
++ * @param bCurrentRoom 布尔开关 `bCurrentRoom`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
++ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++int32 LRMovementRules::ResolveRoomRunTargetLevel(const bool bCurrentRoom, const int32 currentAlert,
++	const ULRGuardTuning& tuning)
++{
++	if (bCurrentRoom)
++	{
++		return FMath::Max(currentAlert, tuning.RoomRunAlertLevel);
++	}
++	return FMath::Max(currentAlert, currentAlert + tuning.AdjacentRoomRunAlertAmount);
++}
+diff --git a/Source/LostRunic/Gameplay/LRMovementRules.h b/Source/LostRunic/Gameplay/LRMovementRules.h
+new file mode 100644
+index 0000000..e80d99e
+--- /dev/null
++++ b/Source/LostRunic/Gameplay/LRMovementRules.h
+@@ -0,0 +1,65 @@
++/**
++ * @file LRMovementRules.h
++ * @brief 提供移动纯规则：状态×步态合法性矩阵、默认步态、步态×环境脚步噪声解析、噪声环境优先级与室内奔跑房间警戒目标值，供运行时组件与 LostRunic.Movement 自动化测试共同调用。
++ *
++ * 关联文件：LRMovementRules.cpp；所属领域：Gameplay。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "Core/LRTypes.h"
++#include "GameplayTagContainer.h"
++
++class ULRGuardTuning;
++class ULRMovementTuning;
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++struct LOSTRUNIC_API FLRNoiseResolution
++{
++	/** Radius 的空间值 `Radius`；距离和位置使用 Unreal 厘米单位。 C++ 安全默认值为 `0.0f`。 */
++	float Radius = 0.0f;
++	/** Tag 的领域数据，由所属类型负责维护和校验。  */
++	FGameplayTag Tag;
++};
++
++namespace LRMovementRules
++{
++	/**
++	 * @brief 判断 Is Pace Allowed 对应条件；Normal 全步态，Perception 仅潜行，Courage 走路+奔跑，Memory 仅走路。
++	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
++	 * @param pace 本次操作使用的 `pace` 枚举或模式值。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API bool IsPaceAllowed(ELRPerceptionMode mode, ELRMovementPace pace);
++	/**
++	 * @brief 查询 Get Default Pace 对应条件；进入状态时强制应用。
++	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API ELRMovementPace GetDefaultPace(ELRPerceptionMode mode);
++	/**
++	 * @brief 按步态×环境解析脚步噪声；潜行无声（半径 0 + Sneak 标签，仅供动画/表现），室内奔跑返回 Run.Indoor 标签（房间传播在组件层处理，此半径仅作无房间兜底）。
++	 * @param pace 本次操作使用的 `pace` 枚举或模式值。
++	 * @param environment 本次操作使用的 `environment` 枚举或模式值。
++	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API FLRNoiseResolution ResolveFootstepNoise(ELRMovementPace pace, ELRNoiseEnvironment environment,
++		const ULRMovementTuning& tuning);
++	/**
++	 * @brief 按固定优先级从重叠集合解析环境：Indoor > OutdoorStealth > Outdoor；空集合默认 Outdoor。
++	 * @param environments 调用方提供的 `environments`，只在本次操作范围内使用。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API ELRNoiseEnvironment ResolveEnvironmentFromSet(const TArray<ELRNoiseEnvironment>& environments);
++	/**
++	 * @brief 解析室内奔跑的房间警戒目标值：当前房间 max(当前警戒, RoomRunAlertLevel)；相邻房间 max(当前警戒, 当前警戒+AdjacentRoomRunAlertAmount)。多房间候选取最大由调用方完成，不在此累加。
++	 * @param bCurrentRoom 布尔开关 `bCurrentRoom`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
++	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	LOSTRUNIC_API int32 ResolveRoomRunTargetLevel(bool bCurrentRoom, int32 currentAlert,
++		const ULRGuardTuning& tuning);
++}
+diff --git a/Source/LostRunic/Gameplay/LRNoiseArea.cpp b/Source/LostRunic/Gameplay/LRNoiseArea.cpp
+index ceda6d6..c6a19e4 100644
+--- a/Source/LostRunic/Gameplay/LRNoiseArea.cpp
++++ b/Source/LostRunic/Gameplay/LRNoiseArea.cpp
+@@ -1,6 +1,6 @@
+ /**
+  * @file LRNoiseArea.cpp
+- * @brief 实现角色移动模式、按移动距离产生脚步和室内外噪声区域等基础玩法能力；数值来自调优资产，不使用无理由 Tick。
++ * @brief 实现角色移动模式、按移动距离产生脚步和室内外噪声区域等基础玩法能力；区域维护进入/退出集合，重叠按固定优先级解析，无区域时默认 Outdoor。
+  *
+  * 关联文件：LRNoiseArea.h；所属领域：Gameplay。
+  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+@@ -9,7 +9,9 @@
+ #include "Gameplay/LRNoiseArea.h"
+ 
+ #include "Components/BoxComponent.h"
++#include "EngineUtils.h"
+ #include "Gameplay/LRLocomotionComponent.h"
++#include "Gameplay/LRMovementRules.h"
+ 
+ /**
+  * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+@@ -21,10 +23,11 @@ ALRNoiseArea::ALRNoiseArea()
+ 	SetRootComponent(Bounds);
+ 	Bounds->SetCollisionProfileName(TEXT("Trigger"));
+ 	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ALRNoiseArea::HandleBeginOverlap);
++	Bounds->OnComponentEndOverlap.AddDynamic(this, &ALRNoiseArea::HandleEndOverlap);
+ }
+ 
+ /**
+- * @brief 处理 Handle Begin Overlap 事件，将引擎回调转换为对应领域状态更新。
++ * @brief 处理 Handle Begin Overlap 事件，将引擎回调转换为对应领域状态更新；加入集合后重新求值环境。
+  * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
+  * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
+  * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
+@@ -35,8 +38,50 @@ ALRNoiseArea::ALRNoiseArea()
+ void ALRNoiseArea::HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+ 	const int32 otherBodyIndex, const bool bFromSweep, const FHitResult& sweepResult)
+ {
+-	if (ULRLocomotionComponent* locomotion = otherActor ? otherActor->FindComponentByClass<ULRLocomotionComponent>() : nullptr)
++	if (!otherActor || !otherActor->FindComponentByClass<ULRLocomotionComponent>())
+ 	{
+-		locomotion->SetNoiseEnvironment(Environment);
++		return;
+ 	}
++	OverlappingActors.AddUnique(otherActor);
++	RefreshActorEnvironment(otherActor);
++}
++
++/**
++ * @brief 处理 Handle End Overlap 事件，将引擎回调转换为对应领域状态更新；离开区域后按剩余重叠集合重新求值环境。
++ * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
++ * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
++ * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
++ * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
++ */
++void ALRNoiseArea::HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
++	const int32 otherBodyIndex)
++{
++	if (!otherActor)
++	{
++		return;
++	}
++	OverlappingActors.Remove(otherActor);
++	RefreshActorEnvironment(otherActor);
++}
++
++/**
++ * @brief 按固定优先级（Indoor > OutdoorStealth > Outdoor）从所有覆盖该角色的噪声区域重新解析环境并应用；无区域时默认 Outdoor。
++ * @param actor 本次查询、交互或事件涉及的 Actor。
++ */
++void ALRNoiseArea::RefreshActorEnvironment(AActor* actor)
++{
++	ULRLocomotionComponent* locomotion = actor ? actor->FindComponentByClass<ULRLocomotionComponent>() : nullptr;
++	if (!locomotion || !GetWorld())
++	{
++		return;
++	}
++	TArray<ELRNoiseEnvironment> environments;
++	for (TActorIterator<ALRNoiseArea> it(GetWorld()); it; ++it)
++	{
++		if (it->OverlappingActors.Contains(actor))
++		{
++			environments.Add(it->Environment);
++		}
++	}
++	locomotion->SetNoiseEnvironment(LRMovementRules::ResolveEnvironmentFromSet(environments));
+ }
+diff --git a/Source/LostRunic/Gameplay/LRNoiseArea.h b/Source/LostRunic/Gameplay/LRNoiseArea.h
+index dc417f2..32ce87b 100644
+--- a/Source/LostRunic/Gameplay/LRNoiseArea.h
++++ b/Source/LostRunic/Gameplay/LRNoiseArea.h
+@@ -14,6 +14,7 @@
+ #include "LRNoiseArea.generated.h"
+ 
+ class UBoxComponent;
++class ULRLocomotionComponent;
+ 
+ /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+ UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Noise Area"))
+@@ -41,10 +42,30 @@ private:
+ 	void HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+ 		int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);
+ 
++	/**
++	 * @brief 处理 Handle End Overlap 事件，将引擎回调转换为对应领域状态更新；离开区域后按剩余重叠集合重新求值环境。
++	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
++	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
++	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
++	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
++	 */
++	UFUNCTION()
++	void HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
++		int32 otherBodyIndex);
++
++	/**
++	 * @brief 按固定优先级（Indoor > OutdoorStealth > Outdoor）从所有覆盖该角色的噪声区域重新解析环境并应用；无区域时默认 Outdoor。
++	 * @param actor 本次查询、交互或事件涉及的 Actor。
++	 */
++	void RefreshActorEnvironment(AActor* actor);
++
+ 	/** Bounds 的开关；true 表示启用，false 表示禁用。 仅在蓝图或详情面板中查看，不可编辑。 */
+ 	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Noise", meta = (AllowPrivateAccess = "true"))
+ 	TObjectPtr<UBoxComponent> Bounds;
+ 
++	/** Overlapping Actors 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	TArray<TWeakObjectPtr<AActor>> OverlappingActors;
++
+ 	/** Environment 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNoiseEnvironment::Indoor`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
+ 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (AllowPrivateAccess = "true"))
+ 	ELRNoiseEnvironment Environment = ELRNoiseEnvironment::Indoor;
+diff --git a/Source/LostRunic/Gameplay/LRRoomVolume.cpp b/Source/LostRunic/Gameplay/LRRoomVolume.cpp
+new file mode 100644
+index 0000000..3eb306c
+--- /dev/null
++++ b/Source/LostRunic/Gameplay/LRRoomVolume.cpp
+@@ -0,0 +1,91 @@
++/**
++ * @file LRRoomVolume.cpp
++ * @brief 实现室内奔跑噪声的房间传播体积：重叠守卫集合维护、相邻房间拓扑与旋转体积的局部空间包含判定。
++ *
++ * 关联文件：LRRoomVolume.h；所属领域：Gameplay。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "Gameplay/LRRoomVolume.h"
++
++#include "AI/LRAlertComponent.h"
++#include "Components/BoxComponent.h"
++#include "EngineUtils.h"
++
++/**
++ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++ */
++ALRRoomVolume::ALRRoomVolume()
++{
++	PrimaryActorTick.bCanEverTick = false;
++	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
++	SetRootComponent(Bounds);
++	Bounds->SetCollisionProfileName(TEXT("Trigger"));
++	Bounds->SetGenerateOverlapEvents(true);
++	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ALRRoomVolume::HandleBeginOverlap);
++	Bounds->OnComponentEndOverlap.AddDynamic(this, &ALRRoomVolume::HandleEndOverlap);
++}
++
++/**
++ * @brief 查询所有覆盖指定位置（支持旋转体积，BoxComponent 局部空间 + extent 判定）的房间体积。
++ * @param world 本次查询所在的 World。
++ * @param location 世界空间位置，Unreal 单位为厘米。
++ * @param outRooms 输出匹配的房间体积集合。
++ */
++void ALRRoomVolume::FindRoomsAtLocation(const UWorld* world, const FVector location, TArray<ALRRoomVolume*>& outRooms)
++{
++	if (!world)
++	{
++		return;
++	}
++	for (TActorIterator<ALRRoomVolume> it(world); it; ++it)
++	{
++		const UBoxComponent* bounds = it->Bounds.Get();
++		if (!bounds)
++		{
++			continue;
++		}
++		// 局部空间 + extent 判定，支持旋转体积；AABB 近似会保守误报，注释说明。
++		const FVector local = bounds->GetComponentTransform().InverseTransformPosition(location);
++		const FVector halfExtent = bounds->GetUnscaledBoxExtent() * bounds->GetComponentScale();
++		if (FMath::Abs(local.X) <= halfExtent.X && FMath::Abs(local.Y) <= halfExtent.Y
++			&& FMath::Abs(local.Z) <= halfExtent.Z)
++		{
++			outRooms.Add(*it);
++		}
++	}
++}
++
++/**
++ * @brief 处理 Handle Begin Overlap 事件：带警戒组件的守卫加入房间集合。
++ * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
++ * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
++ * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
++ * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
++ * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++ * @param sweepResult 本次领域操作的结构化数据 `sweepResult`；字段语义由对应 USTRUCT 定义。
++ */
++void ALRRoomVolume::HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
++	const int32 otherBodyIndex, const bool bFromSweep, const FHitResult& sweepResult)
++{
++	if (otherActor && otherActor->FindComponentByClass<ULRAlertComponent>())
++	{
++		OverlappingGuards.AddUnique(otherActor);
++	}
++}
++
++/**
++ * @brief 处理 Handle End Overlap 事件：守卫离开房间集合。
++ * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
++ * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
++ * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
++ * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
++ */
++void ALRRoomVolume::HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
++	const int32 otherBodyIndex)
++{
++	if (otherActor)
++	{
++		OverlappingGuards.Remove(otherActor);
++	}
++}
+diff --git a/Source/LostRunic/Gameplay/LRRoomVolume.h b/Source/LostRunic/Gameplay/LRRoomVolume.h
+new file mode 100644
+index 0000000..45c5557
+--- /dev/null
++++ b/Source/LostRunic/Gameplay/LRRoomVolume.h
+@@ -0,0 +1,89 @@
++/**
++ * @file LRRoomVolume.h
++ * @brief 定义室内奔跑噪声的房间传播体积：维护重叠守卫集合与相邻房间拓扑，支持旋转体积的局部空间包含判定；房间传播只广播表现事件，不调用 ReportNoiseEvent（防双计）。
++ *
++ * 关联文件：LRRoomVolume.cpp；所属领域：Gameplay。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "GameFramework/Actor.h"
++
++#include "LRRoomVolume.generated.h"
++
++class UBoxComponent;
++class ULRAlertComponent;
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Room Volume"))
++class LOSTRUNIC_API ALRRoomVolume : public AActor
++{
++	GENERATED_BODY()
++
++public:
++	/**
++	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
++	 */
++	ALRRoomVolume();
++
++	/**
++	 * @brief 查询所有覆盖指定位置（支持旋转体积，BoxComponent 局部空间 + extent 判定）的房间体积。
++	 * @param world 本次查询所在的 World。
++	 * @param location 世界空间位置，Unreal 单位为厘米。
++	 * @param outRooms 输出匹配的房间体积集合。
++	 */
++	static void FindRoomsAtLocation(const UWorld* world, const FVector location, TArray<ALRRoomVolume*>& outRooms);
++
++	/**
++	 * @brief 查询 Overlapping Guards；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	const TArray<TWeakObjectPtr<AActor>>& GetOverlappingGuards() const { return OverlappingGuards; }
++
++	/**
++	 * @brief 查询 Adjacent Rooms；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	const TArray<TWeakObjectPtr<ALRRoomVolume>>& GetAdjacentRooms() const { return AdjacentRooms; }
++
++private:
++	/**
++	 * @brief 处理 Handle Begin Overlap 事件：带警戒组件的守卫加入房间集合。
++	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
++	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
++	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
++	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
++	 * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
++	 * @param sweepResult 本次领域操作的结构化数据 `sweepResult`；字段语义由对应 USTRUCT 定义。
++	 */
++	UFUNCTION()
++	void HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
++		int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);
++
++	/**
++	 * @brief 处理 Handle End Overlap 事件：守卫离开房间集合。
++	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
++	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
++	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
++	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
++	 */
++	UFUNCTION()
++	void HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
++		int32 otherBodyIndex);
++
++	/** Bounds 的开关；true 表示启用，false 表示禁用。 仅在蓝图或详情面板中查看，不可编辑。 */
++	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room", meta = (AllowPrivateAccess = "true"))
++	TObjectPtr<UBoxComponent> Bounds;
++
++	/** Room Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在关卡中的蓝图实例详情面板配置。 */
++	UPROPERTY(EditInstanceOnly, Category = "Room")
++	FName RoomId = NAME_None;
++
++	/** Adjacent Rooms 的领域数据，由所属类型负责维护和校验。 可在关卡中的蓝图实例详情面板配置。 */
++	UPROPERTY(EditInstanceOnly, Category = "Room")
++	TArray<TWeakObjectPtr<ALRRoomVolume>> AdjacentRooms;
++
++	/** Overlapping Guards 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	TArray<TWeakObjectPtr<AActor>> OverlappingGuards;
++};
+diff --git a/Source/LostRunic/Input/LRInputConfig.cpp b/Source/LostRunic/Input/LRInputConfig.cpp
+index 876569c..ece667a 100644
+--- a/Source/LostRunic/Input/LRInputConfig.cpp
++++ b/Source/LostRunic/Input/LRInputConfig.cpp
+@@ -24,7 +24,7 @@ bool ULRInputConfig::Validate(FString& outError) const
+ 		outError = TEXT("All four input mapping contexts are required.");
+ 		return false;
+ 	}
+-	if (!MoveAction || !SneakAction || !RunAction || !InteractAction || !CloseEyesAction || !OpenEyesAction)
++	if (!MoveAction || !RunAction || !InteractAction || !CloseEyesAction || !OpenEyesAction)
+ 	{
+ 		outError = TEXT("Gameplay movement, interaction, and state actions are required.");
+ 		return false;
+diff --git a/Source/LostRunic/Input/LRInputConfig.h b/Source/LostRunic/Input/LRInputConfig.h
+index 59fdd92..1920c7e 100644
+--- a/Source/LostRunic/Input/LRInputConfig.h
++++ b/Source/LostRunic/Input/LRInputConfig.h
+@@ -43,8 +43,8 @@ public:
+ 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
+ 	TObjectPtr<UInputAction> MoveAction;
+ 
+-	/** Sneak Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+-	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
++	/** 已废弃：Sneak 与潜行切换语义重复，实际潜行切换由 ToggleCrouchAction 驱动，保留仅用于资产兼容。 */
++	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement|Deprecated", meta = (DeprecatedProperty, DisplayName = "Sneak Action (Deprecated)"))
+ 	TObjectPtr<UInputAction> SneakAction;
+ 
+ 	/** Run Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+diff --git a/Source/LostRunic/State/LRStatePresentationComponent.cpp b/Source/LostRunic/State/LRStatePresentationComponent.cpp
+index c46854e..46d19cd 100644
+--- a/Source/LostRunic/State/LRStatePresentationComponent.cpp
++++ b/Source/LostRunic/State/LRStatePresentationComponent.cpp
+@@ -8,6 +8,10 @@
+  */
+ #include "State/LRStatePresentationComponent.h"
+ 
++#include "Data/LRGameTuningSet.h"
++#include "Data/LRPresentationTuning.h"
++#include "Engine/GameInstance.h"
++#include "Framework/LRGameInstanceSubsystem.h"
+ #include "State/LRStateComponent.h"
+ 
+ /**
+@@ -25,12 +29,60 @@ void ULRStatePresentationComponent::BeginPlay()
+ {
+ 	Super::BeginPlay();
+ 	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
++	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
++	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
++	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Presentation : nullptr;
+ 	if (ensureMsgf(StateComponent, TEXT("%s requires a sibling LRStateComponent."), *GetNameSafe(this)))
+ 	{
+ 		StateComponent->OnStateChanging.AddDynamic(this, &ULRStatePresentationComponent::HandleStateChanging);
+ 	}
+ }
+ 
++/**
++ * @brief 查询 Perception Reveal Radius（角色周围显现半径，设计 4.5m）；艺术表现预留接入点。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++float ULRStatePresentationComponent::GetPerceptionRevealRadius() const
++{
++	return Tuning ? Tuning->PerceptionRevealRadius : GetDefault<ULRPresentationTuning>()->PerceptionRevealRadius;
++}
++
++/**
++ * @brief 查询 Noise Reveal Radius（声源周围显现半径，设计 2m）；艺术表现预留接入点。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++float ULRStatePresentationComponent::GetNoiseRevealRadius() const
++{
++	return Tuning ? Tuning->NoiseRevealRadius : GetDefault<ULRPresentationTuning>()->NoiseRevealRadius;
++}
++
++/**
++ * @brief 查询 Noise Reveal Duration Seconds（声源显现时长，设计 5s）；艺术表现预留接入点。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++float ULRStatePresentationComponent::GetNoiseRevealDurationSeconds() const
++{
++	return Tuning ? Tuning->NoiseRevealDurationSeconds : GetDefault<ULRPresentationTuning>()->NoiseRevealDurationSeconds;
++}
++
++/**
++ * @brief 查询 Perception Blend Weight（感知后处理混合权重）；艺术表现预留接入点。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++float ULRStatePresentationComponent::GetPerceptionBlendWeight() const
++{
++	return Tuning ? Tuning->PerceptionBlendWeight : GetDefault<ULRPresentationTuning>()->PerceptionBlendWeight;
++}
++
++/**
++ * @brief 查询 Courage Blend Weight（勇气后处理混合权重）；艺术表现预留接入点。
++ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++ */
++float ULRStatePresentationComponent::GetCourageBlendWeight() const
++{
++	return Tuning ? Tuning->CourageBlendWeight : GetDefault<ULRPresentationTuning>()->CourageBlendWeight;
++}
++
+ /**
+  * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
+  * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
+diff --git a/Source/LostRunic/State/LRStatePresentationComponent.h b/Source/LostRunic/State/LRStatePresentationComponent.h
+index 28e5a08..38416c0 100644
+--- a/Source/LostRunic/State/LRStatePresentationComponent.h
++++ b/Source/LostRunic/State/LRStatePresentationComponent.h
+@@ -14,6 +14,7 @@
+ 
+ #include "LRStatePresentationComponent.generated.h"
+ 
++class ULRPresentationTuning;
+ class ULRStateComponent;
+ 
+ DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRStatePresentationRequested,
+@@ -47,6 +48,37 @@ public:
+ 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Presentation")
+ 	void CompleteStatePresentation();
+ 
++	/**
++	 * @brief 查询 Perception Reveal Radius（角色周围显现半径，设计 4.5m）；艺术表现预留接入点。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
++	float GetPerceptionRevealRadius() const;
++	/**
++	 * @brief 查询 Noise Reveal Radius（声源周围显现半径，设计 2m）；艺术表现预留接入点。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
++	float GetNoiseRevealRadius() const;
++	/**
++	 * @brief 查询 Noise Reveal Duration Seconds（声源显现时长，设计 5s）；艺术表现预留接入点。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
++	float GetNoiseRevealDurationSeconds() const;
++	/**
++	 * @brief 查询 Perception Blend Weight（感知后处理混合权重）；艺术表现预留接入点。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
++	float GetPerceptionBlendWeight() const;
++	/**
++	 * @brief 查询 Courage Blend Weight（勇气后处理混合权重）；艺术表现预留接入点。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
++	float GetCourageBlendWeight() const;
++
+ 	/** Broadcast when Blueprint should play the visual transition, then report completion. */
+ 	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Presentation")
+ 	FLRStatePresentationRequested OnStatePresentationRequested;
+@@ -74,4 +106,8 @@ private:
+ 	/** State Component 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+ 	UPROPERTY(Transient)
+ 	TObjectPtr<ULRStateComponent> StateComponent;
++
++	/** 表现调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TObjectPtr<ULRPresentationTuning> Tuning;
+ };
+diff --git a/Source/LostRunic/Stealth/LRHideComponent.cpp b/Source/LostRunic/Stealth/LRHideComponent.cpp
+index 877b708..a6e55f3 100644
+--- a/Source/LostRunic/Stealth/LRHideComponent.cpp
++++ b/Source/LostRunic/Stealth/LRHideComponent.cpp
+@@ -9,8 +9,10 @@
+ #include "Stealth/LRHideComponent.h"
+ 
+ #include "Core/LRGameplayTags.h"
++#include "Framework/LRCharacter.h"
+ #include "GameFramework/Character.h"
+ #include "GameFramework/CharacterMovementComponent.h"
++#include "Gameplay/LRLocomotionComponent.h"
+ #include "State/LRStateComponent.h"
+ #include "Stealth/LRHidePoint.h"
+ 
+@@ -30,6 +32,7 @@ void ULRHideComponent::BeginPlay()
+ 	Super::BeginPlay();
+ 	Character = Cast<ACharacter>(GetOwner());
+ 	State = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
++	Locomotion = Cast<ALRCharacter>(GetOwner()) ? Cast<ALRCharacter>(GetOwner())->GetLocomotionComponent() : nullptr;
+ 	ensureMsgf(Character && State, TEXT("%s requires an ACharacter owner and State component."), *GetNameSafe(this));
+ }
+ 
+@@ -62,6 +65,11 @@ bool ULRHideComponent::EnterHidePoint(ALRHidePoint* hidePoint)
+ 		Character->GetCharacterMovement()->DisableMovement();
+ 	}
+ 	State->SetBlockerActive(LRGameplayTags::StateBlockerHidden, true);
++	if (Locomotion)
++	{
++		// 掩体强制潜行覆盖；退出时按当前状态重新求值，不恢复可能过期的缓存步态。
++		Locomotion->OverridePace(ELRMovementPace::Sneak, LRGameplayTags::MovementOverrideHidden);
++	}
+ 	OnHiddenStateChanged.Broadcast(true, hidePoint);
+ 	return true;
+ }
+@@ -85,6 +93,10 @@ bool ULRHideComponent::ExitHidePoint()
+ 	}
+ 	bMovementLockedByHide = false;
+ 	State->SetBlockerActive(LRGameplayTags::StateBlockerHidden, false);
++	if (Locomotion)
++	{
++		Locomotion->ClearPaceOverride(LRGameplayTags::MovementOverrideHidden);
++	}
+ 	OnHiddenStateChanged.Broadcast(false, nullptr);
+ 	return true;
+ }
+diff --git a/Source/LostRunic/Stealth/LRHideComponent.h b/Source/LostRunic/Stealth/LRHideComponent.h
+index 24bf89f..81d780f 100644
+--- a/Source/LostRunic/Stealth/LRHideComponent.h
++++ b/Source/LostRunic/Stealth/LRHideComponent.h
+@@ -15,6 +15,7 @@
+ 
+ class ACharacter;
+ class ALRHidePoint;
++class ULRLocomotionComponent;
+ class ULRStateComponent;
+ 
+ DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRHiddenStateChanged, bool, bHidden, ALRHidePoint*, hidePoint);
+@@ -84,6 +85,10 @@ private:
+ 	UPROPERTY(Transient)
+ 	TObjectPtr<ULRStateComponent> State;
+ 
++	/** Locomotion 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TObjectPtr<ULRLocomotionComponent> Locomotion;
++
+ 	/** Current Hide Point 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+ 	UPROPERTY(Transient)
+ 	TWeakObjectPtr<ALRHidePoint> CurrentHidePoint;
+diff --git a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp
+index aad8d60..46d056a 100644
+--- a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp
++++ b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp
+@@ -8,13 +8,17 @@
+  */
+ #include "Stealth/LRNoiseEmitterComponent.h"
+ 
++#include "AI/LRAlertComponent.h"
+ #include "Core/LRGameplayTags.h"
+ #include "Data/LRGameTuningSet.h"
++#include "Data/LRGuardTuning.h"
+ #include "Data/LRMovementTuning.h"
+ #include "Engine/GameInstance.h"
+ #include "Engine/World.h"
+ #include "Framework/LRGameInstanceSubsystem.h"
+ #include "Gameplay/LRLocomotionComponent.h"
++#include "Gameplay/LRMovementRules.h"
++#include "Gameplay/LRRoomVolume.h"
+ #include "Interaction/LRInteractionComponent.h"
+ #include "Interaction/LRInteractionTypes.h"
+ #include "Perception/AISense_Hearing.h"
+@@ -38,6 +42,7 @@ void ULRNoiseEmitterComponent::BeginPlay()
+ 	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
+ 	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
+ 	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
++	GuardTuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
+ 	if (!ensureMsgf(Locomotion && Interaction && Tuning, TEXT("%s requires locomotion, interaction, and Movement tuning."),
+ 		*GetNameSafe(this)))
+ 	{
+@@ -88,9 +93,76 @@ void ULRNoiseEmitterComponent::EmitNoise(const FVector location, const float rad
+  */
+ void ULRNoiseEmitterComponent::HandleFootstep(const FVector location, const float radius, const FGameplayTag reason)
+ {
++	if (reason == LRGameplayTags::NoiseFootstepRunIndoor)
++	{
++		ApplyIndoorRunNoise(location);
++		return;
++	}
+ 	EmitNoise(location, radius, reason);
+ }
+ 
++/**
++ * @brief 室内奔跑噪声：房间传播优先（当前房警戒至少提升到 RoomRunAlertLevel、相邻房 +1，多房间候选目标值取最大、一次应用）；无房间时回退 1200 半径听觉事件；始终广播 OnNoiseEmitted 供表现钩子，绝不调用 ReportNoiseEvent（防双计）。
++ * @param location 世界空间位置，Unreal 单位为厘米。
++ */
++void ULRNoiseEmitterComponent::ApplyIndoorRunNoise(const FVector location)
++{
++	if (!GetWorld() || !GuardTuning || !Tuning)
++	{
++		return;
++	}
++
++	TArray<ALRRoomVolume*> rooms;
++	ALRRoomVolume::FindRoomsAtLocation(GetWorld(), location, rooms);
++	if (rooms.Num() == 0)
++	{
++		EmitNoise(location, Tuning->IndoorRunNoiseRadius, LRGameplayTags::NoiseFootstepRunIndoor);
++		return;
++	}
++
++	// 对每位守卫收集其所属房间的候选目标值：当前房与相邻房（多房间取最大、不累加），一次应用。
++	TSet<AActor*> applied;
++	for (const ALRRoomVolume* room : rooms)
++	{
++		for (const TWeakObjectPtr<AActor>& guardWeak : room->GetOverlappingGuards())
++		{
++			AActor* guard = guardWeak.Get();
++			ULRAlertComponent* alert = guard ? guard->FindComponentByClass<ULRAlertComponent>() : nullptr;
++			if (!alert || applied.Contains(guard))
++			{
++				continue;
++			}
++			applied.Add(guard);
++			const int32 currentAlert = alert->GetAlertLevel();
++			int32 bestTarget = currentAlert;
++			for (const ALRRoomVolume* containingRoom : rooms)
++			{
++				if (containingRoom->GetOverlappingGuards().Contains(guard))
++				{
++					bestTarget = FMath::Max(bestTarget,
++						LRMovementRules::ResolveRoomRunTargetLevel(true, currentAlert, *GuardTuning));
++				}
++				for (const TWeakObjectPtr<ALRRoomVolume>& adjacentWeak : containingRoom->GetAdjacentRooms())
++				{
++					const ALRRoomVolume* adjacent = adjacentWeak.Get();
++					if (adjacent && adjacent->GetOverlappingGuards().Contains(guard))
++					{
++						bestTarget = FMath::Max(bestTarget,
++							LRMovementRules::ResolveRoomRunTargetLevel(false, currentAlert, *GuardTuning));
++					}
++				}
++			}
++			if (bestTarget > currentAlert)
++			{
++				alert->ApplyAlertDelta(bestTarget - currentAlert, location, GetOwner(), LRGameplayTags::NoiseFootstepRunIndoor);
++			}
++		}
++	}
++
++	// 表现钩子：房间路径只广播表现事件，绝不 ReportNoiseEvent（防与听觉分支双计）。
++	OnNoiseEmitted.Broadcast(location, Tuning->IndoorRunNoiseRadius, LRGameplayTags::NoiseFootstepRunIndoor);
++}
++
+ /**
+  * @brief 处理 Handle Interaction 事件，将引擎回调转换为对应领域状态更新。
+  * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
+diff --git a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h
+index 3cb6ed4..a81a9e6 100644
+--- a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h
++++ b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h
+@@ -13,6 +13,7 @@
+ 
+ #include "LRNoiseEmitterComponent.generated.h"
+ 
++class ULRGuardTuning;
+ class ULRInteractionComponent;
+ class ULRLocomotionComponent;
+ class ULRMovementTuning;
+@@ -65,6 +66,12 @@ private:
+ 	UFUNCTION()
+ 	void HandleFootstep(FVector location, float radius, FGameplayTag reason);
+ 
++	/**
++	 * @brief 室内奔跑噪声：房间传播优先（当前房警戒至少提升到 RoomRunAlertLevel、相邻房 +1，多房间候选目标值取最大、一次应用）；无房间时回退 1200 半径听觉事件；始终广播 OnNoiseEmitted 供表现钩子，绝不调用 ReportNoiseEvent（防双计）。
++	 * @param location 世界空间位置，Unreal 单位为厘米。
++	 */
++	void ApplyIndoorRunNoise(const FVector location);
++
+ 	/**
+ 	 * @brief 处理 Handle Interaction 事件，将引擎回调转换为对应领域状态更新。
+ 	 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
+@@ -83,4 +90,8 @@ private:
+ 	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
+ 	UPROPERTY(Transient)
+ 	TObjectPtr<ULRMovementTuning> Tuning;
++
++	/** Guard 调优缓存；室内奔跑房间警戒目标值来源。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TObjectPtr<ULRGuardTuning> GuardTuning;
+ };
+diff --git a/Source/LostRunic/Tests/LRFrameworkTests.cpp b/Source/LostRunic/Tests/LRFrameworkTests.cpp
+index a90318c..a747436 100644
+--- a/Source/LostRunic/Tests/LRFrameworkTests.cpp
++++ b/Source/LostRunic/Tests/LRFrameworkTests.cpp
+@@ -57,21 +57,21 @@ bool FLRMovementPaceInputTest::RunTest(const FString& parameters)
+ 	ULRLocomotionComponent* locomotion = NewObject<ULRLocomotionComponent>();
+ 	TestEqual(TEXT("Default pace is Walk"), locomotion->GetPace(), ELRMovementPace::Walk);
+ 
+-	locomotion->ToggleSneak();
++	locomotion->RequestToggleSneak();
+ 	TestEqual(TEXT("Toggle enters Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);
+-	locomotion->StartRun();
++	locomotion->RequestStartRun();
+ 	TestEqual(TEXT("Run press enters Run"), locomotion->GetPace(), ELRMovementPace::Run);
+-	locomotion->StopRun();
++	locomotion->RequestStopRun();
+ 	TestEqual(TEXT("Run release restores Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);
+ 
+-	locomotion->ToggleSneak();
+-	locomotion->StartRun();
+-	locomotion->StopRun();
++	locomotion->RequestToggleSneak();
++	locomotion->RequestStartRun();
++	locomotion->RequestStopRun();
+ 	TestEqual(TEXT("Run release restores Walk"), locomotion->GetPace(), ELRMovementPace::Walk);
+ 
+-	locomotion->StartRun();
+-	locomotion->ToggleSneak();
+-	locomotion->StopRun();
++	locomotion->RequestStartRun();
++	locomotion->RequestToggleSneak();
++	locomotion->RequestStopRun();
+ 	TestEqual(TEXT("Sneak toggle during Run changes restored pace"), locomotion->GetPace(), ELRMovementPace::Sneak);
+ 	return true;
+ }
+diff --git a/Source/LostRunic/Tests/LRGuardTests.cpp b/Source/LostRunic/Tests/LRGuardTests.cpp
+index 631367f..a880074 100644
+--- a/Source/LostRunic/Tests/LRGuardTests.cpp
++++ b/Source/LostRunic/Tests/LRGuardTests.cpp
+@@ -30,11 +30,127 @@ bool FLRAlertRulesTest::RunTest(const FString& parameters)
+ 	TestEqual(TEXT("Zero alert patrols"), LRAlertRules::ResolveState(0, false, false), ELRGuardBehaviorState::IdlePatrol);
+ 	TestEqual(TEXT("Low alert is suspicious"), LRAlertRules::ResolveState(5, false, false), ELRGuardBehaviorState::Suspicious);
+ 	TestEqual(TEXT("Mid alert investigates"), LRAlertRules::ResolveState(6, false, false), ELRGuardBehaviorState::Investigate);
+-	TestEqual(TEXT("Max alert searches after sight is lost"), LRAlertRules::ResolveState(11, false, false), ELRGuardBehaviorState::Search);
++	TestEqual(TEXT("Max alert without sight searches"), LRAlertRules::ResolveState(11, false, false), ELRGuardBehaviorState::Search);
+ 	TestEqual(TEXT("Confirmed max alert chases"), LRAlertRules::ResolveState(11, true, false), ELRGuardBehaviorState::Chase);
+-	TestFalse(TEXT("Sight suppresses decay"), LRAlertRules::ShouldDecay(30.0f, 3.0f, true));
+-	TestFalse(TEXT("Observation delay has not elapsed"), LRAlertRules::ShouldDecay(2.99f, 3.0f, false));
+-	TestTrue(TEXT("Observation delay boundary decays"), LRAlertRules::ShouldDecay(3.0f, 3.0f, false));
++	TestEqual(TEXT("Searching in red band searches"), LRAlertRules::ResolveState(6, false, true), ELRGuardBehaviorState::Search);
++	TestEqual(TEXT("Searching below red band is suspicious"), LRAlertRules::ResolveState(5, false, true), ELRGuardBehaviorState::Suspicious);
++	TestFalse(TEXT("Observation suppresses decay"), LRAlertRules::ShouldDecay(true, false, ELRGuardBehaviorState::Suspicious));
++	TestFalse(TEXT("Sight suppresses decay"), LRAlertRules::ShouldDecay(false, true, ELRGuardBehaviorState::Search));
++	TestFalse(TEXT("Investigate holds alert while traveling"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Investigate));
++	TestFalse(TEXT("Chase holds alert"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Chase));
++	TestTrue(TEXT("Suspicious decays after observation"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Suspicious));
++	TestTrue(TEXT("Search decays after observation"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Search));
++	return true;
++}
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseAlertDeltaTest, "LostRunic.AI.NoiseAlertDelta",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRNoiseAlertDeltaTest::RunTest(const FString& parameters)
++{
++	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
++	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
++	{
++		return false;
++	}
++
++	// 室内奔跑：Set 语义，警戒至少提升到 RoomRunAlertLevel，不走吸引 CD。
++	FLRNoiseResponse indoorRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
++		LRGameplayTags::NoiseFootstepRunIndoor, 3, *tuning);
++	TestTrue(TEXT("Indoor run responds"), indoorRun.bRespond);
++	TestEqual(TEXT("Indoor run raises to floor"), indoorRun.Delta, 2);
++	TestFalse(TEXT("Indoor run is not attract"), indoorRun.bIsAttract);
++	indoorRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(LRGameplayTags::NoiseFootstepRunIndoor, 6, *tuning);
++	TestEqual(TEXT("Indoor run above floor is ignored"), indoorRun.Delta, 0);
++
++	// Faint：仅警戒 >=6 的守卫响应，且为吸引语义。
++	FLRNoiseResponse faintLow = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
++		LRGameplayTags::NoiseFootstepWalkFaint, 5, *tuning);
++	TestFalse(TEXT("Faint ignored below six"), faintLow.bRespond);
++	TestTrue(TEXT("Faint is attract"), faintLow.bIsAttract);
++	FLRNoiseResponse faintHigh = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
++		LRGameplayTags::NoiseFootstepWalkFaint, 6, *tuning);
++	TestTrue(TEXT("Faint responds at six"), faintHigh.bRespond);
++	TestEqual(TEXT("Faint attracts one"), faintHigh.Delta, 1);
++
++	// 普通噪声：一律吸引 +1。
++	const FGameplayTag plainReasons[] = {
++		LRGameplayTags::NoiseFootstepWalk.GetTag(),
++		LRGameplayTags::NoiseFootstepRun.GetTag(),
++		LRGameplayTags::NoiseInteraction.GetTag()
++	};
++	for (const FGameplayTag reason : plainReasons)
++	{
++		const FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(reason, 4, *tuning);
++		TestTrue(TEXT("Plain noise responds"), response.bRespond);
++		TestEqual(TEXT("Plain noise attracts one"), response.Delta, tuning->AttractAlertAmount);
++		TestTrue(TEXT("Plain noise is attract"), response.bIsAttract);
++	}
++	return true;
++}
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertIncreaseCooldownTest, "LostRunic.AI.AlertIncreaseCooldown",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRAlertIncreaseCooldownTest::RunTest(const FString& parameters)
++{
++	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
++	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
++	{
++		return false;
++	}
++
++	// 1-5 档与首次进入 6-10 档使用 0.5s，6-10 档后续使用 0.2s。
++	TestEqual(TEXT("Low band uses long cooldown"),
++		LRAlertRules::ResolveAttractIncreaseCooldown(3, false, *tuning), tuning->AlertIncreaseCooldownSeconds);
++	TestEqual(TEXT("First increase in red band uses long cooldown"),
++		LRAlertRules::ResolveAttractIncreaseCooldown(6, true, *tuning), tuning->AlertIncreaseCooldownSeconds);
++	TestEqual(TEXT("Later increases in red band use short cooldown"),
++		LRAlertRules::ResolveAttractIncreaseCooldown(6, false, *tuning), tuning->InvestigateIncreaseCooldownSeconds);
++
++	// 冷却边界：等于冷却时长时允许；冷却被拒绝的刺激完全忽略。
++	TestTrue(TEXT("Cooldown elapsed allows increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.5, 0.5f));
++	TestFalse(TEXT("Cooldown active rejects increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.6, 0.5f));
++	TestTrue(TEXT("Zero cooldown always allows"), LRAlertRules::IsIncreaseAllowed(10.0, 0.0, 0.0f));
++	return true;
++}
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRResolveTargetBehaviorTest, "LostRunic.AI.ResolveTargetBehavior",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRResolveTargetBehaviorTest::RunTest(const FString& parameters)
++{
++	// 眩晕覆盖一切：感知与警戒继续运行，但行为被钉在 Stunned。
++	TestEqual(TEXT("Stun overrides chase"), LRAlertRules::ResolveTargetBehavior(true, 11, true, false),
++		ELRGuardBehaviorState::Stunned);
++	TestEqual(TEXT("Stun overrides idle"), LRAlertRules::ResolveTargetBehavior(true, 0, false, false),
++		ELRGuardBehaviorState::Stunned);
++	// 未眩晕时按警戒推导。
++	TestEqual(TEXT("Resolved idle"), LRAlertRules::ResolveTargetBehavior(false, 0, false, false),
++		ELRGuardBehaviorState::IdlePatrol);
++	TestEqual(TEXT("Resolved suspicious"), LRAlertRules::ResolveTargetBehavior(false, 5, false, false),
++		ELRGuardBehaviorState::Suspicious);
++	TestEqual(TEXT("Resolved investigate"), LRAlertRules::ResolveTargetBehavior(false, 6, false, false),
++		ELRGuardBehaviorState::Investigate);
++	TestEqual(TEXT("Resolved chase"), LRAlertRules::ResolveTargetBehavior(false, 11, true, false),
++		ELRGuardBehaviorState::Chase);
++	// 眩晕结束后按当前警戒与视线恢复。
++	TestEqual(TEXT("Stun recovery resumes chase"), LRAlertRules::ResolveTargetBehavior(false, 11, true, false),
++		ELRGuardBehaviorState::Chase);
++	return true;
++}
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertTierTest, "LostRunic.AI.AlertTierMapping",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRAlertTierTest::RunTest(const FString& parameters)
++{
++	TestEqual(TEXT("Zero alert is hidden"), LRAlertRules::ResolveAlertTier(0), ELRGuardAlertTier::Hidden);
++	TestEqual(TEXT("Low alert is white"), LRAlertRules::ResolveAlertTier(1), ELRGuardAlertTier::White);
++	TestEqual(TEXT("Five is white boundary"), LRAlertRules::ResolveAlertTier(5), ELRGuardAlertTier::White);
++	TestEqual(TEXT("Six is red"), LRAlertRules::ResolveAlertTier(6), ELRGuardAlertTier::Red);
++	TestEqual(TEXT("Ten is red boundary"), LRAlertRules::ResolveAlertTier(10), ELRGuardAlertTier::Red);
++	TestEqual(TEXT("Eleven is full"), LRAlertRules::ResolveAlertTier(11), ELRGuardAlertTier::Full);
+ 	return true;
+ }
+ 
+diff --git a/Source/LostRunic/Tests/LRMovementTests.cpp b/Source/LostRunic/Tests/LRMovementTests.cpp
+new file mode 100644
+index 0000000..e8d197b
+--- /dev/null
++++ b/Source/LostRunic/Tests/LRMovementTests.cpp
+@@ -0,0 +1,139 @@
++/**
++ * @file LRMovementTests.cpp
++ * @brief 提供移动纯规则自动化测试：状态×步态矩阵、默认步态、步态×环境脚步噪声、噪声环境优先级与室内奔跑房间警戒目标值。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
++ *
++ * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#if WITH_DEV_AUTOMATION_TESTS
++
++#include "Misc/AutomationTest.h"
++
++#include "Core/LRGameplayTags.h"
++#include "Core/LRTypes.h"
++#include "Data/LRGuardTuning.h"
++#include "Data/LRMovementTuning.h"
++#include "Gameplay/LRMovementRules.h"
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPaceRulesTest, "LostRunic.Movement.PaceRules",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRPaceRulesTest::RunTest(const FString& parameters)
++{
++	// Normal：全部步态。
++	TestTrue(TEXT("Normal allows sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Sneak));
++	TestTrue(TEXT("Normal allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Walk));
++	TestTrue(TEXT("Normal allows run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Run));
++	// Perception：仅潜行。
++	TestTrue(TEXT("Perception allows sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Sneak));
++	TestFalse(TEXT("Perception forbids walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Walk));
++	TestFalse(TEXT("Perception forbids run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Run));
++	// Courage：走路+奔跑。
++	TestFalse(TEXT("Courage forbids sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Sneak));
++	TestTrue(TEXT("Courage allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Walk));
++	TestTrue(TEXT("Courage allows run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Run));
++	// Memory：仅走路。
++	TestFalse(TEXT("Memory forbids sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Sneak));
++	TestTrue(TEXT("Memory allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Walk));
++	TestFalse(TEXT("Memory forbids run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Run));
++
++	TestEqual(TEXT("Normal defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Normal), ELRMovementPace::Walk);
++	TestEqual(TEXT("Perception defaults to sneak"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Perception), ELRMovementPace::Sneak);
++	TestEqual(TEXT("Courage defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Courage), ELRMovementPace::Walk);
++	TestEqual(TEXT("Memory defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Memory), ELRMovementPace::Walk);
++	return true;
++}
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseResolverTest, "LostRunic.Movement.NoiseResolver",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRNoiseResolverTest::RunTest(const FString& parameters)
++{
++	ULRMovementTuning* tuning = NewObject<ULRMovementTuning>(GetTransientPackage());
++	if (!TestNotNull(TEXT("Movement tuning created"), tuning))
++	{
++		return false;
++	}
++
++	// 潜行：任何环境都无声（半径 0 + Sneak 标签，仅供动画/表现钩子）。
++	for (const ELRNoiseEnvironment environment : { ELRNoiseEnvironment::Indoor, ELRNoiseEnvironment::Outdoor,
++		ELRNoiseEnvironment::OutdoorStealth })
++	{
++		const FLRNoiseResolution sneak = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Sneak, environment, *tuning);
++		TestEqual(TEXT("Sneak radius is zero"), sneak.Radius, 0.0f);
++		TestTrue(TEXT("Sneak uses sneak tag"), sneak.Tag == LRGameplayTags::NoiseFootstepSneak);
++	}
++
++	// 走路：室内 400 / 室外潜行 250 / 室外非潜行 250 + Faint。
++	const FLRNoiseResolution walkIndoor = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::Indoor, *tuning);
++	TestEqual(TEXT("Walk indoor radius"), walkIndoor.Radius, tuning->IndoorWalkNoiseRadius);
++	TestTrue(TEXT("Walk indoor tag"), walkIndoor.Tag == LRGameplayTags::NoiseFootstepWalk);
++	const FLRNoiseResolution walkStealth = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::OutdoorStealth, *tuning);
++	TestEqual(TEXT("Walk outdoor stealth radius"), walkStealth.Radius, tuning->OutdoorNoiseRadius);
++	TestTrue(TEXT("Walk outdoor stealth tag"), walkStealth.Tag == LRGameplayTags::NoiseFootstepWalk);
++	const FLRNoiseResolution walkOpen = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::Outdoor, *tuning);
++	TestEqual(TEXT("Walk outdoor open radius"), walkOpen.Radius, tuning->OutdoorNoiseRadius);
++	TestTrue(TEXT("Walk outdoor open uses faint tag"), walkOpen.Tag == LRGameplayTags::NoiseFootstepWalkFaint);
++
++	// 奔跑：室内 1200 + Run.Indoor / 室外潜行 600 / 室外非潜行 250。
++	const FLRNoiseResolution runIndoor = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::Indoor, *tuning);
++	TestEqual(TEXT("Run indoor radius"), runIndoor.Radius, tuning->IndoorRunNoiseRadius);
++	TestTrue(TEXT("Run indoor tag"), runIndoor.Tag == LRGameplayTags::NoiseFootstepRunIndoor);
++	const FLRNoiseResolution runStealth = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::OutdoorStealth, *tuning);
++	TestEqual(TEXT("Run outdoor stealth radius"), runStealth.Radius, tuning->OutdoorStealthRunNoiseRadius);
++	TestTrue(TEXT("Run outdoor stealth tag"), runStealth.Tag == LRGameplayTags::NoiseFootstepRun);
++	const FLRNoiseResolution runOpen = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::Outdoor, *tuning);
++	TestEqual(TEXT("Run outdoor open radius"), runOpen.Radius, tuning->OutdoorNoiseRadius);
++	TestTrue(TEXT("Run outdoor open tag"), runOpen.Tag == LRGameplayTags::NoiseFootstepRun);
++	return true;
++}
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseEnvironmentPriorityTest, "LostRunic.Movement.NoiseEnvironmentPriority",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRNoiseEnvironmentPriorityTest::RunTest(const FString& parameters)
++{
++	const TArray<ELRNoiseEnvironment> empty;
++	TestEqual(TEXT("No area defaults to outdoor"), LRMovementRules::ResolveEnvironmentFromSet(empty),
++		ELRNoiseEnvironment::Outdoor);
++	TestEqual(TEXT("Single indoor wins"), LRMovementRules::ResolveEnvironmentFromSet(
++		{ ELRNoiseEnvironment::Indoor }), ELRNoiseEnvironment::Indoor);
++	TestEqual(TEXT("Indoor beats outdoor stealth"),
++		LRMovementRules::ResolveEnvironmentFromSet(
++			{ ELRNoiseEnvironment::OutdoorStealth, ELRNoiseEnvironment::Indoor }),
++		ELRNoiseEnvironment::Indoor);
++	TestEqual(TEXT("Indoor beats outdoor"),
++		LRMovementRules::ResolveEnvironmentFromSet(
++			{ ELRNoiseEnvironment::Outdoor, ELRNoiseEnvironment::Indoor }),
++		ELRNoiseEnvironment::Indoor);
++	TestEqual(TEXT("Outdoor stealth beats outdoor"),
++		LRMovementRules::ResolveEnvironmentFromSet(
++			{ ELRNoiseEnvironment::Outdoor, ELRNoiseEnvironment::OutdoorStealth }),
++		ELRNoiseEnvironment::OutdoorStealth);
++	return true;
++}
++
++IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRRoomRunAlertTargetTest, "LostRunic.Movement.RoomAlertTargets",
++	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
++
++bool FLRRoomRunAlertTargetTest::RunTest(const FString& parameters)
++{
++	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
++	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
++	{
++		return false;
++	}
++
++	// 当前房间：至少提升到 RoomRunAlertLevel(5)。
++	TestEqual(TEXT("Current room raises to floor"), LRMovementRules::ResolveRoomRunTargetLevel(true, 3, *tuning), 5);
++	TestEqual(TEXT("Current room at floor stays"), LRMovementRules::ResolveRoomRunTargetLevel(true, 5, *tuning), 5);
++	TestEqual(TEXT("Current room above floor keeps level"), LRMovementRules::ResolveRoomRunTargetLevel(true, 7, *tuning), 7);
++	// 相邻房间：max(当前, 当前+1)。
++	TestEqual(TEXT("Adjacent room raises by amount"), LRMovementRules::ResolveRoomRunTargetLevel(false, 0, *tuning), 1);
++	TestEqual(TEXT("Adjacent room at eight becomes nine"), LRMovementRules::ResolveRoomRunTargetLevel(false, 8, *tuning), 9);
++	TestEqual(TEXT("Adjacent room at ten caps at eleven"), LRMovementRules::ResolveRoomRunTargetLevel(false, 10, *tuning), 11);
++	return true;
++}
++
++#endif
+diff --git a/Source/LostRunic/Tests/LRNarrativeTests.cpp b/Source/LostRunic/Tests/LRNarrativeTests.cpp
+index e3e7944..064be1f 100644
+--- a/Source/LostRunic/Tests/LRNarrativeTests.cpp
++++ b/Source/LostRunic/Tests/LRNarrativeTests.cpp
+@@ -38,6 +38,11 @@ namespace
+ 		contentSet->DialogueTable->RowStruct = FLRDialogueRow::StaticStruct();
+ 		contentSet->ReadingTable = NewObject<UDataTable>(contentSet);
+ 		contentSet->ReadingTable->RowStruct = FLRReadingRow::StaticStruct();
++		// 地图注册校验：NewGameMapId 必须解析到已注册地图（内容集校验的一部分）。
++		FLRMapRegistration mapRegistration;
++		mapRegistration.MapId = TEXT("Map.Test");
++		contentSet->Maps.Add(mapRegistration);
++		contentSet->NewGameMapId = mapRegistration.MapId;
+ 		return contentSet;
+ 	}
+ 
+diff --git a/Source/LostRunic/Tests/LRTuningTests.cpp b/Source/LostRunic/Tests/LRTuningTests.cpp
+index 684c81e..4e52d5d 100644
+--- a/Source/LostRunic/Tests/LRTuningTests.cpp
++++ b/Source/LostRunic/Tests/LRTuningTests.cpp
+@@ -13,6 +13,7 @@
+ #include "Data/LRGuardTuning.h"
+ #include "Data/LRInteractionTuning.h"
+ #include "Data/LRMovementTuning.h"
++#include "Data/LRNPCTuning.h"
+ #include "Data/LRPresentationTuning.h"
+ #include "Data/LRSaveTuning.h"
+ #include "Data/LRStateTuning.h"
+@@ -31,6 +32,7 @@ bool FLRTuningDefaultsTest::RunTest(const FString& parameters)
+ 	TestTrue(TEXT("Save defaults"), NewObject<ULRSaveTuning>()->Validate(error));
+ 	TestTrue(TEXT("UI defaults"), NewObject<ULRUITuning>()->Validate(error));
+ 	TestTrue(TEXT("Presentation defaults"), NewObject<ULRPresentationTuning>()->Validate(error));
++	TestTrue(TEXT("NPC defaults"), NewObject<ULRNPCTuning>()->Validate(error));
+ 	return true;
+ }
+ 
+diff --git a/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.cpp b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.cpp
+new file mode 100644
+index 0000000..f951b33
+--- /dev/null
++++ b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.cpp
+@@ -0,0 +1,58 @@
++/**
++ * @file LRWorldAlertBarWidgetBase.cpp
++ * @brief 世界空间警戒条 Widget 基类实现：守卫初始化时绑定警戒快照，Widget 销毁时解绑；初始快照在绑定后立即推送。
++ *
++ * 关联文件：LRWorldAlertBarWidgetBase.h；所属领域：UI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#include "UI/LRWorldAlertBarWidgetBase.h"
++
++#include "AI/LRAlertComponent.h"
++#include "AI/LRGuardCharacter.h"
++
++/**
++ * @brief 绑定指定守卫的警戒快照并立即推送当前值，避免首帧不同步；重复调用会先解绑旧守卫。
++ * @param guard 本次查询、交互或事件涉及的 Actor。
++ */
++void ULRWorldAlertBarWidgetBase::InitializeForGuard(ALRGuardCharacter* guard)
++{
++	Shutdown();
++	Alert = guard ? guard->GetAlertComponent() : nullptr;
++	if (Alert.IsValid())
++	{
++		Alert->OnAlertSnapshotChanged.AddDynamic(this, &ULRWorldAlertBarWidgetBase::HandleSnapshotChanged);
++		HandleSnapshotChanged(Alert->GetAlertSnapshot());
++	}
++}
++
++/**
++ * @brief 解绑当前守卫的警戒快照；Widget 销毁时自动调用。
++ */
++void ULRWorldAlertBarWidgetBase::Shutdown()
++{
++	if (Alert.IsValid())
++	{
++		Alert->OnAlertSnapshotChanged.RemoveDynamic(this, &ULRWorldAlertBarWidgetBase::HandleSnapshotChanged);
++	}
++	Alert.Reset();
++}
++
++/**
++ * @brief Widget 销毁时解绑警戒快照，避免悬挂委托。
++ */
++void ULRWorldAlertBarWidgetBase::NativeDestruct()
++{
++	Shutdown();
++	Super::NativeDestruct();
++}
++
++/**
++ * @brief 处理 Handle Snapshot Changed 事件，将引擎回调转换为对应领域状态更新。
++ * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
++ */
++void ULRWorldAlertBarWidgetBase::HandleSnapshotChanged(const FLRAlertSnapshot& snapshot)
++{
++	CurrentSnapshot = snapshot;
++	HandleAlertSnapshotChanged(snapshot);
++}
+diff --git a/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.h b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.h
+new file mode 100644
+index 0000000..9830f89
+--- /dev/null
++++ b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.h
+@@ -0,0 +1,73 @@
++/**
++ * @file LRWorldAlertBarWidgetBase.h
++ * @brief 世界空间警戒条 Widget 基类：由守卫初始化并绑定/解绑 ULRAlertComponent 的只读警戒快照，绑定后立即推送一次快照；蓝图只负责表现（0 隐藏 / 1-5 白 / 6-10 红 / 11 满值特效）。
++ *
++ * 关联文件：LRWorldAlertBarWidgetBase.cpp；所属领域：UI。
++ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
++ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
++ */
++#pragma once
++
++#include "AI/LRGuardTypes.h"
++#include "Blueprint/UserWidget.h"
++
++#include "LRWorldAlertBarWidgetBase.generated.h"
++
++class ALRGuardCharacter;
++class ULRAlertComponent;
++
++/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
++UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Lost Runic World Alert Bar Base"))
++class LOSTRUNIC_API ULRWorldAlertBarWidgetBase : public UUserWidget
++{
++	GENERATED_BODY()
++
++public:
++	/**
++	 * @brief 绑定指定守卫的警戒快照并立即推送当前值，避免首帧不同步；重复调用会先解绑旧守卫。
++	 * @param guard 本次查询、交互或事件涉及的 Actor。
++	 */
++	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
++	void InitializeForGuard(ALRGuardCharacter* guard);
++
++	/**
++	 * @brief 解绑当前守卫的警戒快照；Widget 销毁时自动调用。
++	 */
++	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
++	void Shutdown();
++
++	/**
++	 * @brief 查询 Current Snapshot；不修改领域状态。
++	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
++	 */
++	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI|Alert")
++	const FLRAlertSnapshot& GetCurrentSnapshot() const { return CurrentSnapshot; }
++
++	/**
++	 * @brief 警戒快照变化时调用；蓝图覆盖此事件只做表现（进度条、颜色、隐藏与满值特效）。
++	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
++	 */
++	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI|Alert")
++	void HandleAlertSnapshotChanged(const FLRAlertSnapshot& snapshot);
++
++protected:
++	/**
++	 * @brief Widget 销毁时解绑警戒快照，避免悬挂委托。
++	 */
++	virtual void NativeDestruct() override;
++
++private:
++	/**
++	 * @brief 处理 Handle Snapshot Changed 事件，将引擎回调转换为对应领域状态更新。
++	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
++	 */
++	UFUNCTION()
++	void HandleSnapshotChanged(const FLRAlertSnapshot& snapshot);
++
++	/** Alert 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
++	UPROPERTY(Transient)
++	TWeakObjectPtr<ULRAlertComponent> Alert;
++
++	/** Current Snapshot 的运行时状态；由所属类型维护，不在蓝图中配置。 */
++	FLRAlertSnapshot CurrentSnapshot;
++};
+```
diff --git a/Scripts/BuildLostRunicEditor.bat b/Scripts/BuildLostRunicEditor.bat
new file mode 100644
index 0000000..8316883
--- /dev/null
+++ b/Scripts/BuildLostRunicEditor.bat
@@ -0,0 +1,3 @@
+@echo off
+rem Build wrapper: avoids Git Bash / cmd quoting issues with paths containing spaces.
+call "D:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" LostRunicEditor Win64 Development -Project="D:\25DGame\LostRunic\LostRunic.uproject" %*
diff --git a/Scripts/RunLostRunicTests.bat b/Scripts/RunLostRunicTests.bat
new file mode 100644
index 0000000..da414a0
--- /dev/null
+++ b/Scripts/RunLostRunicTests.bat
@@ -0,0 +1,3 @@
+@echo off
+rem Test runner: unattended automation run for all LostRunic tests, exits when done.
+"D:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "D:\25DGame\LostRunic\LostRunic.uproject" -unattended -nopause -ExecCmds="Automation RunTests LostRunic; Quit"
diff --git a/Source/LostRunic/AI/LRAlertComponent.cpp b/Source/LostRunic/AI/LRAlertComponent.cpp
index 26279f4..c623f0f 100644
--- a/Source/LostRunic/AI/LRAlertComponent.cpp
+++ b/Source/LostRunic/AI/LRAlertComponent.cpp
@@ -53,12 +53,13 @@ void ULRAlertComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
 	if (GetWorld())
 	{
 		GetWorld()->GetTimerManager().ClearTimer(DecayTimer);
+		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
 	}
 	Super::EndPlay(endPlayReason);
 }
 
 /**
- * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化。
+ * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化；警戒归零时清理目标与观察状态。
  * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
  * @param location 世界空间位置，Unreal 单位为厘米。
  * @param target 本次规则检查或操作的目标对象。
@@ -80,11 +81,59 @@ void ULRAlertComponent::ApplyAlertDelta(const int32 delta, const FVector locatio
 		bSearching = false;
 	}
 	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
+	if (AlertLevel <= 0)
+	{
+		ClearWhenAlertZero();
+	}
+	BroadcastChange(previousLevel, previousState, reason);
+}
+
+/**
+ * @brief 吸引注意语义入口：按档位冷却门控（CD 内刺激完全忽略，不改变观察状态），每次 +AttractAlertAmount，并重置 3s 观察窗口。
+ * @param location 世界空间位置，Unreal 单位为厘米。
+ * @param target 本次规则检查或操作的目标对象。
+ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+ */
+void ULRAlertComponent::ApplyAttract(const FVector location, AActor* target, const FGameplayTag reason)
+{
+	const ULRGuardTuning& tuning = GetEffectiveTuning();
+	const double now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
+	// 从 0 起的首次吸引立即生效（设计：0 -> 1）；之后的增加受档位冷却门控，CD 内完全忽略。
+	if (AlertLevel > 0
+		&& !LRAlertRules::IsIncreaseAllowed(now, LastIncreaseTimeSeconds,
+			LRAlertRules::ResolveAttractIncreaseCooldown(AlertLevel, bFirstIncreaseInBand, tuning)))
+	{
+		UE_LOG(LogLostRunicAI, Verbose, TEXT("Guard=%s attract ignored (cooldown) reason=%s"),
+			*GetNameSafe(GetOwner()), *reason.GetTagName().ToString());
+		return;
+	}
+
+	const int32 previousLevel = AlertLevel;
+	const ELRGuardBehaviorState previousState = GetBehaviorState();
+	const bool bCrossingIntoBand = AlertLevel < tuning.SightInvestigateLevel;
+	AlertLevel = LRAlertRules::ApplyDelta(AlertLevel, tuning.AttractAlertAmount);
+	LastDisturbanceLocation = location;
+	if (target)
+	{
+		TargetActor = target;
+	}
+	bSearching = false;
+	LastIncreaseTimeSeconds = now;
+	LastStimulusTimeSeconds = now;
+	if (bFirstIncreaseInBand)
+	{
+		bFirstIncreaseInBand = false;
+	}
+	if (bCrossingIntoBand && AlertLevel >= tuning.SightInvestigateLevel)
+	{
+		bFirstIncreaseInBand = true;
+	}
+	StartObservation();
 	BroadcastChange(previousLevel, previousState, reason);
 }
 
 /**
- * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件。
+ * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件；按 4.2.1 分档：<6 看见升到 6，6-10 看见升到 11，11 丢失降回 10。
  * @param target 本次规则检查或操作的目标对象。
  * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
  * @param lastKnownLocation 空间值 `lastKnownLocation`；距离和位置使用 Unreal 厘米单位。
@@ -96,10 +145,23 @@ void ULRAlertComponent::SetSightTarget(AActor* target, const bool bVisible, cons
 	LastDisturbanceLocation = lastKnownLocation;
 	TargetActor = target;
 	bHasConfirmedSight = bVisible;
-	bSearching = !bVisible && AlertLevel > 0;
 	if (bVisible)
 	{
-		AlertLevel = FMath::Max(AlertLevel, GetEffectiveTuning().SightAlertLevel);
+		const ULRGuardTuning& tuning = GetEffectiveTuning();
+		if (AlertLevel < tuning.SightInvestigateLevel)
+		{
+			AlertLevel = tuning.SightInvestigateLevel;
+			bFirstIncreaseInBand = true;
+		}
+		else if (AlertLevel < tuning.SightChaseLevel)
+		{
+			AlertLevel = tuning.SightChaseLevel;
+		}
+	}
+	else if (AlertLevel >= GetEffectiveTuning().SightChaseLevel)
+	{
+		// 11 丢失视线 -> 10，前往最后看见位置；不置 bSearching，使行为解析自然落到 Investigate。
+		AlertLevel = 10;
 	}
 	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
 	BroadcastChange(previousLevel, previousState,
@@ -107,12 +169,13 @@ void ULRAlertComponent::SetSightTarget(AActor* target, const bool bVisible, cons
 }
 
 /**
- * @brief 标记守卫已到达最后异常位置，使 StateTree 从 Investigate 转入 Search。
+ * @brief 标记守卫已到达最后异常位置，使 StateTree 从 Investigate 转入 Search，并开始 3s 抵达观察。
  */
 void ULRAlertComponent::MarkInvestigationReached()
 {
 	const ELRGuardBehaviorState previousState = GetBehaviorState();
 	bSearching = AlertLevel > 0;
+	StartObservation();
 	BroadcastChange(AlertLevel, previousState, LRGameplayTags::SearchReached);
 }
 
@@ -126,7 +189,12 @@ void ULRAlertComponent::ResetAfterSearch()
 	AlertLevel = 0;
 	bSearching = false;
 	bHasConfirmedSight = false;
+	bObserving = false;
 	TargetActor.Reset();
+	if (GetWorld())
+	{
+		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
+	}
 	BroadcastChange(previousLevel, previousState, LRGameplayTags::SearchTimeout);
 }
 
@@ -148,14 +216,66 @@ void ULRAlertComponent::HandleDecayTimer()
 	{
 		return;
 	}
-	const float elapsed = GetWorld()->GetTimeSeconds() - LastStimulusTimeSeconds;
-	if (LRAlertRules::ShouldDecay(elapsed, GetEffectiveTuning().InitialObserveSeconds, bHasConfirmedSight))
+	if (LRAlertRules::ShouldDecay(bObserving, bHasConfirmedSight, GetBehaviorState()))
 	{
 		ApplyAlertDelta(-GetEffectiveTuning().AlertDecayAmount, LastDisturbanceLocation,
 			TargetActor.Get(), LRGameplayTags::SearchAlertDecay);
 	}
 }
 
+/**
+ * @brief 处理 Handle Observation End 事件，将引擎回调转换为对应领域状态更新；观察结束前警戒维持不动。
+ */
+void ULRAlertComponent::HandleObservationEnd()
+{
+	bObserving = false;
+}
+
+/**
+ * @brief 开始 3 秒观察窗口；观察期间衰减被门控。
+ */
+void ULRAlertComponent::StartObservation()
+{
+	if (AlertLevel <= 0 || !GetWorld())
+	{
+		return;
+	}
+	bObserving = true;
+	GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
+	GetWorld()->GetTimerManager().SetTimer(ObservationTimer, this, &ULRAlertComponent::HandleObservationEnd,
+		GetEffectiveTuning().InitialObserveSeconds, false);
+}
+
+/**
+ * @brief 警戒归零时清理目标、搜索与观察状态；不额外广播（归零变化本身已广播）。
+ */
+void ULRAlertComponent::ClearWhenAlertZero()
+{
+	bSearching = false;
+	bHasConfirmedSight = false;
+	bObserving = false;
+	TargetActor.Reset();
+	if (GetWorld())
+	{
+		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
+	}
+}
+
+/**
+ * @brief 查询当前只读警戒快照（等级、归一化进度、显示档位、行为与满值标志）；UI 绑定 OnAlertSnapshotChanged 后应立即读取本值，避免首帧不同步。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+FLRAlertSnapshot ULRAlertComponent::GetAlertSnapshot() const
+{
+	FLRAlertSnapshot snapshot;
+	snapshot.Level = AlertLevel;
+	snapshot.Fraction = AlertLevel / static_cast<float>(LRAlertRules::MaxAlertLevel);
+	snapshot.Tier = LRAlertRules::ResolveAlertTier(AlertLevel);
+	snapshot.Behavior = GetBehaviorState();
+	snapshot.bFullAlert = AlertLevel >= LRAlertRules::MaxAlertLevel;
+	return snapshot;
+}
+
 /**
  * @brief 广播警戒旧值、新值和原因标签，供 StateTree、UI、日志与测试订阅。
  * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
@@ -169,8 +289,9 @@ void ULRAlertComponent::BroadcastChange(const int32 previousLevel, const ELRGuar
 	const ELRGuardBehaviorState currentState = GetBehaviorState();
 	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s alert %d -> %d state %d -> %d reason=%s location=%s"),
 		*GetNameSafe(GetOwner()), previousLevel, AlertLevel, static_cast<int32>(previousState),
-		static_cast<int32>(currentState), *reason.ToString(), *LastDisturbanceLocation.ToCompactString());
+		static_cast<int32>(currentState), *reason.GetTagName().ToString(), *LastDisturbanceLocation.ToCompactString());
 	OnAlertChanged.Broadcast(previousLevel, AlertLevel, currentState, reason, LastDisturbanceLocation);
+	OnAlertSnapshotChanged.Broadcast(GetAlertSnapshot());
 }
 
 /**
diff --git a/Source/LostRunic/AI/LRAlertComponent.h b/Source/LostRunic/AI/LRAlertComponent.h
index 578d2a9..542f594 100644
--- a/Source/LostRunic/AI/LRAlertComponent.h
+++ b/Source/LostRunic/AI/LRAlertComponent.h
@@ -18,6 +18,7 @@ class ULRGuardTuning;
 
 DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLRAlertChanged, int32, previousLevel, int32, currentLevel,
 	ELRGuardBehaviorState, currentState, FGameplayTag, reason, FVector, disturbanceLocation);
+DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRAlertSnapshotChanged, const FLRAlertSnapshot&, snapshot);
 
 /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
 UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Alert"))
@@ -42,7 +43,7 @@ public:
 	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
 
 	/**
-	 * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化。
+	 * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化；警戒归零时清理目标与观察状态。
 	 * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
 	 * @param location 世界空间位置，Unreal 单位为厘米。
 	 * @param target 本次规则检查或操作的目标对象。
@@ -51,6 +52,15 @@ public:
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
 	void ApplyAlertDelta(int32 delta, FVector location, AActor* target, FGameplayTag reason);
 
+	/**
+	 * @brief 吸引注意语义入口：按档位冷却门控（CD 内刺激完全忽略，不改变观察状态），每次 +AttractAlertAmount，并重置 3s 观察窗口。
+	 * @param location 世界空间位置，Unreal 单位为厘米。
+	 * @param target 本次规则检查或操作的目标对象。
+	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+	 */
+	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
+	void ApplyAttract(FVector location, AActor* target, FGameplayTag reason);
+
 	/**
 	 * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件。
 	 * @param target 本次规则检查或操作的目标对象。
@@ -114,15 +124,52 @@ public:
 	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
 	bool HasConfirmedSight() const { return bHasConfirmedSight; }
 
+	/**
+	 * @brief 判断 Is Searching 对应条件；不产生玩法副作用。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
+	bool IsSearching() const { return bSearching; }
+
+	/**
+	 * @brief 判断 Is Observing 对应条件；不产生玩法副作用。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
+	bool IsObserving() const { return bObserving; }
+
+	/**
+	 * @brief 查询当前只读警戒快照（等级、归一化进度、显示档位、行为与满值标志）；UI 绑定 OnAlertSnapshotChanged 后应立即读取本值，避免首帧不同步。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
+	FLRAlertSnapshot GetAlertSnapshot() const;
+
 	/** 当 Alert Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
 	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
 	FLRAlertChanged OnAlertChanged;
 
+	/** 当 Alert Snapshot Changed 发生时广播；世界警戒条 Widget 绑定该只读快照，蓝图只做表现。  */
+	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
+	FLRAlertSnapshotChanged OnAlertSnapshotChanged;
+
 private:
 	/**
 	 * @brief 处理 Handle Decay Timer 事件，将引擎回调转换为对应领域状态更新。
 	 */
 	void HandleDecayTimer();
+	/**
+	 * @brief 处理 Handle Observation End 事件，将引擎回调转换为对应领域状态更新；观察结束前警戒维持不动。
+	 */
+	void HandleObservationEnd();
+	/**
+	 * @brief 开始 3 秒观察窗口；观察期间衰减被门控。
+	 */
+	void StartObservation();
+	/**
+	 * @brief 警戒归零时清理目标、搜索与观察状态；不额外广播（归零变化本身已广播）。
+	 */
+	void ClearWhenAlertZero();
 	/**
 	 * @brief 广播警戒旧值、新值和原因标签，供 StateTree、UI、日志与测试订阅。
 	 * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
@@ -156,10 +203,18 @@ private:
 	FGameplayTag LastReason;
 	/** Last Stimulus Time Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
 	double LastStimulusTimeSeconds = 0.0;
+	/** Last Increase Time Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	double LastIncreaseTimeSeconds = 0.0;
 	/** Has Confirmed Sight 的运行时状态；由所属类型维护，不在蓝图中配置。 */
 	bool bHasConfirmedSight = false;
 	/** Searching 的运行时状态；由所属类型维护，不在蓝图中配置。 */
 	bool bSearching = false;
+	/** Observing 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	bool bObserving = false;
+	/** First Increase In Band 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	bool bFirstIncreaseInBand = false;
 	/** Decay Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
 	FTimerHandle DecayTimer;
+	/** Observation Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
+	FTimerHandle ObservationTimer;
 };
diff --git a/Source/LostRunic/AI/LRAlertRules.cpp b/Source/LostRunic/AI/LRAlertRules.cpp
index 56f1685..be0dae4 100644
--- a/Source/LostRunic/AI/LRAlertRules.cpp
+++ b/Source/LostRunic/AI/LRAlertRules.cpp
@@ -1,6 +1,6 @@
 /**
  * @file LRAlertRules.cpp
- * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
+ * @brief 实现 0-11 警戒纯规则：边界钳制、行为档位解析（4.2.1）、衰减门控与吸引增加冷却。
  *
  * 关联文件：LRAlertRules.h；所属领域：AI。
  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
@@ -8,10 +8,10 @@
  */
 #include "AI/LRAlertRules.h"
 
+#include "Data/LRGuardTuning.h"
+
 namespace
 {
-	constexpr int32 MinAlertLevel = 0;
-	constexpr int32 MaxAlertLevel = 11;
 	constexpr int32 SuspiciousMaxLevel = 5;
 }
 
@@ -27,7 +27,7 @@ int32 LRAlertRules::ApplyDelta(const int32 currentLevel, const int32 delta)
 }
 
 /**
- * @brief 执行 Resolve State 的纯规则或事务判定，失败时提供结构化原因。
+ * @brief 按 4.2.1 档位解析行为：0 巡逻；11+视线 追逐；11 无视线 搜索兜底；搜索且 >=6 搜索；<=5 可疑；否则调查。
  * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
  * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
  * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
@@ -43,7 +43,11 @@ ELRGuardBehaviorState LRAlertRules::ResolveState(const int32 alertLevel, const b
 	{
 		return ELRGuardBehaviorState::Chase;
 	}
-	if (bSearching || alertLevel >= MaxAlertLevel)
+	if (alertLevel >= MaxAlertLevel)
+	{
+		return ELRGuardBehaviorState::Search;
+	}
+	if (bSearching && alertLevel >= SuspiciousMaxLevel + 1)
 	{
 		return ELRGuardBehaviorState::Search;
 	}
@@ -52,13 +56,79 @@ ELRGuardBehaviorState LRAlertRules::ResolveState(const int32 alertLevel, const b
 }
 
 /**
- * @brief 判断 Should Decay 对应条件；不产生玩法副作用。
- * @param secondsSinceStimulus 时间值 `secondsSinceStimulus`，单位为秒。
- * @param observeSeconds 时间值 `observeSeconds`，单位为秒。
+ * @brief 守卫行为唯一权威解析：眩晕覆盖优先返回 Stunned，否则按警戒推导；StateTree 只执行本结果，不自行重新定义警戒语义。
+ * @param bStunned 布尔开关 `bStunned`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
  * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRGuardBehaviorState LRAlertRules::ResolveTargetBehavior(const bool bStunned, const int32 alertLevel,
+	const bool bHasSight, const bool bSearching)
+{
+	return bStunned ? ELRGuardBehaviorState::Stunned : ResolveState(alertLevel, bHasSight, bSearching);
+}
+
+/**
+ * @brief 解析警戒显示档位：0 隐藏、1-5 白色、6-10 红色、11 满值。
+ * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRGuardAlertTier LRAlertRules::ResolveAlertTier(const int32 alertLevel)
+{
+	if (alertLevel <= MinAlertLevel)
+	{
+		return ELRGuardAlertTier::Hidden;
+	}
+	if (alertLevel >= MaxAlertLevel)
+	{
+		return ELRGuardAlertTier::Full;
+	}
+	return alertLevel <= SuspiciousMaxLevel ? ELRGuardAlertTier::White : ELRGuardAlertTier::Red;
+}
+
+/**
+ * @brief 判断 Should Decay 对应条件；观察中、追逐中或调查（前往）中不衰减，其余 0.5s/-1。
+ * @param bObserving 布尔开关 `bObserving`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @param bHasConfirmedSight 布尔开关 `bHasConfirmedSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+bool LRAlertRules::ShouldDecay(const bool bObserving, const bool bHasConfirmedSight,
+	const ELRGuardBehaviorState currentState)
+{
+	if (bObserving || bHasConfirmedSight)
+	{
+		return false;
+	}
+	return currentState != ELRGuardBehaviorState::Investigate && currentState != ELRGuardBehaviorState::Chase;
+}
+
+/**
+ * @brief 解析吸引增加的冷却时长：1-5 档与首次进入 6-10 档使用 AlertIncreaseCooldownSeconds，6-10 档后续使用 InvestigateIncreaseCooldownSeconds。
+ * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
+ * @param bFirstIncreaseInBand 布尔开关 `bFirstIncreaseInBand`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+float LRAlertRules::ResolveAttractIncreaseCooldown(const int32 currentAlert, const bool bFirstIncreaseInBand,
+	const ULRGuardTuning& tuning)
+{
+	if (currentAlert < tuning.SightInvestigateLevel || bFirstIncreaseInBand)
+	{
+		return tuning.AlertIncreaseCooldownSeconds;
+	}
+	return tuning.InvestigateIncreaseCooldownSeconds;
+}
+
+/**
+ * @brief 判断 Is Increase Allowed 对应条件；冷却拒绝的刺激被完全忽略，不改变观察状态。
+ * @param now 时间值 `now`，单位为秒。
+ * @param lastIncreaseTime 时间值 `lastIncreaseTime`，单位为秒。
+ * @param cooldownSeconds 时间值 `cooldownSeconds`，单位为秒。
  * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
  */
-bool LRAlertRules::ShouldDecay(const float secondsSinceStimulus, const float observeSeconds, const bool bHasSight)
+bool LRAlertRules::IsIncreaseAllowed(const double now, const double lastIncreaseTime, const float cooldownSeconds)
 {
-	return !bHasSight && secondsSinceStimulus >= observeSeconds;
+	return cooldownSeconds <= 0.0f || now - lastIncreaseTime >= cooldownSeconds;
 }
diff --git a/Source/LostRunic/AI/LRAlertRules.h b/Source/LostRunic/AI/LRAlertRules.h
index bf2206c..b668f09 100644
--- a/Source/LostRunic/AI/LRAlertRules.h
+++ b/Source/LostRunic/AI/LRAlertRules.h
@@ -1,6 +1,6 @@
 /**
  * @file LRAlertRules.h
- * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
+ * @brief 提供 0-11 警戒纯规则：边界钳制、行为档位解析（4.2.1 语义）、衰减门控与吸引增加冷却，供运行时组件与 LostRunic.AI 自动化测试共同调用。
  *
  * 关联文件：LRAlertRules.cpp；所属领域：AI。
  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
@@ -10,8 +10,15 @@
 
 #include "AI/LRGuardTypes.h"
 
+class ULRGuardTuning;
+
 namespace LRAlertRules
 {
+	/** 警戒上限；供 UI 快照与运行时组件共享。 */
+	inline constexpr int32 MaxAlertLevel = 11;
+	/** 警戒下限。 */
+	inline constexpr int32 MinAlertLevel = 0;
+
 	/**
 	 * @brief 按 0-11 边界应用警戒变化，并返回旧值、新值和原因。
 	 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
@@ -20,7 +27,7 @@ namespace LRAlertRules
 	 */
 	LOSTRUNIC_API int32 ApplyDelta(int32 currentLevel, int32 delta);
 	/**
-	 * @brief 执行 Resolve State 的纯规则或事务判定，失败时提供结构化原因。
+	 * @brief 按 4.2.1 档位解析行为：0 巡逻；11+视线 追逐；11 无视线 搜索兜底；搜索且 >=6 搜索；<=5 可疑；否则调查。
 	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
 	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 	 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
@@ -28,11 +35,44 @@ namespace LRAlertRules
 	 */
 	LOSTRUNIC_API ELRGuardBehaviorState ResolveState(int32 alertLevel, bool bHasSight, bool bSearching);
 	/**
-	 * @brief 判断 Should Decay 对应条件；不产生玩法副作用。
-	 * @param secondsSinceStimulus 时间值 `secondsSinceStimulus`，单位为秒。
-	 * @param observeSeconds 时间值 `observeSeconds`，单位为秒。
+	 * @brief 守卫行为唯一权威解析：眩晕覆盖优先返回 Stunned，否则按警戒推导；StateTree 只执行本结果，不自行重新定义警戒语义。
+	 * @param bStunned 布尔开关 `bStunned`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
 	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(bool bStunned, int32 alertLevel, bool bHasSight,
+		bool bSearching);
+	/**
+	 * @brief 解析警戒显示档位：0 隐藏、1-5 白色、6-10 红色、11 满值。
+	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API ELRGuardAlertTier ResolveAlertTier(int32 alertLevel);
+	/**
+	 * @brief 判断 Should Decay 对应条件；观察中、追逐中或调查（前往）中不衰减，其余 0.5s/-1。
+	 * @param bObserving 布尔开关 `bObserving`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @param bHasConfirmedSight 布尔开关 `bHasConfirmedSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API bool ShouldDecay(bool bObserving, bool bHasConfirmedSight, ELRGuardBehaviorState currentState);
+	/**
+	 * @brief 解析吸引增加的冷却时长：1-5 档与首次进入 6-10 档使用 AlertIncreaseCooldownSeconds，6-10 档后续使用 InvestigateIncreaseCooldownSeconds。
+	 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
+	 * @param bFirstIncreaseInBand 布尔开关 `bFirstIncreaseInBand`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API float ResolveAttractIncreaseCooldown(int32 currentAlert, bool bFirstIncreaseInBand,
+		const ULRGuardTuning& tuning);
+	/**
+	 * @brief 判断 Is Increase Allowed 对应条件；冷却拒绝的刺激被完全忽略，不改变观察状态。
+	 * @param now 时间值 `now`，单位为秒。
+	 * @param lastIncreaseTime 时间值 `lastIncreaseTime`，单位为秒。
+	 * @param cooldownSeconds 时间值 `cooldownSeconds`，单位为秒。
 	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 	 */
-	LOSTRUNIC_API bool ShouldDecay(float secondsSinceStimulus, float observeSeconds, bool bHasSight);
+	LOSTRUNIC_API bool IsIncreaseAllowed(double now, double lastIncreaseTime, float cooldownSeconds);
 }
diff --git a/Source/LostRunic/AI/LRGuardAIController.cpp b/Source/LostRunic/AI/LRGuardAIController.cpp
index 1d5e54d..33e3871 100644
--- a/Source/LostRunic/AI/LRGuardAIController.cpp
+++ b/Source/LostRunic/AI/LRGuardAIController.cpp
@@ -1,6 +1,6 @@
 /**
  * @file LRGuardAIController.cpp
- * @brief 把 AI Perception 的 Sight/Hearing 事件转换为警戒原因标签，并驱动 Idle、Suspicious、Investigate、Search、Chase 行为、导航速度和捕获检测。
+ * @brief 守卫控制器生命周期：构造、BeginPlay/EndPlay、OnPossess/OnUnPossess、调优解析与 StateTree 启动接线。感知与行为实现分别位于 LRGuardAIControllerPerception.cpp / LRGuardAIControllerBehavior.cpp。
  *
  * 关联文件：LRGuardAIController.h；所属领域：AI。
  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
@@ -9,26 +9,22 @@
 #include "AI/LRGuardAIController.h"
 
 #include "AI/LRAlertComponent.h"
+#include "AI/LRAlertRules.h"
 #include "AI/LRGuardCharacter.h"
 #include "AI/LRGuardPerceptionRules.h"
-#include "Components/ActorComponent.h"
 #include "Components/StateTreeAIComponent.h"
-#include "Core/LRGameplayTags.h"
 #include "Core/LRLog.h"
 #include "Data/LRGameTuningSet.h"
+#include "Data/LRGuardDefinition.h"
 #include "Data/LRGuardTuning.h"
-#include "DrawDebugHelpers.h"
+#include "Data/LRStateTuning.h"
 #include "Engine/GameInstance.h"
 #include "Engine/World.h"
 #include "Framework/LRGameInstanceSubsystem.h"
-#include "GameFramework/CharacterMovementComponent.h"
-#include "Navigation/PathFollowingComponent.h"
+#include "Items/LRCourageResponseComponent.h"
 #include "Perception/AIPerceptionComponent.h"
-#include "Perception/AISense_Hearing.h"
-#include "Perception/AISense_Sight.h"
 #include "Perception/AISenseConfig_Hearing.h"
 #include "Perception/AISenseConfig_Sight.h"
-#include "Stealth/LRGuardVisibility.h"
 #include "TimerManager.h"
 
 /**
@@ -41,6 +37,8 @@ ALRGuardAIController::ALRGuardAIController()
 	bStopAILogicOnUnposses = true;
 	bAttachToPawn = true;
 	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
+	// StateTree 由 OnPossess 依次完成定义解析、引用校验、SetStateTree、StartLogic，禁用自动启动。
+	StateTreeAI->SetStartLogicAutomatically(false);
 	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
 	SetPerceptionComponent(*AIPerception);
 	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
@@ -55,8 +53,12 @@ void ALRGuardAIController::BeginPlay()
 	Super::BeginPlay();
 	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
 	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
-	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
-	if (!ensureMsgf(Tuning, TEXT("%s requires Guard tuning."), *GetNameSafe(this)))
+	if (subsystem && subsystem->GetTuningSet())
+	{
+		Tuning = subsystem->GetTuningSet()->Guard;
+		StateTuning = subsystem->GetTuningSet()->State;
+	}
+	if (!ensureMsgf(Tuning && StateTuning, TEXT("%s requires Guard and State tuning."), *GetNameSafe(this)))
 	{
 		return;
 	}
@@ -79,13 +81,13 @@ void ALRGuardAIController::EndPlay(const EEndPlayReason::Type endPlayReason)
 	if (GetWorld())
 	{
 		GetWorld()->GetTimerManager().ClearTimer(CaptureTimer);
-		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
+		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
 	}
 	Super::EndPlay(endPlayReason);
 }
 
 /**
- * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
+ * @brief 处理 On Possess 事件：解析定义并校验引用、SetStateTree 后 StartLogic，绑定警戒与击退事件。
  * @param inPawn Controller 新接管的 Pawn；期望为 ALRGuardCharacter。
  */
 void ALRGuardAIController::OnPossess(APawn* inPawn)
@@ -97,10 +99,31 @@ void ALRGuardAIController::OnPossess(APawn* inPawn)
 	{
 		Alert->OnAlertChanged.AddDynamic(this, &ALRGuardAIController::HandleAlertChanged);
 	}
+	if (guard)
+	{
+		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
+		{
+			courage->OnKnockbackApplied.AddDynamic(this, &ALRGuardAIController::HandleKnockback);
+		}
+		ULRGuardDefinition* definition = guard->GetDefinition();
+		if (definition && definition->Behavior)
+		{
+			StateTreeAI->SetStateTree(definition->Behavior);
+			if (!StateTreeAI->IsRunning())
+			{
+				StateTreeAI->StartLogic();
+			}
+		}
+		else
+		{
+			UE_LOG(LogLostRunicAI, Warning, TEXT("Guard=%s definition or Behavior StateTree is missing; using controller fallback."),
+				*GetNameSafe(guard));
+		}
+	}
 }
 
 /**
- * @brief 处理 On Un Possess 事件，将引擎回调转换为对应领域状态更新。
+ * @brief 处理 On Un Possess 事件：解绑警戒与击退委托，停止 StateTree 逻辑。
  */
 void ALRGuardAIController::OnUnPossess()
 {
@@ -108,268 +131,33 @@ void ALRGuardAIController::OnUnPossess()
 	{
 		Alert->OnAlertChanged.RemoveDynamic(this, &ALRGuardAIController::HandleAlertChanged);
 	}
-	Alert.Reset();
-	Super::OnUnPossess();
-}
-
-/**
- * @brief 进入指定守卫行为，设置移动速度、焦点、导航目标或搜索超时。
- * @param behavior 要进入或退出的守卫 StateTree 行为状态。
- */
-void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
-{
-	ActiveBehavior = behavior;
-	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
-	if (!guard || !Alert.IsValid())
-	{
-		return;
-	}
-	GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
-	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
-	if (behavior == ELRGuardBehaviorState::Chase)
-	{
-		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
-		SetFocus(Alert->GetTargetActor());
-		MoveToActor(Alert->GetTargetActor(), GetEffectiveTuning().CaptureRadius);
-	}
-	else if (behavior == ELRGuardBehaviorState::Investigate)
-	{
-		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
-		MoveToLocation(Alert->GetLastDisturbanceLocation(), GetEffectiveTuning().MoveAcceptanceRadius);
-	}
-	else if (behavior == ELRGuardBehaviorState::Search)
+	if (ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn()))
 	{
-		StopMovement();
-		SetFocalPoint(Alert->GetLastDisturbanceLocation());
-		GetWorld()->GetTimerManager().SetTimer(SearchTimer, this, &ALRGuardAIController::HandleSearchTimeout,
-			GetEffectiveTuning().SearchDurationSeconds, false);
-	}
-	else if (behavior == ELRGuardBehaviorState::Suspicious)
-	{
-		StopMovement();
-		SetFocalPoint(Alert->GetLastDisturbanceLocation());
-	}
-	else
-	{
-		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
-		StartPatrolMove();
-	}
-}
-
-/**
- * @brief 退出指定守卫行为并清理该状态拥有的导航、焦点或计时器。
- * @param behavior 要进入或退出的守卫 StateTree 行为状态。
- */
-void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
-{
-	if (GetWorld())
-	{
-		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
-	}
-	StopMovement();
-	ClearFocus(EAIFocusPriority::Gameplay);
-}
-
-/**
- * @brief 处理 On Move Completed 事件，将引擎回调转换为对应领域状态更新。
- * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
- * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
- */
-void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
-{
-	Super::OnMoveCompleted(requestId, result);
-	if (!result.IsSuccess() || !Alert.IsValid())
-	{
-		return;
-	}
-	if (ActiveBehavior == ELRGuardBehaviorState::Investigate)
-	{
-		Alert->MarkInvestigationReached();
-	}
-	else if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
-	{
-		++PatrolIndex;
-		StartPatrolMove();
-	}
-}
-
-/**
- * @brief 把 UE 感知刺激转换为可见/听见事件、异常位置和警戒原因标签。
- * @param actor 本次查询、交互或事件涉及的 Actor。
- * @param stimulus 时间值 `stimulus`，单位为秒。
- */
-void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
-{
-	if (!actor || !Alert.IsValid())
-	{
-		return;
-	}
-	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
-	{
-		const bool bVisible = stimulus.WasSuccessfullySensed() && CanConfirmSight(actor);
-		Alert->SetSightTarget(actor, bVisible, stimulus.StimulusLocation);
-	}
-	else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
-	{
-		FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
-		if (!reason.IsValid())
+		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
 		{
-			reason = LRGameplayTags::NoiseInteraction;
+			courage->OnKnockbackApplied.RemoveDynamic(this, &ALRGuardAIController::HandleKnockback);
 		}
-		Alert->ApplyAlertDelta(GetEffectiveTuning().HearingAlertAmount, stimulus.StimulusLocation, actor, reason);
-	}
-}
-
-/**
- * @brief 处理 Handle Alert Changed 事件，将引擎回调转换为对应领域状态更新。
- * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
- * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
- * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
- * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
- * @param disturbanceLocation 空间值 `disturbanceLocation`；距离和位置使用 Unreal 厘米单位。
- */
-void ALRGuardAIController::HandleAlertChanged(const int32 previousLevel, const int32 currentLevel,
-	const ELRGuardBehaviorState currentState, const FGameplayTag reason, const FVector disturbanceLocation)
-{
-	StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventAlertChanged, FConstStructView(), reason.GetTagName());
-	if (!StateTreeAI->IsRunning())
-	{
-		EnterBehavior(currentState);
 	}
-}
-
-/**
- * @brief 用 Guard 调优资产配置 UE Sight/Hearing 感知，包括完整视野角换算、距离和阵营检测。
- */
-void ALRGuardAIController::ConfigurePerception()
-{
-	const ULRGuardTuning& tuning = GetEffectiveTuning();
-	SightConfig->SightRadius = tuning.SightRadius;
-	SightConfig->LoseSightRadius = tuning.LoseSightRadius;
-	SightConfig->PeripheralVisionAngleDegrees = tuning.SightConeDegrees * 0.5f;
-	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
-	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
-	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
-	HearingConfig->HearingRange = tuning.MaxHearingRange * tuning.HearingRangeMultiplier;
-	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
-	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
-	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
-	AIPerception->ConfigureSense(*SightConfig);
-	AIPerception->ConfigureSense(*HearingConfig);
-	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
-}
-
-/**
- * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程。
- */
-void ALRGuardAIController::HandleCaptureTimer()
-{
-	if (!Alert.IsValid() || Alert->GetBehaviorState() != ELRGuardBehaviorState::Chase)
-	{
-		return;
-	}
-	AActor* target = Alert->GetTargetActor();
-	if (!CanConfirmSight(target))
-	{
-		Alert->SetSightTarget(target, false, target ? target->GetActorLocation() : FVector::ZeroVector);
-		return;
-	}
-	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
-	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation()) <= GetEffectiveTuning().CaptureRadius)
-	{
-		guard->CaptureTarget(target);
-	}
-}
-
-/**
- * @brief 处理 Handle Search Timeout 事件，将引擎回调转换为对应领域状态更新。
- */
-void ALRGuardAIController::HandleSearchTimeout()
-{
-	if (Alert.IsValid())
-	{
-		Alert->ResetAfterSearch();
-	}
-}
-
-/**
- * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
- */
-void ALRGuardAIController::StartPatrolMove()
-{
-	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
-	if (!guard || guard->GetPatrolPointCount() == 0)
+	if (GetWorld())
 	{
-		StopMovement();
-		return;
+		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
 	}
-	PatrolIndex %= guard->GetPatrolPointCount();
-	MoveToActor(guard->GetPatrolPoint(PatrolIndex), GetEffectiveTuning().MoveAcceptanceRadius);
-}
-
-/**
- * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
- * @param actor 本次查询、交互或事件涉及的 Actor。
- * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
- */
-bool ALRGuardAIController::CanConfirmSight(AActor* actor) const
-{
-	const APawn* guardPawn = GetPawn();
-	if (!actor || !guardPawn)
+	if (StateTreeAI->IsRunning())
 	{
-		return false;
+		StateTreeAI->StopLogic(TEXT("OnUnPossess"));
 	}
-	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
-	const float distance = toTarget.Size2D();
-	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
-		toTarget.GetSafeNormal2D());
-	return LRGuardPerceptionRules::CanConfirmSight(distance, forwardDot, !LineOfSightTo(actor),
-		IsHiddenFromGuard(actor), GetEffectiveTuning());
+	Alert.Reset();
+	Super::OnUnPossess();
 }
 
 /**
- * @brief 判断 Is Hidden From Guard 对应条件；不产生玩法副作用。
- * @param actor 本次查询、交互或事件涉及的 Actor。
+ * @brief 查询 Resolved Behavior；行为状态唯一权威解析（眩晕优先，否则警戒推导），StateTree 只执行该结果。
  * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
  */
-bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
+ELRGuardBehaviorState ALRGuardAIController::GetResolvedBehavior() const
 {
-	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
-	{
-		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor, const_cast<ALRGuardAIController*>(this));
-	}
-	for (UActorComponent* component : actor->GetComponents())
-	{
-		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
-			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component, const_cast<ALRGuardAIController*>(this)))
-		{
-			return true;
-		}
-	}
-	return false;
-}
-
-/**
- * @brief 输出守卫行为、警戒值和最后异常点，并按调试开关绘制视野与听觉范围。
- */
-void ALRGuardAIController::LogAndDrawDiagnostics() const
-{
-	const APawn* guard = GetPawn();
-	if (!guard || !Alert.IsValid())
-	{
-		return;
-	}
-	const ULRGuardTuning& tuning = GetEffectiveTuning();
-	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s Alert=%d State=%d Target=%s Reason=%s Location=%s"),
-		*GetNameSafe(guard), Alert->GetAlertLevel(), static_cast<int32>(Alert->GetBehaviorState()),
-		*GetNameSafe(Alert->GetTargetActor()), *Alert->GetLastReason().ToString(),
-		*Alert->GetLastDisturbanceLocation().ToCompactString());
-	const FVector origin = guard->GetActorLocation();
-	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
-		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
-		16, FColor::Yellow, false, 5.0f);
-	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
-	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
+	return LRAlertRules::ResolveTargetBehavior(bStunned, Alert.IsValid() ? Alert->GetAlertLevel() : 0,
+		Alert.IsValid() && Alert->HasConfirmedSight(), Alert.IsValid() && Alert->IsSearching());
 }
 
 /**
diff --git a/Source/LostRunic/AI/LRGuardAIController.h b/Source/LostRunic/AI/LRGuardAIController.h
index aa0a1e1..bee40bc 100644
--- a/Source/LostRunic/AI/LRGuardAIController.h
+++ b/Source/LostRunic/AI/LRGuardAIController.h
@@ -20,6 +20,7 @@ class UAISenseConfig_Hearing;
 class UAISenseConfig_Sight;
 class ULRAlertComponent;
 class ULRGuardTuning;
+class ULRStateTuning;
 class UStateTreeAIComponent;
 
 /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
@@ -70,6 +71,13 @@ public:
 	 * @param behavior 要进入或退出的守卫 StateTree 行为状态。
 	 */
 	void ExitBehavior(ELRGuardBehaviorState behavior);
+	/**
+	 * @brief 查询 Resolved Behavior；行为状态唯一权威解析（眩晕优先，否则警戒推导），StateTree 只执行该结果。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
+	ELRGuardBehaviorState GetResolvedBehavior() const;
+
 	/**
 	 * @brief 输出守卫行为、警戒值和最后异常点，并按调试开关绘制视野与听觉范围。
 	 */
@@ -109,13 +117,19 @@ private:
 	 */
 	void ConfigurePerception();
 	/**
-	 * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程。
+	 * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程；眩晕期间跳过。
 	 */
 	void HandleCaptureTimer();
 	/**
-	 * @brief 处理 Handle Search Timeout 事件，将引擎回调转换为对应领域状态更新。
+	 * @brief 处理 Handle Knockback 事件：进入 Stunned 覆盖（停止移动、清除焦点），按 Courage 击退时长计时恢复。
+	 * @param direction 击退方向 `direction`；仅用于诊断。
 	 */
-	void HandleSearchTimeout();
+	UFUNCTION()
+	void HandleKnockback(FVector direction);
+	/**
+	 * @brief 眩晕计时结束后按当前警戒与视线重新解析行为并广播恢复事件。
+	 */
+	void HandleStunEnd();
 	/**
 	 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
 	 */
@@ -158,6 +172,10 @@ private:
 	UPROPERTY(Transient)
 	TObjectPtr<ULRGuardTuning> Tuning;
 
+	/** State 调优缓存；眩晕时长与 Courage 击退根运动同源。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TObjectPtr<ULRStateTuning> StateTuning;
+
 	/** Alert 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
 	UPROPERTY(Transient)
 	TWeakObjectPtr<ULRAlertComponent> Alert;
@@ -166,8 +184,10 @@ private:
 	ELRGuardBehaviorState ActiveBehavior = ELRGuardBehaviorState::IdlePatrol;
 	/** Patrol Index 的内部运行时数据；不参与蓝图配置。 */
 	int32 PatrolIndex = 0;
+	/** Stunned 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	bool bStunned = false;
 	/** Capture Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
 	FTimerHandle CaptureTimer;
-	/** Search Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
-	FTimerHandle SearchTimer;
+	/** Stun Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
+	FTimerHandle StunTimer;
 };
diff --git a/Source/LostRunic/AI/LRGuardAIControllerBehavior.cpp b/Source/LostRunic/AI/LRGuardAIControllerBehavior.cpp
new file mode 100644
index 0000000..e350d74
--- /dev/null
+++ b/Source/LostRunic/AI/LRGuardAIControllerBehavior.cpp
@@ -0,0 +1,225 @@
+/**
+ * @file LRGuardAIControllerBehavior.cpp
+ * @brief 守卫控制器行为实现：行为进出与移动驱动、警戒数据变化到 BehaviorChanged 的分派（仅实际变化时广播）、击退晕眩覆盖、巡逻与诊断。
+ *
+ * 关联文件：LRGuardAIController.h；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "AI/LRGuardAIController.h"
+
+#include "AI/LRAlertComponent.h"
+#include "AI/LRGuardCharacter.h"
+#include "Components/StateTreeAIComponent.h"
+#include "Core/LRGameplayTags.h"
+#include "Core/LRLog.h"
+#include "Data/LRGuardTuning.h"
+#include "Data/LRStateTuning.h"
+#include "DrawDebugHelpers.h"
+#include "Engine/World.h"
+#include "GameFramework/CharacterMovementComponent.h"
+#include "Navigation/PathFollowingComponent.h"
+#include "TimerManager.h"
+
+/**
+ * @brief 进入指定守卫行为，设置移动速度、焦点、导航目标；眩晕中仅接受 Stunned。
+ * @param behavior 要进入或退出的守卫 StateTree 行为状态。
+ */
+void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
+{
+	if (bStunned && behavior != ELRGuardBehaviorState::Stunned)
+	{
+		ActiveBehavior = ELRGuardBehaviorState::Stunned;
+		return;
+	}
+	ActiveBehavior = behavior;
+	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
+	if (!guard || !Alert.IsValid())
+	{
+		return;
+	}
+	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
+	if (behavior == ELRGuardBehaviorState::Chase)
+	{
+		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
+		SetFocus(Alert->GetTargetActor());
+		MoveToActor(Alert->GetTargetActor(), GetEffectiveTuning().CaptureRadius);
+	}
+	else if (behavior == ELRGuardBehaviorState::Investigate)
+	{
+		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
+		MoveToLocation(Alert->GetLastDisturbanceLocation(), GetEffectiveTuning().MoveAcceptanceRadius);
+	}
+	else if (behavior == ELRGuardBehaviorState::Search)
+	{
+		// 抵达观察与自然衰减由 AlertComponent 的观察计时与衰减计时驱动，不再使用固定搜索时长。
+		StopMovement();
+		SetFocalPoint(Alert->GetLastDisturbanceLocation());
+	}
+	else if (behavior == ELRGuardBehaviorState::Suspicious)
+	{
+		StopMovement();
+		SetFocalPoint(Alert->GetLastDisturbanceLocation());
+	}
+	else if (behavior == ELRGuardBehaviorState::Stunned)
+	{
+		StopMovement();
+		ClearFocus(EAIFocusPriority::Gameplay);
+	}
+	else
+	{
+		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
+		StartPatrolMove();
+	}
+}
+
+/**
+ * @brief 退出指定守卫行为并清理该状态拥有的导航、焦点或计时器。
+ * @param behavior 要进入或退出的守卫 StateTree 行为状态。
+ */
+void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
+{
+	StopMovement();
+	ClearFocus(EAIFocusPriority::Gameplay);
+}
+
+/**
+ * @brief 处理 On Move Completed 事件：调查抵达转入 Search（开始观察），巡逻点到达续走下一段。
+ * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
+ * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
+ */
+void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
+{
+	Super::OnMoveCompleted(requestId, result);
+	if (!result.IsSuccess() || !Alert.IsValid())
+	{
+		return;
+	}
+	if (ActiveBehavior == ELRGuardBehaviorState::Investigate)
+	{
+		Alert->MarkInvestigationReached();
+	}
+	else if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
+	{
+		++PatrolIndex;
+		StartPatrolMove();
+	}
+}
+
+/**
+ * @brief 处理 Handle Alert Changed 事件：仅表示感知/警戒数据变化；只有当权威解析结果实际变化时才广播 BehaviorChanged，避免衰减计时等数值变化导致 StateTree 无意义重入。
+ * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
+ * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
+ * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
+ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+ * @param disturbanceLocation 空间值 `disturbanceLocation`；距离和位置使用 Unreal 厘米单位。
+ */
+void ALRGuardAIController::HandleAlertChanged(const int32 previousLevel, const int32 currentLevel,
+	const ELRGuardBehaviorState currentState, const FGameplayTag reason, const FVector disturbanceLocation)
+{
+	const ELRGuardBehaviorState resolved = GetResolvedBehavior();
+	if (resolved == ActiveBehavior)
+	{
+		// 同状态 Investigate 的数据级重定位（新调查点），不发行为事件。
+		if (resolved == ELRGuardBehaviorState::Investigate)
+		{
+			EnterBehavior(ELRGuardBehaviorState::Investigate);
+		}
+		return;
+	}
+	if (StateTreeAI->IsRunning())
+	{
+		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
+	}
+	else
+	{
+		EnterBehavior(resolved);
+	}
+}
+
+/**
+ * @brief 处理 Handle Knockback 事件：进入 Stunned 覆盖（停止移动、清除焦点），按 Courage 击退时长计时恢复。
+ * @param direction 击退方向 `direction`；仅用于诊断。
+ */
+void ALRGuardAIController::HandleKnockback(const FVector direction)
+{
+	if (bStunned)
+	{
+		return;
+	}
+	bStunned = true;
+	StopMovement();
+	ClearFocus(EAIFocusPriority::Gameplay);
+	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s stunned for %.2fs direction=%s"), *GetNameSafe(GetPawn()),
+		StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, *direction.ToCompactString());
+	if (StateTreeAI->IsRunning())
+	{
+		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
+	}
+	else
+	{
+		EnterBehavior(ELRGuardBehaviorState::Stunned);
+	}
+	if (GetWorld())
+	{
+		GetWorld()->GetTimerManager().SetTimer(StunTimer, this, &ALRGuardAIController::HandleStunEnd,
+			StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, false);
+	}
+}
+
+/**
+ * @brief 眩晕计时结束后按当前警戒与视线重新解析行为并广播恢复事件；感知与警戒在眩晕期间持续运行。
+ */
+void ALRGuardAIController::HandleStunEnd()
+{
+	bStunned = false;
+	const ELRGuardBehaviorState resolved = GetResolvedBehavior();
+	if (StateTreeAI->IsRunning())
+	{
+		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
+	}
+	else
+	{
+		EnterBehavior(resolved);
+	}
+}
+
+/**
+ * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
+ */
+void ALRGuardAIController::StartPatrolMove()
+{
+	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
+	if (!guard || guard->GetPatrolPointCount() == 0)
+	{
+		StopMovement();
+		return;
+	}
+	PatrolIndex %= guard->GetPatrolPointCount();
+	MoveToActor(guard->GetPatrolPoint(PatrolIndex), GetEffectiveTuning().MoveAcceptanceRadius);
+}
+
+/**
+ * @brief 输出守卫行为、警戒值、观察/眩晕状态和最后异常点，并按调试开关绘制视野与听觉范围。
+ */
+void ALRGuardAIController::LogAndDrawDiagnostics() const
+{
+	const APawn* guard = GetPawn();
+	if (!guard || !Alert.IsValid())
+	{
+		return;
+	}
+	const ULRGuardTuning& tuning = GetEffectiveTuning();
+	UE_LOG(LogLostRunicAI, Display,
+		TEXT("Guard=%s Alert=%d ResolvedState=%d Observing=%d Stunned=%d Target=%s Reason=%s Location=%s"),
+		*GetNameSafe(guard), Alert->GetAlertLevel(), static_cast<int32>(GetResolvedBehavior()),
+		Alert->IsObserving() ? 1 : 0, bStunned ? 1 : 0,
+		*GetNameSafe(Alert->GetTargetActor()), *Alert->GetLastReason().GetTagName().ToString(),
+		*Alert->GetLastDisturbanceLocation().ToCompactString());
+	const FVector origin = guard->GetActorLocation();
+	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
+		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
+		16, FColor::Yellow, false, 5.0f);
+	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
+	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
+}
diff --git a/Source/LostRunic/AI/LRGuardAIControllerPerception.cpp b/Source/LostRunic/AI/LRGuardAIControllerPerception.cpp
new file mode 100644
index 0000000..255508d
--- /dev/null
+++ b/Source/LostRunic/AI/LRGuardAIControllerPerception.cpp
@@ -0,0 +1,148 @@
+/**
+ * @file LRGuardAIControllerPerception.cpp
+ * @brief 守卫控制器感知实现：Sight/Hearing 刺激转换为警戒语义入口、感知配置、遮挡与隐藏判定、捕获计时（眩晕期间跳过）。
+ *
+ * 关联文件：LRGuardAIController.h；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "AI/LRGuardAIController.h"
+
+#include "AI/LRAlertComponent.h"
+#include "AI/LRGuardCharacter.h"
+#include "AI/LRGuardPerceptionRules.h"
+#include "Core/LRGameplayTags.h"
+#include "Data/LRGuardTuning.h"
+#include "Engine/World.h"
+#include "GameFramework/CharacterMovementComponent.h"
+#include "Perception/AIPerceptionComponent.h"
+#include "Perception/AISense_Hearing.h"
+#include "Perception/AISense_Sight.h"
+#include "Perception/AISenseConfig_Hearing.h"
+#include "Perception/AISenseConfig_Sight.h"
+#include "Stealth/LRGuardVisibility.h"
+
+/**
+ * @brief 把 UE 感知刺激转换为可见/听见事件、异常位置和警戒原因标签；听觉走 ResolveNoiseAlertDelta 语义（吸引/Set 分派）。
+ * @param actor 本次查询、交互或事件涉及的 Actor。
+ * @param stimulus 时间值 `stimulus`，单位为秒。
+ */
+void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
+{
+	if (!actor || !Alert.IsValid())
+	{
+		return;
+	}
+	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
+	{
+		const bool bVisible = stimulus.WasSuccessfullySensed() && CanConfirmSight(actor);
+		Alert->SetSightTarget(actor, bVisible, stimulus.StimulusLocation);
+	}
+	else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
+	{
+		FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
+		if (!reason.IsValid())
+		{
+			reason = LRGameplayTags::NoiseInteraction;
+		}
+		const FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
+			reason, Alert->GetAlertLevel(), GetEffectiveTuning());
+		if (!response.bRespond)
+		{
+			return;
+		}
+		if (response.bIsAttract)
+		{
+			Alert->ApplyAttract(stimulus.StimulusLocation, actor, reason);
+		}
+		else
+		{
+			Alert->ApplyAlertDelta(response.Delta, stimulus.StimulusLocation, actor, reason);
+		}
+	}
+}
+
+/**
+ * @brief 用 Guard 调优资产配置 UE Sight/Hearing 感知，包括完整视野角换算、距离和阵营检测。
+ */
+void ALRGuardAIController::ConfigurePerception()
+{
+	const ULRGuardTuning& tuning = GetEffectiveTuning();
+	SightConfig->SightRadius = tuning.SightRadius;
+	SightConfig->LoseSightRadius = tuning.LoseSightRadius;
+	SightConfig->PeripheralVisionAngleDegrees = tuning.SightConeDegrees * 0.5f;
+	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
+	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
+	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
+	HearingConfig->HearingRange = tuning.MaxHearingRange * tuning.HearingRangeMultiplier;
+	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
+	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
+	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
+	AIPerception->ConfigureSense(*SightConfig);
+	AIPerception->ConfigureSense(*HearingConfig);
+	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
+}
+
+/**
+ * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程；眩晕期间跳过。
+ */
+void ALRGuardAIController::HandleCaptureTimer()
+{
+	if (bStunned || !Alert.IsValid() || Alert->GetBehaviorState() != ELRGuardBehaviorState::Chase)
+	{
+		return;
+	}
+	AActor* target = Alert->GetTargetActor();
+	if (!CanConfirmSight(target))
+	{
+		Alert->SetSightTarget(target, false, target ? target->GetActorLocation() : FVector::ZeroVector);
+		return;
+	}
+	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
+	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation()) <= GetEffectiveTuning().CaptureRadius)
+	{
+		guard->CaptureTarget(target);
+	}
+}
+
+/**
+ * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
+ * @param actor 本次查询、交互或事件涉及的 Actor。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+bool ALRGuardAIController::CanConfirmSight(AActor* actor) const
+{
+	const APawn* guardPawn = GetPawn();
+	if (!actor || !guardPawn)
+	{
+		return false;
+	}
+	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
+	const float distance = toTarget.Size2D();
+	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
+		toTarget.GetSafeNormal2D());
+	return LRGuardPerceptionRules::CanConfirmSight(distance, forwardDot, !LineOfSightTo(actor),
+		IsHiddenFromGuard(actor), GetEffectiveTuning());
+}
+
+/**
+ * @brief 判断 Is Hidden From Guard 对应条件；不产生玩法副作用。
+ * @param actor 本次查询、交互或事件涉及的 Actor。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
+{
+	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
+	{
+		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor, const_cast<ALRGuardAIController*>(this));
+	}
+	for (UActorComponent* component : actor->GetComponents())
+	{
+		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
+			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component, const_cast<ALRGuardAIController*>(this)))
+		{
+			return true;
+		}
+	}
+	return false;
+}
diff --git a/Source/LostRunic/AI/LRGuardCharacter.cpp b/Source/LostRunic/AI/LRGuardCharacter.cpp
index 890a040..96fdcec 100644
--- a/Source/LostRunic/AI/LRGuardCharacter.cpp
+++ b/Source/LostRunic/AI/LRGuardCharacter.cpp
@@ -10,12 +10,14 @@
 
 #include "AI/LRAlertComponent.h"
 #include "AI/LRGuardAIController.h"
+#include "Components/WidgetComponent.h"
 #include "Core/LRGameplayTags.h"
 #include "Engine/GameInstance.h"
 #include "Framework/LRCharacter.h"
 #include "Items/LRCourageResponseComponent.h"
 #include "Save/LRSaveSubsystem.h"
 #include "State/LRStateComponent.h"
+#include "UI/LRWorldAlertBarWidgetBase.h"
 
 /**
  * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
@@ -27,6 +29,26 @@ ALRGuardCharacter::ALRGuardCharacter()
 	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
 	Alert = CreateDefaultSubobject<ULRAlertComponent>(TEXT("Alert"));
 	CourageResponse = CreateDefaultSubobject<ULRCourageResponseComponent>(TEXT("CourageResponse"));
+	AlertWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertWidget"));
+	AlertWidget->SetupAttachment(GetMesh());
+	AlertWidget->SetWidgetSpace(EWidgetSpace::Screen);
+	AlertWidget->SetDrawSize(FVector2D(120.0f, 24.0f));
+	AlertWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
+}
+
+/**
+ * @brief 在进入世界后解析运行时依赖：将世界警戒条 Widget 初始化到本守卫的警戒快照。
+ */
+void ALRGuardCharacter::BeginPlay()
+{
+	Super::BeginPlay();
+	if (UUserWidget* widget = AlertWidget->GetWidget())
+	{
+		if (ULRWorldAlertBarWidgetBase* alertBar = Cast<ULRWorldAlertBarWidgetBase>(widget))
+		{
+			alertBar->InitializeForGuard(this);
+		}
+	}
 }
 
 /**
diff --git a/Source/LostRunic/AI/LRGuardCharacter.h b/Source/LostRunic/AI/LRGuardCharacter.h
index 330bec0..c2b9dfc 100644
--- a/Source/LostRunic/AI/LRGuardCharacter.h
+++ b/Source/LostRunic/AI/LRGuardCharacter.h
@@ -17,6 +17,7 @@ class ALRGuardAIController;
 class ULRCourageResponseComponent;
 class ULRAlertComponent;
 class ULRGuardDefinition;
+class UWidgetComponent;
 
 DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRPlayerCaptured, AActor*, playerActor);
 
@@ -32,6 +33,11 @@ public:
 	 */
 	ALRGuardCharacter();
 
+	/**
+	 * @brief 在进入世界后解析运行时依赖：将世界警戒条 Widget 初始化到本守卫的警戒快照。
+	 */
+	virtual void BeginPlay() override;
+
 	/**
 	 * @brief 查询 Alert Component；不修改领域状态。
 	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
@@ -47,6 +53,20 @@ public:
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI")
 	bool CaptureTarget(AActor* target);
 
+	/**
+	 * @brief 查询 Definition；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
+	ULRGuardDefinition* GetDefinition() const { return Definition; }
+
+	/**
+	 * @brief 查询 Courage Response Component；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
+	ULRCourageResponseComponent* GetCourageResponseComponent() const { return CourageResponse; }
+
 	/**
 	 * @brief 查询 Patrol Point；不修改领域状态。
 	 * @param index 目标元素索引，调用前必须满足对应容器边界。
@@ -80,4 +100,8 @@ private:
 	/** Courage Response 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
 	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
 	TObjectPtr<ULRCourageResponseComponent> CourageResponse;
+
+	/** Alert Widget 的世界空间 WidgetComponent；WidgetClass 与样式由蓝图配置，C++ 只负责初始化绑定。 仅在蓝图或详情面板中查看，不可编辑。 */
+	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
+	TObjectPtr<UWidgetComponent> AlertWidget;
 };
diff --git a/Source/LostRunic/AI/LRGuardPerceptionRules.cpp b/Source/LostRunic/AI/LRGuardPerceptionRules.cpp
index bb98311..f9bfdf6 100644
--- a/Source/LostRunic/AI/LRGuardPerceptionRules.cpp
+++ b/Source/LostRunic/AI/LRGuardPerceptionRules.cpp
@@ -8,6 +8,7 @@
  */
 #include "AI/LRGuardPerceptionRules.h"
 
+#include "Core/LRGameplayTags.h"
 #include "Data/LRGuardTuning.h"
 
 /**
@@ -37,3 +38,36 @@ bool LRGuardPerceptionRules::CanHear(const float distance, const float sourceRad
 {
 	return distance <= sourceRadius * tuning.HearingRangeMultiplier;
 }
+
+/**
+ * @brief 按噪声原因标签解析守卫应做的警戒响应；CD 与观察时序由调用方组件执行，本函数只做语义映射。
+ * @param reason 噪声原因 Gameplay Tag，例如 Noise.Footstep.Walk 或 Noise.Footstep.Run.Indoor。
+ * @param currentAlert 守卫当前警戒值 0-11。
+ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+ * @return 结构化响应：是否响应、Delta 与是否走吸引语义（IsAttract 时调用方使用带 CD 门控的 ApplyAttract）。
+ */
+FLRNoiseResponse LRGuardPerceptionRules::ResolveNoiseAlertDelta(const FGameplayTag reason, const int32 currentAlert,
+	const ULRGuardTuning& tuning)
+{
+	FLRNoiseResponse response;
+	if (reason == LRGameplayTags::NoiseFootstepRunIndoor)
+	{
+		// 室内奔跑为「警戒至少提升到 RoomRunAlertLevel」的 Set 语义，不走吸引 CD。
+		response.bRespond = true;
+		response.Delta = FMath::Max(tuning.RoomRunAlertLevel - currentAlert, 0);
+		response.bIsAttract = false;
+		return response;
+	}
+	if (reason == LRGameplayTags::NoiseFootstepWalkFaint)
+	{
+		// 室外非潜行关走路：只有警戒 >=6 的守卫才会被吸引。
+		response.bIsAttract = true;
+		response.bRespond = currentAlert >= tuning.SightInvestigateLevel;
+		response.Delta = response.bRespond ? tuning.AttractAlertAmount : 0;
+		return response;
+	}
+	response.bRespond = true;
+	response.Delta = tuning.AttractAlertAmount;
+	response.bIsAttract = true;
+	return response;
+}
diff --git a/Source/LostRunic/AI/LRGuardPerceptionRules.h b/Source/LostRunic/AI/LRGuardPerceptionRules.h
index 95674cd..f9e65be 100644
--- a/Source/LostRunic/AI/LRGuardPerceptionRules.h
+++ b/Source/LostRunic/AI/LRGuardPerceptionRules.h
@@ -8,8 +8,21 @@
  */
 #pragma once
 
+#include "GameplayTagContainer.h"
+
 class ULRGuardTuning;
 
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+struct LOSTRUNIC_API FLRNoiseResponse
+{
+	/** Respond 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 */
+	bool bRespond = false;
+	/** Delta 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 */
+	int32 Delta = 0;
+	/** Is Attract 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 */
+	bool bIsAttract = false;
+};
+
 namespace LRGuardPerceptionRules
 {
 	LOSTRUNIC_API bool CanConfirmSight(float distance, float forwardDot, bool bOccluded, bool bHidden,
@@ -22,4 +35,13 @@ namespace LRGuardPerceptionRules
 	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 	 */
 	LOSTRUNIC_API bool CanHear(float distance, float sourceRadius, const ULRGuardTuning& tuning);
+	/**
+	 * @brief 按噪声原因标签解析守卫应做的警戒响应；CD 与观察时序由调用方组件执行，本函数只做语义映射。
+	 * @param reason 噪声原因 Gameplay Tag，例如 Noise.Footstep.Walk 或 Noise.Footstep.Run.Indoor。
+	 * @param currentAlert 守卫当前警戒值 0-11。
+	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+	 * @return 结构化响应：是否响应、Delta 与是否走吸引语义（IsAttract 时调用方使用带 CD 门控的 ApplyAttract）。
+	 */
+	LOSTRUNIC_API FLRNoiseResponse ResolveNoiseAlertDelta(FGameplayTag reason, int32 currentAlert,
+		const ULRGuardTuning& tuning);
 }
diff --git a/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp b/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp
index 5d07b0d..d1c17f7 100644
--- a/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp
+++ b/Source/LostRunic/AI/LRGuardStateTreeNodes.cpp
@@ -61,6 +61,6 @@ void FLRGuardBehaviorTask::ExitState(FStateTreeExecutionContext& context,
 bool FLRGuardStateCondition::TestCondition(FStateTreeExecutionContext& context) const
 {
 	const FInstanceDataType& data = context.GetInstanceData(*this);
-	const ULRAlertComponent* alert = data.AIController ? data.AIController->GetAlertComponent() : nullptr;
-	return alert && alert->GetBehaviorState() == ExpectedBehavior;
+	// StateTree 只执行控制器解析的结果，不自行重新定义警戒语义。
+	return data.AIController && data.AIController->GetResolvedBehavior() == ExpectedBehavior;
 }
diff --git a/Source/LostRunic/AI/LRGuardTypes.h b/Source/LostRunic/AI/LRGuardTypes.h
index 82c73e7..90c8bca 100644
--- a/Source/LostRunic/AI/LRGuardTypes.h
+++ b/Source/LostRunic/AI/LRGuardTypes.h
@@ -20,5 +20,43 @@ enum class ELRGuardBehaviorState : uint8
 	Suspicious UMETA(DisplayName = "Suspicious"),
 	Investigate UMETA(DisplayName = "Investigate"),
 	Search UMETA(DisplayName = "Search"),
-	Chase UMETA(DisplayName = "Chase")
+	Chase UMETA(DisplayName = "Chase"),
+	Stunned UMETA(DisplayName = "Stunned")
+};
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Alert Tier"))
+enum class ELRGuardAlertTier : uint8
+{
+	Hidden UMETA(DisplayName = "Hidden"),
+	White UMETA(DisplayName = "White"),
+	Red UMETA(DisplayName = "Red"),
+	Full UMETA(DisplayName = "Full")
+};
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Alert Snapshot"))
+struct LOSTRUNIC_API FLRAlertSnapshot
+{
+	GENERATED_BODY()
+
+	/** Level 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 蓝图可读取但不可写入。 */
+	UPROPERTY(BlueprintReadOnly, Category = "Alert")
+	int32 Level = 0;
+
+	/** Fraction 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.0f`。 蓝图可读取但不可写入。 */
+	UPROPERTY(BlueprintReadOnly, Category = "Alert")
+	float Fraction = 0.0f;
+
+	/** Tier 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardAlertTier::Hidden`。 蓝图可读取但不可写入。 */
+	UPROPERTY(BlueprintReadOnly, Category = "Alert")
+	ELRGuardAlertTier Tier = ELRGuardAlertTier::Hidden;
+
+	/** Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardBehaviorState::IdlePatrol`。 蓝图可读取但不可写入。 */
+	UPROPERTY(BlueprintReadOnly, Category = "Alert")
+	ELRGuardBehaviorState Behavior = ELRGuardBehaviorState::IdlePatrol;
+
+	/** Full Alert 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
+	UPROPERTY(BlueprintReadOnly, Category = "Alert")
+	bool bFullAlert = false;
 };
diff --git a/Source/LostRunic/AI/LRNPCCharacter.cpp b/Source/LostRunic/AI/LRNPCCharacter.cpp
new file mode 100644
index 0000000..49da714
--- /dev/null
+++ b/Source/LostRunic/AI/LRNPCCharacter.cpp
@@ -0,0 +1,141 @@
+/**
+ * @file LRNPCCharacter.cpp
+ * @brief 通用非战斗 NPC 实现：对话交互（Talk 选项经 ULRDialogueSubsystem::StartDialogue）与噪声表现钩子；行为由 StateTree/控制器驱动。
+ *
+ * 关联文件：LRNPCCharacter.h；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "AI/LRNPCCharacter.h"
+
+#include "AI/LRNPCController.h"
+#include "Core/LRGameplayTags.h"
+#include "Core/LRLog.h"
+#include "Data/LRNPCDefinition.h"
+#include "Engine/GameInstance.h"
+#include "Narrative/LRDialogueSubsystem.h"
+
+/**
+ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ */
+ALRNPCCharacter::ALRNPCCharacter()
+{
+	PrimaryActorTick.bCanEverTick = false;
+	AIControllerClass = ALRNPCController::StaticClass();
+	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
+}
+
+/**
+ * @brief 查询 Patrol Point；不修改领域状态。
+ * @param index 目标元素索引，调用前必须满足对应容器边界。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+AActor* ALRNPCCharacter::GetPatrolPoint(const int32 index) const
+{
+	return PatrolPoints.IsValidIndex(index) ? PatrolPoints[index].Get() : nullptr;
+}
+
+/**
+ * @brief 通知 NPC 听见噪声：触发表现钩子与预留委托；Conversation 高优先级时由控制器决定是否切换行为。
+ * @param location 世界空间位置，Unreal 单位为厘米。
+ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+ */
+void ALRNPCCharacter::NotifyNoiseHeard(const FVector location, const FGameplayTag reason)
+{
+	OnNoiseHeard(location, reason);
+	OnNPCAttentionChanged.Broadcast(location, reason);
+}
+
+/**
+ * @brief 查询当前 NPC 行为（由控制器权威解析）；不修改领域状态。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRNPCBehaviorState ALRNPCCharacter::GetActiveBehavior() const
+{
+	const ALRNPCController* controller = Cast<ALRNPCController>(GetController());
+	return controller ? controller->GetActiveBehavior() : ELRNPCBehaviorState::Idle;
+}
+
+/**
+ * @brief 查询 Interaction Options：仅 Talk（对话），要求 Normal 状态。
+ * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+TArray<FLRInteractionOption> ALRNPCCharacter::GetInteractionOptions_Implementation(AActor* interactor)
+{
+	TArray<FLRInteractionOption> options;
+	if (Definition && !Definition->DialogueRowId.IsNone())
+	{
+		FLRInteractionOption option;
+		option.ActionTag = LRGameplayTags::InteractionActionTalk;
+		option.Prompt = NSLOCTEXT("LostRunic", "NPC.TalkPrompt", "对话");
+		option.RequiredMode = ELRPerceptionMode::Normal;
+		options.Add(option);
+	}
+	return options;
+}
+
+/**
+ * @brief 查询 Interaction Location；不修改领域状态。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+FVector ALRNPCCharacter::GetInteractionLocation_Implementation()
+{
+	return GetActorLocation();
+}
+
+/**
+ * @brief 实现 Execute Interaction 对应的领域步骤：Talk 经 ULRDialogueSubsystem::StartDialogue 启动对话并进入 Conversation；对话结束回到默认行为。
+ * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
+ * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+FLRInteractionResult ALRNPCCharacter::ExecuteInteraction_Implementation(AActor* interactor, const FGameplayTag actionTag)
+{
+	FLRInteractionResult result;
+	result.ActionTag = actionTag;
+	if (actionTag != LRGameplayTags::InteractionActionTalk)
+	{
+		result.FailureReason = LRGameplayTags::InteractionRejectState;
+		return result;
+	}
+	ULRDialogueSubsystem* dialogue = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
+	if (!dialogue || !Definition || Definition->DialogueRowId.IsNone())
+	{
+		result.FailureReason = LRGameplayTags::NarrativeRejectMissingContent;
+		return result;
+	}
+	const FLRNarrativeResult narrative = dialogue->StartDialogue(Definition->DialogueRowId);
+	if (!narrative.bSuccess)
+	{
+		result.FailureReason = narrative.FailureReason;
+		return result;
+	}
+	if (ALRNPCController* controller = Cast<ALRNPCController>(GetController()))
+	{
+		controller->NotifyDialogueStarted();
+	}
+	dialogue->OnSessionEnded.AddUniqueDynamic(this, &ALRNPCCharacter::HandleDialogueSessionEnded);
+	result.bSuccess = true;
+	return result;
+}
+
+/**
+ * @brief 处理 Handle Dialogue Session Ended 事件，将引擎回调转换为对应领域状态更新。
+ * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
+ * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
+ */
+void ALRNPCCharacter::HandleDialogueSessionEnded(const ELRNarrativeSessionType sessionType, const FName contentId)
+{
+	if (ALRNPCController* controller = Cast<ALRNPCController>(GetController()))
+	{
+		controller->NotifyDialogueEnded();
+	}
+	if (UGameInstance* gameInstance = GetGameInstance())
+	{
+		if (ULRDialogueSubsystem* dialogue = gameInstance->GetSubsystem<ULRDialogueSubsystem>())
+		{
+			dialogue->OnSessionEnded.RemoveDynamic(this, &ALRNPCCharacter::HandleDialogueSessionEnded);
+		}
+	}
+}
diff --git a/Source/LostRunic/AI/LRNPCCharacter.h b/Source/LostRunic/AI/LRNPCCharacter.h
new file mode 100644
index 0000000..6188982
--- /dev/null
+++ b/Source/LostRunic/AI/LRNPCCharacter.h
@@ -0,0 +1,99 @@
+/**
+ * @file LRNPCCharacter.h
+ * @brief 通用非战斗 NPC：由 StateTree（Idle/Patrol/ReactToNoise/Conversation）驱动；实现对话交互（Talk 选项经 ULRDialogueSubsystem::StartDialogue）与噪声表现钩子（OnNoiseHeard / OnNPCAttentionChanged 预留未来告警/逃离）。
+ *
+ * 关联文件：LRNPCCharacter.cpp；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "AI/LRNPCTypes.h"
+#include "GameFramework/Character.h"
+#include "Interaction/LRInteractable.h"
+
+#include "LRNPCCharacter.generated.h"
+
+class ALRNPCController;
+class ULRNPCDefinition;
+class ULRDialogueSubsystem;
+
+DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNPCAttentionChanged, FVector, location, FGameplayTag, reason);
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Character"))
+class LOSTRUNIC_API ALRNPCCharacter : public ACharacter, public ILRInteractable
+{
+	GENERATED_BODY()
+
+public:
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 */
+	ALRNPCCharacter();
+
+	/**
+	 * @brief 查询 Definition；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|NPC")
+	ULRNPCDefinition* GetDefinition() const { return Definition; }
+
+	/**
+	 * @brief 查询 Patrol Point；不修改领域状态。
+	 * @param index 目标元素索引，调用前必须满足对应容器边界。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	AActor* GetPatrolPoint(int32 index) const;
+	/**
+	 * @brief 查询 Patrol Point Count；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	int32 GetPatrolPointCount() const { return PatrolPoints.Num(); }
+
+	/**
+	 * @brief 通知 NPC 听见噪声：触发表现钩子与预留委托；Conversation 高优先级时由控制器决定是否切换行为。
+	 * @param location 世界空间位置，Unreal 单位为厘米。
+	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+	 */
+	void NotifyNoiseHeard(const FVector location, const FGameplayTag reason);
+
+	/**
+	 * @brief 查询当前 NPC 行为（由控制器权威解析）；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|NPC")
+	ELRNPCBehaviorState GetActiveBehavior() const;
+
+	//~ ILRInteractable
+	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;
+	virtual FVector GetInteractionLocation_Implementation() override;
+	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor, FGameplayTag actionTag) override;
+	//~ End ILRInteractable
+
+	/** 当 Noise Heard 发生时广播；蓝图可绑定该委托以更新表现（转向、表情等），不应在回调中改写核心规则。  */
+	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|NPC")
+	void OnNoiseHeard(const FVector location, const FGameplayTag reason);
+
+	/** 当 NPC Attention Changed 发生时广播；预留未来告警/逃离扩展钩子，本次不实现告警逻辑。  */
+	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|NPC")
+	FLRNPCAttentionChanged OnNPCAttentionChanged;
+
+protected:
+	/** Definition 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
+	TObjectPtr<ULRNPCDefinition> Definition;
+
+	/** Patrol Points 的领域数据，由所属类型负责维护和校验。 可在关卡中的蓝图实例详情面板配置。 */
+	UPROPERTY(EditInstanceOnly, Category = "NPC|Patrol")
+	TArray<TObjectPtr<AActor>> PatrolPoints;
+
+private:
+	/**
+	 * @brief 处理 Handle Dialogue Session Ended 事件，将引擎回调转换为对应领域状态更新。
+	 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
+	 * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
+	 */
+	UFUNCTION()
+	void HandleDialogueSessionEnded(ELRNarrativeSessionType sessionType, FName contentId);
+};
diff --git a/Source/LostRunic/AI/LRNPCController.cpp b/Source/LostRunic/AI/LRNPCController.cpp
new file mode 100644
index 0000000..036702e
--- /dev/null
+++ b/Source/LostRunic/AI/LRNPCController.cpp
@@ -0,0 +1,375 @@
+/**
+ * @file LRNPCController.cpp
+ * @brief 通用 NPC 控制器实现：Hearing 感知、StateTree 生命周期、巡逻、低频玩家朝向与限时噪声反应。
+ *
+ * 关联文件：LRNPCController.h；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "AI/LRNPCController.h"
+
+#include "AI/LRNPCCharacter.h"
+#include "Components/StateTreeAIComponent.h"
+#include "Core/LRGameplayTags.h"
+#include "Core/LRLog.h"
+#include "Data/LRGameTuningSet.h"
+#include "Data/LRNPCDefinition.h"
+#include "Data/LRNPCTuning.h"
+#include "Engine/GameInstance.h"
+#include "Engine/World.h"
+#include "Framework/LRGameInstanceSubsystem.h"
+#include "GameFramework/CharacterMovementComponent.h"
+#include "Navigation/PathFollowingComponent.h"
+#include "Perception/AIPerceptionComponent.h"
+#include "Perception/AISense_Hearing.h"
+#include "Perception/AISenseConfig_Hearing.h"
+#include "TimerManager.h"
+
+/**
+ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ */
+ALRNPCController::ALRNPCController()
+{
+	PrimaryActorTick.bCanEverTick = false;
+	bStartAILogicOnPossess = true;
+	bStopAILogicOnUnposses = true;
+	bAttachToPawn = true;
+	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
+	StateTreeAI->SetStartLogicAutomatically(false);
+	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
+	SetPerceptionComponent(*AIPerception);
+	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
+}
+
+/**
+ * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
+ */
+void ALRNPCController::BeginPlay()
+{
+	Super::BeginPlay();
+	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
+	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
+	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->NPC : nullptr;
+	if (!ensureMsgf(Tuning, TEXT("%s requires NPC tuning."), *GetNameSafe(this)))
+	{
+		return;
+	}
+	HearingConfig->HearingRange = 5000.0f;
+	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
+	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
+	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
+	AIPerception->ConfigureSense(*HearingConfig);
+	AIPerception->SetDominantSense(HearingConfig->GetSenseImplementation());
+	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRNPCController::HandlePerception);
+}
+
+/**
+ * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
+ * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
+ */
+void ALRNPCController::EndPlay(const EEndPlayReason::Type endPlayReason)
+{
+	if (AIPerception)
+	{
+		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRNPCController::HandlePerception);
+	}
+	if (GetWorld())
+	{
+		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
+		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
+	}
+	Super::EndPlay(endPlayReason);
+}
+
+/**
+ * @brief 处理 On Possess 事件：解析定义、SetStateTree 后 StartLogic，并绑定 Hearing 感知。
+ * @param inPawn Controller 新接管的 Pawn；期望为 ALRNPCCharacter。
+ */
+void ALRNPCController::OnPossess(APawn* inPawn)
+{
+	Super::OnPossess(inPawn);
+	Npc = Cast<ALRNPCCharacter>(inPawn);
+	Definition = Npc.IsValid() ? Npc->GetDefinition() : nullptr;
+	if (Definition.IsValid() && Definition->Behavior)
+	{
+		StateTreeAI->SetStateTree(Definition->Behavior);
+		if (!StateTreeAI->IsRunning())
+		{
+			StateTreeAI->StartLogic();
+		}
+	}
+	else
+	{
+		UE_LOG(LogLostRunicAI, Warning, TEXT("NPC=%s definition or Behavior StateTree is missing; using controller fallback."),
+			*GetNameSafe(inPawn));
+	}
+}
+
+/**
+ * @brief 处理 On Un Possess 事件：解绑感知并停止 StateTree 逻辑。
+ */
+void ALRNPCController::OnUnPossess()
+{
+	if (GetWorld())
+	{
+		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
+		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
+	}
+	if (StateTreeAI->IsRunning())
+	{
+		StateTreeAI->StopLogic(TEXT("OnUnPossess"));
+	}
+	Npc.Reset();
+	Definition.Reset();
+	Super::OnUnPossess();
+}
+
+/**
+ * @brief 处理 On Move Completed 事件：巡逻点到达续走下一段。
+ * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
+ * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
+ */
+void ALRNPCController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
+{
+	Super::OnMoveCompleted(requestId, result);
+	if (!result.IsSuccess())
+	{
+		return;
+	}
+	if (ActiveBehavior == ELRNPCBehaviorState::Patrol)
+	{
+		++PatrolIndex;
+		StartPatrolMove();
+	}
+}
+
+/**
+ * @brief 进入指定 NPC 行为：Idle 启动玩家朝向检测、Patrol 巡逻、ReactToNoise 转向声源限时反应、Conversation 停止一切反应。
+ * @param behavior 要进入或退出的 NPC StateTree 行为状态。
+ */
+void ALRNPCController::EnterBehavior(const ELRNPCBehaviorState behavior)
+{
+	ActiveBehavior = behavior;
+	ALRNPCCharacter* npc = Npc.Get();
+	if (!npc)
+	{
+		return;
+	}
+	switch (behavior)
+	{
+	case ELRNPCBehaviorState::Idle:
+		StopMovement();
+		ClearFocus(EAIFocusPriority::Gameplay);
+		StartLookAtTimer();
+		break;
+	case ELRNPCBehaviorState::Patrol:
+		StopLookAtTimer();
+		npc->GetCharacterMovement()->MaxWalkSpeed = GetEffectiveTuning().PatrolSpeedCm;
+		StartPatrolMove();
+		break;
+	case ELRNPCBehaviorState::ReactToNoise:
+		StopLookAtTimer();
+		StopMovement();
+		SetFocalPoint(LastNoiseLocation);
+		StartNoiseReaction(LastNoiseLocation);
+		break;
+	case ELRNPCBehaviorState::Conversation:
+		StopLookAtTimer();
+		StopNoiseReaction();
+		StopMovement();
+		ClearFocus(EAIFocusPriority::Gameplay);
+		break;
+	}
+}
+
+/**
+ * @brief 退出指定 NPC 行为并清理该状态拥有的导航、焦点或计时器。
+ * @param behavior 要进入或退出的 NPC StateTree 行为状态。
+ */
+void ALRNPCController::ExitBehavior(const ELRNPCBehaviorState behavior)
+{
+	StopLookAtTimer();
+	StopNoiseReaction();
+	StopMovement();
+	ClearFocus(EAIFocusPriority::Gameplay);
+}
+
+/**
+ * @brief 启动/停止 Idle 低频玩家朝向检测（任务节点调用）。
+ */
+void ALRNPCController::StartLookAtTimer()
+{
+	if (!GetWorld() || GetWorld()->GetTimerManager().IsTimerActive(LookAtTimer))
+	{
+		return;
+	}
+	GetWorld()->GetTimerManager().SetTimer(LookAtTimer, this, &ALRNPCController::HandleLookAtTimer,
+		GetEffectiveTuning().LookAtIntervalSeconds, true);
+}
+
+/**
+ * @brief 停止 Idle 低频玩家朝向检测（任务节点调用）。
+ */
+void ALRNPCController::StopLookAtTimer()
+{
+	if (GetWorld())
+	{
+		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
+	}
+}
+
+/**
+ * @brief 开始限时噪声反应：转向声源并按 NoiseReactionDurationSeconds 计时，结束后发送 NPCReactionEnded 事件。
+ * @param location 世界空间位置，Unreal 单位为厘米。
+ */
+void ALRNPCController::StartNoiseReaction(const FVector location)
+{
+	LastNoiseLocation = location;
+	if (!GetWorld())
+	{
+		return;
+	}
+	GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
+	GetWorld()->GetTimerManager().SetTimer(ReactionTimer, this, &ALRNPCController::HandleReactionTimeout,
+		GetEffectiveTuning().NoiseReactionDurationSeconds, false);
+}
+
+/**
+ * @brief 结束噪声反应计时（任务退出时调用）。
+ */
+void ALRNPCController::StopNoiseReaction()
+{
+	if (GetWorld())
+	{
+		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
+	}
+}
+
+/**
+ * @brief 对话开始：进入 Conversation（高优先级），停止一切反应。
+ */
+void ALRNPCController::NotifyDialogueStarted()
+{
+	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCDialogueStarted, ELRNPCBehaviorState::Conversation);
+}
+
+/**
+ * @brief 对话结束：回到配置的默认行为。
+ */
+void ALRNPCController::NotifyDialogueEnded()
+{
+	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCDialogueEnded, GetBaseBehavior());
+}
+
+/**
+ * @brief 把 UE 听觉刺激转换为噪声反应；Conversation 期间只触发表现钩子，不切换行为。
+ * @param actor 本次查询、交互或事件涉及的 Actor。
+ * @param stimulus 时间值 `stimulus`，单位为秒。
+ */
+void ALRNPCController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
+{
+	if (!actor || !Npc.IsValid() || stimulus.Type != UAISense::GetSenseID<UAISense_Hearing>()
+		|| !stimulus.WasSuccessfullySensed())
+	{
+		return;
+	}
+	FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
+	if (!reason.IsValid())
+	{
+		reason = LRGameplayTags::NoiseInteraction;
+	}
+	LastNoiseLocation = stimulus.StimulusLocation;
+	Npc->NotifyNoiseHeard(stimulus.StimulusLocation, reason);
+	// Conversation 为高优先级行为：普通噪声不打断对话（表现钩子已触发）。
+	if (ActiveBehavior == ELRNPCBehaviorState::Conversation)
+	{
+		return;
+	}
+	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCNoiseHeard, ELRNPCBehaviorState::ReactToNoise);
+}
+
+/**
+ * @brief 处理 Handle Look At Timer 事件：距离、视线与可达性满足时朝向玩家。
+ */
+void ALRNPCController::HandleLookAtTimer()
+{
+	ALRNPCCharacter* npc = Npc.Get();
+	if (!npc || !GetWorld())
+	{
+		return;
+	}
+	const APawn* playerPawn = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
+	if (!playerPawn)
+	{
+		return;
+	}
+	const float distance = FVector::Dist2D(npc->GetActorLocation(), playerPawn->GetActorLocation());
+	if (distance <= GetEffectiveTuning().LookAtPlayerRadiusCm && LineOfSightTo(playerPawn))
+	{
+		const FVector toPlayer = playerPawn->GetActorLocation() - npc->GetActorLocation();
+		npc->SetActorRotation(FRotator(0.0f, toPlayer.Rotation().Yaw, 0.0f));
+	}
+}
+
+/**
+ * @brief 处理 Handle Reaction Timeout 事件：噪声反应结束，发送 NPCReactionEnded。
+ */
+void ALRNPCController::HandleReactionTimeout()
+{
+	if (ActiveBehavior != ELRNPCBehaviorState::ReactToNoise)
+	{
+		return;
+	}
+	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCReactionEnded, GetBaseBehavior());
+}
+
+/**
+ * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
+ */
+void ALRNPCController::StartPatrolMove()
+{
+	ALRNPCCharacter* npc = Npc.Get();
+	if (!npc || npc->GetPatrolPointCount() == 0)
+	{
+		StopMovement();
+		return;
+	}
+	PatrolIndex %= npc->GetPatrolPointCount();
+	MoveToActor(npc->GetPatrolPoint(PatrolIndex), 50.0f);
+}
+
+/**
+ * @brief 解析配置的默认行为到运行态枚举。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRNPCBehaviorState ALRNPCController::GetBaseBehavior() const
+{
+	return Definition.IsValid() && Definition->DefaultBehavior == ENPCBaseBehavior::Patrol
+		? ELRNPCBehaviorState::Patrol : ELRNPCBehaviorState::Idle;
+}
+
+/**
+ * @brief 向 StateTree 发送行为事件；树未运行时直接进入行为。
+ * @param event 本次领域操作的结构化数据 `event`；字段语义由对应 USTRUCT 定义。
+ * @param behavior 要进入或退出的 NPC StateTree 行为状态。
+ */
+void ALRNPCController::DispatchBehaviorEvent(const FGameplayTag event, const ELRNPCBehaviorState behavior)
+{
+	if (StateTreeAI->IsRunning())
+	{
+		StateTreeAI->SendStateTreeEvent(event, FConstStructView(), FName());
+	}
+	else
+	{
+		EnterBehavior(behavior);
+	}
+}
+
+/**
+ * @brief 查询 Effective Tuning；不修改领域状态。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+const ULRNPCTuning& ALRNPCController::GetEffectiveTuning() const
+{
+	return Tuning ? *Tuning : *GetDefault<ULRNPCTuning>();
+}
diff --git a/Source/LostRunic/AI/LRNPCController.h b/Source/LostRunic/AI/LRNPCController.h
new file mode 100644
index 0000000..b970b3a
--- /dev/null
+++ b/Source/LostRunic/AI/LRNPCController.h
@@ -0,0 +1,182 @@
+/**
+ * @file LRNPCController.h
+ * @brief 通用 NPC 控制器：Hearing 感知驱动噪声反应（Conversation 高优先级不被打断）、StateTree 生命周期（OnPossess 解析定义后启动）、巡逻与低频玩家朝向检测；不实现第二套计时器状态机。
+ *
+ * 关联文件：LRNPCController.cpp；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "AI/LRNPCTypes.h"
+#include "AIController.h"
+#include "GameplayTagContainer.h"
+#include "Perception/AIPerceptionTypes.h"
+
+#include "LRNPCController.generated.h"
+
+class ALRNPCCharacter;
+class UAIPerceptionComponent;
+class UAISenseConfig_Hearing;
+class ULRNPCDefinition;
+class ULRNPCTuning;
+class UStateTreeAIComponent;
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC AI Controller"))
+class LOSTRUNIC_API ALRNPCController : public AAIController
+{
+	GENERATED_BODY()
+
+public:
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 */
+	ALRNPCController();
+
+	/**
+	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
+	 */
+	virtual void BeginPlay() override;
+	/**
+	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
+	 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
+	 */
+	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
+	/**
+	 * @brief 处理 On Possess 事件：解析定义、SetStateTree 后 StartLogic，并绑定 Hearing 感知。
+	 * @param inPawn Controller 新接管的 Pawn；期望为 ALRNPCCharacter。
+	 */
+	virtual void OnPossess(APawn* inPawn) override;
+	/**
+	 * @brief 处理 On Un Possess 事件：解绑感知并停止 StateTree 逻辑。
+	 */
+	virtual void OnUnPossess() override;
+	/**
+	 * @brief 处理 On Move Completed 事件：巡逻点到达续走下一段。
+	 * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
+	 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
+	 */
+	virtual void OnMoveCompleted(FAIRequestID requestId, const FPathFollowingResult& result) override;
+
+	/**
+	 * @brief 进入指定 NPC 行为：Idle 启动玩家朝向检测、Patrol 巡逻、ReactToNoise 转向声源限时反应、Conversation 停止一切反应。
+	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
+	 */
+	void EnterBehavior(ELRNPCBehaviorState behavior);
+	/**
+	 * @brief 退出指定 NPC 行为并清理该状态拥有的导航、焦点或计时器。
+	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
+	 */
+	void ExitBehavior(ELRNPCBehaviorState behavior);
+	/**
+	 * @brief 查询 Active Behavior；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	ELRNPCBehaviorState GetActiveBehavior() const { return ActiveBehavior; }
+
+	/**
+	 * @brief 启动/停止 Idle 低频玩家朝向检测（任务节点调用）。
+	 */
+	void StartLookAtTimer();
+	/**
+	 * @brief 停止 Idle 低频玩家朝向检测（任务节点调用）。
+	 */
+	void StopLookAtTimer();
+	/**
+	 * @brief 开始限时噪声反应：转向声源并按 NoiseReactionDurationSeconds 计时，结束后发送 NPCReactionEnded 事件。
+	 * @param location 世界空间位置，Unreal 单位为厘米。
+	 */
+	void StartNoiseReaction(const FVector location);
+	/**
+	 * @brief 结束噪声反应计时（任务退出时调用）。
+	 */
+	void StopNoiseReaction();
+
+	/**
+	 * @brief 对话开始：进入 Conversation（高优先级），停止一切反应。
+	 */
+	void NotifyDialogueStarted();
+	/**
+	 * @brief 对话结束：回到配置的默认行为。
+	 */
+	void NotifyDialogueEnded();
+	/**
+	 * @brief 查询 Last Noise Location；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	FVector GetLastNoiseLocation() const { return LastNoiseLocation; }
+
+private:
+	/**
+	 * @brief 把 UE 听觉刺激转换为噪声反应；Conversation 期间只触发表现钩子，不切换行为。
+	 * @param actor 本次查询、交互或事件涉及的 Actor。
+	 * @param stimulus 时间值 `stimulus`，单位为秒。
+	 */
+	UFUNCTION()
+	void HandlePerception(AActor* actor, FAIStimulus stimulus);
+
+	/**
+	 * @brief 处理 Handle Look At Timer 事件：距离、视线与可达性满足时朝向玩家。
+	 */
+	void HandleLookAtTimer();
+	/**
+	 * @brief 处理 Handle Reaction Timeout 事件：噪声反应结束，发送 NPCReactionEnded。
+	 */
+	void HandleReactionTimeout();
+	/**
+	 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
+	 */
+	void StartPatrolMove();
+	/**
+	 * @brief 解析配置的默认行为到运行态枚举。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	ELRNPCBehaviorState GetBaseBehavior() const;
+	/**
+	 * @brief 向 StateTree 发送行为事件；树未运行时直接进入行为。
+	 * @param event 本次领域操作的结构化数据 `event`；字段语义由对应 USTRUCT 定义。
+	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
+	 */
+	void DispatchBehaviorEvent(const FGameplayTag event, ELRNPCBehaviorState behavior);
+	/**
+	 * @brief 查询 Effective Tuning；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	const ULRNPCTuning& GetEffectiveTuning() const;
+
+	/** State Tree AI 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
+	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
+	TObjectPtr<UStateTreeAIComponent> StateTreeAI;
+
+	/** AIPerception 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
+	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
+	TObjectPtr<UAIPerceptionComponent> AIPerception;
+
+	/** Hearing Config 的领域数据，由所属类型负责维护和校验。  */
+	UPROPERTY()
+	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
+
+	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TObjectPtr<ULRNPCTuning> Tuning;
+
+	/** Definition 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TWeakObjectPtr<ULRNPCDefinition> Definition;
+
+	/** Npc 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TWeakObjectPtr<ALRNPCCharacter> Npc;
+
+	/** Active Behavior 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	ELRNPCBehaviorState ActiveBehavior = ELRNPCBehaviorState::Idle;
+	/** Patrol Index 的内部运行时数据；不参与蓝图配置。 */
+	int32 PatrolIndex = 0;
+	/** Last Noise Location 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	FVector LastNoiseLocation = FVector::ZeroVector;
+	/** Look At Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
+	FTimerHandle LookAtTimer;
+	/** Reaction Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
+	FTimerHandle ReactionTimer;
+};
diff --git a/Source/LostRunic/AI/LRNPCStateTreeNodes.cpp b/Source/LostRunic/AI/LRNPCStateTreeNodes.cpp
new file mode 100644
index 0000000..cc5f4a8
--- /dev/null
+++ b/Source/LostRunic/AI/LRNPCStateTreeNodes.cpp
@@ -0,0 +1,146 @@
+/**
+ * @file LRNPCStateTreeNodes.cpp
+ * @brief 通用 NPC 的 StateTree 节点实现：行为任务/条件、玩家朝向任务与限时噪声反应任务。
+ *
+ * 关联文件：LRNPCStateTreeNodes.h；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "AI/LRNPCStateTreeNodes.h"
+
+#include "AI/LRNPCController.h"
+#include "StateTreeExecutionContext.h"
+
+/**
+ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ */
+FLRNPCBehaviorTask::FLRNPCBehaviorTask()
+{
+	bShouldCallTick = false;
+}
+
+/**
+ * @brief StateTree 进入节点时让 NPC 控制器进入配置的行为状态。
+ * @param context 当前 StateTree 执行上下文，用于读取实例数据。
+ * @param transition 触发本次进入的状态转换结果。
+ * @return 返回 Running，使行为持续到 StateTree 条件触发下一次转换。
+ */
+EStateTreeRunStatus FLRNPCBehaviorTask::EnterState(FStateTreeExecutionContext& context,
+	const FStateTreeTransitionResult& transition) const
+{
+	const FInstanceDataType& data = context.GetInstanceData(*this);
+	if (!data.AIController)
+	{
+		return EStateTreeRunStatus::Failed;
+	}
+	data.AIController->EnterBehavior(Behavior);
+	return EStateTreeRunStatus::Running;
+}
+
+/**
+ * @brief StateTree 离开节点时通知 NPC 控制器清理行为拥有的导航、焦点和计时器。
+ * @param context 当前 StateTree 执行上下文。
+ * @param transition 触发本次退出的状态转换结果。
+ */
+void FLRNPCBehaviorTask::ExitState(FStateTreeExecutionContext& context,
+	const FStateTreeTransitionResult& transition) const
+{
+	const FInstanceDataType& data = context.GetInstanceData(*this);
+	if (data.AIController)
+	{
+		data.AIController->ExitBehavior(Behavior);
+	}
+}
+
+/**
+ * @brief 比较当前 NPC 行为与 StateTree 条件配置，决定该分支是否可进入；只执行控制器解析结果。
+ * @param context 用于本次条件匹配的 `context` 标签或上下文。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+bool FLRNPCStateCondition::TestCondition(FStateTreeExecutionContext& context) const
+{
+	const FInstanceDataType& data = context.GetInstanceData(*this);
+	return data.AIController && data.AIController->GetActiveBehavior() == ExpectedBehavior;
+}
+
+/**
+ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ */
+FLRNPCLookAtPlayerTask::FLRNPCLookAtPlayerTask()
+{
+	bShouldCallTick = false;
+}
+
+/**
+ * @brief 进入 Idle 时启动低频玩家朝向检测计时器。
+ * @param context 当前 StateTree 执行上下文。
+ * @param transition 触发本次进入的状态转换结果。
+ * @return 返回 Running，使检测持续到离开 Idle。
+ */
+EStateTreeRunStatus FLRNPCLookAtPlayerTask::EnterState(FStateTreeExecutionContext& context,
+	const FStateTreeTransitionResult& transition) const
+{
+	const FInstanceDataType& data = context.GetInstanceData(*this);
+	if (!data.AIController)
+	{
+		return EStateTreeRunStatus::Failed;
+	}
+	data.AIController->StartLookAtTimer();
+	return EStateTreeRunStatus::Running;
+}
+
+/**
+ * @brief 离开 Idle 时停止朝向检测计时器。
+ * @param context 当前 StateTree 执行上下文。
+ * @param transition 触发本次退出的状态转换结果。
+ */
+void FLRNPCLookAtPlayerTask::ExitState(FStateTreeExecutionContext& context,
+	const FStateTreeTransitionResult& transition) const
+{
+	const FInstanceDataType& data = context.GetInstanceData(*this);
+	if (data.AIController)
+	{
+		data.AIController->StopLookAtTimer();
+	}
+}
+
+/**
+ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ */
+FLRNPCReactToNoiseTask::FLRNPCReactToNoiseTask()
+{
+	bShouldCallTick = false;
+}
+
+/**
+ * @brief 进入 ReactToNoise 时启动限时反应（转向声源，到时发送 NPCReactionEnded）。
+ * @param context 当前 StateTree 执行上下文。
+ * @param transition 触发本次进入的状态转换结果。
+ * @return 返回 Running，使反应持续到超时事件。
+ */
+EStateTreeRunStatus FLRNPCReactToNoiseTask::EnterState(FStateTreeExecutionContext& context,
+	const FStateTreeTransitionResult& transition) const
+{
+	const FInstanceDataType& data = context.GetInstanceData(*this);
+	if (!data.AIController)
+	{
+		return EStateTreeRunStatus::Failed;
+	}
+	data.AIController->StartNoiseReaction(data.AIController->GetLastNoiseLocation());
+	return EStateTreeRunStatus::Running;
+}
+
+/**
+ * @brief 离开 ReactToNoise 时停止反应计时器。
+ * @param context 当前 StateTree 执行上下文。
+ * @param transition 触发本次退出的状态转换结果。
+ */
+void FLRNPCReactToNoiseTask::ExitState(FStateTreeExecutionContext& context,
+	const FStateTreeTransitionResult& transition) const
+{
+	const FInstanceDataType& data = context.GetInstanceData(*this);
+	if (data.AIController)
+	{
+		data.AIController->StopNoiseReaction();
+	}
+}
diff --git a/Source/LostRunic/AI/LRNPCStateTreeNodes.h b/Source/LostRunic/AI/LRNPCStateTreeNodes.h
new file mode 100644
index 0000000..45127c7
--- /dev/null
+++ b/Source/LostRunic/AI/LRNPCStateTreeNodes.h
@@ -0,0 +1,157 @@
+/**
+ * @file LRNPCStateTreeNodes.h
+ * @brief 通用 NPC 的 StateTree 节点：行为任务/条件（执行控制器解析结果）、Idle 玩家朝向任务与限时噪声反应任务；计时器由控制器持有，无 Tick。
+ *
+ * 关联文件：LRNPCStateTreeNodes.cpp；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "AI/LRNPCTypes.h"
+#include "StateTreeConditionBase.h"
+#include "StateTreeTaskBase.h"
+
+#include "LRNPCStateTreeNodes.generated.h"
+
+class ALRNPCController;
+
+USTRUCT()
+struct FLRNPCControllerInstanceData
+{
+	GENERATED_BODY()
+
+	/** AIController 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
+	UPROPERTY(EditAnywhere, Category = "Context")
+	TObjectPtr<ALRNPCController> AIController;
+};
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+USTRUCT(meta = (DisplayName = "Run NPC Behavior", Category = "Lost Runic|AI"))
+struct LOSTRUNIC_API FLRNPCBehaviorTask : public FStateTreeTaskCommonBase
+{
+	GENERATED_BODY()
+
+	using FInstanceDataType = FLRNPCControllerInstanceData;
+	/**
+	 * @brief 查询 Instance Data Type；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
+
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 */
+	FLRNPCBehaviorTask();
+	/**
+	 * @brief StateTree 进入节点时让 NPC 控制器进入配置的行为状态。
+	 * @param context 当前 StateTree 执行上下文，用于读取实例数据。
+	 * @param transition 触发本次进入的状态转换结果。
+	 * @return 返回 Running，使行为持续到 StateTree 条件触发下一次转换。
+	 */
+	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
+		const FStateTreeTransitionResult& transition) const override;
+	/**
+	 * @brief StateTree 离开节点时通知 NPC 控制器清理行为拥有的导航、焦点和计时器。
+	 * @param context 当前 StateTree 执行上下文。
+	 * @param transition 触发本次退出的状态转换结果。
+	 */
+	virtual void ExitState(FStateTreeExecutionContext& context,
+		const FStateTreeTransitionResult& transition) const override;
+
+	/** Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNPCBehaviorState::Idle`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
+	UPROPERTY(EditAnywhere, Category = "Behavior")
+	ELRNPCBehaviorState Behavior = ELRNPCBehaviorState::Idle;
+};
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+USTRUCT(meta = (DisplayName = "NPC Behavior Is", Category = "Lost Runic|AI"))
+struct LOSTRUNIC_API FLRNPCStateCondition : public FStateTreeConditionBase
+{
+	GENERATED_BODY()
+
+	using FInstanceDataType = FLRNPCControllerInstanceData;
+	/**
+	 * @brief 查询 Instance Data Type；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
+	/**
+	 * @brief 比较当前 NPC 行为与 StateTree 条件配置，决定该分支是否可进入；只执行控制器解析结果。
+	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
+
+	/** Expected Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNPCBehaviorState::Idle`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
+	UPROPERTY(EditAnywhere, Category = "Behavior")
+	ELRNPCBehaviorState ExpectedBehavior = ELRNPCBehaviorState::Idle;
+};
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+USTRUCT(meta = (DisplayName = "NPC Look At Player", Category = "Lost Runic|AI"))
+struct LOSTRUNIC_API FLRNPCLookAtPlayerTask : public FStateTreeTaskCommonBase
+{
+	GENERATED_BODY()
+
+	using FInstanceDataType = FLRNPCControllerInstanceData;
+	/**
+	 * @brief 查询 Instance Data Type；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
+
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 */
+	FLRNPCLookAtPlayerTask();
+	/**
+	 * @brief 进入 Idle 时启动低频玩家朝向检测计时器。
+	 * @param context 当前 StateTree 执行上下文。
+	 * @param transition 触发本次进入的状态转换结果。
+	 * @return 返回 Running，使检测持续到离开 Idle。
+	 */
+	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
+		const FStateTreeTransitionResult& transition) const override;
+	/**
+	 * @brief 离开 Idle 时停止朝向检测计时器。
+	 * @param context 当前 StateTree 执行上下文。
+	 * @param transition 触发本次退出的状态转换结果。
+	 */
+	virtual void ExitState(FStateTreeExecutionContext& context,
+		const FStateTreeTransitionResult& transition) const override;
+};
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+USTRUCT(meta = (DisplayName = "NPC React To Noise", Category = "Lost Runic|AI"))
+struct LOSTRUNIC_API FLRNPCReactToNoiseTask : public FStateTreeTaskCommonBase
+{
+	GENERATED_BODY()
+
+	using FInstanceDataType = FLRNPCControllerInstanceData;
+	/**
+	 * @brief 查询 Instance Data Type；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
+
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 */
+	FLRNPCReactToNoiseTask();
+	/**
+	 * @brief 进入 ReactToNoise 时启动限时反应（转向声源，到时发送 NPCReactionEnded）。
+	 * @param context 当前 StateTree 执行上下文。
+	 * @param transition 触发本次进入的状态转换结果。
+	 * @return 返回 Running，使反应持续到超时事件。
+	 */
+	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
+		const FStateTreeTransitionResult& transition) const override;
+	/**
+	 * @brief 离开 ReactToNoise 时停止反应计时器。
+	 * @param context 当前 StateTree 执行上下文。
+	 * @param transition 触发本次退出的状态转换结果。
+	 */
+	virtual void ExitState(FStateTreeExecutionContext& context,
+		const FStateTreeTransitionResult& transition) const override;
+};
diff --git a/Source/LostRunic/AI/LRNPCTypes.h b/Source/LostRunic/AI/LRNPCTypes.h
new file mode 100644
index 0000000..be181c7
--- /dev/null
+++ b/Source/LostRunic/AI/LRNPCTypes.h
@@ -0,0 +1,31 @@
+/**
+ * @file LRNPCTypes.h
+ * @brief 声明通用 NPC 的行为状态与定义配置枚举，供 NPC 角色、控制器、StateTree 节点与内容定义共享。
+ *
+ * 关联文件：AI 目录内调用该公共契约的实现文件；所属领域：AI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "CoreMinimal.h"
+
+#include "LRNPCTypes.generated.h"
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UENUM(BlueprintType, meta = (DisplayName = "Lost Runic NPC Behavior"))
+enum class ELRNPCBehaviorState : uint8
+{
+	Idle UMETA(DisplayName = "Idle"),
+	Patrol UMETA(DisplayName = "Patrol"),
+	ReactToNoise UMETA(DisplayName = "React To Noise"),
+	Conversation UMETA(DisplayName = "Conversation")
+};
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UENUM(BlueprintType, meta = (DisplayName = "Lost Runic NPC Base Behavior"))
+enum class ENPCBaseBehavior : uint8
+{
+	Idle UMETA(DisplayName = "Idle"),
+	Patrol UMETA(DisplayName = "Patrol")
+};
diff --git a/Source/LostRunic/Core/LRGameplayTags.cpp b/Source/LostRunic/Core/LRGameplayTags.cpp
index f3a117d..91dae57 100644
--- a/Source/LostRunic/Core/LRGameplayTags.cpp
+++ b/Source/LostRunic/Core/LRGameplayTags.cpp
@@ -25,6 +25,8 @@ namespace LRGameplayTags
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectConcurrentInput, "State.Reject.ConcurrentInput", "Another eye input owns the current press.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectPresentationLocked, "State.Reject.PresentationLocked", "Presentation has not completed.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectAlreadyCurrent, "State.Reject.AlreadyCurrent", "The target mode is already active.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MovementRejectPaceForbidden, "Movement.Reject.PaceForbidden", "The requested pace is forbidden by the current state.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MovementOverrideHidden, "Movement.Override.Hidden", "Pace is temporarily forced to Sneak while hiding.");
 
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionInteract, "Interaction.Action.Interact", "Generic interaction.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionPickup, "Interaction.Action.Pickup", "Pickup interaction.");
@@ -57,12 +59,20 @@ namespace LRGameplayTags
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepWalk, "Noise.Footstep.Walk", "Walking footstep stimulus.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepRun, "Noise.Footstep.Run", "Running footstep stimulus.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseInteraction, "Noise.Interaction", "Interaction-created noise stimulus.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepSneak, "Noise.Footstep.Sneak", "Silent sneak footstep; presentation/animation hook only, never emitted as hearing.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepWalkFaint, "Noise.Footstep.Walk.Faint", "Outdoor open-area walk; only guards with alert at least 6 respond.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepRunIndoor, "Noise.Footstep.Run.Indoor", "Indoor run; propagated through room volumes with alert floor semantics.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SightPlayer, "Sight.Player", "A guard saw the player.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SightPlayerLost, "Sight.Player.Lost", "A guard lost confirmed sight of the player.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchReached, "Search.Reached", "A guard reached the latest disturbance location.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchAlertDecay, "Search.AlertDecay", "Alert decayed after its observation delay.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchTimeout, "Search.Timeout", "A guard search timed out.");
-	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventAlertChanged, "AI.Event.AlertChanged", "Alert state changed and StateTree should reselect.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventAlertChanged, "AI.Event.AlertChanged", "Alert or perception data changed; data-level event, does not drive StateTree selection.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventBehaviorChanged, "AI.Event.BehaviorChanged", "The resolved guard behavior changed; StateTree should reselect.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCNoiseHeard, "AI.Event.NPCNoiseHeard", "An NPC heard a noise and StateTree should react.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCDialogueStarted, "AI.Event.NPCDialogueStarted", "An NPC conversation started.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCDialogueEnded, "AI.Event.NPCDialogueEnded", "An NPC conversation ended.");
+	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventNPCReactionEnded, "AI.Event.NPCReactionEnded", "An NPC noise reaction timed out; return to base behavior.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NarrativeEventCompleted, "Narrative.Event.Completed", "A stable narrative event completed.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NarrativeRejectNoSession, "Narrative.Reject.NoSession", "No narrative session can receive the request.");
 	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NarrativeRejectMissingContent, "Narrative.Reject.MissingContent", "The requested stable content ID is not registered.");
diff --git a/Source/LostRunic/Core/LRGameplayTags.h b/Source/LostRunic/Core/LRGameplayTags.h
index 92dd09f..3221d22 100644
--- a/Source/LostRunic/Core/LRGameplayTags.h
+++ b/Source/LostRunic/Core/LRGameplayTags.h
@@ -87,6 +87,16 @@ namespace LRGameplayTags
 	 * @param StateRejectAlreadyCurrent 调用方提供的 `StateRejectAlreadyCurrent`，只在本次操作范围内使用。
 	 */
 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateRejectAlreadyCurrent);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param MovementRejectPaceForbidden 调用方提供的 `MovementRejectPaceForbidden`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(MovementRejectPaceForbidden);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param MovementOverrideHidden 调用方提供的 `MovementOverrideHidden`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(MovementOverrideHidden);
 
 	/**
 	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
@@ -239,6 +249,21 @@ namespace LRGameplayTags
 	 * @param NoiseInteraction 输入动作或数值 `NoiseInteraction`；不包含写死的具体键位。
 	 */
 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseInteraction);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param NoiseFootstepSneak 调用方提供的 `NoiseFootstepSneak`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepSneak);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param NoiseFootstepWalkFaint 调用方提供的 `NoiseFootstepWalkFaint`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepWalkFaint);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param NoiseFootstepRunIndoor 调用方提供的 `NoiseFootstepRunIndoor`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepRunIndoor);
 	/**
 	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 	 * @param SightPlayer 调用方提供的 `SightPlayer`，只在本次操作范围内使用。
@@ -269,6 +294,31 @@ namespace LRGameplayTags
 	 * @param AIEventAlertChanged 调用方提供的 `AIEventAlertChanged`，只在本次操作范围内使用。
 	 */
 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventAlertChanged);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param AIEventBehaviorChanged 调用方提供的 `AIEventBehaviorChanged`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventBehaviorChanged);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param AIEventNPCNoiseHeard 调用方提供的 `AIEventNPCNoiseHeard`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCNoiseHeard);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param AIEventNPCDialogueStarted 调用方提供的 `AIEventNPCDialogueStarted`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCDialogueStarted);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param AIEventNPCDialogueEnded 调用方提供的 `AIEventNPCDialogueEnded`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCDialogueEnded);
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 * @param AIEventNPCReactionEnded 调用方提供的 `AIEventNPCReactionEnded`，只在本次操作范围内使用。
+	 */
+	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventNPCReactionEnded);
 	/**
 	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 	 * @param NarrativeEventCompleted 调用方提供的 `NarrativeEventCompleted`，只在本次操作范围内使用。
diff --git a/Source/LostRunic/Core/LRTypes.h b/Source/LostRunic/Core/LRTypes.h
index f204d10..009ffdf 100644
--- a/Source/LostRunic/Core/LRTypes.h
+++ b/Source/LostRunic/Core/LRTypes.h
@@ -46,7 +46,8 @@ UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Noise Environment"))
 enum class ELRNoiseEnvironment : uint8
 {
 	Indoor UMETA(DisplayName = "Indoor"),
-	Outdoor UMETA(DisplayName = "Outdoor")
+	Outdoor UMETA(DisplayName = "Outdoor"),
+	OutdoorStealth UMETA(DisplayName = "Outdoor Stealth")
 };
 
 /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
diff --git a/Source/LostRunic/Data/LRGameTuningSet.cpp b/Source/LostRunic/Data/LRGameTuningSet.cpp
index cf6f4e7..7ec612f 100644
--- a/Source/LostRunic/Data/LRGameTuningSet.cpp
+++ b/Source/LostRunic/Data/LRGameTuningSet.cpp
@@ -12,6 +12,7 @@
 #include "Data/LRGuardTuning.h"
 #include "Data/LRInteractionTuning.h"
 #include "Data/LRMovementTuning.h"
+#include "Data/LRNPCTuning.h"
 #include "Data/LRPresentationTuning.h"
 #include "Data/LRSaveTuning.h"
 #include "Data/LRStateTuning.h"
@@ -62,7 +63,8 @@ bool ULRGameTuningSet::Validate(FString& outError) const
 		&& ValidateEntry(TEXT("Guard"), Guard.Get(), outError)
 		&& ValidateEntry(TEXT("Save"), Save.Get(), outError)
 		&& ValidateEntry(TEXT("UI"), UI.Get(), outError)
-		&& ValidateEntry(TEXT("Presentation"), Presentation.Get(), outError);
+		&& ValidateEntry(TEXT("Presentation"), Presentation.Get(), outError)
+		&& ValidateEntry(TEXT("NPC"), NPC.Get(), outError);
 }
 
 /**
@@ -70,9 +72,9 @@ bool ULRGameTuningSet::Validate(FString& outError) const
  */
 void ULRGameTuningSet::LogSources() const
 {
-	UE_LOG(LogLostRunicTuning, Display, TEXT("TuningSet=%s State=%s Movement=%s Interaction=%s Guard=%s Save=%s UI=%s Presentation=%s"),
+	UE_LOG(LogLostRunicTuning, Display, TEXT("TuningSet=%s State=%s Movement=%s Interaction=%s Guard=%s Save=%s UI=%s Presentation=%s NPC=%s"),
 		*GetPathName(), *GetNameSafe(State), *GetNameSafe(Movement), *GetNameSafe(Interaction), *GetNameSafe(Guard),
-		*GetNameSafe(Save), *GetNameSafe(UI), *GetNameSafe(Presentation));
+		*GetNameSafe(Save), *GetNameSafe(UI), *GetNameSafe(Presentation), *GetNameSafe(NPC));
 }
 
 #if WITH_EDITOR
diff --git a/Source/LostRunic/Data/LRGameTuningSet.h b/Source/LostRunic/Data/LRGameTuningSet.h
index f6d4e04..2c249d8 100644
--- a/Source/LostRunic/Data/LRGameTuningSet.h
+++ b/Source/LostRunic/Data/LRGameTuningSet.h
@@ -16,6 +16,7 @@
 class ULRGuardTuning;
 class ULRInteractionTuning;
 class ULRMovementTuning;
+class ULRNPCTuning;
 class ULRPresentationTuning;
 class ULRSaveTuning;
 class ULRStateTuning;
@@ -56,6 +57,10 @@ public:
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
 	TObjectPtr<ULRPresentationTuning> Presentation;
 
+	/** NPC 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
+	TObjectPtr<ULRNPCTuning> NPC;
+
 	/**
 	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 	 * @param outError 输出校验失败原因；成功时保持为空。
diff --git a/Source/LostRunic/Data/LRGuardDefinition.h b/Source/LostRunic/Data/LRGuardDefinition.h
index 357f320..106e5c4 100644
--- a/Source/LostRunic/Data/LRGuardDefinition.h
+++ b/Source/LostRunic/Data/LRGuardDefinition.h
@@ -31,9 +31,9 @@ public:
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
 	TObjectPtr<ULRGuardTuning> Tuning;
 
-	/** Behavior 的开关；true 表示启用，false 表示禁用。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+	/** Behavior 的 StateTree 硬引用；随定义资产同步加载，OnPossess 时确定启动顺序。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
-	TSoftObjectPtr<UStateTree> Behavior;
+	TObjectPtr<UStateTree> Behavior;
 
 	/**
 	 * @brief 查询 Primary Asset Id；不修改领域状态。
diff --git a/Source/LostRunic/Data/LRGuardTuning.cpp b/Source/LostRunic/Data/LRGuardTuning.cpp
index d7c0090..0846296 100644
--- a/Source/LostRunic/Data/LRGuardTuning.cpp
+++ b/Source/LostRunic/Data/LRGuardTuning.cpp
@@ -34,12 +34,16 @@ bool ULRGuardTuning::Validate(FString& outError) const
 		&& LRValidation::RequireRange(TEXT("SightConeDegrees"), SightConeDegrees, 1.0f, 180.0f, outError)
 		&& LRValidation::RequireRange(TEXT("HearingRangeMultiplier"), HearingRangeMultiplier, 0.0f, 10.0f, outError)
 		&& LRValidation::RequireRange(TEXT("MaxHearingRange"), MaxHearingRange, 50.0f, 10000.0f, outError)
-		&& LRValidation::RequireRange(TEXT("HearingAlertAmount"), HearingAlertAmount, 1, 11, outError)
-		&& LRValidation::RequireRange(TEXT("SightAlertLevel"), SightAlertLevel, 1, 11, outError)
+		&& LRValidation::RequireRange(TEXT("AttractAlertAmount"), AttractAlertAmount, 1, 11, outError)
+		&& LRValidation::RequireRange(TEXT("SightInvestigateLevel"), SightInvestigateLevel, 1, 11, outError)
+		&& LRValidation::RequireRange(TEXT("SightChaseLevel"), SightChaseLevel, 1, 11, outError)
+		&& LRValidation::RequireRange(TEXT("AlertIncreaseCooldownSeconds"), AlertIncreaseCooldownSeconds, 0.0f, 10.0f, outError)
+		&& LRValidation::RequireRange(TEXT("InvestigateIncreaseCooldownSeconds"), InvestigateIncreaseCooldownSeconds, 0.0f, 10.0f, outError)
+		&& LRValidation::RequireRange(TEXT("RoomRunAlertLevel"), RoomRunAlertLevel, 0, 11, outError)
+		&& LRValidation::RequireRange(TEXT("AdjacentRoomRunAlertAmount"), AdjacentRoomRunAlertAmount, 1, 11, outError)
 		&& LRValidation::RequireRange(TEXT("AlertDecayAmount"), AlertDecayAmount, 1, 11, outError)
 		&& LRValidation::RequireRange(TEXT("InitialObserveSeconds"), InitialObserveSeconds, 0.1f, 30.0f, outError)
 		&& LRValidation::RequireRange(TEXT("AlertDecayIntervalSeconds"), AlertDecayIntervalSeconds, 0.05f, 10.0f, outError)
-		&& LRValidation::RequireRange(TEXT("SearchDurationSeconds"), SearchDurationSeconds, 0.1f, 60.0f, outError)
 		&& LRValidation::RequireRange(TEXT("CaptureRadius"), CaptureRadius, 10.0f, 500.0f, outError)
 		&& LRValidation::RequireRange(TEXT("CaptureCheckIntervalSeconds"), CaptureCheckIntervalSeconds, 0.02f, 1.0f, outError)
 		&& LRValidation::RequireRange(TEXT("MoveAcceptanceRadius"), MoveAcceptanceRadius, 1.0f, 500.0f, outError)
diff --git a/Source/LostRunic/Data/LRGuardTuning.h b/Source/LostRunic/Data/LRGuardTuning.h
index e20932d..1654ab2 100644
--- a/Source/LostRunic/Data/LRGuardTuning.h
+++ b/Source/LostRunic/Data/LRGuardTuning.h
@@ -39,13 +39,33 @@ public:
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Hearing", meta = (ClampMin = "50.0", ClampMax = "10000.0", Units = "cm"))
 	float MaxHearingRange = 5000.0f;
 
-	/** 有效听觉刺激首次增加的警戒量；默认 6。 C++ 安全默认值为 `6`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
+	/** 吸引注意噪声每次增加的警戒量；设计基线 1（0→1、档内 +1）。 C++ 安全默认值为 `1`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
-	int32 HearingAlertAmount = 6;
+	int32 AttractAlertAmount = 1;
 
-	/** 明确看见玩家后设置的警戒等级；默认 11，进入追逐。 C++ 安全默认值为 `11`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
+	/** 警戒低于 SightInvestigateLevel 时看见玩家设置的目标等级；默认 6，前往调查。 C++ 安全默认值为 `6`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
-	int32 SightAlertLevel = 11;
+	int32 SightInvestigateLevel = 6;
+
+	/** 警戒处于 6-10 档时看见玩家设置的目标等级；默认 11，进入追逐。 C++ 安全默认值为 `11`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
+	int32 SightChaseLevel = 11;
+
+	/** 1-5 档吸引注意增加的冷却时间；默认 0.5 秒；从 0 首次进入 6-10 档的首个增量也使用该值。 C++ 安全默认值为 `0.5f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `10.0`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
+	float AlertIncreaseCooldownSeconds = 0.5f;
+
+	/** 6-10 档前往/观察中吸引注意增加的冷却时间；默认 0.2 秒。 C++ 安全默认值为 `0.2f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `10.0`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
+	float InvestigateIncreaseCooldownSeconds = 0.2f;
+
+	/** 室内奔跑对当前房间守卫的警戒下限；默认 5。 C++ 安全默认值为 `5`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `0`，最大值 `11`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0", ClampMax = "11"))
+	int32 RoomRunAlertLevel = 5;
+
+	/** 室内奔跑对相邻房间守卫的警戒增量；默认 1。 C++ 安全默认值为 `1`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
+	int32 AdjacentRoomRunAlertAmount = 1;
 
 	/** 每个衰减周期降低的警戒值；默认 1。 C++ 安全默认值为 `1`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
@@ -67,8 +87,8 @@ public:
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.05", ClampMax = "10.0", Units = "s"))
 	float AlertDecayIntervalSeconds = 0.5f;
 
-	/** 到达最后异常位置后的搜索持续时间；默认 5 秒。 C++ 安全默认值为 `5.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `60.0`。 */
-	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.1", ClampMax = "60.0", Units = "s"))
+	/** 已废弃：旧固定搜索时长；搜索改为「抵达观察 3s → 自然衰减 → 归零清理」，保留字段仅用于资产序列化兼容，不再参与运行与校验。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert|Deprecated", meta = (DeprecatedProperty, ClampMin = "0.1", ClampMax = "60.0", Units = "s"))
 	float SearchDurationSeconds = 5.0f;
 
 	/** 追逐中判定捕获玩家的距离；默认 75 cm。 C++ 安全默认值为 `75.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `500.0`。 */
diff --git a/Source/LostRunic/Data/LRMovementTuning.cpp b/Source/LostRunic/Data/LRMovementTuning.cpp
index 81f8c33..00c5e17 100644
--- a/Source/LostRunic/Data/LRMovementTuning.cpp
+++ b/Source/LostRunic/Data/LRMovementTuning.cpp
@@ -35,7 +35,7 @@ bool ULRMovementTuning::Validate(FString& outError) const
 		&& LRValidation::RequireRange(TEXT("SampleIntervalSeconds"), SampleIntervalSeconds, 0.02f, 1.0f, outError)
 		&& LRValidation::RequireRange(TEXT("IndoorWalkNoiseRadius"), IndoorWalkNoiseRadius, 0.0f, 5000.0f, outError)
 		&& LRValidation::RequireRange(TEXT("IndoorRunNoiseRadius"), IndoorRunNoiseRadius, 0.0f, 5000.0f, outError)
-		&& LRValidation::RequireRange(TEXT("OutdoorSneakGuardNoiseRadius"), OutdoorSneakGuardNoiseRadius, 0.0f, 5000.0f, outError)
-		&& LRValidation::RequireRange(TEXT("OutdoorAlertGuardNoiseRadius"), OutdoorAlertGuardNoiseRadius, 0.0f, 5000.0f, outError)
+		&& LRValidation::RequireRange(TEXT("OutdoorStealthRunNoiseRadius"), OutdoorStealthRunNoiseRadius, 0.0f, 5000.0f, outError)
+		&& LRValidation::RequireRange(TEXT("OutdoorNoiseRadius"), OutdoorNoiseRadius, 0.0f, 5000.0f, outError)
 		&& LRValidation::RequireRange(TEXT("InteractionNoiseRadius"), InteractionNoiseRadius, 0.0f, 5000.0f, outError);
 }
diff --git a/Source/LostRunic/Data/LRMovementTuning.h b/Source/LostRunic/Data/LRMovementTuning.h
index 07b5395..303c38c 100644
--- a/Source/LostRunic/Data/LRMovementTuning.h
+++ b/Source/LostRunic/Data/LRMovementTuning.h
@@ -51,13 +51,13 @@ public:
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
 	float IndoorRunNoiseRadius = 1200.0f;
 
-	/** Outdoor Sneak Guard Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `600.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
+	/** Outdoor Stealth Run Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位；室外潜行关奔跑脚步噪声半径（设计 6m）。 C++ 安全默认值为 `600.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
-	float OutdoorSneakGuardNoiseRadius = 600.0f;
+	float OutdoorStealthRunNoiseRadius = 600.0f;
 
-	/** Outdoor Alert Guard Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `250.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
+	/** Outdoor Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位；室外潜行走路、室外非潜行走路与奔跑共用（设计 2.5m，走路的非潜行变体经 Faint 标签在守卫侧过滤）。 C++ 安全默认值为 `250.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
-	float OutdoorAlertGuardNoiseRadius = 250.0f;
+	float OutdoorNoiseRadius = 250.0f;
 
 	/** Interaction Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `500.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
diff --git a/Source/LostRunic/Data/LRNPCDefinition.cpp b/Source/LostRunic/Data/LRNPCDefinition.cpp
new file mode 100644
index 0000000..60074b1
--- /dev/null
+++ b/Source/LostRunic/Data/LRNPCDefinition.cpp
@@ -0,0 +1,40 @@
+/**
+ * @file LRNPCDefinition.cpp
+ * @brief 通用 NPC 内容定义 DataAsset 的主资产 ID 与编辑器数据校验。
+ *
+ * 关联文件：LRNPCDefinition.h；所属领域：Data。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "Data/LRNPCDefinition.h"
+
+#include "Misc/DataValidation.h"
+
+/**
+ * @brief 查询 Primary Asset Id；不修改领域状态。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+FPrimaryAssetId ULRNPCDefinition::GetPrimaryAssetId() const
+{
+	return FPrimaryAssetId(TEXT("LostRunicNPC"), NpcId);
+}
+
+#if WITH_EDITOR
+/**
+ * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
+ * @param context 用于本次条件匹配的 `context` 标签或上下文。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+EDataValidationResult ULRNPCDefinition::IsDataValid(FDataValidationContext& context) const
+{
+	if (NpcId.IsNone())
+	{
+		context.AddError(FText::FromString(TEXT("NpcId must not be empty.")));
+	}
+	if (!Behavior)
+	{
+		context.AddError(FText::FromString(TEXT("Behavior StateTree must be assigned.")));
+	}
+	return context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
+}
+#endif
diff --git a/Source/LostRunic/Data/LRNPCDefinition.h b/Source/LostRunic/Data/LRNPCDefinition.h
new file mode 100644
index 0000000..3e0ad94
--- /dev/null
+++ b/Source/LostRunic/Data/LRNPCDefinition.h
@@ -0,0 +1,56 @@
+/**
+ * @file LRNPCDefinition.h
+ * @brief 通用 NPC 的内容定义 DataAsset：StateTree 硬引用、默认行为与对话行 ID；公共调优在 ULRNPCTuning，巡逻点按实例配置。
+ *
+ * 关联文件：LRNPCDefinition.cpp；所属领域：Data。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "AI/LRNPCTypes.h"
+#include "CoreMinimal.h"
+#include "Engine/DataAsset.h"
+
+#include "LRNPCDefinition.generated.h"
+
+class UStateTree;
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Definition"))
+class LOSTRUNIC_API ULRNPCDefinition : public UPrimaryDataAsset
+{
+	GENERATED_BODY()
+
+public:
+	/** Npc Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
+	FName NpcId = NAME_None;
+
+	/** Behavior 的 StateTree 硬引用；随定义资产同步加载，OnPossess 时确定启动顺序。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
+	TObjectPtr<UStateTree> Behavior;
+
+	/** Dialogue Row Id 的稳定 DataTable 行 ID，用于 ULRDialogueSubsystem::StartDialogue。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
+	FName DialogueRowId = NAME_None;
+
+	/** Default Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ENPCBaseBehavior::Idle`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
+	ENPCBaseBehavior DefaultBehavior = ENPCBaseBehavior::Idle;
+
+	/**
+	 * @brief 查询 Primary Asset Id；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
+
+#if WITH_EDITOR
+	/**
+	 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
+	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
+#endif
+};
diff --git a/Source/LostRunic/Data/LRNPCTuning.cpp b/Source/LostRunic/Data/LRNPCTuning.cpp
new file mode 100644
index 0000000..5019e42
--- /dev/null
+++ b/Source/LostRunic/Data/LRNPCTuning.cpp
@@ -0,0 +1,24 @@
+/**
+ * @file LRNPCTuning.cpp
+ * @brief 通用 NPC 公共调优 DataAsset 的校验实现。
+ *
+ * 关联文件：LRNPCTuning.h；所属领域：Data。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "Data/LRNPCTuning.h"
+
+#include "Core/LRValidation.h"
+
+/**
+ * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
+ * @param outError 输出校验失败原因；成功时保持为空。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+bool ULRNPCTuning::Validate(FString& outError) const
+{
+	return LRValidation::RequireRange(TEXT("LookAtPlayerRadiusCm"), LookAtPlayerRadiusCm, 10.0f, 5000.0f, outError)
+		&& LRValidation::RequireRange(TEXT("LookAtIntervalSeconds"), LookAtIntervalSeconds, 0.05f, 2.0f, outError)
+		&& LRValidation::RequireRange(TEXT("NoiseReactionDurationSeconds"), NoiseReactionDurationSeconds, 0.1f, 30.0f, outError)
+		&& LRValidation::RequireRange(TEXT("PatrolSpeedCm"), PatrolSpeedCm, 1.0f, 1000.0f, outError);
+}
diff --git a/Source/LostRunic/Data/LRNPCTuning.h b/Source/LostRunic/Data/LRNPCTuning.h
new file mode 100644
index 0000000..dd25d99
--- /dev/null
+++ b/Source/LostRunic/Data/LRNPCTuning.h
@@ -0,0 +1,44 @@
+/**
+ * @file LRNPCTuning.h
+ * @brief 通用 NPC 的公共调优 DataAsset：玩家朝向检测、噪声反应与巡逻参数；逐 NPC 内容配置在 ULRNPCDefinition，巡逻点按实例配置。
+ *
+ * 关联文件：LRNPCTuning.cpp；所属领域：Data。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "Data/LRTuningAsset.h"
+
+#include "LRNPCTuning.generated.h"
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Tuning"))
+class LOSTRUNIC_API ULRNPCTuning : public ULRTuningAsset
+{
+	GENERATED_BODY()
+
+public:
+	/** NPC 在 Idle 状态检测并朝向玩家的半径；默认 300 cm。 C++ 安全默认值为 `300.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `5000.0`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|LookAt", meta = (ClampMin = "10.0", ClampMax = "5000.0", Units = "cm"))
+	float LookAtPlayerRadiusCm = 300.0f;
+
+	/** Idle 状态玩家朝向检测的低频间隔；默认 0.25 秒，以计时器代替 Tick。 C++ 安全默认值为 `0.25f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.05`，最大值 `2.0`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|LookAt", meta = (ClampMin = "0.05", ClampMax = "2.0", Units = "s"))
+	float LookAtIntervalSeconds = 0.25f;
+
+	/** NPC 听见噪声后的限时反应时长；结束后回到配置的默认行为。 C++ 安全默认值为 `3.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `30.0`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Noise", meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
+	float NoiseReactionDurationSeconds = 3.0f;
+
+	/** NPC 巡逻移动速度；默认 150 cm/s。 C++ 安全默认值为 `150.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
+	float PatrolSpeedCm = 150.0f;
+
+	/**
+	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
+	 * @param outError 输出校验失败原因；成功时保持为空。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	virtual bool Validate(FString& outError) const override;
+};
diff --git a/Source/LostRunic/Framework/LRPlayerController.cpp b/Source/LostRunic/Framework/LRPlayerController.cpp
index eaf2f5a..7801a18 100644
--- a/Source/LostRunic/Framework/LRPlayerController.cpp
+++ b/Source/LostRunic/Framework/LRPlayerController.cpp
@@ -163,7 +163,7 @@ void ALRPlayerController::HandleSneakToggle()
 	}
 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
 	{
-		character->GetLocomotionComponent()->ToggleSneak();
+		character->GetLocomotionComponent()->RequestToggleSneak();
 	}
 }
 
@@ -178,7 +178,7 @@ void ALRPlayerController::HandleRunStarted()
 	}
 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
 	{
-		character->GetLocomotionComponent()->StartRun();
+		character->GetLocomotionComponent()->RequestStartRun();
 	}
 }
 
@@ -193,7 +193,7 @@ void ALRPlayerController::HandleRunStopped()
 	}
 	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
 	{
-		character->GetLocomotionComponent()->StopRun();
+		character->GetLocomotionComponent()->RequestStopRun();
 	}
 }
 
diff --git a/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp b/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp
index 1cdc675..ad68967 100644
--- a/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp
+++ b/Source/LostRunic/Gameplay/LRLocomotionComponent.cpp
@@ -1,6 +1,6 @@
 /**
  * @file LRLocomotionComponent.cpp
- * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。
+ * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。玩家请求经状态步态规则验证，组件内部应用与掩体覆盖走独立通道。
  *
  * 关联文件：LRLocomotionComponent.h；所属领域：Gameplay。
  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
@@ -14,9 +14,12 @@
 #include "Data/LRMovementTuning.h"
 #include "Engine/GameInstance.h"
 #include "Engine/World.h"
+#include "Framework/LRCharacter.h"
 #include "Framework/LRGameInstanceSubsystem.h"
 #include "GameFramework/Character.h"
 #include "GameFramework/CharacterMovementComponent.h"
+#include "Gameplay/LRMovementRules.h"
+#include "State/LRStateComponent.h"
 #include "TimerManager.h"
 
 /**
@@ -37,13 +40,18 @@ void ULRLocomotionComponent::BeginPlay()
 	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
 	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
 	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
+	State = Cast<ALRCharacter>(GetOwner()) ? Cast<ALRCharacter>(GetOwner())->GetStateComponent() : nullptr;
 	if (!ensureMsgf(Character && Tuning, TEXT("%s requires an ACharacter owner and Movement tuning."), *GetNameSafe(this)))
 	{
 		return;
 	}
 
 	LastSampleLocation = Character->GetActorLocation();
-	SetPace(Pace);
+	ApplyPace(Pace, FGameplayTag());
+	if (State.IsValid())
+	{
+		State->OnStateChanged.AddDynamic(this, &ULRLocomotionComponent::HandleStateChanged);
+	}
 	GetWorld()->GetTimerManager().SetTimer(SampleTimer, this, &ULRLocomotionComponent::SampleTravelDistance,
 		Tuning->SampleIntervalSeconds, true);
 }
@@ -54,6 +62,10 @@ void ULRLocomotionComponent::BeginPlay()
  */
 void ULRLocomotionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
 {
+	if (State.IsValid())
+	{
+		State->OnStateChanged.RemoveDynamic(this, &ULRLocomotionComponent::HandleStateChanged);
+	}
 	if (GetWorld())
 	{
 		GetWorld()->GetTimerManager().ClearTimer(SampleTimer);
@@ -62,69 +74,137 @@ void ULRLocomotionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
 }
 
 /**
- * @brief 更新 Pace，并在需要时同步组件状态或广播变化事件。
- * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
+ * @brief 请求切换潜行与走路；受当前状态步态规则验证，Perception 强制潜行，Courage 禁止潜行，Memory 仅走路。
  */
-void ULRLocomotionComponent::SetPace(const ELRMovementPace newPace)
+void ULRLocomotionComponent::RequestToggleSneak()
 {
-	Pace = newPace;
-	if (!Character || !Tuning)
+	ELRMovementPace& targetPace = Pace == ELRMovementPace::Run ? PaceBeforeRun : Pace;
+	const ELRMovementPace candidate = targetPace == ELRMovementPace::Sneak ? ELRMovementPace::Walk : ELRMovementPace::Sneak;
+	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), candidate))
 	{
+		RejectPaceRequest(candidate);
 		return;
 	}
-
-	float speed = Tuning->WalkSpeed;
-	if (Pace == ELRMovementPace::Sneak)
+	if (Pace != ELRMovementPace::Run)
 	{
-		speed = Tuning->SneakSpeed;
+		ApplyPace(candidate, FGameplayTag());
 	}
-	else if (Pace == ELRMovementPace::Run)
+	else
 	{
-		speed = Tuning->RunSpeed;
+		targetPace = candidate;
 	}
-	Character->GetCharacterMovement()->MaxWalkSpeed = speed;
 }
 
 /**
- * @brief 在状态允许时切换潜行与走路；Perception 强制潜行，Courage 禁止潜行。
+ * @brief 请求开始奔跑；当前状态禁止奔跑时拒绝并广播拒绝原因。
  */
-void ULRLocomotionComponent::ToggleSneak()
+void ULRLocomotionComponent::RequestStartRun()
 {
-	ELRMovementPace& targetPace = Pace == ELRMovementPace::Run ? PaceBeforeRun : Pace;
-	targetPace = targetPace == ELRMovementPace::Sneak ? ELRMovementPace::Walk : ELRMovementPace::Sneak;
-	if (Pace != ELRMovementPace::Run)
+	if (Pace == ELRMovementPace::Run)
 	{
-		SetPace(targetPace);
+		return;
 	}
+	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), ELRMovementPace::Run))
+	{
+		RejectPaceRequest(ELRMovementPace::Run);
+		return;
+	}
+
+	PaceBeforeRun = Pace;
+	ApplyPace(ELRMovementPace::Run, FGameplayTag());
 }
 
 /**
- * @brief 开始 Start Run 流程，建立本次操作拥有的状态、委托或计时器。
+ * @brief 请求结束奔跑，恢复到奔跑前的步态。
  */
-void ULRLocomotionComponent::StartRun()
+void ULRLocomotionComponent::RequestStopRun()
 {
 	if (Pace == ELRMovementPace::Run)
 	{
-		return;
+		ApplyPace(PaceBeforeRun, FGameplayTag());
 	}
+}
 
-	PaceBeforeRun = Pace;
-	SetPace(ELRMovementPace::Run);
+/**
+ * @brief 组件内部应用步态（状态同步、掩体、调试）；玩家输入请走 Request* 入口。
+ * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
+ * @param source 来源 Gameplay Tag，用于日志与诊断；None 表示常规状态应用。
+ */
+void ULRLocomotionComponent::ApplyPace(const ELRMovementPace newPace, const FGameplayTag source)
+{
+	if (Pace != newPace)
+	{
+		UE_LOG(LogLostRunicState, Verbose, TEXT("Locomotion=%s pace %d -> %d source=%s"), *GetNameSafe(this),
+			static_cast<int32>(Pace), static_cast<int32>(newPace), *source.ToString());
+	}
+	Pace = newPace;
+	SyncMovementSpeed();
 }
 
 /**
- * @brief 结束或取消 Stop Run 流程，并清理本次操作拥有的临时状态。
+ * @brief 带来源标识的临时步态覆盖（如掩体强制潜行）；清除时按当前状态重新求值合法步态。
+ * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
+ * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
  */
-void ULRLocomotionComponent::StopRun()
+void ULRLocomotionComponent::OverridePace(const ELRMovementPace newPace, const FGameplayTag source)
 {
-	if (Pace == ELRMovementPace::Run)
+	PaceOverride = newPace;
+	PaceOverrideSource = source;
+	SyncMovementSpeed();
+}
+
+/**
+ * @brief 清除指定来源的临时步态覆盖；覆盖期间的基础步态可能因状态规则过期，清除后重新求值。
+ * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
+ */
+void ULRLocomotionComponent::ClearPaceOverride(const FGameplayTag source)
+{
+	if (!PaceOverrideSource.IsValid() || PaceOverrideSource != source)
+	{
+		return;
+	}
+	PaceOverrideSource = FGameplayTag();
+	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), Pace))
+	{
+		Pace = LRMovementRules::GetDefaultPace(GetEffectiveMode());
+	}
+	SyncMovementSpeed();
+}
+
+/**
+ * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新；清空掩体覆盖并按状态默认步态应用。
+ * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
+ * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+ */
+void ULRLocomotionComponent::HandleStateChanged(const ELRPerceptionMode currentMode, const FGameplayTag reason)
+{
+	if (PaceOverrideSource.IsValid())
 	{
-		SetPace(PaceBeforeRun);
+		ClearPaceOverride(PaceOverrideSource);
 	}
+	ApplyPace(LRMovementRules::GetDefaultPace(currentMode), reason);
+}
+
+/**
+ * @brief 查询 Effective Mode；State 组件缺失时回退 Normal（无 World 测试场景）。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRPerceptionMode ULRLocomotionComponent::GetEffectiveMode() const
+{
+	return State.IsValid() ? State->GetCurrentMode() : ELRPerceptionMode::Normal;
 }
 
 /**
- * @brief 以低频计时器累计角色实际位移，达到步长后发布脚步而不使用 Tick。
+ * @brief 查询 Effective Pace；掩体等覆盖存在时返回覆盖值，否则返回基础步态。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRMovementPace ULRLocomotionComponent::GetEffectivePace() const
+{
+	return PaceOverrideSource.IsValid() ? PaceOverride : Pace;
+}
+
+/**
+ * @brief 以低频计时器累计角色实际位移，达到步长后按步态×环境解析并发布脚步而不使用 Tick。
  */
 void ULRLocomotionComponent::SampleTravelDistance()
 {
@@ -143,8 +223,8 @@ void ULRLocomotionComponent::SampleTravelDistance()
 	}
 
 	DistanceSinceFootstep = FMath::Fmod(DistanceSinceFootstep, stepDistance);
-	const FGameplayTag noiseTag = Pace == ELRMovementPace::Run ? LRGameplayTags::NoiseFootstepRun : LRGameplayTags::NoiseFootstepWalk;
-	OnFootstep.Broadcast(location, GetNoiseRadius(), noiseTag);
+	const FLRNoiseResolution resolution = LRMovementRules::ResolveFootstepNoise(GetEffectivePace(), NoiseEnvironment, *Tuning);
+	OnFootstep.Broadcast(location, resolution.Radius, resolution.Tag);
 }
 
 /**
@@ -153,22 +233,40 @@ void ULRLocomotionComponent::SampleTravelDistance()
  */
 float ULRLocomotionComponent::GetStepDistance() const
 {
-	return Pace == ELRMovementPace::Run ? Tuning->RunStepDistance : Tuning->WalkStepDistance;
+	return GetEffectivePace() == ELRMovementPace::Run ? Tuning->RunStepDistance : Tuning->WalkStepDistance;
 }
 
 /**
- * @brief 查询 Noise Radius；不修改领域状态。
- * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ * @brief 按有效步态同步 CharacterMovement 的 MaxWalkSpeed。
  */
-float ULRLocomotionComponent::GetNoiseRadius() const
+void ULRLocomotionComponent::SyncMovementSpeed()
 {
-	if (Pace == ELRMovementPace::Sneak)
+	if (!Character || !Tuning)
 	{
-		return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorSneakGuardNoiseRadius : 0.0f;
+		return;
 	}
-	if (Pace == ELRMovementPace::Run)
+
+	float speed = Tuning->WalkSpeed;
+	const ELRMovementPace effectivePace = GetEffectivePace();
+	if (effectivePace == ELRMovementPace::Sneak)
 	{
-		return Tuning->IndoorRunNoiseRadius;
+		speed = Tuning->SneakSpeed;
 	}
-	return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorAlertGuardNoiseRadius : Tuning->IndoorWalkNoiseRadius;
+	else if (effectivePace == ELRMovementPace::Run)
+	{
+		speed = Tuning->RunSpeed;
+	}
+	Character->GetCharacterMovement()->MaxWalkSpeed = speed;
+}
+
+/**
+ * @brief 对禁止的步态请求统一记录日志并广播拒绝事件。
+ * @param requestedPace 本次操作使用的 `requestedPace` 枚举或模式值。
+ */
+void ULRLocomotionComponent::RejectPaceRequest(const ELRMovementPace requestedPace)
+{
+	UE_LOG(LogLostRunicState, Warning, TEXT("Locomotion=%s pace request %d rejected in mode %d reason=%s"),
+		*GetNameSafe(this), static_cast<int32>(requestedPace), static_cast<int32>(GetEffectiveMode()),
+		*LRGameplayTags::MovementRejectPaceForbidden.GetTag().ToString());
+	OnPaceRequestRejected.Broadcast(requestedPace, LRGameplayTags::MovementRejectPaceForbidden);
 }
diff --git a/Source/LostRunic/Gameplay/LRLocomotionComponent.h b/Source/LostRunic/Gameplay/LRLocomotionComponent.h
index 45dde49..f264fcd 100644
--- a/Source/LostRunic/Gameplay/LRLocomotionComponent.h
+++ b/Source/LostRunic/Gameplay/LRLocomotionComponent.h
@@ -1,6 +1,6 @@
 /**
  * @file LRLocomotionComponent.h
- * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。
+ * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。玩家请求经状态步态规则验证，组件内部应用与掩体覆盖走独立通道。
  *
  * 关联文件：LRLocomotionComponent.cpp；所属领域：Gameplay。
  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
@@ -16,8 +16,10 @@
 
 class ACharacter;
 class ULRMovementTuning;
+class ULRStateComponent;
 
 DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRFootstepEvent, FVector, location, float, radius, FGameplayTag, noiseTag);
+DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRPaceRequestRejected, ELRMovementPace, requestedPace, FGameplayTag, reason);
 
 /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
 UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Locomotion"))
@@ -42,39 +44,55 @@ public:
 	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
 
 	/**
-	 * @brief 更新 Pace，并在需要时同步组件状态或广播变化事件。
-	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
+	 * @brief 请求切换潜行与走路；受当前状态步态规则验证，Perception 强制潜行，Courage 禁止潜行，Memory 仅走路。
 	 */
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
-	void SetPace(ELRMovementPace newPace);
+	void RequestToggleSneak();
 
 	/**
-	 * @brief 在状态允许时切换潜行与走路；Perception 强制潜行，Courage 禁止潜行。
+	 * @brief 请求开始奔跑；当前状态禁止奔跑时拒绝并广播拒绝原因。
 	 */
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
-	void ToggleSneak();
+	void RequestStartRun();
 
 	/**
-	 * @brief 开始 Start Run 流程，建立本次操作拥有的状态、委托或计时器。
+	 * @brief 请求结束奔跑，恢复到奔跑前的步态。
 	 */
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
-	void StartRun();
+	void RequestStopRun();
+
+	/**
+	 * @brief 查询有效步态（掩体覆盖优先）；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
+	ELRMovementPace GetPace() const { return GetEffectivePace(); }
 
 	/**
-	 * @brief 结束或取消 Stop Run 流程，并清理本次操作拥有的临时状态。
+	 * @brief 组件内部应用步态（状态同步、掩体、调试）；玩家输入请走 Request* 入口。
+	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
+	 * @param source 来源 Gameplay Tag，用于日志与诊断；None 表示常规状态应用。
 	 */
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
-	void StopRun();
+	void ApplyPace(ELRMovementPace newPace, FGameplayTag source = FGameplayTag());
 
 	/**
-	 * @brief 查询 Pace；不修改领域状态。
-	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 * @brief 带来源标识的临时步态覆盖（如掩体强制潜行）；清除时按当前状态重新求值合法步态。
+	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
+	 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
 	 */
-	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
-	ELRMovementPace GetPace() const { return Pace; }
+	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
+	void OverridePace(ELRMovementPace newPace, FGameplayTag source);
+
+	/**
+	 * @brief 清除指定来源的临时步态覆盖；覆盖期间的基础步态可能因状态规则过期，清除后重新求值。
+	 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
+	 */
+	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
+	void ClearPaceOverride(FGameplayTag source);
 
 	/**
-	 * @brief 更新 Noise Environment，并在需要时同步组件状态或广播变化事件。
+	 * @brief 更新 Noise Environment，并在需要时同步组件状态或广播变化事件；仅由 ALRNoiseArea 调用。
 	 * @param newEnvironment 调用方提供的 `newEnvironment`，只在本次操作范围内使用。
 	 */
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
@@ -84,9 +102,31 @@ public:
 	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
 	FLRFootstepEvent OnFootstep;
 
+	/** 当 Pace Request Rejected 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
+	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Movement")
+	FLRPaceRequestRejected OnPaceRequestRejected;
+
 private:
 	/**
-	 * @brief 以低频计时器累计角色实际位移，达到步长后发布脚步而不使用 Tick。
+	 * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新；清空掩体覆盖并按状态默认步态应用。
+	 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
+	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
+	 */
+	UFUNCTION()
+	void HandleStateChanged(ELRPerceptionMode currentMode, FGameplayTag reason);
+
+	/**
+	 * @brief 查询 Effective Mode；State 组件缺失时回退 Normal（无 World 测试场景）。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	ELRPerceptionMode GetEffectiveMode() const;
+	/**
+	 * @brief 查询 Effective Pace；掩体等覆盖存在时返回覆盖值，否则返回基础步态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	ELRMovementPace GetEffectivePace() const;
+	/**
+	 * @brief 以低频计时器累计角色实际位移，达到步长后按步态×环境解析并发布脚步而不使用 Tick。
 	 */
 	void SampleTravelDistance();
 	/**
@@ -95,10 +135,14 @@ private:
 	 */
 	float GetStepDistance() const;
 	/**
-	 * @brief 查询 Noise Radius；不修改领域状态。
-	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 * @brief 按有效步态同步 CharacterMovement 的 MaxWalkSpeed。
 	 */
-	float GetNoiseRadius() const;
+	void SyncMovementSpeed();
+	/**
+	 * @brief 对禁止的步态请求统一记录日志并广播拒绝事件。
+	 * @param requestedPace 本次操作使用的 `requestedPace` 枚举或模式值。
+	 */
+	void RejectPaceRequest(ELRMovementPace requestedPace);
 
 	/** Character 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
 	UPROPERTY(Transient)
@@ -108,12 +152,20 @@ private:
 	UPROPERTY(Transient)
 	TObjectPtr<ULRMovementTuning> Tuning;
 
+	/** State 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TWeakObjectPtr<ULRStateComponent> State;
+
 	/** Pace 的内部运行时数据；不参与蓝图配置。 */
 	ELRMovementPace Pace = ELRMovementPace::Walk;
 	/** Pace Before Run 的内部运行时数据；不参与蓝图配置。 */
 	ELRMovementPace PaceBeforeRun = ELRMovementPace::Walk;
 	/** Noise Environment 的内部运行时数据；不参与蓝图配置。 */
-	ELRNoiseEnvironment NoiseEnvironment = ELRNoiseEnvironment::Indoor;
+	ELRNoiseEnvironment NoiseEnvironment = ELRNoiseEnvironment::Outdoor;
+	/** Pace Override 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	ELRMovementPace PaceOverride = ELRMovementPace::Walk;
+	/** Pace Override Source 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	FGameplayTag PaceOverrideSource;
 	/** Last Sample Location 的运行时状态；由所属类型维护，不在蓝图中配置。 */
 	FVector LastSampleLocation = FVector::ZeroVector;
 	/** Distance Since Footstep 的内部运行时数据；不参与蓝图配置。 */
diff --git a/Source/LostRunic/Gameplay/LRMovementRules.cpp b/Source/LostRunic/Gameplay/LRMovementRules.cpp
new file mode 100644
index 0000000..a100536
--- /dev/null
+++ b/Source/LostRunic/Gameplay/LRMovementRules.cpp
@@ -0,0 +1,123 @@
+/**
+ * @file LRMovementRules.cpp
+ * @brief 实现移动纯规则：状态×步态合法性矩阵、默认步态、步态×环境脚步噪声解析、噪声环境优先级与室内奔跑房间警戒目标值。
+ *
+ * 关联文件：LRMovementRules.h；所属领域：Gameplay。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "Gameplay/LRMovementRules.h"
+
+#include "Core/LRGameplayTags.h"
+#include "Data/LRGuardTuning.h"
+#include "Data/LRMovementTuning.h"
+
+/**
+ * @brief 判断 Is Pace Allowed 对应条件；Normal 全步态，Perception 仅潜行，Courage 走路+奔跑，Memory 仅走路。
+ * @param mode 本次操作使用的 `mode` 枚举或模式值。
+ * @param pace 本次操作使用的 `pace` 枚举或模式值。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+bool LRMovementRules::IsPaceAllowed(const ELRPerceptionMode mode, const ELRMovementPace pace)
+{
+	switch (mode)
+	{
+	case ELRPerceptionMode::Normal:
+		return true;
+	case ELRPerceptionMode::Perception:
+		return pace == ELRMovementPace::Sneak;
+	case ELRPerceptionMode::Courage:
+		return pace != ELRMovementPace::Sneak;
+	case ELRPerceptionMode::Memory:
+		return pace == ELRMovementPace::Walk;
+	}
+	return false;
+}
+
+/**
+ * @brief 查询 Get Default Pace 对应条件；进入状态时强制应用。
+ * @param mode 本次操作使用的 `mode` 枚举或模式值。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRMovementPace LRMovementRules::GetDefaultPace(const ELRPerceptionMode mode)
+{
+	return mode == ELRPerceptionMode::Perception ? ELRMovementPace::Sneak : ELRMovementPace::Walk;
+}
+
+/**
+ * @brief 按步态×环境解析脚步噪声；潜行无声（半径 0 + Sneak 标签，仅供动画/表现），室内奔跑返回 Run.Indoor 标签（房间传播在组件层处理，此半径仅作无房间兜底）。
+ * @param pace 本次操作使用的 `pace` 枚举或模式值。
+ * @param environment 本次操作使用的 `environment` 枚举或模式值。
+ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+FLRNoiseResolution LRMovementRules::ResolveFootstepNoise(const ELRMovementPace pace,
+	const ELRNoiseEnvironment environment, const ULRMovementTuning& tuning)
+{
+	FLRNoiseResolution resolution;
+	switch (pace)
+	{
+	case ELRMovementPace::Sneak:
+		resolution.Radius = 0.0f;
+		resolution.Tag = LRGameplayTags::NoiseFootstepSneak;
+		return resolution;
+	case ELRMovementPace::Run:
+		if (environment == ELRNoiseEnvironment::Indoor)
+		{
+			resolution.Radius = tuning.IndoorRunNoiseRadius;
+			resolution.Tag = LRGameplayTags::NoiseFootstepRunIndoor;
+			return resolution;
+		}
+		resolution.Radius = environment == ELRNoiseEnvironment::OutdoorStealth
+			? tuning.OutdoorStealthRunNoiseRadius : tuning.OutdoorNoiseRadius;
+		resolution.Tag = LRGameplayTags::NoiseFootstepRun;
+		return resolution;
+	case ELRMovementPace::Walk:
+		break;
+	}
+
+	resolution.Radius = environment == ELRNoiseEnvironment::Indoor
+		? tuning.IndoorWalkNoiseRadius : tuning.OutdoorNoiseRadius;
+	resolution.Tag = environment == ELRNoiseEnvironment::Outdoor
+		? LRGameplayTags::NoiseFootstepWalkFaint : LRGameplayTags::NoiseFootstepWalk;
+	return resolution;
+}
+
+/**
+ * @brief 按固定优先级从重叠集合解析环境：Indoor > OutdoorStealth > Outdoor；空集合默认 Outdoor。
+ * @param environments 调用方提供的 `environments`，只在本次操作范围内使用。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+ELRNoiseEnvironment LRMovementRules::ResolveEnvironmentFromSet(const TArray<ELRNoiseEnvironment>& environments)
+{
+	ELRNoiseEnvironment resolved = ELRNoiseEnvironment::Outdoor;
+	for (const ELRNoiseEnvironment environment : environments)
+	{
+		if (environment == ELRNoiseEnvironment::Indoor)
+		{
+			return ELRNoiseEnvironment::Indoor;
+		}
+		if (environment == ELRNoiseEnvironment::OutdoorStealth)
+		{
+			resolved = ELRNoiseEnvironment::OutdoorStealth;
+		}
+	}
+	return resolved;
+}
+
+/**
+ * @brief 解析室内奔跑的房间警戒目标值：当前房间 max(当前警戒, RoomRunAlertLevel)；相邻房间 max(当前警戒, 当前警戒+AdjacentRoomRunAlertAmount)。多房间候选取最大由调用方完成，不在此累加。
+ * @param bCurrentRoom 布尔开关 `bCurrentRoom`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
+ * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+int32 LRMovementRules::ResolveRoomRunTargetLevel(const bool bCurrentRoom, const int32 currentAlert,
+	const ULRGuardTuning& tuning)
+{
+	if (bCurrentRoom)
+	{
+		return FMath::Max(currentAlert, tuning.RoomRunAlertLevel);
+	}
+	return FMath::Max(currentAlert, currentAlert + tuning.AdjacentRoomRunAlertAmount);
+}
diff --git a/Source/LostRunic/Gameplay/LRMovementRules.h b/Source/LostRunic/Gameplay/LRMovementRules.h
new file mode 100644
index 0000000..e80d99e
--- /dev/null
+++ b/Source/LostRunic/Gameplay/LRMovementRules.h
@@ -0,0 +1,65 @@
+/**
+ * @file LRMovementRules.h
+ * @brief 提供移动纯规则：状态×步态合法性矩阵、默认步态、步态×环境脚步噪声解析、噪声环境优先级与室内奔跑房间警戒目标值，供运行时组件与 LostRunic.Movement 自动化测试共同调用。
+ *
+ * 关联文件：LRMovementRules.cpp；所属领域：Gameplay。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "Core/LRTypes.h"
+#include "GameplayTagContainer.h"
+
+class ULRGuardTuning;
+class ULRMovementTuning;
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+struct LOSTRUNIC_API FLRNoiseResolution
+{
+	/** Radius 的空间值 `Radius`；距离和位置使用 Unreal 厘米单位。 C++ 安全默认值为 `0.0f`。 */
+	float Radius = 0.0f;
+	/** Tag 的领域数据，由所属类型负责维护和校验。  */
+	FGameplayTag Tag;
+};
+
+namespace LRMovementRules
+{
+	/**
+	 * @brief 判断 Is Pace Allowed 对应条件；Normal 全步态，Perception 仅潜行，Courage 走路+奔跑，Memory 仅走路。
+	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
+	 * @param pace 本次操作使用的 `pace` 枚举或模式值。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API bool IsPaceAllowed(ELRPerceptionMode mode, ELRMovementPace pace);
+	/**
+	 * @brief 查询 Get Default Pace 对应条件；进入状态时强制应用。
+	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API ELRMovementPace GetDefaultPace(ELRPerceptionMode mode);
+	/**
+	 * @brief 按步态×环境解析脚步噪声；潜行无声（半径 0 + Sneak 标签，仅供动画/表现），室内奔跑返回 Run.Indoor 标签（房间传播在组件层处理，此半径仅作无房间兜底）。
+	 * @param pace 本次操作使用的 `pace` 枚举或模式值。
+	 * @param environment 本次操作使用的 `environment` 枚举或模式值。
+	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API FLRNoiseResolution ResolveFootstepNoise(ELRMovementPace pace, ELRNoiseEnvironment environment,
+		const ULRMovementTuning& tuning);
+	/**
+	 * @brief 按固定优先级从重叠集合解析环境：Indoor > OutdoorStealth > Outdoor；空集合默认 Outdoor。
+	 * @param environments 调用方提供的 `environments`，只在本次操作范围内使用。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API ELRNoiseEnvironment ResolveEnvironmentFromSet(const TArray<ELRNoiseEnvironment>& environments);
+	/**
+	 * @brief 解析室内奔跑的房间警戒目标值：当前房间 max(当前警戒, RoomRunAlertLevel)；相邻房间 max(当前警戒, 当前警戒+AdjacentRoomRunAlertAmount)。多房间候选取最大由调用方完成，不在此累加。
+	 * @param bCurrentRoom 布尔开关 `bCurrentRoom`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
+	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	LOSTRUNIC_API int32 ResolveRoomRunTargetLevel(bool bCurrentRoom, int32 currentAlert,
+		const ULRGuardTuning& tuning);
+}
diff --git a/Source/LostRunic/Gameplay/LRNoiseArea.cpp b/Source/LostRunic/Gameplay/LRNoiseArea.cpp
index ceda6d6..c6a19e4 100644
--- a/Source/LostRunic/Gameplay/LRNoiseArea.cpp
+++ b/Source/LostRunic/Gameplay/LRNoiseArea.cpp
@@ -1,6 +1,6 @@
 /**
  * @file LRNoiseArea.cpp
- * @brief 实现角色移动模式、按移动距离产生脚步和室内外噪声区域等基础玩法能力；数值来自调优资产，不使用无理由 Tick。
+ * @brief 实现角色移动模式、按移动距离产生脚步和室内外噪声区域等基础玩法能力；区域维护进入/退出集合，重叠按固定优先级解析，无区域时默认 Outdoor。
  *
  * 关联文件：LRNoiseArea.h；所属领域：Gameplay。
  * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
@@ -9,7 +9,9 @@
 #include "Gameplay/LRNoiseArea.h"
 
 #include "Components/BoxComponent.h"
+#include "EngineUtils.h"
 #include "Gameplay/LRLocomotionComponent.h"
+#include "Gameplay/LRMovementRules.h"
 
 /**
  * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
@@ -21,10 +23,11 @@ ALRNoiseArea::ALRNoiseArea()
 	SetRootComponent(Bounds);
 	Bounds->SetCollisionProfileName(TEXT("Trigger"));
 	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ALRNoiseArea::HandleBeginOverlap);
+	Bounds->OnComponentEndOverlap.AddDynamic(this, &ALRNoiseArea::HandleEndOverlap);
 }
 
 /**
- * @brief 处理 Handle Begin Overlap 事件，将引擎回调转换为对应领域状态更新。
+ * @brief 处理 Handle Begin Overlap 事件，将引擎回调转换为对应领域状态更新；加入集合后重新求值环境。
  * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
  * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
  * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
@@ -35,8 +38,50 @@ ALRNoiseArea::ALRNoiseArea()
 void ALRNoiseArea::HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
 	const int32 otherBodyIndex, const bool bFromSweep, const FHitResult& sweepResult)
 {
-	if (ULRLocomotionComponent* locomotion = otherActor ? otherActor->FindComponentByClass<ULRLocomotionComponent>() : nullptr)
+	if (!otherActor || !otherActor->FindComponentByClass<ULRLocomotionComponent>())
 	{
-		locomotion->SetNoiseEnvironment(Environment);
+		return;
 	}
+	OverlappingActors.AddUnique(otherActor);
+	RefreshActorEnvironment(otherActor);
+}
+
+/**
+ * @brief 处理 Handle End Overlap 事件，将引擎回调转换为对应领域状态更新；离开区域后按剩余重叠集合重新求值环境。
+ * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
+ * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
+ * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
+ * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
+ */
+void ALRNoiseArea::HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+	const int32 otherBodyIndex)
+{
+	if (!otherActor)
+	{
+		return;
+	}
+	OverlappingActors.Remove(otherActor);
+	RefreshActorEnvironment(otherActor);
+}
+
+/**
+ * @brief 按固定优先级（Indoor > OutdoorStealth > Outdoor）从所有覆盖该角色的噪声区域重新解析环境并应用；无区域时默认 Outdoor。
+ * @param actor 本次查询、交互或事件涉及的 Actor。
+ */
+void ALRNoiseArea::RefreshActorEnvironment(AActor* actor)
+{
+	ULRLocomotionComponent* locomotion = actor ? actor->FindComponentByClass<ULRLocomotionComponent>() : nullptr;
+	if (!locomotion || !GetWorld())
+	{
+		return;
+	}
+	TArray<ELRNoiseEnvironment> environments;
+	for (TActorIterator<ALRNoiseArea> it(GetWorld()); it; ++it)
+	{
+		if (it->OverlappingActors.Contains(actor))
+		{
+			environments.Add(it->Environment);
+		}
+	}
+	locomotion->SetNoiseEnvironment(LRMovementRules::ResolveEnvironmentFromSet(environments));
 }
diff --git a/Source/LostRunic/Gameplay/LRNoiseArea.h b/Source/LostRunic/Gameplay/LRNoiseArea.h
index dc417f2..32ce87b 100644
--- a/Source/LostRunic/Gameplay/LRNoiseArea.h
+++ b/Source/LostRunic/Gameplay/LRNoiseArea.h
@@ -14,6 +14,7 @@
 #include "LRNoiseArea.generated.h"
 
 class UBoxComponent;
+class ULRLocomotionComponent;
 
 /** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
 UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Noise Area"))
@@ -41,10 +42,30 @@ private:
 	void HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
 		int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);
 
+	/**
+	 * @brief 处理 Handle End Overlap 事件，将引擎回调转换为对应领域状态更新；离开区域后按剩余重叠集合重新求值环境。
+	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
+	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
+	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
+	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
+	 */
+	UFUNCTION()
+	void HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+		int32 otherBodyIndex);
+
+	/**
+	 * @brief 按固定优先级（Indoor > OutdoorStealth > Outdoor）从所有覆盖该角色的噪声区域重新解析环境并应用；无区域时默认 Outdoor。
+	 * @param actor 本次查询、交互或事件涉及的 Actor。
+	 */
+	void RefreshActorEnvironment(AActor* actor);
+
 	/** Bounds 的开关；true 表示启用，false 表示禁用。 仅在蓝图或详情面板中查看，不可编辑。 */
 	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Noise", meta = (AllowPrivateAccess = "true"))
 	TObjectPtr<UBoxComponent> Bounds;
 
+	/** Overlapping Actors 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	TArray<TWeakObjectPtr<AActor>> OverlappingActors;
+
 	/** Environment 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNoiseEnvironment::Indoor`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (AllowPrivateAccess = "true"))
 	ELRNoiseEnvironment Environment = ELRNoiseEnvironment::Indoor;
diff --git a/Source/LostRunic/Gameplay/LRRoomVolume.cpp b/Source/LostRunic/Gameplay/LRRoomVolume.cpp
new file mode 100644
index 0000000..3eb306c
--- /dev/null
+++ b/Source/LostRunic/Gameplay/LRRoomVolume.cpp
@@ -0,0 +1,91 @@
+/**
+ * @file LRRoomVolume.cpp
+ * @brief 实现室内奔跑噪声的房间传播体积：重叠守卫集合维护、相邻房间拓扑与旋转体积的局部空间包含判定。
+ *
+ * 关联文件：LRRoomVolume.h；所属领域：Gameplay。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "Gameplay/LRRoomVolume.h"
+
+#include "AI/LRAlertComponent.h"
+#include "Components/BoxComponent.h"
+#include "EngineUtils.h"
+
+/**
+ * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+ */
+ALRRoomVolume::ALRRoomVolume()
+{
+	PrimaryActorTick.bCanEverTick = false;
+	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
+	SetRootComponent(Bounds);
+	Bounds->SetCollisionProfileName(TEXT("Trigger"));
+	Bounds->SetGenerateOverlapEvents(true);
+	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ALRRoomVolume::HandleBeginOverlap);
+	Bounds->OnComponentEndOverlap.AddDynamic(this, &ALRRoomVolume::HandleEndOverlap);
+}
+
+/**
+ * @brief 查询所有覆盖指定位置（支持旋转体积，BoxComponent 局部空间 + extent 判定）的房间体积。
+ * @param world 本次查询所在的 World。
+ * @param location 世界空间位置，Unreal 单位为厘米。
+ * @param outRooms 输出匹配的房间体积集合。
+ */
+void ALRRoomVolume::FindRoomsAtLocation(const UWorld* world, const FVector location, TArray<ALRRoomVolume*>& outRooms)
+{
+	if (!world)
+	{
+		return;
+	}
+	for (TActorIterator<ALRRoomVolume> it(world); it; ++it)
+	{
+		const UBoxComponent* bounds = it->Bounds.Get();
+		if (!bounds)
+		{
+			continue;
+		}
+		// 局部空间 + extent 判定，支持旋转体积；AABB 近似会保守误报，注释说明。
+		const FVector local = bounds->GetComponentTransform().InverseTransformPosition(location);
+		const FVector halfExtent = bounds->GetUnscaledBoxExtent() * bounds->GetComponentScale();
+		if (FMath::Abs(local.X) <= halfExtent.X && FMath::Abs(local.Y) <= halfExtent.Y
+			&& FMath::Abs(local.Z) <= halfExtent.Z)
+		{
+			outRooms.Add(*it);
+		}
+	}
+}
+
+/**
+ * @brief 处理 Handle Begin Overlap 事件：带警戒组件的守卫加入房间集合。
+ * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
+ * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
+ * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
+ * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
+ * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+ * @param sweepResult 本次领域操作的结构化数据 `sweepResult`；字段语义由对应 USTRUCT 定义。
+ */
+void ALRRoomVolume::HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+	const int32 otherBodyIndex, const bool bFromSweep, const FHitResult& sweepResult)
+{
+	if (otherActor && otherActor->FindComponentByClass<ULRAlertComponent>())
+	{
+		OverlappingGuards.AddUnique(otherActor);
+	}
+}
+
+/**
+ * @brief 处理 Handle End Overlap 事件：守卫离开房间集合。
+ * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
+ * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
+ * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
+ * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
+ */
+void ALRRoomVolume::HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+	const int32 otherBodyIndex)
+{
+	if (otherActor)
+	{
+		OverlappingGuards.Remove(otherActor);
+	}
+}
diff --git a/Source/LostRunic/Gameplay/LRRoomVolume.h b/Source/LostRunic/Gameplay/LRRoomVolume.h
new file mode 100644
index 0000000..45c5557
--- /dev/null
+++ b/Source/LostRunic/Gameplay/LRRoomVolume.h
@@ -0,0 +1,89 @@
+/**
+ * @file LRRoomVolume.h
+ * @brief 定义室内奔跑噪声的房间传播体积：维护重叠守卫集合与相邻房间拓扑，支持旋转体积的局部空间包含判定；房间传播只广播表现事件，不调用 ReportNoiseEvent（防双计）。
+ *
+ * 关联文件：LRRoomVolume.cpp；所属领域：Gameplay。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "GameFramework/Actor.h"
+
+#include "LRRoomVolume.generated.h"
+
+class UBoxComponent;
+class ULRAlertComponent;
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Room Volume"))
+class LOSTRUNIC_API ALRRoomVolume : public AActor
+{
+	GENERATED_BODY()
+
+public:
+	/**
+	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
+	 */
+	ALRRoomVolume();
+
+	/**
+	 * @brief 查询所有覆盖指定位置（支持旋转体积，BoxComponent 局部空间 + extent 判定）的房间体积。
+	 * @param world 本次查询所在的 World。
+	 * @param location 世界空间位置，Unreal 单位为厘米。
+	 * @param outRooms 输出匹配的房间体积集合。
+	 */
+	static void FindRoomsAtLocation(const UWorld* world, const FVector location, TArray<ALRRoomVolume*>& outRooms);
+
+	/**
+	 * @brief 查询 Overlapping Guards；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	const TArray<TWeakObjectPtr<AActor>>& GetOverlappingGuards() const { return OverlappingGuards; }
+
+	/**
+	 * @brief 查询 Adjacent Rooms；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	const TArray<TWeakObjectPtr<ALRRoomVolume>>& GetAdjacentRooms() const { return AdjacentRooms; }
+
+private:
+	/**
+	 * @brief 处理 Handle Begin Overlap 事件：带警戒组件的守卫加入房间集合。
+	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
+	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
+	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
+	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
+	 * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
+	 * @param sweepResult 本次领域操作的结构化数据 `sweepResult`；字段语义由对应 USTRUCT 定义。
+	 */
+	UFUNCTION()
+	void HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+		int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);
+
+	/**
+	 * @brief 处理 Handle End Overlap 事件：守卫离开房间集合。
+	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
+	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
+	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
+	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
+	 */
+	UFUNCTION()
+	void HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
+		int32 otherBodyIndex);
+
+	/** Bounds 的开关；true 表示启用，false 表示禁用。 仅在蓝图或详情面板中查看，不可编辑。 */
+	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room", meta = (AllowPrivateAccess = "true"))
+	TObjectPtr<UBoxComponent> Bounds;
+
+	/** Room Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在关卡中的蓝图实例详情面板配置。 */
+	UPROPERTY(EditInstanceOnly, Category = "Room")
+	FName RoomId = NAME_None;
+
+	/** Adjacent Rooms 的领域数据，由所属类型负责维护和校验。 可在关卡中的蓝图实例详情面板配置。 */
+	UPROPERTY(EditInstanceOnly, Category = "Room")
+	TArray<TWeakObjectPtr<ALRRoomVolume>> AdjacentRooms;
+
+	/** Overlapping Guards 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	TArray<TWeakObjectPtr<AActor>> OverlappingGuards;
+};
diff --git a/Source/LostRunic/Input/LRInputConfig.cpp b/Source/LostRunic/Input/LRInputConfig.cpp
index 876569c..ece667a 100644
--- a/Source/LostRunic/Input/LRInputConfig.cpp
+++ b/Source/LostRunic/Input/LRInputConfig.cpp
@@ -24,7 +24,7 @@ bool ULRInputConfig::Validate(FString& outError) const
 		outError = TEXT("All four input mapping contexts are required.");
 		return false;
 	}
-	if (!MoveAction || !SneakAction || !RunAction || !InteractAction || !CloseEyesAction || !OpenEyesAction)
+	if (!MoveAction || !RunAction || !InteractAction || !CloseEyesAction || !OpenEyesAction)
 	{
 		outError = TEXT("Gameplay movement, interaction, and state actions are required.");
 		return false;
diff --git a/Source/LostRunic/Input/LRInputConfig.h b/Source/LostRunic/Input/LRInputConfig.h
index 59fdd92..1920c7e 100644
--- a/Source/LostRunic/Input/LRInputConfig.h
+++ b/Source/LostRunic/Input/LRInputConfig.h
@@ -43,8 +43,8 @@ public:
 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
 	TObjectPtr<UInputAction> MoveAction;
 
-	/** Sneak Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
-	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
+	/** 已废弃：Sneak 与潜行切换语义重复，实际潜行切换由 ToggleCrouchAction 驱动，保留仅用于资产兼容。 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement|Deprecated", meta = (DeprecatedProperty, DisplayName = "Sneak Action (Deprecated)"))
 	TObjectPtr<UInputAction> SneakAction;
 
 	/** Run Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
diff --git a/Source/LostRunic/State/LRStatePresentationComponent.cpp b/Source/LostRunic/State/LRStatePresentationComponent.cpp
index c46854e..46d19cd 100644
--- a/Source/LostRunic/State/LRStatePresentationComponent.cpp
+++ b/Source/LostRunic/State/LRStatePresentationComponent.cpp
@@ -8,6 +8,10 @@
  */
 #include "State/LRStatePresentationComponent.h"
 
+#include "Data/LRGameTuningSet.h"
+#include "Data/LRPresentationTuning.h"
+#include "Engine/GameInstance.h"
+#include "Framework/LRGameInstanceSubsystem.h"
 #include "State/LRStateComponent.h"
 
 /**
@@ -25,12 +29,60 @@ void ULRStatePresentationComponent::BeginPlay()
 {
 	Super::BeginPlay();
 	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
+	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
+	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
+	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Presentation : nullptr;
 	if (ensureMsgf(StateComponent, TEXT("%s requires a sibling LRStateComponent."), *GetNameSafe(this)))
 	{
 		StateComponent->OnStateChanging.AddDynamic(this, &ULRStatePresentationComponent::HandleStateChanging);
 	}
 }
 
+/**
+ * @brief 查询 Perception Reveal Radius（角色周围显现半径，设计 4.5m）；艺术表现预留接入点。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+float ULRStatePresentationComponent::GetPerceptionRevealRadius() const
+{
+	return Tuning ? Tuning->PerceptionRevealRadius : GetDefault<ULRPresentationTuning>()->PerceptionRevealRadius;
+}
+
+/**
+ * @brief 查询 Noise Reveal Radius（声源周围显现半径，设计 2m）；艺术表现预留接入点。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+float ULRStatePresentationComponent::GetNoiseRevealRadius() const
+{
+	return Tuning ? Tuning->NoiseRevealRadius : GetDefault<ULRPresentationTuning>()->NoiseRevealRadius;
+}
+
+/**
+ * @brief 查询 Noise Reveal Duration Seconds（声源显现时长，设计 5s）；艺术表现预留接入点。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+float ULRStatePresentationComponent::GetNoiseRevealDurationSeconds() const
+{
+	return Tuning ? Tuning->NoiseRevealDurationSeconds : GetDefault<ULRPresentationTuning>()->NoiseRevealDurationSeconds;
+}
+
+/**
+ * @brief 查询 Perception Blend Weight（感知后处理混合权重）；艺术表现预留接入点。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+float ULRStatePresentationComponent::GetPerceptionBlendWeight() const
+{
+	return Tuning ? Tuning->PerceptionBlendWeight : GetDefault<ULRPresentationTuning>()->PerceptionBlendWeight;
+}
+
+/**
+ * @brief 查询 Courage Blend Weight（勇气后处理混合权重）；艺术表现预留接入点。
+ * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+ */
+float ULRStatePresentationComponent::GetCourageBlendWeight() const
+{
+	return Tuning ? Tuning->CourageBlendWeight : GetDefault<ULRPresentationTuning>()->CourageBlendWeight;
+}
+
 /**
  * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
  * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
diff --git a/Source/LostRunic/State/LRStatePresentationComponent.h b/Source/LostRunic/State/LRStatePresentationComponent.h
index 28e5a08..38416c0 100644
--- a/Source/LostRunic/State/LRStatePresentationComponent.h
+++ b/Source/LostRunic/State/LRStatePresentationComponent.h
@@ -14,6 +14,7 @@
 
 #include "LRStatePresentationComponent.generated.h"
 
+class ULRPresentationTuning;
 class ULRStateComponent;
 
 DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRStatePresentationRequested,
@@ -47,6 +48,37 @@ public:
 	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Presentation")
 	void CompleteStatePresentation();
 
+	/**
+	 * @brief 查询 Perception Reveal Radius（角色周围显现半径，设计 4.5m）；艺术表现预留接入点。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
+	float GetPerceptionRevealRadius() const;
+	/**
+	 * @brief 查询 Noise Reveal Radius（声源周围显现半径，设计 2m）；艺术表现预留接入点。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
+	float GetNoiseRevealRadius() const;
+	/**
+	 * @brief 查询 Noise Reveal Duration Seconds（声源显现时长，设计 5s）；艺术表现预留接入点。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
+	float GetNoiseRevealDurationSeconds() const;
+	/**
+	 * @brief 查询 Perception Blend Weight（感知后处理混合权重）；艺术表现预留接入点。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
+	float GetPerceptionBlendWeight() const;
+	/**
+	 * @brief 查询 Courage Blend Weight（勇气后处理混合权重）；艺术表现预留接入点。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
+	float GetCourageBlendWeight() const;
+
 	/** Broadcast when Blueprint should play the visual transition, then report completion. */
 	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Presentation")
 	FLRStatePresentationRequested OnStatePresentationRequested;
@@ -74,4 +106,8 @@ private:
 	/** State Component 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
 	UPROPERTY(Transient)
 	TObjectPtr<ULRStateComponent> StateComponent;
+
+	/** 表现调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TObjectPtr<ULRPresentationTuning> Tuning;
 };
diff --git a/Source/LostRunic/Stealth/LRHideComponent.cpp b/Source/LostRunic/Stealth/LRHideComponent.cpp
index 877b708..a6e55f3 100644
--- a/Source/LostRunic/Stealth/LRHideComponent.cpp
+++ b/Source/LostRunic/Stealth/LRHideComponent.cpp
@@ -9,8 +9,10 @@
 #include "Stealth/LRHideComponent.h"
 
 #include "Core/LRGameplayTags.h"
+#include "Framework/LRCharacter.h"
 #include "GameFramework/Character.h"
 #include "GameFramework/CharacterMovementComponent.h"
+#include "Gameplay/LRLocomotionComponent.h"
 #include "State/LRStateComponent.h"
 #include "Stealth/LRHidePoint.h"
 
@@ -30,6 +32,7 @@ void ULRHideComponent::BeginPlay()
 	Super::BeginPlay();
 	Character = Cast<ACharacter>(GetOwner());
 	State = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
+	Locomotion = Cast<ALRCharacter>(GetOwner()) ? Cast<ALRCharacter>(GetOwner())->GetLocomotionComponent() : nullptr;
 	ensureMsgf(Character && State, TEXT("%s requires an ACharacter owner and State component."), *GetNameSafe(this));
 }
 
@@ -62,6 +65,11 @@ bool ULRHideComponent::EnterHidePoint(ALRHidePoint* hidePoint)
 		Character->GetCharacterMovement()->DisableMovement();
 	}
 	State->SetBlockerActive(LRGameplayTags::StateBlockerHidden, true);
+	if (Locomotion)
+	{
+		// 掩体强制潜行覆盖；退出时按当前状态重新求值，不恢复可能过期的缓存步态。
+		Locomotion->OverridePace(ELRMovementPace::Sneak, LRGameplayTags::MovementOverrideHidden);
+	}
 	OnHiddenStateChanged.Broadcast(true, hidePoint);
 	return true;
 }
@@ -85,6 +93,10 @@ bool ULRHideComponent::ExitHidePoint()
 	}
 	bMovementLockedByHide = false;
 	State->SetBlockerActive(LRGameplayTags::StateBlockerHidden, false);
+	if (Locomotion)
+	{
+		Locomotion->ClearPaceOverride(LRGameplayTags::MovementOverrideHidden);
+	}
 	OnHiddenStateChanged.Broadcast(false, nullptr);
 	return true;
 }
diff --git a/Source/LostRunic/Stealth/LRHideComponent.h b/Source/LostRunic/Stealth/LRHideComponent.h
index 24bf89f..81d780f 100644
--- a/Source/LostRunic/Stealth/LRHideComponent.h
+++ b/Source/LostRunic/Stealth/LRHideComponent.h
@@ -15,6 +15,7 @@
 
 class ACharacter;
 class ALRHidePoint;
+class ULRLocomotionComponent;
 class ULRStateComponent;
 
 DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRHiddenStateChanged, bool, bHidden, ALRHidePoint*, hidePoint);
@@ -84,6 +85,10 @@ private:
 	UPROPERTY(Transient)
 	TObjectPtr<ULRStateComponent> State;
 
+	/** Locomotion 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TObjectPtr<ULRLocomotionComponent> Locomotion;
+
 	/** Current Hide Point 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
 	UPROPERTY(Transient)
 	TWeakObjectPtr<ALRHidePoint> CurrentHidePoint;
diff --git a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp
index aad8d60..46d056a 100644
--- a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp
+++ b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.cpp
@@ -8,13 +8,17 @@
  */
 #include "Stealth/LRNoiseEmitterComponent.h"
 
+#include "AI/LRAlertComponent.h"
 #include "Core/LRGameplayTags.h"
 #include "Data/LRGameTuningSet.h"
+#include "Data/LRGuardTuning.h"
 #include "Data/LRMovementTuning.h"
 #include "Engine/GameInstance.h"
 #include "Engine/World.h"
 #include "Framework/LRGameInstanceSubsystem.h"
 #include "Gameplay/LRLocomotionComponent.h"
+#include "Gameplay/LRMovementRules.h"
+#include "Gameplay/LRRoomVolume.h"
 #include "Interaction/LRInteractionComponent.h"
 #include "Interaction/LRInteractionTypes.h"
 #include "Perception/AISense_Hearing.h"
@@ -38,6 +42,7 @@ void ULRNoiseEmitterComponent::BeginPlay()
 	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
 	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
 	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
+	GuardTuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
 	if (!ensureMsgf(Locomotion && Interaction && Tuning, TEXT("%s requires locomotion, interaction, and Movement tuning."),
 		*GetNameSafe(this)))
 	{
@@ -88,9 +93,76 @@ void ULRNoiseEmitterComponent::EmitNoise(const FVector location, const float rad
  */
 void ULRNoiseEmitterComponent::HandleFootstep(const FVector location, const float radius, const FGameplayTag reason)
 {
+	if (reason == LRGameplayTags::NoiseFootstepRunIndoor)
+	{
+		ApplyIndoorRunNoise(location);
+		return;
+	}
 	EmitNoise(location, radius, reason);
 }
 
+/**
+ * @brief 室内奔跑噪声：房间传播优先（当前房警戒至少提升到 RoomRunAlertLevel、相邻房 +1，多房间候选目标值取最大、一次应用）；无房间时回退 1200 半径听觉事件；始终广播 OnNoiseEmitted 供表现钩子，绝不调用 ReportNoiseEvent（防双计）。
+ * @param location 世界空间位置，Unreal 单位为厘米。
+ */
+void ULRNoiseEmitterComponent::ApplyIndoorRunNoise(const FVector location)
+{
+	if (!GetWorld() || !GuardTuning || !Tuning)
+	{
+		return;
+	}
+
+	TArray<ALRRoomVolume*> rooms;
+	ALRRoomVolume::FindRoomsAtLocation(GetWorld(), location, rooms);
+	if (rooms.Num() == 0)
+	{
+		EmitNoise(location, Tuning->IndoorRunNoiseRadius, LRGameplayTags::NoiseFootstepRunIndoor);
+		return;
+	}
+
+	// 对每位守卫收集其所属房间的候选目标值：当前房与相邻房（多房间取最大、不累加），一次应用。
+	TSet<AActor*> applied;
+	for (const ALRRoomVolume* room : rooms)
+	{
+		for (const TWeakObjectPtr<AActor>& guardWeak : room->GetOverlappingGuards())
+		{
+			AActor* guard = guardWeak.Get();
+			ULRAlertComponent* alert = guard ? guard->FindComponentByClass<ULRAlertComponent>() : nullptr;
+			if (!alert || applied.Contains(guard))
+			{
+				continue;
+			}
+			applied.Add(guard);
+			const int32 currentAlert = alert->GetAlertLevel();
+			int32 bestTarget = currentAlert;
+			for (const ALRRoomVolume* containingRoom : rooms)
+			{
+				if (containingRoom->GetOverlappingGuards().Contains(guard))
+				{
+					bestTarget = FMath::Max(bestTarget,
+						LRMovementRules::ResolveRoomRunTargetLevel(true, currentAlert, *GuardTuning));
+				}
+				for (const TWeakObjectPtr<ALRRoomVolume>& adjacentWeak : containingRoom->GetAdjacentRooms())
+				{
+					const ALRRoomVolume* adjacent = adjacentWeak.Get();
+					if (adjacent && adjacent->GetOverlappingGuards().Contains(guard))
+					{
+						bestTarget = FMath::Max(bestTarget,
+							LRMovementRules::ResolveRoomRunTargetLevel(false, currentAlert, *GuardTuning));
+					}
+				}
+			}
+			if (bestTarget > currentAlert)
+			{
+				alert->ApplyAlertDelta(bestTarget - currentAlert, location, GetOwner(), LRGameplayTags::NoiseFootstepRunIndoor);
+			}
+		}
+	}
+
+	// 表现钩子：房间路径只广播表现事件，绝不 ReportNoiseEvent（防与听觉分支双计）。
+	OnNoiseEmitted.Broadcast(location, Tuning->IndoorRunNoiseRadius, LRGameplayTags::NoiseFootstepRunIndoor);
+}
+
 /**
  * @brief 处理 Handle Interaction 事件，将引擎回调转换为对应领域状态更新。
  * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
diff --git a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h
index 3cb6ed4..a81a9e6 100644
--- a/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h
+++ b/Source/LostRunic/Stealth/LRNoiseEmitterComponent.h
@@ -13,6 +13,7 @@
 
 #include "LRNoiseEmitterComponent.generated.h"
 
+class ULRGuardTuning;
 class ULRInteractionComponent;
 class ULRLocomotionComponent;
 class ULRMovementTuning;
@@ -65,6 +66,12 @@ private:
 	UFUNCTION()
 	void HandleFootstep(FVector location, float radius, FGameplayTag reason);
 
+	/**
+	 * @brief 室内奔跑噪声：房间传播优先（当前房警戒至少提升到 RoomRunAlertLevel、相邻房 +1，多房间候选目标值取最大、一次应用）；无房间时回退 1200 半径听觉事件；始终广播 OnNoiseEmitted 供表现钩子，绝不调用 ReportNoiseEvent（防双计）。
+	 * @param location 世界空间位置，Unreal 单位为厘米。
+	 */
+	void ApplyIndoorRunNoise(const FVector location);
+
 	/**
 	 * @brief 处理 Handle Interaction 事件，将引擎回调转换为对应领域状态更新。
 	 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
@@ -83,4 +90,8 @@ private:
 	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
 	UPROPERTY(Transient)
 	TObjectPtr<ULRMovementTuning> Tuning;
+
+	/** Guard 调优缓存；室内奔跑房间警戒目标值来源。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TObjectPtr<ULRGuardTuning> GuardTuning;
 };
diff --git a/Source/LostRunic/Tests/LRFrameworkTests.cpp b/Source/LostRunic/Tests/LRFrameworkTests.cpp
index a90318c..a747436 100644
--- a/Source/LostRunic/Tests/LRFrameworkTests.cpp
+++ b/Source/LostRunic/Tests/LRFrameworkTests.cpp
@@ -57,21 +57,21 @@ bool FLRMovementPaceInputTest::RunTest(const FString& parameters)
 	ULRLocomotionComponent* locomotion = NewObject<ULRLocomotionComponent>();
 	TestEqual(TEXT("Default pace is Walk"), locomotion->GetPace(), ELRMovementPace::Walk);
 
-	locomotion->ToggleSneak();
+	locomotion->RequestToggleSneak();
 	TestEqual(TEXT("Toggle enters Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);
-	locomotion->StartRun();
+	locomotion->RequestStartRun();
 	TestEqual(TEXT("Run press enters Run"), locomotion->GetPace(), ELRMovementPace::Run);
-	locomotion->StopRun();
+	locomotion->RequestStopRun();
 	TestEqual(TEXT("Run release restores Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);
 
-	locomotion->ToggleSneak();
-	locomotion->StartRun();
-	locomotion->StopRun();
+	locomotion->RequestToggleSneak();
+	locomotion->RequestStartRun();
+	locomotion->RequestStopRun();
 	TestEqual(TEXT("Run release restores Walk"), locomotion->GetPace(), ELRMovementPace::Walk);
 
-	locomotion->StartRun();
-	locomotion->ToggleSneak();
-	locomotion->StopRun();
+	locomotion->RequestStartRun();
+	locomotion->RequestToggleSneak();
+	locomotion->RequestStopRun();
 	TestEqual(TEXT("Sneak toggle during Run changes restored pace"), locomotion->GetPace(), ELRMovementPace::Sneak);
 	return true;
 }
diff --git a/Source/LostRunic/Tests/LRGuardTests.cpp b/Source/LostRunic/Tests/LRGuardTests.cpp
index 631367f..a880074 100644
--- a/Source/LostRunic/Tests/LRGuardTests.cpp
+++ b/Source/LostRunic/Tests/LRGuardTests.cpp
@@ -30,11 +30,127 @@ bool FLRAlertRulesTest::RunTest(const FString& parameters)
 	TestEqual(TEXT("Zero alert patrols"), LRAlertRules::ResolveState(0, false, false), ELRGuardBehaviorState::IdlePatrol);
 	TestEqual(TEXT("Low alert is suspicious"), LRAlertRules::ResolveState(5, false, false), ELRGuardBehaviorState::Suspicious);
 	TestEqual(TEXT("Mid alert investigates"), LRAlertRules::ResolveState(6, false, false), ELRGuardBehaviorState::Investigate);
-	TestEqual(TEXT("Max alert searches after sight is lost"), LRAlertRules::ResolveState(11, false, false), ELRGuardBehaviorState::Search);
+	TestEqual(TEXT("Max alert without sight searches"), LRAlertRules::ResolveState(11, false, false), ELRGuardBehaviorState::Search);
 	TestEqual(TEXT("Confirmed max alert chases"), LRAlertRules::ResolveState(11, true, false), ELRGuardBehaviorState::Chase);
-	TestFalse(TEXT("Sight suppresses decay"), LRAlertRules::ShouldDecay(30.0f, 3.0f, true));
-	TestFalse(TEXT("Observation delay has not elapsed"), LRAlertRules::ShouldDecay(2.99f, 3.0f, false));
-	TestTrue(TEXT("Observation delay boundary decays"), LRAlertRules::ShouldDecay(3.0f, 3.0f, false));
+	TestEqual(TEXT("Searching in red band searches"), LRAlertRules::ResolveState(6, false, true), ELRGuardBehaviorState::Search);
+	TestEqual(TEXT("Searching below red band is suspicious"), LRAlertRules::ResolveState(5, false, true), ELRGuardBehaviorState::Suspicious);
+	TestFalse(TEXT("Observation suppresses decay"), LRAlertRules::ShouldDecay(true, false, ELRGuardBehaviorState::Suspicious));
+	TestFalse(TEXT("Sight suppresses decay"), LRAlertRules::ShouldDecay(false, true, ELRGuardBehaviorState::Search));
+	TestFalse(TEXT("Investigate holds alert while traveling"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Investigate));
+	TestFalse(TEXT("Chase holds alert"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Chase));
+	TestTrue(TEXT("Suspicious decays after observation"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Suspicious));
+	TestTrue(TEXT("Search decays after observation"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Search));
+	return true;
+}
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseAlertDeltaTest, "LostRunic.AI.NoiseAlertDelta",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRNoiseAlertDeltaTest::RunTest(const FString& parameters)
+{
+	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
+	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
+	{
+		return false;
+	}
+
+	// 室内奔跑：Set 语义，警戒至少提升到 RoomRunAlertLevel，不走吸引 CD。
+	FLRNoiseResponse indoorRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
+		LRGameplayTags::NoiseFootstepRunIndoor, 3, *tuning);
+	TestTrue(TEXT("Indoor run responds"), indoorRun.bRespond);
+	TestEqual(TEXT("Indoor run raises to floor"), indoorRun.Delta, 2);
+	TestFalse(TEXT("Indoor run is not attract"), indoorRun.bIsAttract);
+	indoorRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(LRGameplayTags::NoiseFootstepRunIndoor, 6, *tuning);
+	TestEqual(TEXT("Indoor run above floor is ignored"), indoorRun.Delta, 0);
+
+	// Faint：仅警戒 >=6 的守卫响应，且为吸引语义。
+	FLRNoiseResponse faintLow = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
+		LRGameplayTags::NoiseFootstepWalkFaint, 5, *tuning);
+	TestFalse(TEXT("Faint ignored below six"), faintLow.bRespond);
+	TestTrue(TEXT("Faint is attract"), faintLow.bIsAttract);
+	FLRNoiseResponse faintHigh = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
+		LRGameplayTags::NoiseFootstepWalkFaint, 6, *tuning);
+	TestTrue(TEXT("Faint responds at six"), faintHigh.bRespond);
+	TestEqual(TEXT("Faint attracts one"), faintHigh.Delta, 1);
+
+	// 普通噪声：一律吸引 +1。
+	const FGameplayTag plainReasons[] = {
+		LRGameplayTags::NoiseFootstepWalk.GetTag(),
+		LRGameplayTags::NoiseFootstepRun.GetTag(),
+		LRGameplayTags::NoiseInteraction.GetTag()
+	};
+	for (const FGameplayTag reason : plainReasons)
+	{
+		const FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(reason, 4, *tuning);
+		TestTrue(TEXT("Plain noise responds"), response.bRespond);
+		TestEqual(TEXT("Plain noise attracts one"), response.Delta, tuning->AttractAlertAmount);
+		TestTrue(TEXT("Plain noise is attract"), response.bIsAttract);
+	}
+	return true;
+}
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertIncreaseCooldownTest, "LostRunic.AI.AlertIncreaseCooldown",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRAlertIncreaseCooldownTest::RunTest(const FString& parameters)
+{
+	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
+	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
+	{
+		return false;
+	}
+
+	// 1-5 档与首次进入 6-10 档使用 0.5s，6-10 档后续使用 0.2s。
+	TestEqual(TEXT("Low band uses long cooldown"),
+		LRAlertRules::ResolveAttractIncreaseCooldown(3, false, *tuning), tuning->AlertIncreaseCooldownSeconds);
+	TestEqual(TEXT("First increase in red band uses long cooldown"),
+		LRAlertRules::ResolveAttractIncreaseCooldown(6, true, *tuning), tuning->AlertIncreaseCooldownSeconds);
+	TestEqual(TEXT("Later increases in red band use short cooldown"),
+		LRAlertRules::ResolveAttractIncreaseCooldown(6, false, *tuning), tuning->InvestigateIncreaseCooldownSeconds);
+
+	// 冷却边界：等于冷却时长时允许；冷却被拒绝的刺激完全忽略。
+	TestTrue(TEXT("Cooldown elapsed allows increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.5, 0.5f));
+	TestFalse(TEXT("Cooldown active rejects increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.6, 0.5f));
+	TestTrue(TEXT("Zero cooldown always allows"), LRAlertRules::IsIncreaseAllowed(10.0, 0.0, 0.0f));
+	return true;
+}
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRResolveTargetBehaviorTest, "LostRunic.AI.ResolveTargetBehavior",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRResolveTargetBehaviorTest::RunTest(const FString& parameters)
+{
+	// 眩晕覆盖一切：感知与警戒继续运行，但行为被钉在 Stunned。
+	TestEqual(TEXT("Stun overrides chase"), LRAlertRules::ResolveTargetBehavior(true, 11, true, false),
+		ELRGuardBehaviorState::Stunned);
+	TestEqual(TEXT("Stun overrides idle"), LRAlertRules::ResolveTargetBehavior(true, 0, false, false),
+		ELRGuardBehaviorState::Stunned);
+	// 未眩晕时按警戒推导。
+	TestEqual(TEXT("Resolved idle"), LRAlertRules::ResolveTargetBehavior(false, 0, false, false),
+		ELRGuardBehaviorState::IdlePatrol);
+	TestEqual(TEXT("Resolved suspicious"), LRAlertRules::ResolveTargetBehavior(false, 5, false, false),
+		ELRGuardBehaviorState::Suspicious);
+	TestEqual(TEXT("Resolved investigate"), LRAlertRules::ResolveTargetBehavior(false, 6, false, false),
+		ELRGuardBehaviorState::Investigate);
+	TestEqual(TEXT("Resolved chase"), LRAlertRules::ResolveTargetBehavior(false, 11, true, false),
+		ELRGuardBehaviorState::Chase);
+	// 眩晕结束后按当前警戒与视线恢复。
+	TestEqual(TEXT("Stun recovery resumes chase"), LRAlertRules::ResolveTargetBehavior(false, 11, true, false),
+		ELRGuardBehaviorState::Chase);
+	return true;
+}
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertTierTest, "LostRunic.AI.AlertTierMapping",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRAlertTierTest::RunTest(const FString& parameters)
+{
+	TestEqual(TEXT("Zero alert is hidden"), LRAlertRules::ResolveAlertTier(0), ELRGuardAlertTier::Hidden);
+	TestEqual(TEXT("Low alert is white"), LRAlertRules::ResolveAlertTier(1), ELRGuardAlertTier::White);
+	TestEqual(TEXT("Five is white boundary"), LRAlertRules::ResolveAlertTier(5), ELRGuardAlertTier::White);
+	TestEqual(TEXT("Six is red"), LRAlertRules::ResolveAlertTier(6), ELRGuardAlertTier::Red);
+	TestEqual(TEXT("Ten is red boundary"), LRAlertRules::ResolveAlertTier(10), ELRGuardAlertTier::Red);
+	TestEqual(TEXT("Eleven is full"), LRAlertRules::ResolveAlertTier(11), ELRGuardAlertTier::Full);
 	return true;
 }
 
diff --git a/Source/LostRunic/Tests/LRMovementTests.cpp b/Source/LostRunic/Tests/LRMovementTests.cpp
new file mode 100644
index 0000000..e8d197b
--- /dev/null
+++ b/Source/LostRunic/Tests/LRMovementTests.cpp
@@ -0,0 +1,139 @@
+/**
+ * @file LRMovementTests.cpp
+ * @brief 提供移动纯规则自动化测试：状态×步态矩阵、默认步态、步态×环境脚步噪声、噪声环境优先级与室内奔跑房间警戒目标值。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
+ *
+ * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#if WITH_DEV_AUTOMATION_TESTS
+
+#include "Misc/AutomationTest.h"
+
+#include "Core/LRGameplayTags.h"
+#include "Core/LRTypes.h"
+#include "Data/LRGuardTuning.h"
+#include "Data/LRMovementTuning.h"
+#include "Gameplay/LRMovementRules.h"
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPaceRulesTest, "LostRunic.Movement.PaceRules",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRPaceRulesTest::RunTest(const FString& parameters)
+{
+	// Normal：全部步态。
+	TestTrue(TEXT("Normal allows sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Sneak));
+	TestTrue(TEXT("Normal allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Walk));
+	TestTrue(TEXT("Normal allows run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Run));
+	// Perception：仅潜行。
+	TestTrue(TEXT("Perception allows sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Sneak));
+	TestFalse(TEXT("Perception forbids walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Walk));
+	TestFalse(TEXT("Perception forbids run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Run));
+	// Courage：走路+奔跑。
+	TestFalse(TEXT("Courage forbids sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Sneak));
+	TestTrue(TEXT("Courage allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Walk));
+	TestTrue(TEXT("Courage allows run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Run));
+	// Memory：仅走路。
+	TestFalse(TEXT("Memory forbids sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Sneak));
+	TestTrue(TEXT("Memory allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Walk));
+	TestFalse(TEXT("Memory forbids run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Run));
+
+	TestEqual(TEXT("Normal defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Normal), ELRMovementPace::Walk);
+	TestEqual(TEXT("Perception defaults to sneak"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Perception), ELRMovementPace::Sneak);
+	TestEqual(TEXT("Courage defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Courage), ELRMovementPace::Walk);
+	TestEqual(TEXT("Memory defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Memory), ELRMovementPace::Walk);
+	return true;
+}
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseResolverTest, "LostRunic.Movement.NoiseResolver",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRNoiseResolverTest::RunTest(const FString& parameters)
+{
+	ULRMovementTuning* tuning = NewObject<ULRMovementTuning>(GetTransientPackage());
+	if (!TestNotNull(TEXT("Movement tuning created"), tuning))
+	{
+		return false;
+	}
+
+	// 潜行：任何环境都无声（半径 0 + Sneak 标签，仅供动画/表现钩子）。
+	for (const ELRNoiseEnvironment environment : { ELRNoiseEnvironment::Indoor, ELRNoiseEnvironment::Outdoor,
+		ELRNoiseEnvironment::OutdoorStealth })
+	{
+		const FLRNoiseResolution sneak = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Sneak, environment, *tuning);
+		TestEqual(TEXT("Sneak radius is zero"), sneak.Radius, 0.0f);
+		TestTrue(TEXT("Sneak uses sneak tag"), sneak.Tag == LRGameplayTags::NoiseFootstepSneak);
+	}
+
+	// 走路：室内 400 / 室外潜行 250 / 室外非潜行 250 + Faint。
+	const FLRNoiseResolution walkIndoor = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::Indoor, *tuning);
+	TestEqual(TEXT("Walk indoor radius"), walkIndoor.Radius, tuning->IndoorWalkNoiseRadius);
+	TestTrue(TEXT("Walk indoor tag"), walkIndoor.Tag == LRGameplayTags::NoiseFootstepWalk);
+	const FLRNoiseResolution walkStealth = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::OutdoorStealth, *tuning);
+	TestEqual(TEXT("Walk outdoor stealth radius"), walkStealth.Radius, tuning->OutdoorNoiseRadius);
+	TestTrue(TEXT("Walk outdoor stealth tag"), walkStealth.Tag == LRGameplayTags::NoiseFootstepWalk);
+	const FLRNoiseResolution walkOpen = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::Outdoor, *tuning);
+	TestEqual(TEXT("Walk outdoor open radius"), walkOpen.Radius, tuning->OutdoorNoiseRadius);
+	TestTrue(TEXT("Walk outdoor open uses faint tag"), walkOpen.Tag == LRGameplayTags::NoiseFootstepWalkFaint);
+
+	// 奔跑：室内 1200 + Run.Indoor / 室外潜行 600 / 室外非潜行 250。
+	const FLRNoiseResolution runIndoor = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::Indoor, *tuning);
+	TestEqual(TEXT("Run indoor radius"), runIndoor.Radius, tuning->IndoorRunNoiseRadius);
+	TestTrue(TEXT("Run indoor tag"), runIndoor.Tag == LRGameplayTags::NoiseFootstepRunIndoor);
+	const FLRNoiseResolution runStealth = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::OutdoorStealth, *tuning);
+	TestEqual(TEXT("Run outdoor stealth radius"), runStealth.Radius, tuning->OutdoorStealthRunNoiseRadius);
+	TestTrue(TEXT("Run outdoor stealth tag"), runStealth.Tag == LRGameplayTags::NoiseFootstepRun);
+	const FLRNoiseResolution runOpen = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::Outdoor, *tuning);
+	TestEqual(TEXT("Run outdoor open radius"), runOpen.Radius, tuning->OutdoorNoiseRadius);
+	TestTrue(TEXT("Run outdoor open tag"), runOpen.Tag == LRGameplayTags::NoiseFootstepRun);
+	return true;
+}
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseEnvironmentPriorityTest, "LostRunic.Movement.NoiseEnvironmentPriority",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRNoiseEnvironmentPriorityTest::RunTest(const FString& parameters)
+{
+	const TArray<ELRNoiseEnvironment> empty;
+	TestEqual(TEXT("No area defaults to outdoor"), LRMovementRules::ResolveEnvironmentFromSet(empty),
+		ELRNoiseEnvironment::Outdoor);
+	TestEqual(TEXT("Single indoor wins"), LRMovementRules::ResolveEnvironmentFromSet(
+		{ ELRNoiseEnvironment::Indoor }), ELRNoiseEnvironment::Indoor);
+	TestEqual(TEXT("Indoor beats outdoor stealth"),
+		LRMovementRules::ResolveEnvironmentFromSet(
+			{ ELRNoiseEnvironment::OutdoorStealth, ELRNoiseEnvironment::Indoor }),
+		ELRNoiseEnvironment::Indoor);
+	TestEqual(TEXT("Indoor beats outdoor"),
+		LRMovementRules::ResolveEnvironmentFromSet(
+			{ ELRNoiseEnvironment::Outdoor, ELRNoiseEnvironment::Indoor }),
+		ELRNoiseEnvironment::Indoor);
+	TestEqual(TEXT("Outdoor stealth beats outdoor"),
+		LRMovementRules::ResolveEnvironmentFromSet(
+			{ ELRNoiseEnvironment::Outdoor, ELRNoiseEnvironment::OutdoorStealth }),
+		ELRNoiseEnvironment::OutdoorStealth);
+	return true;
+}
+
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRRoomRunAlertTargetTest, "LostRunic.Movement.RoomAlertTargets",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FLRRoomRunAlertTargetTest::RunTest(const FString& parameters)
+{
+	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
+	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
+	{
+		return false;
+	}
+
+	// 当前房间：至少提升到 RoomRunAlertLevel(5)。
+	TestEqual(TEXT("Current room raises to floor"), LRMovementRules::ResolveRoomRunTargetLevel(true, 3, *tuning), 5);
+	TestEqual(TEXT("Current room at floor stays"), LRMovementRules::ResolveRoomRunTargetLevel(true, 5, *tuning), 5);
+	TestEqual(TEXT("Current room above floor keeps level"), LRMovementRules::ResolveRoomRunTargetLevel(true, 7, *tuning), 7);
+	// 相邻房间：max(当前, 当前+1)。
+	TestEqual(TEXT("Adjacent room raises by amount"), LRMovementRules::ResolveRoomRunTargetLevel(false, 0, *tuning), 1);
+	TestEqual(TEXT("Adjacent room at eight becomes nine"), LRMovementRules::ResolveRoomRunTargetLevel(false, 8, *tuning), 9);
+	TestEqual(TEXT("Adjacent room at ten caps at eleven"), LRMovementRules::ResolveRoomRunTargetLevel(false, 10, *tuning), 11);
+	return true;
+}
+
+#endif
diff --git a/Source/LostRunic/Tests/LRTuningTests.cpp b/Source/LostRunic/Tests/LRTuningTests.cpp
index 684c81e..4e52d5d 100644
--- a/Source/LostRunic/Tests/LRTuningTests.cpp
+++ b/Source/LostRunic/Tests/LRTuningTests.cpp
@@ -13,6 +13,7 @@
 #include "Data/LRGuardTuning.h"
 #include "Data/LRInteractionTuning.h"
 #include "Data/LRMovementTuning.h"
+#include "Data/LRNPCTuning.h"
 #include "Data/LRPresentationTuning.h"
 #include "Data/LRSaveTuning.h"
 #include "Data/LRStateTuning.h"
@@ -31,6 +32,7 @@ bool FLRTuningDefaultsTest::RunTest(const FString& parameters)
 	TestTrue(TEXT("Save defaults"), NewObject<ULRSaveTuning>()->Validate(error));
 	TestTrue(TEXT("UI defaults"), NewObject<ULRUITuning>()->Validate(error));
 	TestTrue(TEXT("Presentation defaults"), NewObject<ULRPresentationTuning>()->Validate(error));
+	TestTrue(TEXT("NPC defaults"), NewObject<ULRNPCTuning>()->Validate(error));
 	return true;
 }
 
diff --git a/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.cpp b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.cpp
new file mode 100644
index 0000000..f951b33
--- /dev/null
+++ b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.cpp
@@ -0,0 +1,58 @@
+/**
+ * @file LRWorldAlertBarWidgetBase.cpp
+ * @brief 世界空间警戒条 Widget 基类实现：守卫初始化时绑定警戒快照，Widget 销毁时解绑；初始快照在绑定后立即推送。
+ *
+ * 关联文件：LRWorldAlertBarWidgetBase.h；所属领域：UI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#include "UI/LRWorldAlertBarWidgetBase.h"
+
+#include "AI/LRAlertComponent.h"
+#include "AI/LRGuardCharacter.h"
+
+/**
+ * @brief 绑定指定守卫的警戒快照并立即推送当前值，避免首帧不同步；重复调用会先解绑旧守卫。
+ * @param guard 本次查询、交互或事件涉及的 Actor。
+ */
+void ULRWorldAlertBarWidgetBase::InitializeForGuard(ALRGuardCharacter* guard)
+{
+	Shutdown();
+	Alert = guard ? guard->GetAlertComponent() : nullptr;
+	if (Alert.IsValid())
+	{
+		Alert->OnAlertSnapshotChanged.AddDynamic(this, &ULRWorldAlertBarWidgetBase::HandleSnapshotChanged);
+		HandleSnapshotChanged(Alert->GetAlertSnapshot());
+	}
+}
+
+/**
+ * @brief 解绑当前守卫的警戒快照；Widget 销毁时自动调用。
+ */
+void ULRWorldAlertBarWidgetBase::Shutdown()
+{
+	if (Alert.IsValid())
+	{
+		Alert->OnAlertSnapshotChanged.RemoveDynamic(this, &ULRWorldAlertBarWidgetBase::HandleSnapshotChanged);
+	}
+	Alert.Reset();
+}
+
+/**
+ * @brief Widget 销毁时解绑警戒快照，避免悬挂委托。
+ */
+void ULRWorldAlertBarWidgetBase::NativeDestruct()
+{
+	Shutdown();
+	Super::NativeDestruct();
+}
+
+/**
+ * @brief 处理 Handle Snapshot Changed 事件，将引擎回调转换为对应领域状态更新。
+ * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
+ */
+void ULRWorldAlertBarWidgetBase::HandleSnapshotChanged(const FLRAlertSnapshot& snapshot)
+{
+	CurrentSnapshot = snapshot;
+	HandleAlertSnapshotChanged(snapshot);
+}
diff --git a/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.h b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.h
new file mode 100644
index 0000000..9830f89
--- /dev/null
+++ b/Source/LostRunic/UI/LRWorldAlertBarWidgetBase.h
@@ -0,0 +1,73 @@
+/**
+ * @file LRWorldAlertBarWidgetBase.h
+ * @brief 世界空间警戒条 Widget 基类：由守卫初始化并绑定/解绑 ULRAlertComponent 的只读警戒快照，绑定后立即推送一次快照；蓝图只负责表现（0 隐藏 / 1-5 白 / 6-10 红 / 11 满值特效）。
+ *
+ * 关联文件：LRWorldAlertBarWidgetBase.cpp；所属领域：UI。
+ * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
+ * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
+ */
+#pragma once
+
+#include "AI/LRGuardTypes.h"
+#include "Blueprint/UserWidget.h"
+
+#include "LRWorldAlertBarWidgetBase.generated.h"
+
+class ALRGuardCharacter;
+class ULRAlertComponent;
+
+/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
+UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Lost Runic World Alert Bar Base"))
+class LOSTRUNIC_API ULRWorldAlertBarWidgetBase : public UUserWidget
+{
+	GENERATED_BODY()
+
+public:
+	/**
+	 * @brief 绑定指定守卫的警戒快照并立即推送当前值，避免首帧不同步；重复调用会先解绑旧守卫。
+	 * @param guard 本次查询、交互或事件涉及的 Actor。
+	 */
+	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
+	void InitializeForGuard(ALRGuardCharacter* guard);
+
+	/**
+	 * @brief 解绑当前守卫的警戒快照；Widget 销毁时自动调用。
+	 */
+	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
+	void Shutdown();
+
+	/**
+	 * @brief 查询 Current Snapshot；不修改领域状态。
+	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
+	 */
+	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI|Alert")
+	const FLRAlertSnapshot& GetCurrentSnapshot() const { return CurrentSnapshot; }
+
+	/**
+	 * @brief 警戒快照变化时调用；蓝图覆盖此事件只做表现（进度条、颜色、隐藏与满值特效）。
+	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
+	 */
+	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI|Alert")
+	void HandleAlertSnapshotChanged(const FLRAlertSnapshot& snapshot);
+
+protected:
+	/**
+	 * @brief Widget 销毁时解绑警戒快照，避免悬挂委托。
+	 */
+	virtual void NativeDestruct() override;
+
+private:
+	/**
+	 * @brief 处理 Handle Snapshot Changed 事件，将引擎回调转换为对应领域状态更新。
+	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
+	 */
+	UFUNCTION()
+	void HandleSnapshotChanged(const FLRAlertSnapshot& snapshot);
+
+	/** Alert 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
+	UPROPERTY(Transient)
+	TWeakObjectPtr<ULRAlertComponent> Alert;
+
+	/** Current Snapshot 的运行时状态；由所属类型维护，不在蓝图中配置。 */
+	FLRAlertSnapshot CurrentSnapshot;
+};
```
