# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

LostRunic（工作名《不要忘记阿黛尔》）是 **Unreal Engine 5.8** 的 Windows 单机俯视角叙事潜行解谜游戏（`LostRunic.uproject` 的 `EngineAssociation` 为 `5.8`）。核心规则全部由 C++ 实现，蓝图只负责装配、表现与配置；关卡布局与场景布置由项目负责人在编辑器中完成，不代做。

详细的代码规范、调优政策、错误处理与验收标准见 **`AGENTS.md`**（仓库权威指南，Codex/Claude 共用）；设计上下文与四状态系统等玩法规则见 **`.agents/ue-project-context.md`**（已合并设计摘要与技术设计，含状态标记：`已确定`/`基线`/`待决策`）。本文档只补充 Claude 工作所需的工程要点，不重复 AGENTS.md 内容。

## 构建、运行与测试

在 Developer Command Prompt 中运行，将 `UE_ROOT` 替换为已安装的 UE 5.8 引擎路径（项目不含源码引擎，无 UBT 全局命令）：

```powershell
# 编译编辑器目标
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" LostRunicEditor Win64 Development -Project="$PWD\LostRunic.uproject"

# 启动编辑器
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\LostRunic.uproject"
```

- 修改 C++ 后必须重新编译编辑器（或使用 Live Coding）再进 PIE；不要在未编译状态下创建/修改依赖反射的蓝图资产。
- 仓库提供构建/测试包装脚本（避免 Git Bash→cmd 的引号问题）：`Scripts/BuildLostRunicEditor.bat`、`Scripts/RunLostRunicTests.bat`（引擎路径硬编码在脚本内，移动引擎时需同步修改）。
- 自动化测试位于 `Source/LostRunic/Tests/LR*Tests.cpp`（`WITH_DEV_AUTOMATION_TESTS` 下编译），无独立测试套件；可经 Session Frontend 运行，也可命令行跑：
  `UnrealEditor.exe "$PWD\LostRunic.uproject" -unattended -nopause -ExecCmds="Automation RunTests <Test名或前缀>; Quit"`
- 冒烟测试：在两个变体地图分别 PIE，检查 Output Log 警告/错误，AI/UI 改动同时验证键鼠与手柄。
- 调试命令：`LR.Debug.State` / `.Alert` / `.Interaction` / `.Save` / `.Tuning`；日志分类 `LogLostRunicState`、`LogLostRunicInteraction`、`LogLostRunicAI`、`LogLostRunicNarrative`、`LogLostRunicSave`、`LogLostRunicUI`、`LogLostRunicTuning`。
- 不手动编辑或提交 `Binaries/`、`Intermediate/`、`DerivedDataCache/`、`Saved/`。

## 架构总览

单 Runtime 模块 `LostRunic`（`Source/LostRunic/LostRunic.Build.cs`），目录按领域组织：

| 目录 | 职责 |
|---|---|
| `Core/` | 日志、Gameplay Tags、通用类型、验证、调试命令 |
| `Data/` | Tuning DataAsset、内容定义（物品/收藏品/守卫/关卡事件）、`ULRProjectSettings`、聚合资产 `ULRGameTuningSet` |
| `Framework/` | `ALRGameMode`、`ALRPlayerController`、`ALRCharacter`、`ALRGameState`、`ULRGameInstanceSubsystem` |
| `State/` | 四状态组件与规则（Normal/Perception/Courage/Memory）、状态表现组件（含艺术表现预留 getter） |
| `AI/` | 守卫（控制器按生命周期/感知/行为拆 3 cpp）、警戒组件（4.2.1 全量语义）、通用 NPC（`LRNPC*`，StateTree 驱动）、感知规则、StateTree 节点（`LRGuardStateTreeNodes`/`LRNPCStateTreeNodes`） |
| `Interaction/` | `ILRInteractable` 接口、交互筛选组件、门/拾取/世界交互 Actor |
| `Items/` | 背包、物品使用统一解析（`ULRItemUseResolver`，快捷栏与交互选择器双入口共用） |
| `Stealth/` | 掩体（`ULRHideComponent`，进掩体强制潜行覆盖）、噪声发射（`ULRNoiseEmitterComponent`）、守卫可见性 |
| `Gameplay/` | 移动（`ULRLocomotionComponent`：Request*/ApplyPace/OverridePace 权限拆分）、步态×噪声纯规则（`LRMovementRules`）、噪声区域（`ALRNoiseArea`）、室内奔跑房间体积（`ALRRoomVolume`） |
| `Narrative/` | `ULRDialogueSubsystem`（DataTable 遍历、条件、分支、一次性剧情事件） |
| `Save/` | SaveGame 分块结构、保存队列、`FLRResumeAnchor` 恢复锚点 |
| `UI/` | HUD、Widget Controller（对话/菜单/过渡）、`ULRPlayerUIComponent` |
| `Variant_Strategy/` `Variant_TwinStick/` | 两个玩法变体（策略 / twin-stick），各含 `AI/`、`Gameplay/`、`UI/` 子目录 |
| `Tests/` | 自动化测试（框架、守卫、交互、物品、叙事、存档、状态、调优） |

要点：

- **职责划分**：Actor 负责生命周期与组件组合，Component 负责独立能力，Subsystem 负责跨 Actor 长期状态。`ALRPlayerController` 是 Enhanced Input 上下文、输入模式、光标状态的唯一所有者；`ULRGameInstanceSubsystem` 持有已验证的内容与调优根；Widget 只接收不可变表现数据，不得反向决定规则。
- **模板遗留**：模块根部的 `LostRunicCharacter`、`LostRunicGameMode`、`LostRunicPlayerController`（及 `Content/TopDown/`）是 UE 模板残留。新功能优先复用 `LR*` 类，避免形成第二套框架。
- **调优与数据**：影响手感/平衡的值必须落在 `Content/LostRunic/Data/Tuning/` 的领域 DataAsset（`ULRStateTuning`、`ULRInteractionTuning`、`ULRMovementTuning`、`ULRGuardTuning`、`ULRSaveTuning`、`ULRUITuning`、`ULRNPCTuning` 等），由 `ULRGameTuningSet` 聚合，`DefaultGame.ini` 指定默认集。C++ 默认值只是安全回退；稳定 ID 用 `FName`/GUID；非关键资源用软引用异步加载；禁止散落硬编码 `/Game/...` 路径。**字段重命名必须同步 `Config/DefaultEngine.ini` 的 `+PropertyRedirects`（先例：`HearingAlertAmount`→`AttractAlertAmount` 等）**。
- **输入**：Enhanced Input 语义动作，代码绑定动作不绑按键；长按阈值/死区/曲线放入输入与调优资产；上下文切换时 `bIgnoreAllPressedKeysUntilRelease`（`IMC_LRGameplay` / `IMC_LRDialogue` / `IMC_LRMenu` / `IMC_LRTransition`）。
- **AI**：StateTree + 统一声源/视线事件，警戒 0–11 必须记录原因 Gameplay Tag（如 `Noise.Footstep`、`Sight.Player`）；禁止随机 Tick 分支替代状态图。守卫行为由 `ResolveTargetBehavior`（`LRAlertRules` 纯规则，眩晕优先）唯一权威解析，StateTree 只执行结果：树**仅由 `AI.Event.BehaviorChanged` 驱动**（`AI.Event.AlertChanged` 只表示数据变化）；`ST_Guard`/`ST_NPC` 资产需在编辑器人工创建（MCP 只能检查）并挂在 `DA_LRGuardDefinition`/`DA_LRNPCDefinition.Behavior`（**硬引用**）。
- **潜行噪声语义（4.2）**：潜行完全无声；`ALRNoiseArea` 三环境（Indoor/Outdoor/OutdoorStealth，重叠按 Indoor>OutdoorStealth>Outdoor 解析，**无区域默认 Outdoor**）；室内奔跑走 `ALRRoomVolume` 房间传播（当前房警戒至少 5、相邻房 +1，多房间取最大，无房间回退 1200 半径听觉事件，传播路径绝不发 `ReportNoiseEvent` 防双计）；`Walk.Faint` 仅警戒 ≥6 守卫响应。
- **输入**：Enhanced Input 语义动作，代码绑定动作不绑按键；长按阈值/死区/曲线放入输入与调优资产；上下文切换时 `bIgnoreAllPressedKeysUntilRelease`（`IMC_LRGameplay` / `IMC_LRDialogue` / `IMC_LRMenu` / `IMC_LRTransition`）。
- **AI**：StateTree + 统一声源/视线事件，警戒 0–11 必须记录原因 Gameplay Tag（如 `Noise.Footstep`、`Sight.Player`）；禁止随机 Tick 分支替代状态图。
- **默认禁止 Tick**：优先委托、计时器、StateTree、Gameplay Tags、事件队列。
- **有意不做**：GAS、网络/复制、CommonUI、Excel 直接读取（用 UTF-8 CSV/DataTable）。新增模块依赖前先确认模块所有权（可引入 OnlineSubsystem/Steam 的注释已在 Build.cs 中预留）。
- **地图**：主切片 `Content/LostRunic/Levels/Home/L_Home.umap`；变体地图 `Content/Variant_Strategy/LVL_Strategy.umap`、`Content/Variant_TwinStick/LVL_TwinStick.umap`；`Content/TopDown/Lvl_TopDown.umap` 为模板残留。

## Claude 工作流约定

- **蓝图工具**：编辑器中操作蓝图优先用 unreal-mcp（`http://127.0.0.1:8000/mcp`，需编辑器运行）或仓库内 `ue-*` skill（`.agents/skills/`、`~/.claude/skills/`）；无匹配 MCP 能力时才用 Unreal Editor Python 或人工流程并记录替代方案。实现游戏功能优先 C++ 而非蓝图。
- **蓝图配置登记**：任何完成后需在蓝图中装配/派生/配置的 C++ 功能，必须同步登记到 `Docs/Technical/06_BlueprintConfigurationGuide.md`（含资产路径、配置步骤、参数来源、PIE 验收）；代码接口或资产变化也须在该文档同步维护。
- **实现状态**：Home 垂直切片各阶段状态与稳定内容 ID（`Home_Dorothy_001`、`Home_Note_Mother`、`Home_Doll` 等）见 `Docs/Technical/05_HomeSliceImplementation.md`（阶段 8 因 Home 资产/地图未就绪暂停中）。核心玩法批次（2026-08-14）：4.1 四状态 + 4.2 潜行（步态权限、噪声语义、敌人警戒 4.2.1 全量、房间传播、掩体、通用 NPC、警戒条数据层）**C++ 已实现**，`ST_Guard`/`ST_NPC`/`DA_LRGuardDefinition`/`DA_LRNPCDefinition`/`DA_LRNPCTuning`/`BP_Guard`/`BP_NPC`/`WBP_GuardAlertBar` 等资产待装配、`DA_LRGuardTuning.AttractAlertAmount` 待改为 1——详见 `Docs/Technical/06_BlueprintConfigurationGuide.md`「核心玩法机制」章节与 `.agents/ue-project-context.md`。
- **提交**：仓库有 Git 历史（main 分支，无远程）；工作区可能混有负责人进行中的其他批次改动，提交前先确认范围。如需要拉取 GitHub 内容，github.com 直连不可达，用 `https://ghfast.top/<原URL>` 镜像。
