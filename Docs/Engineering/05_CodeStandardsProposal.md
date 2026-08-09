# LostRunic 游戏代码规范提案

> 状态：待审核提案。当前不修改根目录 `AGENTS.md`。
> 适用范围：未来 `Source/LostRunic` Runtime 代码、可选 `LostRunicEditor` 代码和相关蓝图接口。
> 主依据：Epic Unreal C++ Coding Standard；补充参考 C++ Core Guidelines 与 Cognitive Complexity。

## 1. 规则级别

- **必须**：违反即不能合并，除非提交例外说明。
- **应当**：默认遵守，特殊情况需在评审中说明。
- **建议**：用于提高可读性和长期维护性。

## 2. 文件、函数与复杂度

- 单个 `.h` 或 `.cpp` 有效代码行数 **250 行为软上限，400 行为硬上限**。空行、纯注释和 UHT 生成文件不计入；超过 250 行必须在评审中说明职责是否可拆，超过 400 行必须拆分或记录批准的例外。
- 单个函数建议不超过 40 行，80 行为硬上限。超出时优先提取命名清晰的私有函数。
- 嵌套控制流不超过 4 层。使用早返回、守卫条件或策略对象降低嵌套。
- 圈复杂度目标不超过 10，15 为硬上限。状态机和解析器可申请例外，但必须有测试覆盖。
- 函数参数不超过 5 个；相关参数使用 `USTRUCT` 或参数对象。
- 一个类型只承担一个主要变化原因；一个文件对只放一个主要反射类型，紧密相关的小型枚举/结构体除外。

## 3. Unreal 命名与布局

- 遵循 `A/U/F/E/I` 反射前缀：Actor、UObject、结构体、枚举、接口。
- 类型与函数使用 `PascalCase`；局部变量和参数使用 `camelCase`；布尔值使用 `b` 前缀。
- 文件名与主类型一致，例如 `LRStateComponent.h/.cpp`。
- 头文件使用 `#pragma once` 和最小前置声明；按 IWYU 规则包含直接依赖。
- Include 顺序：对应头文件、项目头文件、引擎头文件、第三方/标准库头文件。
- 领域目录与命名空间保持一致：`State`、`Interaction`、`Stealth`、`Narrative` 等。

## 4. UObject、所有权与数据

- UObject 成员默认使用 `TObjectPtr`；非拥有引用使用 `TWeakObjectPtr`；可异步加载或可选资源使用 `TSoftObjectPtr/TSoftClassPtr`。
- 不保存裸 UObject 指针，不在 UObject 析构时手动删除受 GC 管理对象。
- `UPROPERTY` 必须明确可见性和 Blueprint 权限；运行时缓存使用 `Transient`，存档字段使用 `SaveGame` 或显式序列化。
- DataAsset/DataTable 保存内容定义；Actor 保存运行时状态；不要把大段对白、物品参数或关卡规则硬编码在 C++/蓝图节点中。
- 稳定 ID 使用 `FName` 或 GUID；禁止使用数组序号、对象显示名或关卡内临时名称作为存档键。

### 4.1 参数数据化与编辑器调优

- **必须**：所有会影响手感、难度、节奏或内容平衡的数值都能在 UE 编辑器中调整。包括移动速度、距离/角度、持续时间、冷却、警戒阈值、噪声、击退、输入长按、动画安全超时、存档防抖、重试次数和 UI 打字速度。
- **必须**：运行时规则函数和蓝图流程图中不得散落玩法“魔法数字”。C++ 负责定义类型、校验和算法；值来自 Data Asset、DataTable、Input Action/Mapping Context、Curve、Developer Settings 或明确的蓝图 Class Defaults。
- **必须**：每项参数只有一个权威来源。禁止在 C++、Data Asset 和多个蓝图实例中各保存一份相同数值；需要覆盖时必须定义清晰的优先级，例如“项目默认 -> 关卡配置 -> 实例覆盖”。
- **应当**：全局/原型参数使用按领域拆分的 `UDataAsset`，例如 State、Interaction、Movement、Guard、Save；物品和敌人原型使用 `UPrimaryDataAsset`；批量表格内容使用 DataTable；一次性关卡差异才使用 `EditInstanceOnly`。
- **应当**：调优属性默认使用 `EditDefaultsOnly, BlueprintReadOnly`，并通过 `ClampMin/ClampMax`、`UIMin/UIMax`、单位元数据、Category 和 ToolTip 限制误配。只有确实需要运行时改写的状态才使用 `BlueprintReadWrite`。
- **应当**：C++ 字段初始化值只作为安全回退或新资产默认值，不能成为隐藏的第二配置源。缺少必需配置、越界或单位错误必须在资产校验、`ensureMsgf` 或明确日志中暴露，不能静默回退后继续产生不同规则。
- **允许**：数学恒等值、无效索引、枚举边界和真正不允许设计修改的协议常量可保留在代码中；非显然值使用有语义的 `constexpr`/命名常量，并注明为何不是调优参数。

## 5. 反射与 Blueprint API

- 每个 `UCLASS/USTRUCT/UENUM` 必须有清晰的 Category、DisplayName 和注释。
- `BlueprintCallable` 只暴露稳定、低副作用的领域动作；查询使用 `BlueprintPure`。
- 可调属性对蓝图默认只读；蓝图可以配置默认值，但不得在任意运行时节点绕过领域 API 直接改写规则源。
- 事件优先使用委托或 `BlueprintImplementableEvent`，而不是让蓝图每帧查询 C++ 状态。
- 不在蓝图中重写核心判定：状态合法性、警戒升降、存档和剧情条件必须由 C++ 维护。
- BlueprintImplementableEvent 的 C++ 侧必须提供无蓝图时的安全行为或明确日志。

## 6. 运行时逻辑

- 默认禁止 Tick。只有连续时间逻辑确实需要 Tick 时才启用，并在类注释中写明频率、成本和关闭条件。
- 优先使用委托、计时器、StateTree、Gameplay Tags 和事件队列。
- Actor 负责生命周期和组合；组件负责独立能力；Subsystem 负责跨 Actor 的长期状态。
- 不在组件构造函数中访问 World、资产实例或玩家对象；将运行时依赖放在 BeginPlay/初始化阶段并检查失败。
- 不用全局可变状态保存玩家进度；跨地图数据进入 GameInstance Subsystem/SaveGame。
- 不用字符串拼接驱动核心分支；使用枚举、Gameplay Tags 和稳定 ID。
- 同一时间值不得同时硬编码为动画长度、计时器延迟和规则阈值。优先监听动画/异步操作完成事件；必要的安全超时从调优资产读取。

## 7. 输入、AI 与状态

- 所有玩家输入通过 Enhanced Input Action；C++ 绑定动作语义，不绑定具体按键。
- 输入长按阈值、死区、响应曲线和可重复间隔在 Input 资产或领域 Tuning Asset 中配置；不得把 `0.8f`、`0.3f` 等手感参数写在输入处理函数中。
- 输入层只提出请求，状态组件决定是否接受；拒绝必须提供可调试原因。
- AI 感知使用统一声源/视线事件；敌人 StateTree 不直接读取玩家私有实现细节。
- 状态转换必须可记录、可重放、可测试；每次转换包含前状态、后状态、来源和拒绝原因。
- AI 的每个状态必须有进入、运行、退出和超时行为；不可用“随机 Tick 分支”代替状态图。

## 8. 资产与路径

- 禁止在运行时代码中散落硬编码 `/Game/...` 路径；使用软引用、DataAsset 或配置属性。
- 资产引用按加载成本分类：启动必需资源可硬引用，非关键资源使用软引用并异步加载。
- Niagara、材质、动画和 Widget 的表现参数由状态/数据接口提供，不由多个 Actor 重复设置。
- 调优资产按领域放在 `Content/LostRunic/Data/Tuning/`；Input Action 与 Mapping Context 分别放在 `Input/Actions/` 和 `Input/Contexts/`。
- 资源命名和目录必须与 `Docs/Technical/04_TechnicalDesign.md` 一致。

## 9. 错误处理、日志与断言

- 可恢复的用户/数据错误使用显式返回、提示和 `Warning` 日志；不能静默失败。
- 程序员不变量使用 `check/checkf`；可继续运行的异常使用 `ensure/ensureMsgf`；需要始终执行副作用时使用 `verify`。
- 每个领域使用统一日志类别，例如 `LogLostRunicState`、`LogLostRunicAI`、`LogLostRunicSave`。
- 日志必须包含对象名、事件 ID 或状态原因；禁止每帧输出 `Log` 级别日志。
- Shipping 构建不能依赖 `check` 表达式执行副作用。

## 10. 测试要求

- 新增独立规则必须有 Automation Test：状态转换、警戒、交互、库存、对话条件和存档迁移优先。
- 对调优值有边界依赖的测试至少覆盖默认值、最小值、最大值和非法值校验；测试不得依赖某个关卡蓝图里未声明的实例覆盖。
- 跨 Actor 行为使用 Functional Test 或专用测试地图。
- AI/UI 修改至少验证“家”切片，并在适用时覆盖键鼠和手柄。
- 修复 Bug 时增加能够复现原问题的回归测试或明确的手动验收步骤。
- 测试名称描述行为，例如 `LostRunic.State.PerceptionToMemoryOnDeath`。

## 11. 评审清单

- [ ] 文件和函数没有超过硬上限，例外已经记录。
- [ ] 类型职责单一，组件/Subsystem 边界合理。
- [ ] UObject 所有权、GC、软引用和异步加载正确。
- [ ] 反射属性、Blueprint API 和 Category 可被内容团队理解。
- [ ] 玩法数值可在 UE 编辑器中找到唯一权威来源，带单位/范围，代码和蓝图节点中无魔法数字。
- [ ] 无无理由 Tick、全局可变状态、硬编码资产路径和蓝图轮询。
- [ ] 失败路径有返回值、日志或用户反馈。
- [ ] 关键规则有自动化/功能测试或可复现验收步骤。
- [ ] 文档、Gameplay Tags、DataTable 行 ID 和资源目录已同步。

## 12. 例外流程

例外说明必须包含：违反的规则、必要原因、影响范围、替代方案评估、测试覆盖、计划中的偿还工作和批准人。例外不应成为长期绕过职责拆分的默认方式。

## 13. Definition of Done

一项代码任务只有在以下条件全部满足时才算完成：

1. 行为和公共接口有文档说明。
2. 核心规则与表现逻辑分离。
3. 失败路径和日志可诊断。
4. 相关自动化/功能测试或明确 PIE 验收已完成。
5. 代码、资产路径、数据表、调优资产和蓝图接口没有引入新的隐式约定。
6. 评审清单通过；若有例外，例外记录已附在变更说明中。
7. 新增或修改的玩法参数可在 UE 编辑器中定位、修改、校验，并能从调试输出确认实际生效来源。

## 14. 规范来源

- [Epic C++ Coding Standard for Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Cognitive Complexity - SonarSource](https://www.sonarsource.com/resources/cognitive-complexity/)

## 更新记录

| 日期 | 变更 |
|---|---|
| 2026-08-09 | 增加玩法参数数据化、编辑器调优、唯一权威来源、属性元数据和魔法数字限制。 |
| 2026-08-09 | 首次提案；待审核，不自动合并到根目录 `AGENTS.md`。 |
