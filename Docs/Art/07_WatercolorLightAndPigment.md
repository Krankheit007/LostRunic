# 水彩光照与阴影积色（2026-09-09）

状态：已直接实现并接入 L_Art_Demo 的可调候选，未通过最终风格与性能门禁。使用 LostRunic MCP 修改材质图、MI 和受影响关卡，未使用子代理或修改引擎。保留原贴图、相机 500 cm / -52° / 68°、2px StyleOutline 与独立白色 InteractionOutline。

## 光照

`/Game/LostRunic/Materials/Lighting/M_LR_WatercolorLightWash` 使用世界坐标两层低频三维噪声调制直接光，避免屏幕漂移、时间动画和高频颗粒。不是阴影可见性变形，也不改变物理半影规律。

| 实例 | 路径（/Game/LostRunic/Materials/ 下） | PatchSizeCm | Strength |
| --- | --- | --- | --- |
| 局部灯 | Lighting/MI_LR_WatercolorLocalLight | 24 | 0.50 |
| 方向光 | Instances/Scenes/MI_LR_WatercolorDirectional | 65 | 0.65 |

方向光绑定新实例，旧 M_LR_DirectionalWindowLightFunction_LowFreq 保留。ConsoleGlow、CabinetGlow、SofaGlow、SconceLeft、SconceRight、FireGlow 六个点光源绑定局部实例。此次实测非 Atlas 世界坐标函数在这些灯启用 Allow Mega Lights 时没有预期效果，因此仅关闭这六盏灯的 Allow Mega Lights；没有关闭全局 MegaLights。SconceLeft 从 Static 改为 Movable，其余五灯已为 Movable。保持原强度与 Cast Shadows=false。校准灯、填充灯、RectLight 未改。

## 阴影后处理

`/Game/LostRunic/Materials/PostProcess/M_PP_LR_WatercolorPigment`，实例 `/Game/LostRunic/Materials/Instances/Scenes/MI_PP_LR_WatercolorPigment`。After DOF、Priority=-10、Disable Pre Exposure Scale=true，使用同一原始预曝光尺度计算邻域相对光照。

通过 SceneColor / BaseColor 估计光照，结合深度、法线与底色差异拒绝几何和材质边界。四向近邻提取暗侧短渐变积色，四向远邻产生受世界空间低频色块调制的柔边明暗与蓝紫/淡粉色变化。它不读取真实 VSM shadow mask，不能准确区分所有纹理、反射和阴影。不是新增物体轮廓线。

| 参数 | 当前值 | 含义 |
| --- | --- | --- |
| Strength | 1 | 整体混合；0 为旁路 |
| PatchSizeCm | 65 | 世界空间晕染尺度 |
| RimStrength | 0.35 | 暗侧积色强度 |
| BleedStrength | 0.35 | 柔边斑驳强度 |
| RimWidthPx | 2 | 近邻采样范围，不等于固定描边线宽 |
| SoftWidthPx | 10 | 柔边采样范围 |
| DebugView | 0 | 1 显示红=积色、绿=柔边、蓝=暗侧估计 |

LRScene_PostProcess 的 Weighted Blendables 保留原 Style/Interaction 两项，追加此实例，权重 1。LR_NormalOutlineGate 作为现有 Normal 状态门控输入，不新增 MPC 写入者。此次只验证接线，未完成 Perception 往返视觉验收。

源码镜像在 `Tools/Watercolor/*.custom.hlsl`；运行时权威是材质资产的 Custom 节点。编辑镜像不会自动更新材质，需同步 Custom Code、保留所有输入并重新编译。

## 验证与证据

因问题依赖现有关卡灯光与家具投影，在 L_Art_Demo 做 PIE 与 Simulate 对照。固定临时 PIE 起点 (-150,180,100)，没有移动已保存的 PlayerStart。PIE 已停止，编辑器镜头已恢复。

- `ArtSource/Evidence/Watercolor_20260909/Before_WashPigment.png`：修改前。
- `Final_Gameplay.png`：最终灯光、积色开启。
- `Pigment_Off.png`：同组灯光、积色 Strength=0；随后恢复为 1。
- `Final_Wall_Simulate.png`：局部灯光近景，阴影过滤阈值收紧前拍摄，仅作为灯光证据。
- `After_WashPigment.png`、`Pigment_Debug_Initial.png` 为中间版本，不作为最终验收图。

最终两张母材质显式 recompile 成功；五个新材质/实例以包路径保存并读回 dirty=false。构图可见但积色仍偏克制，距离参考的丰富水彩层次仍有差距。修改 Custom 输入并重新接线期间产生过编译警告，最终重新编译未报错；不将整个会话宣称为零警告。既有 Cutaway / SaveAnchor 问题未纳入本轮修复。

截图为编辑器窗口，不能用作 1080p/60fps 性能通过证据。后处理每像素八个邻域、约 32 次额外纹理读取，GPU 时间、镜头移动时序稳定性、透明材质及 Normal/Perception 往返仍待定向验收。不应继续叠加采样或直接推广到所有关卡。

## 回退

临时对照：积色实例 Strength=0，光照实例 Strength=0（无调制）。完全回退：移除 LRScene_PostProcess 中新增积色项；按 `Tools/Watercolor/BeforeBindings.json` 恢复灯光旧 Light Function；六点光源 Allow Mega Lights 恢复 true；SconceLeft Mobility 恢复 Static。保留原两项描边。此次记录是本轮操作快照，不覆盖 ArtBaseline_20260907 不可变基线。
