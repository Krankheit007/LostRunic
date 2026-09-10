# P4 StyleOutline 层级验证记录

**执行日期**：2026-09-08（MI 轮廓宽度跟进：2026-09-09）
**资产**：`/Game/LostRunic/Materials/PostProcess/M_PP_LR_StyleOutline`
**实例读回**：`/Game/LostRunic/Materials/Instances/Scenes/MI_PP_LR_LivingRoomOutline`
**操作记录**：`Tools/ArtOutlineHierarchy/OutlineHierarchyPlan.md`

## 结果范围

本轮只修改 StyleOutline 母材质的后处理图，完成了分层边缘 mask、WorldNormal 结构邻域缩放和 Style 专属亮暗弱调制。InteractionOutline 的白色交互提示、交互状态控制、贴图、模型、关卡布局和用户镜头均未改动。P0 基线 `ArtSource/Baselines/ArtBaseline_20260907` 未改动；P5 NormalVolume 未创建。

2026-09-09 的跟进只修改了 `MI_PP_LR_LivingRoomOutline` 的 `OutlineWidthPx` 实例值，将主轮廓参数从 `1` 调为 `2`，保留 `StructureWidthScale=0.5` 和全部现有层级/门控接线。母材质图没有再次写入。

材质图从 176 个表达式节点扩展到 195 个。没有修改 `MF_LR_ArtEdgeDetector` 或 `MF_LR_OutlineKernel` 的资产、接口和行为，也没有增加 SceneTexture 采样节点。

## 实际图结构

### 结构邻域

- `ScalarParameter_9`：`StructureWidthScale`，组 `10_Outline`，默认 `0.5`，范围 `[0.25, 1.0]`。
- 当前 MI 主轮廓 `OutlineWidthPx=2`；因此 Kernel 2 的结构邻域宽度为 `2 * 0.5 = 1` 的采样宽度候选。
- `MaterialFunctionCall_0` 仍调用 `MF_LR_OutlineKernel`，`WidthPx=OutlineWidthPx`，继续服务四个 SceneDepth 邻域采样 `SceneTexture_2..5`。
- `Multiply_17 = OutlineWidthPx * StructureWidthScale`。
- `MaterialFunctionCall_2` 仍调用同一个 `MF_LR_OutlineKernel`，`ScreenUV` 和 `InvSize` 与 Kernel 0 相同，`WidthPx=Multiply_17`。
- 四个 WorldNormal 邻域采样 `SceneTexture_7..10` 改接 Kernel 2 的 `UVLeft/UVRight/UVUp/UVDown`；中心 `SceneTexture_6` 不变。中心 normal 加四个邻域 normal taps、四个 depth taps 的数量保持不变。

### 层级 mask

当前层级链为：

```text
RelativeDepthEdge ─────────────┐
                               Max_17 ─ Multiply_18 ─ Saturate_4
NormalEdge ─ Multiply_20(×1) ─┘             ▲
                                            │
                     1 - SmoothStep_14(CenterDepth,
                                        DistanceFadeStart,
                                        DistanceFadeEnd)
```

`Max_17.A` 读回为 `MF_LR_ArtEdgeDetector.RelativeDepthEdge`。由于 UE MCP 的 `get_expression_inputs` 在同一多输出函数节点被多个输入引用时，内部按源节点查询首个 output name，直接把 `NormalEdge` 接到 `Max_17.B` 的读回会显示为首个输出名。实际图使用 `Multiply_20` 作为可审查的无采样适配器：`Multiply_20.A=MFcall1.NormalEdge`，`Multiply_20.B=Constant_10(r=1)`，再把其默认输出接到 `Max_17.B`。因此读回明确区分了 RelativeDepthEdge 和 NormalEdge，且没有改变共享函数或增加采样。

`SmoothStep_14` 使用现有 `DistanceFadeStart`、`DistanceFadeEnd` 和中心 SceneDepth，`Subtract_22` 计算一次反向 fade，`Multiply_18` 在合并点统一乘 fade。没有使用 RawDepthEdge，也没有新增 `SilhouetteWeight` 或 `StructureWeight`；`NormalEdge` 已经包含 `InternalEdgeStrength`，本轮没有再次乘该参数。

`Multiply_16.A` 最终接 `Saturate_5` 的 Style mask，`Multiply_16.B` 仍接 `Subtract_13` 的 Cutaway 抑制。`Lerp_1.Alpha` 仍为 `Multiply_16`，所以 Cutaway 继续作用于 Style alpha。

### Style 亮暗调制

亮暗链只作用于 Style mask：

```text
SceneColorRGB -> Max_19(., 0) -> DotProduct_0(0.2126, 0.7152, 0.0722)
             -> Add_4(+OutlineLumaReference)
             -> Max_18(., 1e-6)
             -> Divide_1
             -> SmoothStep_15(OutlineLumaLow, OutlineLumaHigh)
             -> Subtract_23(1 - Dark01)

Lerp_3(A=1-strength, B=1+strength, Alpha=Dark01)
Multiply_19(Saturate_4, Lerp_3) -> Saturate_5 -> Multiply_16.A
```

四个新增标量参数均由母材质暴露给 MI，当前有效值为：

| 参数 | 当前值 | 范围/用途 |
| --- | ---: | --- |
| `OutlineLightDarkStrength` | `0.15` | `[0, 0.35]`；0 只关闭亮暗调制 |
| `OutlineLumaReference` | `1.0` | 正 HDR/有效预曝光参考尺度，下限 `0.001` |
| `OutlineLumaLow` | `0.2` | 归一化亮度过渡下界 |
| `OutlineLumaHigh` | `0.7` | 归一化亮度过渡上界 |

这里的 SceneColor 位于 `BL_SceneColorAfterDOF`，仍是 tonemapping 前的 HDR/有效预曝光空间；`bDisablePreExposureScale=false` 保持不变。因此这条链提供的是有界的亮暗调制，受曝光、自发光和物体 albedo 影响，不能作为真实阴影检测。参数值是本轮候选起点，后续 P3 灯光校准可以调整。

## 资产和门控读回

`M_PP_LR_StyleOutline` 读回为：

- `materialDomain=MD_PostProcess`
- `blendableLocation=BL_SceneColorAfterDOF`
- `bDisablePreExposureScale=false`
- `bIsBlendable=true`
- `MP_EmissiveColor` 仍由 `Lerp_2` 驱动
- `Lerp_2.A=ComponentMask_0`、`Lerp_2.B=If_3`、`Lerp_2.Alpha=CollectionParameter_0`（`LR_NormalOutlineGate`）
- `If_3` 的 equality 分支仍接 `Multiply_16`，用于 Debug4；Debug1/2/3 的原调试选择链未改
- `Multiply_16.B=Subtract_13`，`Subtract_13=1-Max_16` 的 Cutaway 来源未改

MI 的 14 个标量有效值如下；旧参数由实例继承/保留：

| 参数 | 有效值 |
| --- | ---: |
| `OutlineWidthPx` | `2.0`（2026-09-09 MI 跟进） |
| `DepthThreshold` | `0.002` |
| `NormalThreshold` | `0.18` |
| `InternalEdgeStrength` | `0.45` |
| `DistanceFadeStart` | `1000` |
| `DistanceFadeEnd` | `4000` |
| `ArtEdgeDebugView` | `0` |
| `OutlineDarkenFactor` | `0.55` |
| `OutlineBaseHueInfluence` | `0.15` |
| `StructureWidthScale` | `0.5` |
| `OutlineLightDarkStrength` | `0.15` |
| `OutlineLumaReference` | `1.0` |
| `OutlineLumaLow` | `0.2` |
| `OutlineLumaHigh` | `0.7` |

## 编译和 PIE

- `MaterialTools.recompile` 对 `M_PP_LR_StyleOutline` 返回成功；随后只保存该母材质。没有进行 `LostRunicEditor` C++ 构建。
- 写入前复核到 `D:/25DGame/LostRunic/LostRunic.uproject` 对应的 UE/MCP 实例；当前关卡为 `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo`。没有操作另一 UE 项目或进程。
- 在 `L_Art_Demo` 使用现有 PlayerStart 启动 PIE，未传入生成位置、旋转或 FOV 覆盖；完成一帧 Slate 截图后停止 PIE。
- 截图证据：`ArtSource/Evidence/ArtPipeline_20260908/After_P4_PIE.png`，有效 PNG，大小 `4,550,035` bytes；未覆盖 `Before_PIE.png` 或 `After_PIE.png`。此前同名 0 字节临时文件不计入证据。
- 2026-09-09 参数跟进：操作前确认当前关卡为 `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo` 且日志项目路径为 `D:/25DGame/LostRunic/LostRunic.uproject`；读取 MI 原值 `OutlineWidthPx=1`、`StructureWidthScale=0.5`，只设置 `OutlineWidthPx=2`，保存返回成功，回读为 `OutlineWidthPx=2`、`StructureWidthScale=0.5`，MI `is_dirty=false`。本次没有修改 InteractionOutline 参数，也没有改镜头、贴图或布局。
- 2026-09-09 2px 中心姿态复核：使用 `L_Art_Demo` 的临时 PIE `startTransform`（位置 `(-150,180,100)`、yaw `0`、单位缩放）捕获中心画面，未保存或修改 PlayerStart；相机与关卡设置保持原值。证据为 `ArtSource/Evidence/ArtPipeline_20260909/After_P4_2px_PIE_Center.png`，有效 PNG，大小 `4,588,299` bytes；完成后已停止 PIE。
- 本轮没有启动 `L_PIE_Test` 做 Interaction/Perception/Cutaway 状态往返；Debug4 的结构接线已读回，但没有把 `ArtEdgeDebugView` 改为 4 后另存运行时图。移动镜头稳定性和 TAA/GBuffer 点采样观察仍待后续定向验证。

## 限制和后续复核

`OutlineWidthPx=2` 是当前 MI 的主轮廓参数；`StructureWidthScale=0.5` 使结构采样邻域宽度候选为 `1`，不等同最终屏幕栅格中的精确 2px/1px 线宽。当前证据是一帧固定 PlayerStart 画面，不能据此宣称移动时无闪烁。需要后续验证时，优先在同一用户镜头做 Debug4、Cutaway 和 NormalGate 的定向状态切换，并检查斜视大平面不会因 RawDepth 分支产生整面低强度暗化；本轮层级主链已切换到 RelativeDepthEdge 以避免该问题。

未修改贴图和水彩样本，未添加随机噪声、模糊或重滤镜，也未把 Style 亮暗调制接入 InteractionOutline。没有 commit 或 push。
