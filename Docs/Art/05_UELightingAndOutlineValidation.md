# UE Lighting and Outline Validation

日期：2026-09-08
关卡：`/Game/LostRunic/Levels/PIE_Test/L_Art_Demo`
范围：P1 后处理装配、P3 局部灯光与色调小步调优。

## 约束与保存边界

- 所有现有贴图保留，未使用 `WatercolorSamples`，未新建纹理、共享材质或 Shader。
- 未移动、缩放或重排家具、建筑、灯光 Actor 和相机；`DA_LRPresentationTuning.DefaultCameraDistanceCm=500` 保持不变，PIE 使用现有 PlayerStart。
- P0 冻结基线及 `ArtSource/Baselines/ArtBaseline_20260907/` 未修改。实际写入仅为本关卡现有 Actor 的可逆实例覆盖值。
- 保持 DefaultLit、Lumen、VSM；未创建 `ALRNormalVisualStyleVolume`，未改变 Perception/Cutaway 门控。

## P1：Normal 风格线稿与交互描边

实际 PlayerStart 为 `(-290, 180, 100)`。`ArtBench_PPV` 的实时 bounds 为 `(2700, -500, -25)` 到 `(4100, 500, 675)`，不覆盖 Gameplay 起点。因此 Interaction 描边放到全局 PPV，避免玩家区失效。

| 现有 Actor | 保存前 Weighted Blendables | 保存后 Weighted Blendables | 现有 PPV 配置 |
| --- | --- | --- | --- |
| `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo.L_Art_Demo:PersistentLevel.PostProcessVolume_0` (`ArtBench_PPV`) | `MI_PP_LR_StyleOutline_Benchmark` ×1；`MI_PP_LR_InteractionOutline_Diag0707` ×1 | 空 | Priority 100，Blend Radius 0，Blend Weight 1，Unbound=false，Enabled=true |
| `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo.L_Art_Demo:PersistentLevel.PostProcessVolume_1` (`LRScene_PostProcess`) | `MI_PP_LR_LivingRoomOutline` ×1 | `MI_PP_LR_LivingRoomOutline` ×1；`MI_PP_LR_InteractionOutline_Diag0707` ×1 | Priority 20，Blend Radius 100，Blend Weight 1，Unbound=true，Enabled=true |

这样只消除了重复的 StyleOutline 入口，并保留两个独立职责：

- StyleOutline：`MI_PP_LR_LivingRoomOutline`，SceneDepth + WorldNormal。当前实例参数读回为 `OutlineWidthPx=1`、`DepthThreshold=0.002`、`NormalThreshold=0.18`、`InternalEdgeStrength=0.45`、`DistanceFadeStart=1000`、`DistanceFadeEnd=4000`、`OutlineDarkenFactor=0.55`、`OutlineBaseHueInfluence=0.15`。
- InteractionOutline：`MI_PP_LR_InteractionOutline_Diag0707`，CustomDepth + CustomStencil，独立白色材质、参数和交互状态控制；实例读回为宽度 1、对角缩放 `0.70710677`、Depth Bias 1。未与 StyleOutline 合并。

未修改 `M_PP_LR_StyleOutline`、`M_PP_InteractionOutline`、材质实例、Perception/Cutaway 设置或蓝图装配。

## P3：局部灯光与后处理

下表的保存前值来自修改前实时 Actor 值，保存后值为本次 `ObjectTools.set_properties` 后读回值。单位沿用 UE 编辑器（Point/Rect 光源强度为 lm，DirectionalLight 强度为 lux，色温为 K，尺寸为 cm）。

| Actor / 组件 | 保存前 | 保存后 | 目的与未改字段 |
| --- | --- | --- | --- |
| `PointLight_3` / Console | 14 lm，2900 K，Source Radius 12 | 20 lm，2700 K，Source Radius 18 | 小幅暖化并增大光源过渡/高光半径；不改变阴影（CastShadows=false），Attenuation Radius 300、CastDynamicShadows=true 保持 |
| `PointLight_4` / Cabinet | 14 lm，2900 K，Radius 12 | 20 lm，2600 K，Radius 18 | 柜面暖池并增大光源过渡半径；其余阴影状态同上 |
| `PointLight_5` / Sofa | 14 lm，2900 K，Radius 12 | 18 lm，2800 K，Radius 18 | 沙发区域局部暖光并增大光源过渡半径；其余阴影状态同上 |
| `PointLight_6` / SconceLeft | 14 lm，2900 K，Radius 12 | 20 lm，2700 K，Radius 18 | 左侧灯池并增大光源过渡半径；其余阴影状态同上 |
| `PointLight_7` / SconceRight | 14 lm，2900 K，Radius 12 | 20 lm，2700 K，Radius 18 | 右侧灯池并增大光源过渡半径；其余阴影状态同上 |
| `PointLight_8` / FireGlow | 22 lm，2900 K，Radius 12 | 35 lm，2300 K，Radius 24 | 火光作为局部暖焦点并增大发光/高光半径；不改变阴影（CastShadows=false），Attenuation Radius 300、CastDynamicShadows=true 保持 |
| `RectLight_0` / WindowFill | 66 lm，5200 K，180×180 | 100 lm，4300 K，260×220 | 窗侧形成较宽的柔暖窗光；Attenuation Radius 1200、CastShadows=false、CastDynamicShadows=true 保持 |
| `RectLight_1` / CoolFill | 64 lm，8500 K，180×180 | 50 lm，8500 K，260×220 | 保留冷色暗部分离并降低正面填充；Attenuation Radius 1200、CastShadows=false、CastDynamicShadows=true 保持 |
| `DirectionalLight_0` | 12 lux，4800 K，Source Angle 0.8，Soft Angle 0 | 12 lux，4800 K，Source Angle 1.2，Soft Angle 0.3 | 仅软化方向光阴影；CastShadows=true、CastDynamicShadows=true 保持 |
| `SkyLight_0` | 0.5 | 0.5 | 保持全局基准；CastShadows=true、CastDynamicShadows=true 读回 |

远处 Benchmark 光源（`ArtBench_CalibrationLight`、`ArtBench_FillLight`、`ArtBench_KeyLight`、`ArtBench_WindowLight`）没有改动。

`LRScene_PostProcess` 的实例覆盖保存为：`BloomIntensity=0.22`（override=true）、`BloomThreshold=1.0`（override=true）、`ColorSaturationShadows=(1.0,0.98,1.05,1.0)`（override=true）、`ColorGainShadows=(0.94,0.93,1.05,1.0)`（override=true）。原有 `AEM_Manual`、`AutoExposureBias=0`、`DepthOfFieldScale=0`、`MotionBlurAmount=0` 保持。`ArtBench_PPV` 的原有曝光、景深、运动模糊覆盖保持，StyleOutline 入口已清空。

## 雾与体积层

通过 `SceneTools.find_actors` 检查当前关卡，没有 `ExponentialHeightFog`、`VolumetricFog` 或 `AtmosphericFog` Actor。第一轮未引入雾，体积层尚未实现；后续如需要应由独立、受控的小步候选处理。

## 实际操作与证据

1. 确认 PIE 已停止后，将 P1 两个 PPV 的 `settings.weightedBlendables` 和 P3 现有灯光/PPV 实例覆盖写入。
2. 使用 Slate `PressKey(Ctrl+S)` 保存关卡；`AssetTools.is_dirty(/Game/LostRunic/Levels/PIE_Test/L_Art_Demo)=false`。
3. 用 `EditorApp.StartPIE`（`bSimulate=false`、`PlayMode_InViewPort`、Warmup 3 秒）启动真实 PIE，使用 `SlateInspectorToolset.Screenshot(ref=w1)` 保存完整编辑器窗口中的实际 PIE viewport：
   - 修改前画面：[Before_PIE.png](../../ArtSource/Evidence/ArtPipeline_20260908/Before_PIE.png)
   - 修改后画面：[After_PIE.png](../../ArtSource/Evidence/ArtPipeline_20260908/After_PIE.png)
4. 修改前画面是在最终 P3 写入前恢复实时 baseline 值后捕获，恢复操作未保存；修改后画面是在最终写入并保存后捕获。两次均确认标准 PIE 启动、TuningSet 初始化和正常退出，最后 `IsPIERunning=false`。

Output Log 的本次 PIE 记录未见 Fatal/Error 字样；日志中存在既有 `LogLostRunicCutaway` 警告（多个 CoolPlaster 槽位要求 Masked，以及 `StaticMeshActor_91` 的 Affected Primitive 无效，导致 Cutaway writes disabled），另有 MCP 属性探查产生的 `LogJson`/`LogScript` schema warnings。这些均未由本次灯光或 PPV 写入修复或掩盖，不能据此宣称 Cutaway/门控或 PIE“零 Warning”通过。

本次未运行构建、全量测试或修改 `Docs/Technical/06_BlueprintConfigurationGuide.md`；没有新增蓝图装配或代码接口。所有 Actor 覆盖均可按上表保存前数值恢复。

## P3 第二轮候选（只读设计，未写入）

以下参数已提交 root 评审，当前关卡没有应用这些值，不能作为已实现效果或测试通过项：

- `DirectionalLight_0`：Source Angle 1.2° → 3.0°（可先用 2.5° A/B），Soft Angle 0.3° → 0.5°；保持 12 lux、CastShadows/CastDynamicShadows 和 VSM 设置。风险是半影变宽、远处细节变软以及少量 VSM 成本。
- 仅对 `LRScene_PostProcess` 阴影通道做蓝紫 lift：`ColorSaturationShadows=(1.02,0.96,1.10,1)`、`ColorGainShadows=(0.96,0.90,1.08,1)`、`ColorOffsetShadows=(0.004,0.002,0.010,0)`，亮部/中间调/曝光保持。风险是暖木材阴影偏紫或通道裁剪，需 PIE A/B。
- `CoolFill`：50 → 35 lm，保留 8500 K 与 260×220，候选 `LightColor=(0.72,0.78,1.0,1)`；`WindowFill` 保持 100 lm/4300 K/260×220，候选 `LightColor=(1.0,0.86,0.70,1)`；若环境仍过平，`SkyFill` 0.5 → 0.40。颜色与色温叠加可能过饱和，Sky/Lumen 间接光降低可能压暗可读性。
- 单一新 `ExponentialHeightFog` 候选：Fog Density 0.004、Height Falloff 0.15、Fog InscatteringColor=(0.80,0.84,1.0)；开启 Volumetric Fog，Albedo=(0.78,0.84,1.0)、Extinction Scale 0.15、Scattering Distribution 0.25、Start Distance 100、Near Fade In 200、View Distance 2500 cm；Directional Inscattering 保持中性/关闭。风险是 GPU 体积 pass、室内雾化轮廓，以及 RectLight 对体积贡献有限；需真实 PIE 验证，不等同于颜料水彩。
- 先确认实际主窗光贡献后，若窗光仍平直，再新建独立 Light Function 域材质 `M_LR_WindowLightFunction_LowFreq`，只挂到 `LRScene_WindowFill.LightFunctionMaterial`。使用一层、最多两层固定低频 Noise（Scale≈0.003 cm⁻¹、Levels=2、固定坐标/种子、无 Time/Panner/逐帧随机），输出 0.82..1.0 灰度、Strength 0.18，平均能量接近不变。风险是投影方向/RectLight 支持、重复纹理块和额外成本；清空引用即可回滚，不触碰任何表面贴图。

第二轮建议顺序：Directional 软阴影与 CoolFill 有色降平 → 阴影蓝紫 → SkyFill → 单雾候选 → 单窗光 LightFunction；每步独立保存、读回并捕获真实 PIE A/B。

## P3 第二轮实际执行（2026-09-09）

root 释放 P4 写权后，本轮在已核实的 `L_Art_Demo` 上执行了候选顺序中的方向光柔化、冷填充有色降平、阴影通道蓝紫 lift、SkyFill 降低和一个受控的 `ExponentialHeightFog` 体积候选。没有创建或引用 Light Function；`WindowFill.LightFunctionMaterial` 读回仍为 `None`，保留给后续基于画面需要的独立 A/B。

本轮只写入现有灯光、`LRScene_PostProcess` 的实例覆盖，以及新建的雾 Actor；没有改变家具、建筑、Actor 位置、保存的 PlayerStart 或相机。强度单位已按 UE 实时属性记录：Point/Rect 为 lm，`DirectionalLight` 为 lux。

| Actor / 组件 | 第二轮写入前 | 第二轮写入后（读回） | 有效阴影与保留字段 |
| --- | --- | --- | --- |
| `DirectionalLight_0` / `DirectionalLightComponent0` | `Source Angle=1.2°`，`Soft Angle=0.3°`，12 lux，4800 K | `Source Angle=3.0°`，`Soft Angle=0.5°`，12 lux，4800 K | `CastShadows=true`、`CastDynamicShadows=true`；`VolumetricScatteringIntensity=1` 保持。方向光柔影来自有效的 CastShadows 开关与角度，未把无效的 SourceRadius 解释为阴影开关。 |
| `RectLight_1` / CoolFill | 50 lm，8500 K，260×220 cm，默认白色 | 35 lm，8500 K，260×220 cm，`LightColor=(0.7216,0.7804,1.0,1)` | `CastShadows=false`，所以不投影阴影；`CastDynamicShadows=true` 单独保留但不能覆盖总开关；Attenuation Radius 1200 cm 保持。 |
| `RectLight_0` / WindowFill | 100 lm，4300 K，260×220 cm，默认白色 | 100 lm，4300 K，260×220 cm，`LightColor=(1.0,0.8588,0.7020,1)` | `CastShadows=false`，`CastDynamicShadows=true`，Attenuation Radius 1200 cm 保持；Light Function 未挂载。 |
| `SkyLight_0` / SkyLightComponent0 | Intensity 0.5 | Intensity 0.4 | `CastShadows=true`、`CastDynamicShadows=true` 保持。 |

`LRScene_PostProcess` 保留 Priority 20、Blend Radius 100 cm、Blend Weight 1、Unbound=true、Enabled=true，Style 与 Interaction 两个 Weighted Blendables 各保留一个。第二轮只扩展阴影通道覆盖：

- `ColorSaturationShadows=(1.02,0.96,1.10,1.0)`，override=true；
- `ColorGainShadows=(0.96,0.90,1.08,1.0)`，override=true；
- `ColorOffsetShadows=(0.004,0.002,0.010,0.0)`，override=true。

`BloomIntensity=0.22`、`BloomThreshold=1.0`、`AutoExposureBias=0`、`DepthOfFieldScale=0`、`MotionBlurAmount=0` 沿用第一轮值，没有用 Bloom 代替体积层。

### 受控雾候选

新增 Actor：`/Game/LostRunic/Levels/PIE_Test/L_Art_Demo.L_Art_Demo:PersistentLevel.ExponentialHeightFog_0`，组件为 `HeightFogComponent0`，文件夹为 `Lighting`。编辑器生成的实例路径保留为 `ExponentialHeightFog_0`；请求的显示标签 `LR_P3_VolumeFog` 没有覆盖这个对象路径。其实际保存值为：

```text
FogDensity=0.004
FogHeightFalloff=0.15
FogInscatteringLuminance=(0.80,0.84,1.0,1)
bEnableVolumetricFog=true
VolumetricFogScatteringDistribution=0.25
VolumetricFogAlbedo=(0.7804,0.8392,1.0,1)
VolumetricFogExtinctionScale=0.15
VolumetricFogDistance=2500 cm
VolumetricFogStartDistance=100 cm
VolumetricFogNearFadeInDistance=200 cm
bOverrideLightColorsWithFogInscatteringColors=false
bEnableFSSS=false
```

该 Actor 是本轮唯一新增的雾/体积 Actor；用途是以低密度蓝冷散射增加空间层次，参数可直接在 `HeightFogComponent0` 的 Exponential Height Fog 与 Volumetric Fog 分类中回退。它不改变任何表面贴图，也不代表颜料水彩材质已经实现。

### 中心 Gameplay A/B 证据

为让窗、壁炉、沙发和地面同时进入画面，本轮使用临时 PIE `startTransform` 平移出生位置到 `(-150,180,100)` cm，Yaw 保持实际读出的 0°；保存的 PlayerStart 仍为 `(-290,180,100)`，相机绝对旋转、`500 cm / -52° / FOV 68°` 未改。两张正式对比图均为真实 `PlayMode_InViewPort` PIE 画面、同一姿态，且在停止 PIE 后确认 `IsPIERunning=false`：

- Before（第二轮写入前）：[Before_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/Before_PIE_Center.png)
- After（第二轮参数与雾保存后）：[After_P3R2_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/After_P3R2_PIE_Center.png)

早先批准的 `(0,300,100)`、Yaw 0° 临时姿态会把相机置于墙体/近裁剪面，得到的 [Before_PIE_Center_InvalidWall.png](../../ArtSource/Evidence/ArtPipeline_20260909/Before_PIE_Center_InvalidWall.png) 只作失败诊断，不能与正式 Before/After 比较，也没有覆盖正式证据。

保存步骤使用 Slate `Ctrl+S`；保存后 `AssetTools.is_dirty(/Game/LostRunic/Levels/PIE_Test/L_Art_Demo)=false`。实时回读确认 Directional、CoolFill、WindowFill、SkyLight、Fog 和 PP 阴影覆盖均为上表/清单值。

本次 PIE 的相关日志仍包含已有 `LogLostRunicCutaway` 警告（CoolPlaster 槽位要求 Masked、`StaticMeshActor_91` 的 Affected Primitive 无效，Cutaway writes disabled）；历史日志还留有既有 `SaveAnchor` 空/重复 ID ensure。`04:17:47` 这次第二轮启动到 `04:17:54` 退出区间未见新增 Error/Fatal，但不能据此宣称 Cutaway 门控或整体 PIE 零 Warning 通过。

若 root 视觉复核仍需要窗光斑驳，再单独制作 `M_LR_WindowLightFunction_LowFreq` 并只对实际主窗光做 A/B；候选限制为固定坐标的一至两层低频 Noise、输出 0.82..1.0、Strength 0.18、无 Time/Panner/逐帧随机，且随时清空引用回退。

## WindowFill 回白单变量 A/B（2026-09-09）

root 视觉复核指出 R2 亮部整片偏橙黄，因此执行了一个单变量 A/B：只将 `RectLight_0 / LRScene_WindowFill / LightComponent0.LightColor` 从 `(1.0,0.8588,0.7020,1)` 恢复为 `(1.0,1.0,1.0,1)`。100 lm、4300 K、260×220 cm、Attenuation Radius 1200 cm、`CastShadows=false`、`CastDynamicShadows=true`、Movable 和 `LightFunctionMaterial=None` 均保持。没有同时创建 Light Function 或改变任何其他灯光/PP 参数。

保存后实时读回为：

```text
Intensity=100 lm
Temperature=4300 K
LightColor=(1.0,1.0,1.0,1.0)
SourceWidth=260 cm; SourceHeight=220 cm
AttenuationRadius=1200 cm
CastShadows=false; CastDynamicShadows=true
LightFunctionMaterial=None
Mobility=Movable
```

使用与 R2 相同的临时 Gameplay 姿态 `(-150,180,100)` cm、Yaw 0° 和 `500 cm / -52° / FOV 68°` 捕获真实 PIE 对照；R2 图保留为原证据，白色 A/B 单独保存为 [After_P3R2_WindowWhite_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/After_P3R2_WindowWhite_PIE_Center.png)。A/B 图尺寸为 2560×1380，与 R2 图的 RGB mean absolute diff 为 `(1.5338,3.2267,3.8337)`，63.50% 像素发生变化；该统计只说明颜色确实进入渲染结果，视觉取舍交由 root 审查。

本次 `Ctrl+S` 后 `AssetTools.is_dirty(/Game/LostRunic/Levels/PIE_Test/L_Art_Demo)=false`，随后 `IsPIERunning=false`。回滚只需将上述 `LightColor` 恢复为 R2 的 `(1.0,0.8588,0.7020,1)`；R2 的 [After_P3R2_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/After_P3R2_PIE_Center.png) 未覆盖。

## Directional Light Function 最小 A/B（2026-09-09）

root 复核 R2 画面后批准只对实际主窗光方向源做一个最小 Light Function 候选。本步没有改变 WindowFill 白色 A/B、CoolFill、雾、PP、家具/建筑、纹理、Interaction 或相机；只新增并挂载一个独立的方向光函数材质，便于观察固定低频斑驳对 Directional 投影的实际贡献。

新增材质资产：`/Game/LostRunic/Materials/Lighting/M_LR_DirectionalWindowLightFunction_LowFreq`。编译前回读并设置为 `MaterialDomain=MD_LightFunction`、`BlendMode=BLEND_Opaque`、`ShadingModel=MSM_Unlit`，输出接 `MP_EmissiveColor`。`bForceCompatibleWithLightFunctionAtlas=false`，未强制 atlas 兼容；具体执行路径未通过 GPU 捕获验证，因此不作路径或接缝结论。`MaterialTools.recompile` 返回成功且无异常，资产标签回读为 `MaterialDomain=MD_LightFunction`、`ShadingModels=MSM_Unlit`、`BlendMode=BLEND_Opaque`。

材质图节点与坐标如下，节点均位于该新材质内：

| 节点 | 编辑器坐标 | 参数/连线 |
| --- | --- | --- |
| `TextureCoordinate` | `(-900,0)` | `coordinateIndex=0`，UV 平铺 1×1；读取 Light Function UV |
| `Constant` | `(-900,180)` | `r=1.0`，同时作为 AppendVector 的 Z 与 Subtract 的 A |
| `AppendVector` | `(-650,0)` | `TextureCoordinate → A`，`Constant(1) → B`，把 UV 与固定 Z 合成 Noise 的 float3 输入 |
| `Noise` | `(-400,0)` | `noiseFunction=NOISEFUNCTION_GradientALU`、`scale=2`、`quality=1`、`levels=1`、`bTurbulence=false`、`outputMin=0`、`outputMax=1`、`bTiling=false` |
| `Saturate` | `(-150,0)` | Noise 输出限幅到 0..1 |
| `OneMinus` | `(50,0)` | 计算 `1-Saturate(Noise)` |
| `ScalarParameter Strength` | `(-150,220)` | 默认 `0.18`，滑块 0..0.5，分组 `LR Window LightFunction` |
| `Multiply` | `(300,80)` | `OneMinus → A`，`Strength → B` |
| `Subtract` | `(550,80)` | `Constant(1) → A`，`Multiply → B`；最终 `1 - Strength*(1-Saturate(Noise))` |

因此 `Strength=0` 时输出恒为 1，可无损关闭调制；本步没有使用 Time、Panner、逐帧随机或多层 Noise。`GradientALU` 是一个包含多条 shader 指令的 Noise 实现，本记录不把它描述成“一条 ALU”，也没有在未实测时给出固定指令数或性能承诺。

编译成功并保存材质后，才将其挂到现有 `DirectionalLight_0/LightComponent0.LightFunctionMaterial`。保存后的引用回读为上述材质路径；Directional 实际 `LightFunctionScale=(1024,1024,1024)`、`LightFunctionFadeDistance=100000 cm`、`DisabledBrightness=0.5`，其它实值保持 `Intensity=12 lux`、`Temperature=4800 K`、`SourceAngle=3.0°`、`SourceSoftAngle=0.5°`、`CastShadows=true`、`CastDynamicShadows=true`、`Mobility=Movable`。WindowFill 的 `LightFunctionMaterial` 仍为 `None`；Rect 不是本轮窗格阴影来源，因此本轮选择 Directional 做 A/B，没有据此推断 Rect Light Function 在其它路径上无效。

使用与 R2/WindowWhite 相同的真实 `PlayMode_InViewPort` PIE 中心姿态 `startTransform=(-150,180,100)` cm、Yaw 0°、相机 `500 cm / -52° / FOV 68°` 捕获，随后停止 PIE 并回读 `IsPIERunning=false`。证据单独保存为 [After_P3R2_DirectionalLF_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/After_P3R2_DirectionalLF_PIE_Center.png)（PNG 4,587,781 bytes，2560×1380），未覆盖任何早先 Before/After。`AssetTools.is_dirty(/Game/LostRunic/Levels/PIE_Test/L_Art_Demo)=false`，材质资产也已保存且 dirty=false。

本步已完成材质编译、引用读回、保存和真实 PIE 取证；GPU 指令/帧时间性能实测尚未执行，不能据此宣称性能开销。若视觉复核不接受该候选，回滚步骤是将 `DirectionalLight_0.LightComponent0.LightFunctionMaterial` 清空为 `None`，保存 `L_Art_Demo`；新材质可保留为未引用候选，或由 root 后续决定清理。回滚不触及 WindowWhite、R2 参数、纹理、几何、Interaction 或相机。

## Directional Light Function Scale 诊断 A/B（2026-09-09）

root 复核此前 Directional LF 的 ON/OFF 结果为“仅有轻微广泛压暗，未见明确树叶/笔触斑驳”，因此本次只做尺度诊断，不改变 `Strength` 或其它灯光、材质、PP、雾参数。写入前重新核验的 Unreal MCP 端口为 `127.0.0.1:8000`，监听进程 PID 35120、窗口标题 `LostRunic - 虚幻编辑器`，当前关卡为 `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo`，PIE 已停止，关卡 dirty=false。

UE 5.8 `LightComponent.h` 的 `LightFunctionScale` 属性说明其 X/Y 缩放轴垂直于光源方向，Z 轴沿光源方向（本机引擎源码 `Engine/Source/Runtime/Engine/Classes/Components/LightComponent.h:213-215`）。本次只依据该属性语义做 1024→256 的同值尺度 A/B；未通过 GPU 捕获解析未强制 atlas 兼容路径中的最终 UV/频率对应关系，因此不把尺度变化解释成已验证的世界空间笔触尺寸。

### 操作与实际回读

1. 写入前 Directional `LightFunctionMaterial` 为 `/Game/LostRunic/Materials/Lighting/M_LR_DirectionalWindowLightFunction_LowFreq`，`LightFunctionScale=(1024,1024,1024)`，材质 `Strength=0.18` 保持不动。`Intensity=12 lux`、`Temperature=4800 K`、`LightSourceAngle=3.0°`、`LightSourceSoftAngle=0.5°`、`CastShadows=true`、`CastDynamicShadows=true`、`Mobility=Movable` 全部保持。
2. 只将 Directional `LightComponent0.LightFunctionScale` 临时设为 `(256,256,256)`，未保存该临时尺度；使用真实 `PlayMode_InViewPort` PIE，`warmupSeconds=4`，临时 `startTransform=(-150,180,100)` cm、Yaw/Pitch/Roll=`(0,0,0)`。保存的 PlayerStart、相机 `500 cm / -52° / FOV 68°`、家具/建筑、纹理、StyleOutline 2 px 与独立 Interaction 入口均未改。
3. Slate `Screenshot(ref=w1)` 捕获实际 PIE 窗口并直接从 MCP base64 写入证据文件：[After_P3R2_DirectionalLF_Scale256_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/After_P3R2_DirectionalLF_Scale256_PIE_Center.png)。文件回读为 PNG `2560×1380`、`4,605,762` bytes；该文件没有覆盖此前 R2、WindowWhite 或 LF ON/OFF 证据。
4. 停止 PIE 后将尺度恢复为 `(1024,1024,1024)`，保存限定包 `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo`，并回读：`LightFunctionMaterial` 仍为上述材质、`LightFunctionScale=(1024,1024,1024)`、`Strength=0.18` 未变，`AssetTools.is_dirty(L_Art_Demo)=false`、`IsPIERunning=false`。

Scale256 图仅作为同姿态尺度诊断证据，尚未由本记录自动接纳为生产参数；斑驳质量、可见尺度和是否保留 256 候选由 root 结合图片复核决定。没有修改贴图、几何、相机、Interaction、共享 Shader 或 Perception/Cutaway 门控，也没有新增材质节点或 Light Function 层。

## Directional Light Function ON/OFF 实际贡献 A/B（2026-09-09）

为确认该 Light Function 是否实际进入渲染，本次重新核验后只切换 `DirectionalLight_0.LightComponent0.LightFunctionMaterial` 这一变量。MCP 端口 `127.0.0.1:8000` 的监听进程只读回为 `UnrealEditor`，可执行文件为 UE 5.8；当前 `Saved/Logs/LostRunic.log` 的项目命令行记录为 `D:/25DGame/LostRunic/LostRunic.uproject`，当前关卡为 `/Game/LostRunic/Levels/PIE_Test/L_Art_Demo`。未操作另一 UE 项目。

两次均使用同一个临时 PIE `startTransform=(-150,180,100)` cm、Yaw 0°、相机 `500 cm / -52° / FOV 68°`、相同 4 秒 warmup 和 `PlayMode_InViewPort`；保存的 PlayerStart、镜头、家具/建筑、纹理、StyleOutline 2 px 与 Interaction 独立入口均未改：

- **ON（A）**：Direction Light 引用 LF 材质，`LightFunctionScale=(1024,1024,1024)`、`Strength=0.18`，证据为 [After_P3R2_DirectionalLF_On_AB_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/After_P3R2_DirectionalLF_On_AB_PIE_Center.png)，PNG 4,592,497 bytes。
- **OFF（B）**：仅临时将 `LightFunctionMaterial=None`，Directional 的 12 lux、4800 K、3°/0.5°、阴影开关等保持，证据为 [After_P3R2_DirectionalLF_Off_AB_PIE_Center.png](../../ArtSource/Evidence/ArtPipeline_20260909/After_P3R2_DirectionalLF_Off_AB_PIE_Center.png)，PNG 4,578,336 bytes。
- A/B 完成后恢复同一 LF 引用并保存；最终回读引用为 `/Game/LostRunic/Materials/Lighting/M_LR_DirectionalWindowLightFunction_LowFreq`，`AssetTools.is_dirty(L_Art_Demo)=false`、材质 dirty=false、`IsPIERunning=false`。

在只为确认贡献而使用的 viewport crop（排除编辑器右侧面板）上，ON 相对 OFF 的有符号平均 RGB 变化约为 `(-2.68,-2.88,-2.43)`，有符号亮度均值约 `-2.81`；这说明 ON 调制确实进入画面，表现主要是全局偏平的轻微压暗。视觉复核仍未见清晰的树叶/笔触组织或明显窗格斑驳，不能把该像素差统计当作水彩质量通过，也不据此证明材质路径或 atlas 行为。下一步若继续诊断，应单独检查 Light Function UV 与 `LightFunctionScale` 的空间对应关系；本次不调整 Scale 或 Strength，候选按 ON 状态保留。

## Directional Light Function 连线与 UV 只读诊断（2026-09-09）

root 已独立回读并确认本次 Scale256 图没有清晰斑驳，因此不接受 256 为生产参数；本节只记录图连线和引擎源码证据，不再写入 UE。MCP 对材质 `/Game/LostRunic/Materials/Lighting/M_LR_DirectionalWindowLightFunction_LowFreq` 的实际读回如下：

- `MaterialExpressionAppendVector_0` 的 `A` 接 `MaterialExpressionTextureCoordinate_0`，`B` 接 `MaterialExpressionConstant_0`。
- `MaterialExpressionNoise_0` 的实际输入名为 `World Position` 与 `FilterWidth`；`World Position` 接 `AppendVector_0`，`FilterWidth` 未连接。也就是说，Noise 的坐标链没有断开，不存在误把 `Noise.Position` 留空的情况。
- `TextureCoordinate_0` 为 `coordinateIndex=0`、U/V tiling=`1/1`；Noise 为 `worldPositionOriginType=Absolute`、`scale=2`、`quality=1`、`noiseFunction=NOISEFUNCTION_GradientALU`、`bTurbulence=false`、`levels=1`、`outputMin/outputMax=0/1`、`levelScale=2`、`bTiling=false`、`repeatSize=4`。
- Noise 输出接 `Saturate_0`，再接 `OneMinus_0`；`OneMinus_0` 与 `ScalarParameter Strength`（读回默认值 `0.1800000072`、参数名 `Strength`、组 `LR Window LightFunction`、滑块 `0..0.5`）接 `Multiply_0`；`Constant_0` 与 `Multiply_0` 接 `Subtract_0`；`Subtract_0` 接 `MP_EmissiveColor`。材质仍为 `MD_LightFunction / BLEND_Opaque / MSM_Unlit`，`bForceCompatibleWithLightFunctionAtlas=false`。

本机 UE 5.8 源码给出以下 UV/Scale 关系：

- 非 atlas 的方向光函数路径在 `Engine/Source/Runtime/Renderer/Private/LightFunctionRendering.cpp:84-110` 读取 `GetLightFunctionScale()`，构造 `InverseScale=(1/Scale.Z,1/Scale.Y,1/Scale.X)`，将其乘进 `WorldToLight` 并生成 `SvPositionToLight` 矩阵；`LightFunctionPixelShader.usf:45-46` 用该矩阵得到 `LightVector`。
- `Engine/Shaders/Private/LightFunctionCommon.ush:39-44` 将 `LightVector` 重排后取其垂直于方向的 XY 作为 `LightFunctionUVs`；`:61-65` 把该 UV 写入 `MaterialParameters.TexCoords[]`，所以图中的 `TextureCoordinate_0` 读取的是引擎注入的 Light Function UV，而非表面贴图 UV。
- atlas 生成路径也在 `Engine/Source/Runtime/Renderer/Private/LightFunctionAtlas.cpp:640-647` 以同样的逆尺度构造 `TranslatedWorldToLight`；`Engine/Shaders/Private/LightFunctionAtlas/LightFunctionAtlasCommon.usf:104-106,144-146` 再计算/取模 Light Function UV 并采样 atlas。当前材质没有强制 atlas 兼容，且本轮没有 GPU capture，因此只能确认两套源码关系，不能断言本实例实际走哪条路径。
- `Engine/Source/Runtime/Engine/Private/Materials/HLSLMaterialTranslator.cpp:1761-1769` 说明 atlas 兼容性会受坐标操作影响，并可由 `GetForceCompatibleWithLightFunctionAtlas()` 覆盖；本材质保持 false，未为消除提示而强制覆盖。

该读回排除了坐标链断开的原因，但没有把 `Noise scale=2` 转换成已验证的世界厘米或可见笔触周期；Scale256 只作为独立同姿态诊断图保留。实验结束时 Directional `LightFunctionScale` 已恢复并保存为 `(1024,1024,1024)`，`AssetTools.is_dirty(L_Art_Demo)=false`、`IsPIERunning=false`。下一次如要继续，只能先明确引擎注入 UV 的单位/范围与 Noise 的空间尺度，再设计新的只读或单变量诊断；本轮不再盲调 Strength。
