# 水彩美术管线实施计划与记录

2026-09-10 补记：本轮水彩灯光与阴影积色由主代理直接实现，未使用子代理。已接入 L_Art_Demo，详细资产、配置、截图、回退与剩余验收见 [07_WatercolorLightAndPigment](07_WatercolorLightAndPigment.md)。P3 仍为候选，不冻结 P5 默认参数。

日期：2026-09-08。依据本任务已确认的 P0–P6 计划；Luna（max）实施，主代理 review。状态记录不等同于用户视觉签字或最低配置性能通过。

## 目标与约束

- 采用已确认新参考图的彩色阴影、斑驳暖光、局部 Bloom 与体积层次；表面干净、低频、简明图案，无默认旧化与满面噪声。
- 描边按轮廓/结构/亮侧/暗侧建立层级，不使用随机变化。
- 用户当前相机合同：距离 500 cm、俯角 52°、FOV 68°。读取实际来源后记录；不重新构图或覆盖用户镜头。
- 保留当前 BP_Ruth 与 L_Art_Demo 的用户修改和建筑、家具布局。场景美术在 L_Art_Demo 验证，通用功能在 L_PIE_Test 定向验证。
- 保留 Default Lit、Lumen、VSM 和既有状态职责；无引擎修改、无重型全屏滤镜。开发机 1080p / SP100 / 60 FPS 目标，仅做趋势验证。
- 不默认 commit/push，不纳入工作区备份或生成目录。基线只追加不覆盖。
- 用户后续修订：新生成贴图被否决，现有贴图保持不动。停止 P2 贴图替换和扩展，后续直接调整 UE 光照、后处理与描边；源候选不接入 Content。
- 双 UE 实例约束：仅使用已核验指向 `D:/25DGame/LostRunic/LostRunic.uproject` 的 `unreal_mcp`。当前地图与项目日志必须匹配后才写入；不凭前台窗口选择实例，不对另一项目发保存、PIE 或材质修改命令。
- 本轮双实例核对：LostRunic PID 18008 与监听 `127.0.0.1:8000` 的所属 PID 一致；这是当次会话证据，编辑器重启后必须重新识别。另一 UE 实例不关闭、不暂停；性能记录需注明并行编辑器可能竞争 GPU，不能当作独占开发机或最低配置验收。

## 工作包与验收

| 阶段 | 实施内容 | 验收依据 | 状态 |
| --- | --- | --- | --- |
| P0 | 保存新标杆、更新 ArtBible；不可变场景/相机/曝光/灯光/PPV/MI/CVar/资产版本快照 | 实际采集清单、hash、原始画面、配置来源和缺失项 | 有限范围封存通过；缺失项见 CaptureNotes |
| P1 | 唯一 Normal 全局艺术描边入口；实际 Gameplay Camera 对照与可恢复诊断 | 场景配置与前后图；不提前引入 Normal Volume 类型 | 装配保存及root读回通过；玩法提示定向验收仍待验证 |
| P2 | 墙、地板、木家具、织物、地毯、玻璃六类样板 | 中性/最终光照、无描边、缩略图、玻璃显隐 A/B | 新贴图方案被用户否决；保留现有贴图 |
| P2.5 | 固化 BC/SMK/Normal/UV/单位/Pivot/材质槽/回读制作标准 | 样板源文件、构建入口、导出与UE回读证据 | 标准文档已review；样板回读待验证 |
| P3 | 彩色暗部、窗光层级、低频光照图案、Bloom/体积光 | 同镜头画面和时序；不把光照图案当任意阴影水彩化 | 第一轮已保存；root视觉审查要求继续迭代 |
| P4 | 用户观察版主轮廓参数2px、结构宽度比例0.5、亮弱暗强 | 实际分辨率与移动稳定；交互/Perception门控 | 2px参数root读回及单帧review通过；时序、状态验收待验证 |
| P5 | 参数归属稳定后新增 Normal 专用定义与应用体积；构建导入接口 | 编译、配置校验、重复导入、状态往返与配置指南 | 未开始 |
| P6 | 首批资源的表现验收，补充必要火焰/植物表现 | 分批review、实际PIE、GPU趋势；现有贴图不替换 | 未开始 |

## 参数与接口边界

- Art Outline：SceneDepth + WorldNormal；Interaction Outline：CustomDepth + CustomStencil。维持各自检测、合成顺序和状态门控。
- 用户明确职责：StyleOutline 是 Normal 状态美术渲染；InteractionOutline 是可交互物体的白色外轮廓反馈。可共用轮廓形状或检测工具，但必须保留独立材质入口、调优参数和玩法状态控制；“唯一入口”仅清理重复 StyleOutline，不把两种功能合并。
- Normal 全局曝光/调色/Bloom/描边最终归 ULRNormalVisualStyleDefinition；ALRNormalVisualStyleVolume 仅在 P5 创建，初始化/ApplyStyle 应用，无持续 Tick。
- 灯光位置、方向、强度、范围与局部雾归关卡；BC/SMK/Roughness/Normal/图案尺度归 MI 与 UV。
- 现有 ULRVisualStyleDefinition 保持 Perception 专用与序列化兼容；本轮不创建 Courage/Memory 空类型。
- 玻璃沿用 M_LR_StylizedGlass 的 Thin Translucent / ForwardShading / 无 Refraction 合同。
- SMK.G 只表达指定结构线，禁止整模型 AO/自动曲率黑化；BC 无灯影/AO/高光烘焙。

## Review 与执行记录

- 2026-09-08：用户授权按修订计划实施，指定 Luna max 子代理，主代理 review。
- 2026-09-08：开始前发现 BP_Ruth.uasset 与 L_Art_Demo.umap 存在用户修改，作为当前基线保护。
- 2026-09-08：创建同目录实施分支 `codex/watercolor-art-pipeline-20260908`，起点 `b5a83f1fc3dfd35abd3bf97599451c776b18a1a4`；保留用户未提交内容，无 commit/push。
- Ruling：使用用户当前 UE 工作目录执行资产与关卡接入，不切换检出目录；编辑器当前内存状态与用户相机调整是本轮基线。以不可变快照和限定改动范围提供回退，不将旧 HEAD 视为用户当前状态。
- 共享文件/接口预检：P0建立不可变参考，P1只修装配；P2产出样板，P2.5固定交付合同；P3/P4的参数经过验证后才由P5固化；P6仅推广通过样板。ArtBible/配置指南由当前阶段独占维护；UE/Blender修改串行。各阶段证据缺失保持待验证，不冒充完成。

后续阶段的实际结果、review发现、修复及剩余门禁追加到此记录。

- 2026-09-09 P3 R2 root 视觉复核：已查看同姿态 `Before_PIE_Center.png` / `After_P3R2_PIE_Center.png`，蓝紫暗部和更柔的投影可见，但大片亮部偏橙黄，斑驳窗光与体积深度仍不明显。参数保存通过不等于目标风格通过；继续独立小步 A/B，现有表面贴图不动。临时出生位置仅用于取景，不修改保存的 PlayerStart。
- 2026-09-09 用户要求更明显的观察版描边：复用原 outline_hierarchy 子智能体，将场景 StyleOutline 的 OutlineWidthPx 调至2；StructureWidthScale 保持0.5。InteractionOutline 宽度和玩法职责不随之改变。该数值是采样宽度参数，不能等同于所有分辨率下最终栅格线条恰好2px。
- 2px root 验证：独立 MCP 读回 OutlineWidthPx=2；已查看 `ArtSource/Evidence/ArtPipeline_20260909/After_P4_2px_PIE_Center.png`（4,588,299 bytes）。桌腿、沙发等外缘较前一版更明确，单帧未见明显异常；此证据不替代移动稳定性或玩法门控测试。
- P3 WindowFill 单变量复核：保留4300K/100lm，将额外橙色 LightColor 恢复白色。root已查看 `After_P3R2_WindowWhite_PIE_Center.png`，亮部重复暖染减少，保留此候选。后续低频 LightFunction 只试验实际产生窗格投影的 Directional；必须有 Strength=0 关闭路径，不把一个Noise节点误报为一条ALU指令，也不在未证明平铺兼容性时强制开启Atlas兼容覆盖。

- P0 root 验证：`Verify-Baseline.ps1` 最终 PASS，26 个归档文件与 detached manifest hash 一致；`Diff-Preview.ps1` 的 14 个可回退资产全部 MATCH。相机在用户修改 DA 后的实际 PIE 读回为 500 cm / -52° / FOV 68°。基线只读封存，不再改写；未取得干净 Gameplay 截图、完整 runtime blended PP stack、全部 Scene MI 与 GPU Profile，见 `ArtSource/Baselines/ArtBaseline_20260907/CaptureNotes.md`，不称全量验收完成。
- P1 root 直接 UE 读回：局部 ArtBench_PPV 的 blendables 已清空，全局 LRScene_PostProcess 包含 StyleOutline 与独立 InteractionOutline，各权重 1。实际 PlayerStart 不在 ArtBench bounds，迁移解决覆盖范围问题；未合并两种功能。
- P3 root 已查看 `ArtSource/Evidence/ArtPipeline_20260908/Before_PIE.png` 与 `After_PIE.png`：局部光池更暖、更亮，但阴影仍偏灰褐且硬，未出现目标的斑驳与体积层。当前只通过参数写入/保存检查，不通过目标风格视觉验收。日志既有 Cutaway 配置警告保持显式记录。
- P4 review：原共享 Detector 的 FinalArtEdge 使用 RawDepthEdge（绝对深度差）而非 RelativeDepthEdge。获准仅在 Style 父图使用 RelativeDepthEdge 与已乘 InternalEdgeStrength 的 NormalEdge，统一一次距离衰减；另给现有法线 taps 独立半宽 UV，不增加采样。亮暗调制只改 Style，保留 Cutaway/NormalGate，Interaction 材质不改。
- 2026-09-09 P4：有效 `After_P4_PIE.png`（4,550,035 bytes）已由 root 查看，195 expressions 的候选已编译保存。root 独立读回 Relative/Normal 分支、乘法适配常量 1、深度/法线 Kernel 分离、Cutaway 与最终 NormalGate 通过。单帧无明显异常；未据此声称 1080p 精确半像素线宽、移动无闪烁或完整状态往返通过。P3 第二轮接续实施，现有贴图保持不变。

- 2026-09-08 P2.5 review：Luna max 完成 [Asset Authoring Standard](04_AssetAuthoringStandard.md)，root 核对并要求修正当前/历史 SMK 混淆、保留已有槽位、唯一单位转换、静态合并后的 Pivot 边界及相机/测试关卡合同；修订通过文档审查。未宣称 UE 回读或视觉通过。

### 初始差异审查（2026-09-08）

以下视觉判断依据用户提供的 PIE 图与概念图，不以不配准图片的像素差给出虚假的相似百分比。

| 维度 | 当前接近程度与差异 | 管线处理 |
| --- | --- | --- |
| 布局与大形 | 房间、壁炉、窗、家具色块和俯视阅读关系已相当接近；是可继续用的第一批资产 | 保留用户摆放，先修表面与照明；仅在样板证明必要时修改局部形体 |
| 表面 | 木、布、墙可分辨，但墙面碎斑、地板细纹与目标的低频水彩色块不一致 | 从 BC 源制作修正，不能仅靠降低 NormalStrength 或增加后处理解决 |
| 明暗构图 | 当前大面积直射亮地毯、明亮地板和均匀可见墙面削弱了概念图的局部光池及空间层次 | 锁定镜头与曝光后组织窗光、局部暖光与冷色环境填充 |
| 阴影 | 当前投影较硬、色相层次少；目标暗部有蓝紫色与水彩柔边 | 分开验证光色、光源角度/尺寸、低频斑驳和局部体积效果；灯光图案不等于所有物体阴影水彩化 |
| 线稿 | 当前截图的体积主要靠常规明暗，尚未体现稳定的轮廓/结构/亮暗层级 | 先确认唯一 Normal 描边入口，再分别调整检测与合成，避免随机线宽掩盖阈值问题 |
| 玻璃与空间 | 当前窗外蓝底和空白壁炉减弱空间、反射与暖光叙事；植物剪影偏简 | 玻璃列入六类样板；火焰/植物在 P6 补齐，不能用全屏 Bloom 代替 |

用户所给 Blue Prince 截图主要提供结构线稿层级，CAIRN 截图提供简化色块与绘画化表面，No Rest for the Wicked 截图提供集中光源、体积与冷暖空间关系。这些是截图视觉分析，不推断各游戏未公开的 Shader 实现，也不平均混合三种风格。

已核实的技术偏差与审查项：

- `ArtSource/LivingRoom/Iteration02/Analysis.md` 曾将旧灰蓝漆、稀疏脱落、细胡桃木纹、旧橡木列为优化方向；四张修订 BC 由 imagegen 生成。这与本轮已确认的干净低频目标发生方向偏移，需更新制作输入和验收标准。
- 已读取墙面 MI 的 `NormalStrength=0`，因此不能把截图上的细碎表面全部归因于法线。织物为专属 UV atlas，修纹理时必须保留岛与材质分区。
- 当前关卡有两个启用的 PPV，分别包含 Benchmark 与场景艺术描边实例。相同父材质实例可能参与参数混合；仅凭两条引用不能声称实际执行两次 Pass。P1 应核对覆盖范围、有效权重、材质 Override 开关和父材质设置。
- 旧 UE 导入说明写曝光补偿 -4，而当前读取值为 0；旧相机说明是正交展示相机。两者是历史值与当前状态的差异，不能拿旧 README 充当不可变基线，也不自动判定用户调参错误。
- `ULRCameraRigComponent::BeginPlay` 会从 Presentation 调优资产读取距离并设置 SpringArm。用户随后确认已将 `DA_LRPresentationTuning.DefaultCameraDistanceCm` 改为 500，配置冲突已由用户修正；基线纳入这一当前值，PIE 仅核对生效结果，不再修改相机。
- 当前先采集到的是编辑器视口截图，不等于 500 cm / -52° / 68° 的 Gameplay 对照。实际 PIE 镜头与性能证据未取得时继续标记待验证。

### P5 最小契约（设计审查，未实施；2026-09-09）

P3 仍是候选阶段。P3 R2 当前读回的暗部调色候选为 `ColorSaturationShadows=(1.02,0.96,1.10,1)`、`ColorGainShadows=(0.96,0.90,1.08,1)`、`ColorOffsetShadows=(0.004,0.002,0.010,0)`；这些数值只记录来源，不冻结为 P5 默认。P4 当前 StyleOutline 读回为 `OutlineWidthPx=2`、`StructureWidthScale=0.5`，2px 是采样宽度，不等同所有分辨率下的最终栅格线宽；移动、时序和状态往返仍待验收。

P5 的 `ULRNormalVisualStyleDefinition` 只保存经过阶段门禁的 Normal 视觉候选：曝光、引擎原生调色、Bloom 和 StyleOutline 参数。曝光不能只映射 `AutoExposureBias` 并凭建议强制 `Manual`：冻结字段前必须读回实际 AutoExposure override 集合（至少包括 `AutoExposureMethod`、`AutoExposureBiasCurve`、Min/Max、速度和 `AutoExposureApplyPhysicalCameraExposure`）、物理相机曝光设置、所有启用 PPV/相机额外后处理以及最终合成栈。P5 不因设计推断改变曝光 Method、物理相机曝光或既有曝光行为。

曝光、调色和 Bloom 的语义是章节/场景基础 PP，跨 Normal 与 Perception 作为同一基础输入/输出链存在；它们不是由 Volume 根据状态切换的 Normal-only 值。因 `M_PP_LR_PerceptionComposite` 位于 Before Tonemapping，P5 必须在相同相机下读回并对照 Normal/Perception 的实际曝光、调色、Bloom 和 Composite 权重，不能因资产名为 Normal 就声称 Perception 的 pre-tonemap 色彩或曝光不受影响。StyleOutline 的状态门控仍只由现有 `ULRPerceptionPresentationComponent::WriteStyleParameters` 写入 `LR_NormalOutlineGate=1-CurrentBlend`；`LR_InteractionPresentationGate` 也由该组件独占写入。新 Volume 不写任何 Perception MID/MPC 参数，Interaction 白线继续由既有 Interaction 组件和其状态逻辑负责。

未来 `ALRNormalVisualStyleVolume` 只在初始化和显式 `ApplyStyle` 时读取并校验定义，关闭 Tick；先完成校验，再更新自身 `FPostProcessSettings` 和自己拥有的唯一 StyleOutline Blendable。重复 `ApplyStyle` 必须移除/更新自己旧的 Blendable 后得到相同结果，不扫描其他 Volume、不修改灯光、雾、表面 MI、贴图、Perception 或 Interaction。当前场景不迁移；P5 迁移前必须逐项读回旧 PPV 的 Normal 字段、Style/Interaction Blendable 数量与权重、最终栈和回退快照。迁移时仅清理旧 PPV 的 Normal 曝光/调色/Bloom/Style 入口，保留 Interaction 入口及关卡灯光、雾和表面设置，确认全局只有一个 StyleOutline 和一个独立 InteractionOutline 后才可保存。
