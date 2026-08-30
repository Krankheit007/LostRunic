# LostRunic Art Bible v1 / Technical Art Standard v1

版本：1.1  
日期：2026-08-28  
适用范围：Normal 状态、Windows PC、UE 5.8、俯视角 3D Gameplay Camera  
文档状态：**Approved for Vertical Slice / 候选生产基线**；本版不定义 Perception、Courage、Memory 的最终效果

本文档已获准指导 Normal Technical Art Vertical Slice 施工，但尚未获准驱动全项目材质批量生产。只有在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 的 Mansion Benchmark 完成画面、Temporal、Interaction Outline 和目标最低配置 GPU Profile 验收后，文档状态才升级为 **Production Baseline**。

## 1. 目标与权威边界

Normal 状态的目标不是“普通 PBR 画面叠加 NPR 滤镜”，而是：

> 静止截图像手绘插画；运动时仍是稳定、清晰、具有可信空间关系的 3D 游戏。

四个职责必须保持清晰：

```text
Asset       = 风格来源
Material    = 风格统一器
Lighting    = 空间塑造器
PostProcess = 最后收束器
```

视觉决策优先级如下：

1. 第一张目标图决定模型形体、BaseColor、材质简化、细节密度与轮廓语言。
2. 第二张参考图只决定灯光、空间纵深、明暗构图和局部高光，不作为材质写实度参考。
3. Blue Prince 提供“有限 Shader、有限 Texture Vocabulary、严格复用”的生产思想，不字面复制“一套 Master Material”。
4. Cairn 提供“艺术家可绘制 Mask、自定义工具、参数化 Shader”的方法论，不假定其全局水彩风由某个公开技术单独实现。
5. Disco Elysium 提供允许抽象、大色块和局部笔触的尺度。
6. No Rest for the Wicked 提供光照、体积和空间重量的上限参考。

Normal 必须相对克制，为 Perception、Courage、Memory 留出明显的颜色、曝光、边缘、显现和失真变化空间。

## 2. 视觉目标图

### 2.1 Normal 主目标：形、材质、色块

![Normal 主目标：绘画化形体、材质和色块](References/NormalTarget_A_Painterly.png)

该图是 Normal 的主要美术目标。需要继承：

- 清楚的房间、门、楼梯、家具和角色 Silhouette。
- 低频大色块、有限纹理和有选择的结构线。
- 暗部仍有蓝、紫、褐等彩色信息，不坠为纯黑。
- 细而有变化的环境染色墨线，而非统一纯黑漫画边。
- 木、布、墙纸、金属依靠形体、方向性细节和粗糙度区分，而非微观 PBR 噪声。

### 2.2 Normal 灯光参考：光、空间、明暗构图

![Normal 灯光参考：真实光照、空间纵深和局部高光](References/NormalLightingReference_B.png)

该图只提供 Lighting Reference。需要继承：

- 窗、壁灯、台灯形成清楚的 Light Pool。
- 暖光焦点与冷色暗部划分房间和路线。
- Lumen GI、接触阴影和体积空气共同提供可信空间感。
- 暗区服务潜行构图，但不牺牲门、掩体、守卫和路径可读性。

不得继承：照片式材质密度、全表面微观纹理、过深黑位和 Gameplay 中明显景深。

### 2.3 视觉判断句

所有资产和截图评审都必须能回答：

> 第一张是否仍然决定“它长什么样”，第二张是否只帮助“光如何落在它上面”？

如果第二张开始决定木纹、墙面噪声、布料微法线或金属划痕密度，资产应退回修改。

## 3. 正式渲染路线

### 3.1 生产主线

```text
Default Lit
        +
Stylized Asset Authoring
        +
少量 Master Material + 共享 Material Functions
        +
Lumen + Virtual Shadow Maps
        +
低成本 Screen-space Art Outline
        +
CustomDepth/Stencil Interaction Outline
        +
章节 Color Grading
```

约束：

- 不修改引擎源码。
- 不创建自定义 Shading Model。
- 不依赖重型全屏水彩、Kuwahara、多层 Blur 或纸张卷积。
- Substrate Toon 只用于 A/B Prototype，不作为当前生产资产的唯一依赖。
- UE 5.8 项目现有 Lumen、VSM、DX12、SM6 和 Substrate 开关保持为工程事实，但生产材质作者只维护 Default Lit 一套 Authoring 语义。
- Substrate Toon A/B Prototype 使用独立实验材质，不作为生产 Master/MI 的父级，也不要求生产资产维护第二套参数或贴图逻辑。

### 3.2 不采用的路线

| 路线 | 结论 | 原因 |
| --- | --- | --- |
| 全项目 Substrate Toon | 只做原型 | UE 5.8 中 NPR Toon 仍为 Experimental，不能让资产不可逆依赖 |
| 自定义 Shading Model | 不采用 | 引擎维护、版本升级、Shader 编译与调试成本不符合项目规模 |
| 重型全屏水彩滤镜 | Normal 不采用 | 破坏时序稳定、材质识别和潜行可读性，且成本不可控 |
| 单一无限 Uber Material | 不采用 | Static Switch、Permutation、编辑复杂度和耦合会持续膨胀 |

## 4. 内容目录与命名

建议目录：

```text
Content/LostRunic/Materials/
  Master/
  Functions/
  Instances/Environment/
  Instances/Characters/
  PostProcess/
  Decals/
  SharedTextures/
Content/LostRunic/Data/Tuning/
Content/LostRunic/Art/Palettes/
Content/LostRunic/Art/TrimSheets/
Content/LostRunic/Art/Atlases/
```

命名：

| 类型 | 前缀与示例 |
| --- | --- |
| Static Mesh | `SM_LR_Home_Wardrobe_A` |
| Skeletal Mesh | `SK_LR_Ruth_Child` |
| Material | `M_LR_StylizedOpaque` |
| Material Function | `MF_LR_PainterlyVariation` |
| Material Instance | `MI_LR_Home_Wood_Dark` |
| Texture | `T_LR_Home_Wardrobe_BC` |
| Packed Stylized Mask | `T_LR_Home_Wardrobe_SMK` |
| Normal | `T_LR_Home_Wardrobe_NRM` |
| Decal Material | `M_D_LR_Home_WallCrack` |
| Material Parameter Collection | `MPC_LR_VisualStyle` |
| DataAsset | `DA_LR_Home_VisualStyle` |
| Post Process Material | `M_PP_LR_StyleOutline` |

规则：

- 名称描述内容和职责，不把临时版本号、作者名或日期写入正式资产名。
- Material Slot 使用稳定语义名，例如 `Body`, `Wood`, `Glass`，不使用 `Material_0`。
- 同一参数在所有 Master 中使用相同名称、类型、范围和分组。

## 5. 模型标准

### 5.1 Shape Language

建模优先级：

1. Silhouette。
2. 大、中、小形层级。
3. Gameplay Camera 下的结构面和明暗面。
4. 材质分区。
5. 最后才是小细节。

建议形体信息比重为“大形主导、中形辅助、小形点缀”。小形不能覆盖或破坏大形轮廓。

### 5.2 Bevel 与法线

- 重要边缘使用真实 Bevel，让第二张参考中的 Light Pool 和 VSM 高光能读出体积。
- Bevel 宽度由最终屏幕占比决定；在 Gameplay Camera 中不足约 1 像素的 Bevel 通常没有生产价值。
- 家具、门框、楼梯、柜门、角色装备等 Hero Edge 优先保留真实倒角。
- 使用平滑组、Weighted Normal 或手工法线维持清楚的大面；不得用强 Normal Map 代替本应存在的形体。
- 不为每道木板缝、墙纸图案和布纹建几何；这些属于贴图或有限结构线。

### 5.3 Nanite

Nanite 适用于：

- 高密度静态建筑、岩石、雕塑和需要真实 Bevel 的环境 Hero Asset。
- 视角中尺寸变化较大、传统 LOD 制作成本高的静态资产。

Nanite 不强制用于：

- 简单低密度道具。
- 透明玻璃组件。
- 高频 Masked 草叶和小型植被。
- 依赖明显 WPO 的资产，除非经过 VSM、Lumen 和性能验证。

透明部分必须拆为独立非 Nanite 组件，避免整个主体退回不合适的渲染路径。

### 5.4 角色与头发

- 角色在相同屏幕面积下允许比环境稍强的手绘色块和结构线，以保持人物识别。
- 面部依靠轮廓、明暗块和关键特征，不依赖皮肤毛孔。
- 头发采用大块 Hair Clump、少量层次和明确外轮廓；Normal 状态不以 Strand Hair 为基线。
- 衣物褶皱保留能改变大形和中形的部分，小褶皱主要烘焙或绘制，Normal 强度受控。

### 5.5 模型验收

每个资产至少通过：

- Gameplay Camera 原尺寸截图。
- 25% 缩略图测试。
- 纯黑 Silhouette 测试。
- 无贴图灰模测试。
- 关闭艺术描边后的材质与形体识别测试。

无法在这些测试中辨认的细节不得依靠加强 Outline 解决。

## 6. UV、Trim Sheet 与 Atlas

### 6.1 UV 原则

- 普通 UV 是默认方案。
- UV0 保存主要颜色、结构和方向性细节。
- UV1 可用于低频 Macro Variation、辅助 Mask 或未来烘焙需求，但不得与 UV0 形成重复权威来源。
- 静态建筑可以使用 World Space Macro Variation；移动角色、门、箱子和可推动道具不得使用会产生“纹理锁在世界中”的纯 World Space 颜色变化。
- World Aligned / Triplanar 仅用于巨型墙体、山体和无法合理展开的表面，不得成为所有材质的默认路径。

**Texture Scale Authority**

对具有明确方向和图案尺度的建筑材质（Wallpaper、Wood Plank、Tiles 等），正式资产的纹理物理尺度由 Mesh UV/Trim UV 作为主要权威；Material Instance Tiling 用于材质家族调整和例外补偿，不按墙体长度创建材质实例。

大尺度无方向综合色变化允许使用 UV1 或 World Space。WorldAligned/Triplanar 仅用于大型静态无规则表面或经批准的特殊资产，不作为所有建筑材质默认路径。

正式模块化建筑避免依靠显著 Non-uniform Actor Scale 改变尺寸。

### 6.2 Trim Sheet

优先使用 Trim Sheet 的内容：

- 门框、踢脚线、墙板、楼梯边、柜体边框。
- 重复木板、金属包边和建筑装饰条。
- 同章节反复出现且需要统一线条语言的结构件。

Trim Sheet 规则：

- 章节内优先复用，跨章节只有视觉语言一致时才复用。
- 线宽、倒角高光、木纹方向和磨损密度在 Gameplay Camera 下统一。
- 典型 Trim Sheet 以 2048 为生产起点；只有屏幕占比和复用价值证明不足时才升级。

### 6.3 Atlas

- 小型重复道具、Decal、植物簇和故事标记优先使用 Atlas。
- Atlas 必须保留足够 Padding 和 Mip 安全区，远景不得串色。
- 一个 Atlas 只服务一个清晰材质家族，不把无关资产塞入巨型公共图集。

## 7. 贴图与纹理标准

### 7.1 BaseColor

BaseColor 首先完成颜色简化，不依赖 Shader Posterize 或 Quantize。

必须：

- 以低频大色块为主。
- 保留材质必要的方向性：木纹方向、地板缝、墙纸节奏。
- 将结构暗线绘制到真正需要的门框、柜门、抽屉、楼梯和板缝。
- 暗色仍含色相，不直接压成无信息黑色。

禁止：

- 照片式高频噪声铺满全部表面。
- 把 AO、镜面高光和真实灯影烘进 BaseColor。
- 使用重复污点填补空白区域。
- 依赖实时色阶量化把普通 PBR 贴图变成插画。

### 7.2 Packed Stylized Mask

项目统一使用 `SMK`：

| 通道 | 权威语义 | 典型用途 |
| --- | --- | --- |
| R | `DetailAmount` | 允许木纹、墙纸、布纹等选择性细节出现 |
| G | `StructureCavity` | 仅艺术家指定的门缝、柜门、楼梯接缝等结构线；不是传统 AO |
| B | `MacroVariation` | 大块磨损、故事变化、色彩和粗糙度的低频变化 |
| A | `SecondaryMaterial` | 第二材质或特殊区域混合；不需要时不存储 Alpha |

优先级：

```text
资产 SMK
→ Material Instance 强度
→ Vertex Color 实例/区域覆盖
→ 章节 Global 参数
```

任何通道语义变化必须升级全项目规范，不能由单个资产自行解释。

`StructureCavity` 的使用边界：

- 只压暗艺术家认为应存在的门缝、柜门、抽屉、楼梯接缝、板缝和少量关键折痕。
- 不得直接使用整模型烘焙 AO、自动 Cavity Map 或全表面曲率暗化作为 `SMK.G`。
- 不得对所有凹槽统一乘黑；Lumen、接触阴影和环境遮蔽继续负责真实光照关系。
- 同一区域若已由灯光/AO形成足够层次，`StructureCavity` 必须减弱或关闭，避免双重脏黑边。
- 资产评审必须通过独立的 `SMK.G` Debug View 检查其覆盖范围。

### 7.3 Vertex Color

Vertex Color 只提供空间覆盖，不取代资产贴图：

| 通道 | 语义 |
| --- | --- |
| R | `PaletteVariation`：章节允许范围内的综合色变化 |
| G | `WearOverride`：局部磨损、灰尘或故事痕迹 |
| B | `DetailMultiplier`：压低或增强已由 SMK 允许的细节 |
| A | `SecondaryBlend`：第二材质区域混合 |

未使用通道保持默认值；不得用 Vertex Color 储存玩法状态。

### 7.4 Normal

- Normal 明显弱于传统写实 PBR，只表达中尺度结构。
- 典型强度从 `0.2–0.6` 范围调试；最终以 Gameplay Camera 为准，不要求所有资产都有 Normal。
- 木材不表现毛刺级细纹；墙纸不表现纸纤维；布料只保留能帮助识别的织物方向或大褶皱。
- 切线空间 Normal 使用 Normal 压缩、关闭 sRGB；不得以灰度高度图错误导入。

### 7.5 Roughness 与 Metallic

- Roughness 由材质类别和大块变化决定，不使用随机噪声。
- 木、布、石膏、纸默认偏哑光；金属和上漆表面只在需要的边缘和焦点产生受控高光。
- `MF_LR_StylizedRoughness` 对输入设置艺术范围，防止偶然出现镜面噪点。
- Metallic 遵循物理二元语义；非金属不通过“半金属”制造假高光。

### 7.6 纹理分辨率

最终屏幕占比是最高权威。以下为 1080p Gameplay Camera 的生产起点：

| 最终可见尺寸/用途 | 建议最大尺寸 |
| --- | ---: |
| 小于约 100 px 的小道具 | 256–512 |
| 约 100–400 px 的常规道具/家具 | 512–1024 |
| 约 400–900 px 的 Hero 资产 | 1024–2048 |
| 角色主要贴图集 | 2048 |
| 高复用 Trim/Atlas | 2048 |
| 4096 | 仅高复用大型表面或镜头级 Hero Asset，经截图和内存评审批准 |

辅助密度参考：背景环境约 128 px/m、常规环境约 256 px/m、近景可交互 Hero Asset 约 512 px/m。若与最终屏幕测试冲突，以屏幕测试为准。

纹理导入：

- BaseColor：sRGB 开启。
- SMK：sRGB 关闭，Masks 类压缩。
- Normal：sRGB 关闭，Normalmap/BC5 路径。
- 所有纹理生成 Mip；UI 或特殊数据纹理由各自规范处理。
- 未使用 Alpha 时移除 Alpha，避免无意义内存和压缩成本。
- 默认使用普通 Texture Streaming；只有大型高分辨率资产经数据证明受益时才使用 Streaming Virtual Texture。

## 8. Decal 与故事性细节

Decal 负责局部故事信息，不负责挽救平庸 BaseColor。

适合：

- 裂纹、渗水、焦痕、脚印、海报、墙上文字和一次性叙事痕迹。
- 局部大块综合色差异。
- 少量地面方向提示与环境磨损。

约束：

- 优先使用共享 Atlas。
- 同一表面常态重叠不超过两层；超过时应合并或回到资产贴图。
- 避免沿墙面和地板随机撒污迹。
- DBuffer Decal 只写所需通道，不为纯颜色痕迹额外写 Normal/Roughness。
- 关键叙事 Decal 必须在 Gameplay Camera 和章节 Palette 下可读。

## 9. Master Material 与 Material Functions

### 9.1 Master 划分

| Master | 用途 | 核心限制 |
| --- | --- | --- |
| `M_LR_StylizedOpaque` | 墙、木、石、家具和大多数环境 | 默认生产主材质 |
| `M_LR_StylizedMasked` | 植被、纸片、部分布料 | 控制 Overdraw，不自动继承全部 Opaque 功能 |
| `M_LR_StylizedCharacter` | 角色、衣物、Hair Clump | 允许角色专用颜色和边缘控制 |
| `M_LR_StylizedGlass` | 少量玻璃 | 单独组件，限制透明层和屏幕覆盖 |
| Decal 系列 | 污迹、裂纹、故事信息 | 每个系列只写必要 GBuffer 通道 |

每个 Master 建议不超过 4 个经过批准的 Static Switch。Static Switch 用于真正改变 Shader 路径的功能，不用于普通强度为零即可关闭的调参项。

### 9.2 共享 Material Functions

| Function | 职责 |
| --- | --- |
| `MF_LR_StylizedColor` | Saturation、Contrast、MacroTint、CavityTint、ColorFamilyTint；不 Posterize |
| `MF_LR_PainterlyVariation` | UV/Object/有限 World Space 的低频综合色变化 |
| `MF_LR_SelectDetail` | 按 SMK.R 与距离控制选择性细节 |
| `MF_LR_StylizedRoughness` | 材质类别、范围限制和大块变化 |
| `MF_LR_NormalSoftening` | 统一 Normal 强度、远距衰减和关闭路径 |
| `MF_LR_OutlineKernel` | 只提供像素偏移、分辨率感知宽度、邻域布局、SmoothStep 与线型整形；不检测任何边缘 |
| `MF_LR_ArtEdgeDetector` | 只根据 SceneDepth、WorldNormal、距离和阈值生成艺术边缘 Mask |
| `MF_LR_InteractionEdgeDetector` | 只根据 CustomDepth、CustomStencil 和选择状态生成交互外轮廓 Mask |

### 9.3 Texture Sample 预算

典型路径目标：

| Master | 常态采样目标 | 说明 |
| --- | ---: | --- |
| Opaque | 3–5 | BaseColor、SMK、可选 Normal、Macro、Detail |
| Masked | 3–5 | 包含 Opacity，优先降低 Overdraw |
| Character | 4–7 | 允许角色专用颜色/Normal，但仍限制微细节 |
| Glass | 1–3 | 不叠加完整 Painterly 功能 |

超过目标必须在 Material Stats 和目标 GPU Profile 中证明价值；不允许因为“节点已经存在”而默认启用。

### 9.4 参数分组

统一分组：

```text
00_Base
10_Color
20_Painterly
30_Detail
40_Roughness
50_Normal
60_Secondary
90_Debug
```

调优参数使用明确名称、Clamp 和 ToolTip。实例参数只控制资产表现，不保存玩法规则。

## 10. Painterly Variation

Painterly 感主要来自：

```text
Hand-painted BaseColor
+ Macro Variation
+ Soft Lighting
+ Colored Shadow
+ Selective Structure Lines
+ Colored Outline
+ Chapter Color Grade
```

而不是 Watercolor Post Process。

### 10.1 静态建筑

- 可用 World Position 驱动低频综合色变化，让相邻墙面共享“大画布”。
- 优先 UV1 或大尺度普通 UV；只有巨型表面才使用 Triplanar。
- 变化频率必须大于木纹、墙纸等材质细节，避免形成第二层噪声。

### 10.2 移动物体

- 使用 UV、Object Position 或 Local Position。
- 运动、开门和推动时图案必须固定在模型上。
- Skeletal Mesh 不使用纯 World Space 颜色纹理。

### 10.3 摄像机稳定性

- 不使用贴在屏幕上的高对比纸纹作为 Normal 常驻效果。
- 若使用极弱 Film Grain 或纸纹，只能作为最后收束，必须在相机平移和 TSR 下无明显游动。

## 11. 艺术描边

### 11.1 输入与位置

`M_PP_LR_StyleOutline`：

- Material Domain：Post Process。
- Blendable Location：Before Tonemapping。
- 输入：SceneDepth、WorldNormal、PostProcessInput0、InvViewSize。
- 不依赖 CustomDepth 选择全场景对象。
- 输出线色由章节 Ink Tint 主导，Local Scene Tint 只做弱影响。

### 11.2 分层算法基线

共享层 `MF_LR_OutlineKernel` 只负责：

```text
PixelOffset / InvViewSize
ResolutionAwareWidth
4/8 Neighbor Sampling Layout
SmoothStep / Line Shaping
```

Detector 层彼此独立：

- `MF_LR_ArtEdgeDetector` 读取 SceneDepth 与 WorldNormal，输出艺术边缘 Mask。
- `MF_LR_InteractionEdgeDetector` 读取 CustomDepth 与 CustomStencil，输出被选对象的交互外轮廓 Mask。
- 两套 Detector 共享 Sampling Kernel 的线宽和边缘整形语言，但不共享输入判断、阈值或玩法数据。

Art Edge Detector 必须包含：

```text
DepthEdge
NormalEdge
DistanceFade
DepthThreshold
NormalThreshold
InternalEdgeStrength
```

质量档：

- 默认档使用 4 个主方向邻域采样。
- 高档允许加入对角采样，但必须通过 GPU 预算。
- 不在 Outline Pass 中加入 Blur、Kuwahara、水彩扩散或多层卷积。

### 11.3 线宽

1080p 起始目标：

| 类型 | 目标宽度 |
| --- | ---: |
| 主体/环境 Silhouette | 0.8–1.2 px |
| 主要内部结构边 | 0.5–1.0 px |
| 小细节 | 不自动描边 |
| 交互外轮廓 | 1.5–2.0 px |

宽度必须通过 `InvViewSize` 或 SceneTexture Size 以屏幕像素定义；1440p、4K 与动态分辨率下保持视觉宽度一致。

### 11.4 距离自适应

- 近景保留适量 Normal Edge。
- 随距离降低 Normal Edge 到近景强度的约 10–20%。
- Depth Silhouette 保留得更多，以维持房间、门、角色和大型家具边界。
- 远景小面不得形成满屏线稿。

### 11.5 Ink Tint

每章节提供 `InkTint`。合成原则：

```text
InkColor = ChapterInkTint 为主 + 少量 LocalSceneTint
```

不直接取邻域最暗色，避免 Bloom、彩色材质边界和 Temporal Jitter 造成线色跳变。

### 11.6 资产绘制结构线

屏幕空间描边负责 Silhouette 和必要折面；内部结构线由艺术家决定，主要来自：

- BaseColor 中受控暗线。
- SMK.G `StructureCavity`。
- 真实 Bevel 与阴影。
- 必要时的 Decal。

门框、柜门、抽屉、楼梯边和木板缝不能全部交给 Scene Normal 自动判断。

## 12. 交互描边

艺术描边与交互描边共享像素宽度、采样布局和平滑整形语言，但使用彼此独立的 Edge Detector，并且不共享玩法数据来源。

```text
ArtEdge:
  SceneDepth + WorldNormal

InteractionEdge:
  CustomDepth + CustomStencil

Composite:
  InteractionEdge > ArtEdge > SceneColor
```

### 12.1 候选视觉参数

- 现有 `ULRInteractionPresentationComponent` 继续决定哪些组件启用 CustomDepth。
- Custom Depth-Stencil Pass 必须设置为 Enabled with Stencil。
- `M_PP_InteractionOutline` 后续组合 `MF_LR_InteractionEdgeDetector` 与 `MF_LR_OutlineKernel`；`M_PP_LR_StyleOutline` 组合 `MF_LR_ArtEdgeDetector` 与同一 Kernel。两者保持独立 Blendable、独立阈值和独立数据源。
- 交互线候选宽度为 1.5–2.0 px，并覆盖艺术外轮廓；不把所有内部结构暗线变成白线。
- 交互颜色候选值以白色为主体，混入约 20% 章节 Ink/Scene Tint；暖房间略暖，冷走廊略冷，但必须仍明显识别为白色功能提示。
- 交互线不使用强 Bloom，不表现为贴在世界外的霓虹 UI。
- Stencil 值按视觉语义集中登记，禁止单个蓝图自行占用随机值。

以上宽度、染色比例和覆盖关系是 Vertical Slice 起始值，不是未经实测即可推广到全项目的最终常量。

### 12.2 Composite 与 Blendable Location A/B

交互描边的最终 Composite 和 Blendable Location 必须在 Mansion Benchmark 中完成 A/B，不在本版靠理论锁死：

| 方案 | 候选位置 | 重点检查 |
| --- | --- | --- |
| A | Before Tonemapping | HDR SceneColor、Depth/Stencil 边缘稳定、TSR 与曝光前合成表现 |
| B | After Tonemapping 或经 Benchmark 证明更合适的 Scene Color 阶段 | 白色功能线在 Exposure、Color Grade 和章节 LUT 后的颜色一致性 |

A/B 必须使用同一相机、同一交互对象、同一 1080p/目标分辨率和同一章节 Color Grade，对比：

- 白色是否仍是明确功能提示，同时自然接受少量环境染色。
- InteractionEdge 是否稳定覆盖 ArtEdge，且不穿透遮挡物或污染内部结构线。
- 相机移动、TSR、动态分辨率、曝光和 Bloom 下是否闪烁、变色或线宽跳变。
- GPU 成本和 Blendable 顺序是否满足候选预算。

现有 `M_PP_InteractionOutline` 的 Blendable Location 只代表当前实现状态，不构成本规范的最终结论。只有 A/B 记录通过评审后，选定位置与参数才可升级为 Production Baseline。

### 12.3 Normal Vertical Slice 实施值（2026-08-30）

- `M_PP_InteractionOutline` 使用固定 8 taps（4 cardinal + 4 diagonal），最终 `InteractionDiagonalScale = 0.70710678`；`1.0` 版本保留为 `/Game/LostRunic/Materials/Benchmark/Instances/MI_PP_LR_InteractionOutline_Diag100` A/B 基准。
- 当前候选线宽为 `InteractionOutlineWidthPx = 1.0`，线色为白色；只生成 `Saturate(Expanded - SelectedCenter)` 外轮廓，不读取选中模型 WorldNormal，不产生内部白线，不使用 Blur。
- 可见性先用 Stencil 选择结果过滤对应的 CustomDepth tap，再与 SceneDepth 比较；未选中 tap 的远平面深度不得参与深度汇总。PIE 中以 Plaster 临时遮挡 Wallpaper Cube 下半部验证，遮挡区域不透线。
- `M_PP_InteractionOutline` 与艺术描边同处 `After DOF` 候选阶段，`BlendablePriority = 10`，保证交互白线最后覆盖艺术黑线。本值只作为 A–F 候选基线；最低配置 GPU 与 Before/After Tonemapping 完整 Profile 未锁定前，不升级为 Production Baseline。
- `LRCustomStencil::InteractionSelected = 1` 是交互选择唯一登记值；`ULRInteractionPresentationComponent` 只处理带 `InteractionOutline` Component Tag 的 Primitive，并按状态切换 Render CustomDepth。

## 13. 植被

植被以色块、AO、阴影和簇轮廓表达，不描每片叶子和每根草。

| 类型 | Normal Edge | Depth Silhouette | 资产策略 |
| --- | --- | --- | --- |
| 树干/粗枝 | 保留 | 保留 | 普通 Opaque 风格 |
| 大型叶簇 | 弱 | 保留 | Cluster 造型、平滑法线、大色块 |
| 小叶片 | 关闭/极弱 | 仅簇外轮廓 | Atlas、少量内部线 |
| 草 | 关闭 | 默认不强调 | Clump 而非单叶表达 |

优先通过平滑/簇法线、较高 Normal Threshold 和资产形体降低噪声。只有确有需要时才用保留的 Custom Stencil 类别屏蔽特定大型簇；不得让每根草为排除描边额外进入 CustomDepth Pass。

## 14. Lighting

### 14.1 基线

- Lumen GI 与 Lumen Reflections 保留。
- Virtual Shadow Maps 保留。
- 静态光照关闭，符合当前工程设置。
- 第二张概念图是灯光、GI、空间和局部高光参考。

### 14.2 Light Pool

每个可见空间必须能说明：

- 主亮区在哪里。
- 玩家和敌人在哪个明暗层级。
- 路线如何从一个 Light Pool 过渡到另一个。
- 哪些暗区用于潜行，哪些只是构图背景。

Light Pool 优先通过窗光、一个 Hero Local Light 和受控补光建立，不通过大量同强度灯具平均照亮。

### 14.3 Local Light

- 默认每个房间最多一个投动态阴影的 Hero Local Light。
- 第二个 Shadowed Local Light 只有在其阴影形状具有明确构图或玩法价值、并通过 Benchmark Profile 时使用。
- 装饰灯可使用不投阴影 Local Light、Emissive 或低成本补光；“灯具可见”不等于“必须投动态阴影”。
- Emissive 不作为主要照明的唯一来源，避免 Lumen 噪声和不稳定亮度。
- 同一 Gameplay Camera 可见的 Shadowed Local Light 总数以 GPU Profile 为最终权威。

### 14.4 暗部

- 暗部保持颜色信息，不以纯黑隐藏未完成资产。
- 门、掩体、守卫轮廓和下一段路线在无交互描边时仍可辨认。
- 允许深暗背景，但 Gameplay 区域的综合色和边缘必须稳定。

### 14.5 Fog

- 使用成熟的 Exponential Height Fog、Volumetric Fog 和必要的 Local Fog Volume。
- Fog 服务窗光柱、灰尘、暖灯散射和空间分层。
- Gameplay 区域不允许全面雾化。
- UE 5.8 Experimental FSSS 不作为生产基础。

## 15. Post Process 与章节色彩

### 15.1 Exposure

- Normal 使用固定或极窄范围曝光，不允许自动曝光改变已批准的明暗构图。
- 每章节在 Visual Style DataAsset 中保存曝光基线；关卡实例不得随意建立第二套值。
- 调试截图必须显示实际曝光来源。

### 15.2 Color Grading

每章节必须有 Color Script：

| 字段 | 说明 |
| --- | --- |
| Dominant Hue | 章节主综合色相 |
| Shadow Hue | 暗部保留的彩色色相 |
| Warm Accent | 灯光和焦点暖色 |
| Narrative Accent | 花朵、玩偶、线索等有限强调色 |
| Ink Tint | 艺术描边基础颜色 |
| Black Floor | 暗部最低可读范围 |

Color Grading/LUT 用于章节整体收束，不用于修复单个资产错误的 BaseColor。

### 15.3 其他效果

| 效果 | Normal 标准 |
| --- | --- |
| Bloom | 克制，只服务高亮窗光和灯具 |
| DOF | Gameplay 关闭或极弱；剧情镜头可单独使用 |
| Chromatic Aberration | 基本关闭 |
| Motion Blur | 极弱或关闭，以轮廓稳定和输入清晰为先 |
| Vignette | 极弱，不遮挡边缘 Gameplay 信息 |
| Film Grain | 极弱，必须通过 TSR/相机移动稳定性测试 |

## 16. 数据、MPC 与状态控制

### 16.1 章节 Visual Style DataAsset

每章节使用一个视觉定义资产，例如 `DA_LR_Home_VisualStyle`，保存：

- Palette 与 Ink Tint。
- Color Grading LUT 或颜色参数。
- Exposure 基线。
- Outline 强度、阈值与距离衰减。
- Bloom、Fog 和 Normal 状态允许的全局表现范围。

资产保存内容定义；运行时状态仍由现有 State 系统维护。

### 16.2 MPC

`MPC_LR_VisualStyle` 只保存真正 Global 的参数：

- `GlobalOutlineStrength`
- `GlobalInkTint`
- `GlobalPainterlyStrength`
- `StateBlend`
- `PerceptionIntensity`

木柜、墙面、角色等自身的 Grain、Roughness、NormalStrength 和 DetailStrength 必须留在 Material Instance。

### 16.3 运行时边界

```text
ULRStateComponent
        ↓ 当前状态与合法性
ULRStatePresentationComponent
        ↓ OnStatePresentationRequested
Renderer-facing Visual Presentation Adapter
        ↓
PP MID + MPC + Chapter Visual Style
```

Visual Presentation Adapter：

- 不决定当前状态。
- 不复制状态合法性。
- 缓存 PP MID、MPC 实例和章节视觉定义。
- 收到事件后只写入起始值、目标值、开始时间和时长。
- 使用材质时间参数和一次性 Timer 完成插值与回调，不永久 Tick。
- 完成后调用现有 `CompleteStatePresentation` 释放表现锁。
- 缺失资产或参数时使用 Normal 安全回退并输出可诊断 Warning。

正式实现后，按项目要求同步更新 `Docs/Technical/06_BlueprintConfigurationGuide.md` 的具体资产路径、组件配置、参数来源和 PIE 验收。

### 16.4 Debug View Contract

Vertical Slice 必须提供以下可独立查看的调试模式：

| Mode | 显示内容 |
| --- | --- |
| `Final` | 正常最终画面 |
| `SMK_R_DetailAmount` | `SMK.R` 灰度覆盖 |
| `SMK_G_StructureCavity` | `SMK.G` 灰度覆盖，用于发现 AO 化和全表面脏黑 |
| `SMK_B_MacroVariation` | `SMK.B` 灰度覆盖 |
| `SMK_A_SecondaryMaterial` | `SMK.A` 灰度覆盖 |
| `PainterlyVariation` | 仅显示 Painterly Variation 的综合色或强度 |
| `ArtDepthEdge` | 仅显示艺术描边的 Depth Edge Mask |
| `ArtNormalEdge` | 仅显示艺术描边的 Normal Edge Mask |
| `InteractionEdge` | 仅显示 CustomDepth/Stencil 交互边缘 Mask |
| `Roughness` | 最终写入材质的 Roughness 灰度 |

约束：

- 一次只显示一种模式，Mask 使用固定 0–1 灰度；不得叠加 LUT、Bloom、Vignette 等妨碍诊断的收束效果。
- Debug Mode 由一个全局枚举/标量入口控制，可由 PP MID 或 `MPC_LR_VisualStyle` 承载；不得为每种模式建立独立常驻系统。
- 只在 Editor/Development 调试流程使用，Shipping 默认 `Final`，不作为玩法路径或持续 Tick 来源。
- Debug View 用于定位问题来源，不允许成为修复错误 BaseColor、Normal、Roughness、Mask 或灯光的最终补丁。

## 17. 候选性能预算

基准目标：目标最低配置 GPU、1920×1080、实际 Gameplay Camera、60 FPS。以下数值是 Vertical Slice 的候选目标，不代表已经实测通过。最低配置尚未锁定前，开发机只用于趋势比较，不能替代最终验收。

| 项目 | Vertical Slice 候选目标 |
| --- | --- |
| 整帧 | 16.67 ms 目标 |
| `M_PP_LR_StyleOutline` | 最低配置 1080p 目标不超过 0.50 ms，且不超过整帧 3% |
| Outline Pass | 单一廉价常驻 Pass；默认 4 邻域采样 |
| Opaque 典型采样 | 3–5 个 Texture Sample |
| Character 典型采样 | 4–7 个 Texture Sample |
| Static Switch | 每 Master 建议不超过 4 个批准项 |
| Shadowed Local Light | 默认每房间 1 个；第二个必须 Profile |
| 透明 | 只用于必要玻璃/FX，控制层数与屏幕覆盖 |
| 植被 | 优先控制 Overdraw，不为每片叶子启用描边排除 Pass |

Profile 必须保存：

- `stat GPU` 或 GPU Visualizer 截图。
- Base Pass、Shadow Depths/VSM、Lumen、Translucency、Post Processing 分项。
- 1080p 与目标最高常用分辨率结果。
- 静止相机和正常移动相机结果。
- Outline 开/关差值。

## 18. Mansion Vertical Slice / Benchmark Scene

为遵守项目测试关卡约束，第一轮 Art Benchmark 组装在：

`/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`

由项目负责人在 Unreal Editor 中完成摆放；Codex 不代为布置关卡。Benchmark 区域固定包含：

- 一段走廊。
- 客厅。
- 餐厅。
- 一个楼梯区域。
- 木地板、壁纸、石膏墙、木家具、布沙发、金属灯具、玻璃、人物和一株植物。
- 至少一扇可交互门与一个可交互道具。
- 窗光、一个 Hero Shadow Light、装饰灯和暗区。

评审相机固定使用实际 Gameplay Camera；禁止为了截图临时更换 FOV、曝光或隐藏 Gameplay 难读区域。

## 19. Vertical Slice 验收

| 项目 | 合格标准 |
| --- | --- |
| Silhouette | 主角、门、家具在正常游戏缩放下一眼可辨 |
| 大中小形 | 缩到 25% 后仍读出主要房间、路线和焦点 |
| BaseColor | 大色块主导，无照片式高频噪声 |
| Material | 关闭 Outline 后仍能辨认木、布、石膏、金属和玻璃 |
| Normal Edge | 不出现满屏细碎线稿；远景主要保留 Silhouette |
| Temporal | 相机移动、TSR 和动态分辨率下无明显闪烁、爬线和线宽跳变 |
| Interaction | 以 1.5–2 px、环境染色白线为起点完成 Composite/Blendable A/B；覆盖艺术外轮廓且不漂白内部结构线 |
| Color | 缩略图仍能读出暖区、冷区、路线与人物位置 |
| Darkness | 暗部有颜色，不是纯黑；门和掩体不依赖交互线才可见 |
| Lighting | 每个 Light Pool 有清楚焦点，装饰灯不全部投动态阴影 |
| Fog | 只增强空间/窗光，不雾化 Gameplay 区域 |
| Foliage | 草和小叶不产生高频 Normal Edge |
| Debug Views | `SMK.R/G/B/A`、Painterly Variation、Art Depth/Normal Edge、Interaction Edge、Roughness 均可独立诊断 |
| PP Cost | Outline 以 1080p 0.50 ms/3% 为候选目标；锁定最低配置 GPU 后实测决定是否升级为硬门槛 |
| States | Normal 有明确风格，同时保留其他三状态的视觉提升空间 |
| Output Log | 无新增项目级 Rendering/Material/Texture Warning 或 Error |

### 19.1 Normal Rendering A–F 验证记录（2026-08-30）

- Surface：五类代表材质与五个 MF、`M_LR_StylizedOpaque` 已编译；用户已通过 Surface Direction Gate。Wallpaper 的 `DetailTiling = 2`，方向性建筑纹理继续以 Mesh UV/Trim UV 为尺度权威。
- Exposure：`BenchmarkCalibratedEV = -2.14`；Gray18 ROI 为 `(0.43, 0.68)–(0.57, 0.75)`，线性中位亮度 `0.1774`，满足 0.18 目标与 ±0.03 容差。
- Lighting：固定 `ArtBench_LightingROI = (0.02, 0.02)–(0.98, 0.98)`；Isolation 对照通过。截图/Profile PIE 临时禁用项为 `DirectionalLight_1`、`SkyLight_1`、`RectLight_0`、`RectLight_3–11`、`PostProcessVolume_1`，停止 PIE 后恢复。
- Art Outline：`M_PP_LR_StyleOutline` 的 Raw Depth、Relative Depth、Normal 与 FinalArtEdge 调试输出均已分别验证；材质编译无错误。
- Temporal：TSR（`r.AntiAliasingMethod=4`）下完成固定相机连续平移、动态分辨率以及 1080p、1440p、4K 检查，未观察到明显闪烁、爬线、线宽跳变或远景爆线。证据位于 `Saved/ArtBenchmark/`。
- Interaction：`MF_LR_InteractionEdgeDetector`、8-tap `M_PP_InteractionOutline`、0.707/1.0 A/B 实例已编译；圆角/斜边方向选择 0.707。无遮挡、部分遮挡、艺术线叠加和内部线检查通过。
- 自动化：`LostRunic.Interaction.OutlineStencilClearsAcrossTargets` 通过，覆盖 A 选中、A→B、B→None 与无残留 CustomDepth；无 Warning/Error。
- 构建：`LostRunicEditor Win64 Development` 完整构建通过。当前会话未发现 Shader/Material Error。
- 性能：本轮只记录实现趋势；最低 GPU 未锁定，GPU Visualizer 的硬预算验收仍是 Production Baseline 升级前置项。

## 20. Asset Acceptance Checklist

### 20.1 DCC 交付

- [ ] 命名、轴向、单位、Pivot 与项目一致。
- [ ] Gameplay Camera 下 Silhouette 清楚。
- [ ] 大中小形层级明确，小细节不破坏轮廓。
- [ ] Hero Edge 使用真实 Bevel；无价值的亚像素倒角已移除。
- [ ] 平滑组/自定义法线无明显断裂和黑面。
- [ ] 材质槽数量最小且名称稳定。
- [ ] Nanite 使用具有明确收益，透明/WPO 部分正确拆分。

### 20.2 UV 与贴图

- [ ] UV0 方向、接缝和密度符合材质需要。
- [ ] Trim/Atlas 已优先复用，Padding 可承受 Mip。
- [ ] 分辨率由最终屏幕占比决定，无无理由 4K。
- [ ] BaseColor 没有烘焙灯影、照片噪声和全表面污迹。
- [ ] SMK 通道严格符合统一语义。
- [ ] `SMK.G StructureCavity` 只覆盖艺术指定结构线，未复用整模型 AO/Cavity。
- [ ] Normal 只表达中尺度结构，强度不过量。
- [ ] Roughness 为材质类别和大块变化，不是随机噪声。
- [ ] sRGB、Compression、Texture Group、Mip 和 Alpha 配置正确。

### 20.3 材质

- [ ] 使用批准的 Master 和共享 MF，没有资产私建重复 Shader。
- [ ] 参数命名、分组、Clamp 和 ToolTip 正确。
- [ ] 普通资产不使用 Everywhere Triplanar。
- [ ] 移动物体 Painterly Variation 固定在模型上。
- [ ] Texture Sample 与 Static Switch 在预算内。
- [ ] 关闭 Outline 后材质仍可识别。

### 20.4 场景与灯光

- [ ] 章节 Palette、Ink Tint 和 Exposure 来源唯一。
- [ ] Light Pool 清楚，暗部保留彩色信息。
- [ ] Hero Shadow Light 数量受控并完成 Profile。
- [ ] Fog、Bloom、DOF、Chromatic Aberration 符合 Normal 限制。
- [ ] 植被没有高频边缘噪声。

### 20.5 Gameplay 与性能

- [ ] 主角、守卫、门、掩体和路线无需交互白边也能识别。
- [ ] 艺术线与交互线数据源分离，线条语言一致。
- [ ] 交互描边已完成 Composite/Blendable Location A/B，并保存选型依据。
- [ ] 所有规定 Debug View 可独立显示且语义正确。
- [ ] 1080p、1440p/目标常用分辨率下线宽一致。
- [ ] 相机移动时没有明显 Temporal 闪烁。
- [ ] GPU Profile 满足 Outline、Base Pass、Light 和 Translucency 预算。
- [ ] 在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 完成 PIE，Output Log 无项目级 Warning/Error。

## 21. 评审流程与变更控制

1. 灰模评审：Silhouette、Gameplay Camera、路线和 Bevel。
2. Flat Color 评审：大色块、材质家族和章节 Palette。
3. Material 评审：SMK、Normal、Roughness、Detail Budget。
4. Lighting 评审：Light Pool、暗部颜色、阴影形状和 Fog。
5. Outline 评审：静态、运动、距离、分辨率和植被。
6. Debug 评审：逐项检查 SMK、Painterly Variation、Art Edge、Interaction Edge 和 Roughness。
7. Gameplay 评审：交互线 Composite/Blendable A/B、角色/守卫/掩体可读性。
8. Profile 评审：目标分辨率和最低配置 GPU。

任何例外必须记录：违反规则、视觉或玩法原因、性能影响、替代方案、验证结果、批准人和未来偿还条件。

## 22. 公开资料与推断边界

- [Blue Prince: Beyond The Doors — Blender Conference 2025](https://conference.blender.org/2025/presentations/3964/)：官方介绍确认房间视觉使用高度复用的“一套 Shader 与四张纹理”方法；本项目学习其有限视觉词汇与复用思想，不字面复制 UE Master 数量。
- [Cairn: Technical Art of No-Piton Surfaces](https://unity.com/blog/cairn-technical-art-rendering-gameplay-rock-materials)：确认自定义 TexturePainter、2D/3D Blend Map、Shader 参数和 Compute Shader 同步玩法/视觉；不能据此声称其完整水彩渲染由 3D Blend Map 实现。
- [UE 5.8 Post Process Materials](https://dev.epicgames.com/documentation/unreal-engine/post-process-materials-in-unreal-engine)：支持 Before Tonemapping、SceneTexture、CustomDepth/Stencil，并建议能用内置 PPV 功能时优先使用内置功能。
- [UE 5.8 Rendering Settings](https://dev.epicgames.com/documentation/unreal-engine/rendering-settings-in-the-unreal-engine-project-settings)：Custom Depth-Stencil、Lumen、Texture Streaming 等项目设置依据。
- [UE 5.8 Material Instances](https://dev.epicgames.com/documentation/unreal-engine/instanced-materials-in-unreal-engine)：Material Instance、Static Parameter 和 Shader Permutation 风险依据。
- [UE 5.8 Substrate Materials](https://dev.epicgames.com/documentation/unreal-engine/substrate-materials-in-unreal-engine)：Substrate 仍标记为 Beta；生产资产只维护 Default Lit，Substrate Toon 仅使用独立实验材质做 A/B。
- [UE 5.8 Release Notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes)：Substrate NPR Toon 与 FSSS 仍为 Experimental，本版不作为生产基础。

Cairn 与 Blue Prince 未公开的完整轮廓、色阶和水彩算法均属于根据用户截图进行的视觉反推，不陈述为开发团队已公开事实。

## 23. 概念图生成记录

工具：Codex 内置 `image_gen`。两张图作为方向探索与 Art Bible 参考，不是可直接投入游戏的最终资产。

### 23.1 主目标图 Prompt

```text
Create an original top-down/isometric real-time 3D game environment concept for LostRunic's Home chapter: a small war-era European townhouse interior with hallway, sitting room, staircase, wooden floor, faded wallpaper, dining table hiding place, a child protagonist and an adult search silhouette. Use painterly oil-like value masses, restrained watercolor washes, simplified color blocks, colored ink outlines and sparse selective material detail. Keep the complete playable layout readable, use gray-violet, dusty blue, warm ochre and aged cream, and avoid photorealism, thick black outlines, texture noise, UI, text, logos and watermarks.
```

### 23.2 灯光参考图 Prompt

```text
Create the same original top-down/isometric LostRunic Home scene with grounded physically convincing light, sculpted stylized-realistic geometry, watercolor-like color grouping, matte materials and restrained colored outlines. Emphasize plausible window shafts, warm practical light pools, deep but colored cool shadows, subtle volumetric dust and strong spatial depth. Keep texture density low and the playable layout readable; avoid anime, thick black outlines, noisy normal maps, UI, text, logos and watermarks.
```

## 24. Source of Truth

- 本文档是 Normal Technical Art Vertical Slice 的美术生产、技术美术和资产验收 Source of Truth；通过 Mansion Benchmark 后才升级为全项目 Production Baseline。
- 工程运行时架构继续以 `Docs/Technical/08_ArchitectureBoundaries.md` 为权威。
- 蓝图装配与实际资产路径继续维护在 `Docs/Technical/06_BlueprintConfigurationGuide.md`。
- UE 工程事实以 `LostRunic.uproject`、`Config/DefaultEngine.ini`、现有 C++ 和实际 Content 资产为准。
- Perception、Courage、Memory 的最终视觉标准另行评审；它们不得反向破坏本版 Normal 的候选生产基线。
