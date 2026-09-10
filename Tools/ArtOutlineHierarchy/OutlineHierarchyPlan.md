# P4 StyleOutline 层级执行记录

**日期**：2026-09-08
**状态**：已按批准方案完成 UE 材质图写入、材质重编译、保存和定向 PIE 截图；本文同时保留操作脚本和实际节点/参数读回。

## 范围和不变量

- 目标资产：`/Game/LostRunic/Materials/PostProcess/M_PP_LR_StyleOutline`。
- 只改 StyleOutline 的美术层级。InteractionOutline 的白色交互提示、CustomDepth/Stencil 路径和交互状态控制不参与亮暗调制。
- 不改现有贴图、`WatercolorSamples`、镜头布局、`MF_LR_ArtEdgeDetector` 的接口/行为、`MF_LR_OutlineKernel` 的函数接口，或 P5 NormalVolume。
- P0 只读基线 `ArtSource/Baselines/ArtBaseline_20260907` 及其 blob 不可修改。`LightDarkStrength=0` 只关闭新增亮暗调制，不承诺恢复旧的 RawDepth 最终合成；完整回退使用 P0 备份。
- 保留现有 Perception（`MPC_LR_VisualStyle.LR_NormalOutlineGate`）、Cutaway（`MPC_LR_CutawayView`）和 NormalGate。Cutaway mask 继续由 `Subtract_13` 提供给最终 alpha。
- 不加随机噪声、额外模糊或重滤镜；保持中心 WorldNormal 加四个邻域 WorldNormal taps，保持四个深度 taps。

## 现有图核对

### StyleOutline 主图

材质为 Post Process、`BL_SceneColorAfterDOF`、Opaque/Unlit，`bDisablePreExposureScale=false`，当前优先级为 0。AfterDOF 的 SceneColor 仍在 tonemapping 前的 HDR/有效预曝光空间，不能把它当作最终屏幕 0..1 亮度。

当前关键节点和接线如下（节点编号为当前资产中的表达式编号）：

| 节点 | 当前作用/接线 |
| --- | --- |
| `MaterialExpressionMaterialFunctionCall_0` | `MF_LR_OutlineKernel`；`ScreenUV=ScreenPosition_0.ViewportUV`，`InvSize=SceneTexture_1.InvSize`，`WidthPx=ScalarParameter_0 (OutlineWidthPx)`。输出 `UVLeft/Right/Up/Down`。 |
| `SceneTexture_1` | 中心 `PPI_SceneDepth`，UV 为 `ScreenPosition_0.ViewportUV`。 |
| `SceneTexture_2..5` | 四个 `PPI_SceneDepth`，分别使用 Kernel 0 的 left/right/up/down UV；深度采样数为 4。 |
| `SceneTexture_6` | 中心 `PPI_WorldNormal`。 |
| `SceneTexture_7..10` | 四个 `PPI_WorldNormal`，当前也使用 Kernel 0 的四个 UV；法线邻域采样数为 4。 |
| `MaterialExpressionMaterialFunctionCall_1` | `MF_LR_ArtEdgeDetector`。`RawDepthDelta=Max_2`、`CenterDepth=SceneTexture_1.Color`、`NormalDelta=Max_5`、`DepthThreshold=ScalarParam1`、`NormalThreshold=ScalarParam2`、`InternalEdgeStrength=ScalarParam3`、`DistanceFadeStart=ScalarParam4`、`DistanceFadeEnd=ScalarParam5`、`DepthEpsilon=Constant_0`。 |
| `Multiply_16` | 当前 `MFcall1.FinalArtEdge * Subtract_13`；其输出进入 `Lerp_1.Alpha`，也作为 Debug4 当前输出。`Subtract_13` 是 `1 - Max_16` 的 Cutaway 抑制。 |
| `Lerp_1` | A 为当前 SceneColor RGB，B 为 `Multiply_3` 的墨色结果，Alpha 为 `Multiply_16`。 |
| `Lerp_2` | A 为 SceneColor，B 为 Debug/Style 结果 `If_3`，Alpha 为 MPC `LR_NormalOutlineGate`；保留此 Perception 门控。 |
| `Multiply_0/3` | `Multiply_0 = max(SceneColorRGB,0) * OutlineDarkenFactor`；`Multiply_3 = Multiply_0 * Lerp_0`，负责墨色和已有色相影响；不把亮暗调制接入 Interaction 路径。 |
| `If_0..If_3` | 由 `ArtEdgeDebugView` 选择调试分支；Debug1/2/3 保持已有输出，Debug4 通过 `Multiply_16` 读回新的层级 mask。 |

### Detector 公式核对

`MF_LR_ArtEdgeDetector` 内部已确认存在四个输出：`RawDepthEdge`、`RelativeDepthEdge`、`NormalEdge`、`FinalArtEdge`。本次只使用前两个合适的层级输出，保留函数本身不变：

```text
RawDepthEdge      = Saturate(RawDepthDelta / max(DepthEpsilon, 1e-6))
RelativeDepthEdge = SmoothStep(DepthThreshold,
                               DepthThreshold * 2,
                               RawDepthDelta / max(CenterDepth, 1e-6))
NormalEdge        = SmoothStep(NormalThreshold,
                               NormalThreshold * 2,
                               NormalDelta) * InternalEdgeStrength
FinalArtEdge      = max(RawDepthEdge, NormalEdge)
                    * (1 - SmoothStep(DistanceFadeStart,
                                      DistanceFadeEnd,
                                      CenterDepth))
```

`DepthEpsilon` 当前为 1 cm；`RawDepthEdge` 是绝对深度差归一化，不能作为 silhouette 层的稳定来源。`RelativeDepthEdge` 按中心深度归一化后再与阈值比较；本次不把厘米阈值和相对阈值混接。`NormalEdge` 已经乘过 `InternalEdgeStrength`，新增主链不再给它乘第二次权重。

## 获准后的精确操作脚本

以下步骤按顺序执行。步骤 1–3 先保存当前编辑器状态和资产读回结果；本轮已在 P3 截图完成且 root 开放 UE 写权限后执行。

### 1. 新增 Structure 采样宽度参数，不增采样

1. 在 `M_PP_LR_StyleOutline` 新增标量参数 `StructureWidthScale`，组 `10_Outline`，默认值 **0.5**（1080p 的 .5 px 候选），范围建议 `[0.25, 1.0]`，`1.0` 是原有法线邻域宽度回退值。该参数只改变 UV 偏移，不是第二个结构权重。
2. 新增一个 `MF_LR_OutlineKernel` 调用（称为 **Kernel Structure**）。它的 `ScreenUV` 和 `InvSize` 与 Kernel 0 完全相同。
3. 在 Kernel Structure 的 `WidthPx` 前加入乘法：`OutlineWidthPx * StructureWidthScale`。
4. 仅把 `SceneTexture_7..10` 的 UV 改接到 Kernel Structure 的 `UVLeft/Right/Up/Down`；`SceneTexture_2..5` 继续使用 Kernel 0，中心深度和中心法线不改。
5. 读回确认：图中仍只有中心深度 + 4 深度邻域、中心法线 + 4 法线邻域；新增 Kernel 只算 UV，不产生 SceneTexture 采样。

### 2. 构造分层 mask，统一做一次 fade

1. 从 `MFcall1.RelativeDepthEdge` 和 `MFcall1.NormalEdge` 接入 `Max` 节点：

   ```text
   EdgeLayers = Max(RelativeDepthEdge, NormalEdge)
   ```

   `RelativeDepthEdge` 作为 silhouette 来源，默认自然接近 1；`NormalEdge` 复用已有 `InternalEdgeStrength`，作为 Structure 来源。不要新增 `SilhouetteWeight` 或 `StructureWeight`。

2. 在主图重新计算一次距离 fade，使用现有参数和中心深度：

   ```text
   Fade01     = SmoothStep(DistanceFadeStart,
                           DistanceFadeEnd,
                           SceneTexture_1.Color)
   DistanceFade = 1 - Fade01
   ```

3. 将 `EdgeLayers * DistanceFade` 再接 `Saturate`，命名为 `LayeredEdgeMask`：

   ```text
   LayeredEdgeMask = Saturate(Max(RelativeDepthEdge, NormalEdge)
                              * (1 - SmoothStep(DistanceFadeStart,
                                                DistanceFadeEnd,
                                                CenterDepth)))
   ```

   只在这个合并点乘一次 fade；不在 Relative/Normal 分支分别乘 fade，也不使用 `MFcall1.FinalArtEdge`，避免 RawDepth 分支重新进入 Style 层。

4. 先让 `Multiply_16.A = LayeredEdgeMask`，保留 `Multiply_16.B = Subtract_13`。因此 Cutaway 抑制继续只作用于最终 Style alpha，`LayeredEdgeMask * Subtract_13` 的现有功能不变。
5. 该接线让 Debug4（`ArtEdgeDebugView=4`）读到 `LayeredEdgeMask * Subtract_13`；Debug1/2/3 的已有调试输出保持原接线，便于判断 Cutaway 与层级差异。

   UE MCP 的 `get_expression_inputs` 对同一多输出函数节点存在只按源节点返回首个 output name 的读回歧义。为让图上连线可独立审查，实际使用 `Multiply_20 = MFcall1.NormalEdge * Constant_10(1)` 作为无采样适配器，再接入 `Max_17.B`；适配器输入读回为 `NormalEdge`，不改变共享函数或采样。

### 3. Style 专属亮暗调制

新增四个标量参数，组 `10_Outline`，均可在 MI 中调节：

| 参数 | 默认候选 | 建议范围 | 语义 |
| --- | ---: | ---: | --- |
| `OutlineLightDarkStrength` | 0.15 | `[0, 0.35]` | Style mask 的亮暗弱调制幅度；0 只关闭此调制。 |
| `OutlineLumaReference` | 1.0 候选 | `> 0`，编辑器下限 `0.001` | 当前 AfterDOF HDR/有效预曝光 SceneColor 的亮度参考尺度；不代表阴影阈值。 |
| `OutlineLumaLow` / `OutlineLumaHigh` | 0.2 / 0.7 候选 | 归一化亮度域 `[0,1]`，且 `High > Low` | 由暗到亮的过渡区间；两者都是独立调优参数，不在图中硬编码最终阈值。 |

实现公式（采用有效预曝光 SceneColor 的稳定归一化，不宣称真实阴影检测）如下：

```text
SceneRGB       = max(SceneColorRGB, 0)
LumaHDR        = Dot(SceneRGB, (0.2126, 0.7152, 0.0722))
Luma01         = LumaHDR / max(LumaHDR + OutlineLumaReference, 1e-6)
Dark01         = 1 - SmoothStep(OutlineLumaLow,
                                OutlineLumaHigh,
                                Luma01)
LightDark      = Lerp(1 - OutlineLightDarkStrength,
                       1 + OutlineLightDarkStrength,
                       Dark01)
StyledMask     = Saturate(LayeredEdgeMask * LightDark)
```

`1e-6` 只作为数学除法保护常量；`OutlineLumaReference`、`OutlineLumaLow`、`OutlineLumaHigh` 和强度由材质实例提供。由于材质保持 `bDisablePreExposureScale=false`，参数参考的是当前有效预曝光空间；本轮以可调参考值完成实现，后续视觉校准仍需结合曝光、自发光和 albedo 判断。不得把固定的最终屏幕 0..1 阈值写死，也不得把亮暗因子接到 InteractionOutline。

接线顺序：

1. 从当前 SceneColor RGB（`ComponentMask_0`）生成 `LumaHDR`，依次接入 `max`、加法/除法、`SmoothStep` 和 `OneMinus`。
2. 用 `OutlineLightDarkStrength` 生成 `1-strength` 与 `1+strength`，以 `Dark01` 驱动 `Lerp` 得到 `LightDark`。
3. `LayeredEdgeMask * LightDark` 后 `Saturate` 得 `StyledMask`。
4. `Multiply_16.A = StyledMask`，仍保留 `Multiply_16.B = Subtract_13`。亮暗调制只改变 StyleOutline 的 alpha/墨色强度，不改变 Perception gate、Cutaway 数据源或 Interaction 白色提示。
5. `OutlineLightDarkStrength=0` 时，`LightDark=1`，故 `StyledMask=LayeredEdgeMask`；这不是 P0 的完整旧图回退。

### 4. 参数和接线读回

授权写入后，必须在 UE 中读回以下事实：

- `M_PP_LR_StyleOutline` 仍为 AfterDOF、priority 0、`bDisablePreExposureScale=false`。
- Kernel 0 的 WidthPx 仍为 `OutlineWidthPx`；Kernel Structure 的 WidthPx 为 `OutlineWidthPx * StructureWidthScale`。
- 四个 SceneDepth taps 仍由 Kernel 0 提供，四个 WorldNormal taps 仅改为 Kernel Structure；SceneTexture 节点数量不增加。
- `MF_LR_ArtEdgeDetector` 的输入参数和函数资产未被修改；`RelativeDepthEdge` 与 `NormalEdge` 只在合并点各使用一次，`InternalEdgeStrength` 未双乘。
- 新主链为 `Max(RelativeDepthEdge, NormalEdge) -> one shared DistanceFade -> Saturate -> LightDark -> Multiply_16.A`。
- `Multiply_16.B` 仍是 `Subtract_13`，Debug4 能读到新 Style mask，`LR_NormalOutlineGate` 仍在最终 `Lerp_2`。
- InteractionOutline 资产/材质未被编辑，且没有任何新亮暗节点连接到其白色提示链。
- MI `MI_PP_LR_LivingRoomOutline` 具有新参数并保留旧参数；当前跟进值为 `OutlineWidthPx=2`、`StructureWidthScale=0.5`，其余旧参数为 `DepthThreshold=0.002`、`NormalThreshold=0.18`、`InternalEdgeStrength=0.45`、`DistanceFadeStart=1000`、`DistanceFadeEnd=4000`、`OutlineDarkenFactor=0.55`、`OutlineBaseHueInfluence=0.15`。新参数的实际参考值必须记录在验证文档，不得凭空声称已完成标定。

### 5. 编译和定向验收

在写入完成后执行材质重编译和与本次修改直接相关的 PIE 验收；材质图改动不要求 `LostRunicEditor` C++ 全构建，只有后续加入 C++/P5 时才升级为构建验证。视觉主验收使用 `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo` 和现有用户镜头/PlayerStart；`/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 仅在需要确认共享 Interaction/Perception/Cutaway 门控时做定向冒烟，不为本次修改改变该关卡镜头或布局。

验收记录至少包含：

1. `ArtEdgeDebugView=4`：silhouette 使用 RelativeDepthEdge，structure 受 `InternalEdgeStrength` 控制且采样邻域宽度可由 `StructureWidthScale=0.5` 收细；0.5 是邻域宽度候选，不自动等于最终屏幕 0.5px 线宽。结合 GBuffer 点采样和 TAA 检查边缘稳定性，记录实际观察，不预宣称无闪烁；无随机噪声。
2. `OutlineLightDarkStrength=0`：Style 亮暗调制关闭，结构 mask 仍存在；亮暗调制不影响 Interaction 白色外轮廓。
3. 调整 `OutlineLumaReference`、`OutlineLumaLow`、`OutlineLumaHigh` 和 `OutlineLightDarkStrength` 后亮暗变化有界；说明这是基于 HDR/有效预曝光 SceneColor 的弱调制，受曝光、自发光和 albedo 影响，不是真实阴影检测。
4. Cutaway 开关/过渡时 `Subtract_13` 仍抑制 Style mask；Perception/NormalGate 关闭时最终 Style 输出仍被门控。
5. Output Log 无项目级 Warning/Error；编译/PIE 截图、参数读回和限制写入 `Docs/Art/06_OutlineHierarchyValidation.md`。

## 实际执行结果（2026-09-08）

- 写入前复核到 `D:/25DGame/LostRunic/LostRunic.uproject` 对应的 MCP 实例；当前关卡为 `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo`，未操作另一 UE 进程。
- `StructureWidthScale` 为 `ScalarParameter_9=0.5`；`MaterialFunctionCall_2` 使用 `Multiply_17 = OutlineWidthPx * StructureWidthScale`，仅把 `SceneTexture_7..10` 改接 Kernel 2，深度四 taps 保持 Kernel 0。
- 层级节点为 `Max_17`、`SmoothStep_14`、`Subtract_22`、`Multiply_18`、`Saturate_4`；`Max_17.A=RelativeDepthEdge`，`Max_17.B=Multiply_20`，其中 `Multiply_20.A=NormalEdge`、`B=Constant_10(r=1)`。统一 fade 后接 `Saturate_4`，再进入 Style 亮暗链。
- 亮暗参数为 `ScalarParameter_10..13`：`OutlineLightDarkStrength=0.15`、`OutlineLumaReference=1.0`、`OutlineLumaLow=0.2`、`OutlineLumaHigh=0.7`；节点为 `Max_19`、`DotProduct_0`、`Add_4`、`Max_18`、`Divide_1`、`SmoothStep_15`、`Subtract_23/24`、`Add_5`、`LinearInterpolate_3`、`Multiply_19`、`Saturate_5`。
- `Multiply_16.A=Saturate_5`，`Multiply_16.B=Subtract_13`；`Lerp_1.Alpha` 仍为 `Multiply_16`；`Lerp_2` 仍以 `LR_NormalOutlineGate` MPC Collection 参数作为最终门控；`If_3` 的 Debug4 equality 分支仍读 `Multiply_16`。
- `M_PP_LR_StyleOutline` 属性读回为 `MD_PostProcess`、`BL_SceneColorAfterDOF`、`bDisablePreExposureScale=false`、`bIsBlendable=true`；MI 暴露 14 个标量参数，当前 `OutlineWidthPx=2`、`StructureWidthScale=0.5`，其余旧参数值保持 `DepthThreshold=0.002`、`NormalThreshold=0.18`、`InternalEdgeStrength=0.45`、`DistanceFadeStart=1000`、`DistanceFadeEnd=4000`、`ArtEdgeDebugView=0`、`OutlineDarkenFactor=0.55`、`OutlineBaseHueInfluence=0.15`。
- 材质重编译返回成功并保存母材质；未进行 C++ 构建，未创建 P5 NormalVolume，未改贴图、模型、布局、镜头或 InteractionOutline。
- PIE 使用现有关卡 PlayerStart 启动后停止；证据文件为 `ArtSource/Evidence/ArtPipeline_20260908/After_P4_PIE.png`（有效 PNG，4,550,035 bytes），未覆盖 `Before_PIE.png` 或 `After_PIE.png`。
- 2026-09-09 将 MI `OutlineWidthPx` 调为 `2` 后，使用临时 `startTransform=(-150,180,100)`、yaw `0` 做中心姿态 PIE 复核；另存 `ArtSource/Evidence/ArtPipeline_20260909/After_P4_2px_PIE_Center.png`（有效 PNG，4,588,299 bytes），未保存或修改 PlayerStart，完成后停止 PIE。
- 当前限制：本轮 `AfterDOF` HDR/有效预曝光 SceneColor 亮暗调制只作为可调弱调制，不能区分真实阴影、曝光、自发光和 albedo；`StructureWidthScale=0.5` 只是 WorldNormal 采样邻域候选，不等同最终屏幕 0.5px 线宽，也未预宣称消除 GBuffer/TAA 闪烁。Debug4 的地板/边缘分布及参数校准由 root 复核截图后决定。
