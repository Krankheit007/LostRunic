# LostRunic Art Bible v1 / Technical Art Standard v1

版本：1.1  
日期：2026-09-05
适用范围：Normal 状态、Windows PC、UE 5.8、俯视角 3D Gameplay Camera  
文档状态：**Approved for Vertical Slice / 候选生产基线**；Normal 仍沿用既有候选基线，Perception 以本文专章作为视觉与技术合同，Courage/Memory 的最终效果仍未定义

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

`M_LR_StylizedOpaque` 统一提供 `SurfaceUTiling` / `SurfaceVTiling`。Production 规则仅允许具有明确方向性或平铺尺度需求的环境材质修改；Wood、Wallpaper、Tile 可以按材质家族或经批准的特殊资产调整，Plaster、Metal、Cloth 默认保持 `1.0`。这些参数是 Benchmark 补偿与例外修正工具，不替代正式 Mesh 的 Physical UV / Texel Density，也不得按 2m、4m、6m 墙长分别创建 MI。

Opaque 环境材质的数据流固定为：

```text
SurfaceUV   = UV0 × float2(SurfaceUTiling, SurfaceVTiling)
BaseColor   → SurfaceUV
SMK         → SurfaceUV
Normal      → SurfaceUV
DetailUV    = SurfaceUV × DetailTiling
PainterlyUV = UV0 × PainterlyTiling
```

Surface Pattern、Detail 与 Painterly 是三个独立频率层；Painterly Wash 不随 Wallpaper Pattern 一起变密。Character 使用精确 Atlas `UV0`，Production Character MI 禁止调整 Surface U/V。

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
| `M_LR_StylizedFoliageMasked` | 双面植被卡片 | `Masked + Default Lit + Two Sided`；不冒充通用 Masked |
| `M_LR_StylizedCharacter` | 角色、衣物、Hair Clump | 固定 Atlas UV0；常态路径不超过 5 samples |
| `M_LR_StylizedGlass` | 少量单层玻璃 | `Thin Translucent + Surface ForwardShading`，无 Refraction |
| `M_D_LR_StylizedColor` | 纯颜色污迹、裂纹和故事信息 | 只写 Color；Receiver 按类别收紧 |

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
| Foliage Masked | 3–5 | 包含 Opacity，优先降低 Overdraw |
| Character | ≤5 | 先复用资源已有 packed map；不为凑接口拆贴图 |
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

### 9.5 Normal Material Coverage G–L

**G Surface UV Contract**

- `M_LR_StylizedOpaque` 统一暴露 `SurfaceUTiling` / `SurfaceVTiling`，默认均为 `1.0`；正式调整权限遵循 6.1 的 Texture Scale Authority。
- Detail 必须由 `SurfaceUV × DetailTiling` 采样；Painterly Wash 必须由独立的 `UV0 × PainterlyTiling` 采样。
- Character 的 BaseColor/packed maps/Normal 固定使用 UV0，禁止通过 Surface U/V 改动 Atlas。

**H Masked / Foliage**

- 当前专用 Master 为 `M_LR_StylizedFoliageMasked`，固定 `Masked / Default Lit / Two Sided On`；以后需要铁艺、破洞布或 Hair Card 时另建职责明确的通用 Masked 路径。
- Prototype Alpha 必须使用大叶簇 Silhouette，不使用几十片独立小叶子制造高频 Art Edge；本阶段不宣称解决正式密集 foliage 的艺术描边策略。
- 固定 Gameplay Camera 与叶簇屏幕覆盖率，对比 No Foliage / Foliage Prototype，并检查 Shader Complexity / Quad Overdraw 趋势。
- 同一侧上方 Key Light 下绕卡片检查正反面；背面不得出现明显反转、发黑或异常高光。只有实际发现 Tangent-space Normal 问题后才处理 Two-Sided Normal 语义。

**I Character**

- Character 常态路径 `≤5 Texture Samples`，贴图组织以已有资源事实为准；可复用 ORM/RMA，不要求拆成独立 Roughness/Metallic。
- Painterly Wash 只做低强度、低频综合色变化。Gameplay Camera 移动时检查脸、手臂、裙子、身体正背面和腿部 UV seam；不得表现成角色身上贴着不同相位的噪声块。出现问题优先降低 Painterly Strength。

**J Glass**

- `M_LR_StylizedGlass` 固定 `Translucent / Thin Translucent / Surface ForwardShading / Two Sided Off`，用于单层薄玻璃；不使用 Refraction、Scene Color、WPO 或完整 Opaque 功能。
- `Opacity = Saturate(OpacityBase + Fresnel × EdgeOpacityStrength)`。Painterly Wash 只能轻微调 Tint / Transmittance Color，不得控制 Opacity。
- Fresnel 只强化玻璃存在感，不得形成比 Art Outline 更醒目的第二圈轮廓。

**K Decal**

- `M_D_LR_StylizedColor` 只写 Color；`M_LR_StylizedOpaque` 的 Decal Response 为 Color，Foliage/Character/Glass 默认为 None。
- Decal 的世界覆盖尺寸由 Projector Bounds 决定；Projector 尺寸必须尊重源 Mask 设计比例，不使用极端 Non-uniform Scale 将单张 Decal 拉伸覆盖任意墙长。覆盖不足时更换 Decal 或摆放第二个 Decal。
- 验收重点是 Projection Bleeding：不得穿透至墙背面、相邻墙/地板、Ruth、Glass 或其他非目标 Receiver。

**L Combined Material Gate**

- 固定 Camera 按 `Opaque only → +Foliage → +Character → +Glass → +Decal` 留存隔离截图，再检查全部类别同时出现。
- 至少包含一个 `InteractionSelected` Benchmark Actor；同时开启 Art Outline 与 Interaction Outline，验证新材质类别未改变后处理顺序或深度关系。
- 至少覆盖 1080p、1440p 与 Screen Percentage 70%/100%；4K 为 Smoke Test。记录 Base Pass、Translucency、Post Processing、Art/Interaction Outline、Shader Complexity / Quad Overdraw 与可用的 DBuffer Decals 趋势。
- 本阶段只形成 Candidate Baseline；最低 GPU 未锁定前不升级 Production Baseline。

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
- Blendable Location：After DOF。
- Blendable Priority：0。
- 输入：SceneDepth、WorldNormal、PostProcessInput0、InvViewSize。
- 不依赖 CustomDepth 选择全场景对象。
- 输出线色由当前像素的 `PostProcessInput0` 经对象相对暗化产生；BaseColor 只提供受门控的弱色相影响，不使用章节 InkTint/LocalSceneTint 作为主动着色来源。
- 采样契约：本轮不新增资产 `Texture Sample`；Art Outline 最多增加一次当前像素的 GBuffer BaseColor `SceneTexture` 查找，不读取邻域 BaseColor。SceneDepth/WorldNormal 的邻域采样仍只服务边缘检测，不得复制 BaseColor 查找。

`After DOF / Priority 0` 是 Normal A–F Vertical Slice 当前权威配置。最低配置 GPU Profile 仍决定是否升级为 Production Baseline，但不使本阶段的 Blendable Location 保持未决。

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

### 11.5 对象相对描边色

`M_PP_LR_StyleOutline` 的主动颜色契约固定为当前像素相对暗化，避免统一黑线压平材质识别：

```text
Lit = max(PostProcessInput0.rgb, 0)
DarkenedLit = Lit * OutlineDarkenFactor       // 默认 0.55，编辑器范围 0.35–0.60
BaseMax = max(BaseColor.r, BaseColor.g, BaseColor.b)
BaseChroma = BaseMax - min(BaseColor.r, BaseColor.g, BaseColor.b)
HueValidity = smoothstep(0.02, 0.08, BaseMax) * smoothstep(0.015, 0.08, BaseChroma)
BaseHue = BaseColor / max(BaseMax, epsilon)
HueTint = lerp(1, BaseHue, OutlineBaseHueInfluence * HueValidity) // 默认 0.15，范围 0–0.25
OutlineColor = DarkenedLit * HueTint
```

`OutlineDarkenFactor` 与 `OutlineBaseHueInfluence` 位于材质实例的 `40_OutlineColor` 分组。黑色/近灰 BaseColor 必须退回中性 `HueTint=1`；有效材质的描边 RGB 与线性亮度均不得高于 `Lit * 0.60`。旧的 `InkTint`、`LocalSceneTint` 参数和实例覆盖可保留为历史序列化数据，但不再连接到主动输出。历史 A–F 的 `BenchmarkCalibratedEV = -2.14` 仅是曝光记录，不是描边颜色来源。

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

### 12.2 Composite 与 Blendable Location 基线

Normal A–F Benchmark 已将艺术描边与交互描边统一在 `After DOF`，以 Priority 明确覆盖顺序：

| Blendable | 当前权威位置 | Priority | 数据源与职责 |
| --- | --- | ---: | --- |
| `M_PP_LR_StyleOutline` | After DOF | 0 | SceneDepth + WorldNormal；输出艺术黑线 |
| `M_PP_InteractionOutline` | After DOF | 10 | CustomDepth + CustomStencil；输出交互白色外轮廓并覆盖艺术线 |

`Before Tonemapping` 仅保留为历史 A/B 备选，不是当前配置。后续若改变位置，必须使用同一相机、同一交互对象、同一分辨率和同一章节 Color Grade 重新验证：

- 白色是否仍是明确功能提示，同时自然接受少量环境染色。
- InteractionEdge 是否稳定覆盖 ArtEdge，且不穿透遮挡物或污染内部结构线。
- 相机移动、TSR、动态分辨率、曝光和 Bloom 下是否闪烁、变色或线宽跳变。
- GPU 成本和 Blendable 顺序是否满足候选预算。

当前 `After DOF` 选型是 Vertical Slice 的 canonical truth；最低配置 GPU 的绝对成本验收仍是升级 Production Baseline 的独立前置项。

### 12.3 Normal Vertical Slice 实施值（2026-08-30）

- `M_PP_InteractionOutline` 使用固定 8 taps（4 cardinal + 4 diagonal），最终 `InteractionDiagonalScale = 0.70710678`；`1.0` 版本保留为 `/Game/LostRunic/Materials/Benchmark/Instances/MI_PP_LR_InteractionOutline_Diag100` A/B 基准。
- 当前候选线宽为 `InteractionOutlineWidthPx = 1.0`，线色为白色；只生成 `Saturate(Expanded - SelectedCenter)` 外轮廓，不读取选中模型 WorldNormal，不产生内部白线，不使用 Blur。
- 可见性先用 Stencil 选择结果过滤对应的 CustomDepth tap，再与 SceneDepth 比较；未选中 tap 的远平面深度不得参与深度汇总。PIE 中以 Plaster 临时遮挡 Wallpaper Cube 下半部验证，遮挡区域不透线。
- `M_PP_LR_StyleOutline` 与 `M_PP_InteractionOutline` 均固定在 `After DOF`；前者 `BlendablePriority = 0`，后者 `BlendablePriority = 10`，保证交互白线最后覆盖艺术黑线。这是 A–F 当前权威基线；最低配置 GPU Profile 未通过前仍不升级为 Production Baseline。
- `L_PIE_Test` 的全局 `LR_PostProcess`（`Priority 0 / Unbound`）现在按该顺序承载艺术描边实例与最终 `MI_PP_LR_InteractionOutline_Diag0707`；局部 `ArtBench_PPV` 仅保留其 Review Lighting/颜色覆盖，不再重复挂载 Blendable。`ArtBench_M1_NeutralPPV` 的体积边界覆盖完整 `ArtBenchmark/Normal/FoliageM1`（含约 100cm 安全边界）。
- `LRCustomStencil::InteractionSelected = 1` 是交互选择唯一登记值；`ULRInteractionPresentationComponent` 只处理带 `InteractionOutline` Component Tag 的 Primitive，并按状态切换 Render CustomDepth。
- `LRCustomStencil::PlayerOccluded = 2` 由 `ULRCameraCutawayComponent` 集中写入所有角色视觉 Primitive（Body 及带 `CharacterVisual` Component Tag 的附件）。`M_PP_LR_PlayerOcclusion` 固定 `After DOF / Priority 20`，只在 `CustomDepth > SceneDepth + OcclusionDepthBiasCm` 时输出白色填充与 `1px @ 1080p Reference` 外轮廓；当前 Bias 起点为 1cm。

### 12.4 Camera Cutaway 材质基线（2026-09-01）

- Gate 0 资产位于 `/Game/LostRunic/Materials/Benchmark/CutawayGate/`：Opaque 父材质连接 Opacity Mask，普通实例保持 Opaque，Masked 实例用 Base Property Override。正式路径采用同一策略；若未来引擎升级导致普通 Static Mesh 路径回归，回退为专用 `M_LR_StylizedCutawayMasked` 并共享现有 Surface 逻辑。
- `M_LR_StylizedOpaque` 与真实植被 Master `M_LR_StylizedFoliageMasked` 都读取 CPD 0–8。前者只有采用 Masked Override 的 Cutaway MI 才裁像素；后者将既有 Alpha 与 Cutaway Dither 相乘。不新增资产 `Texture Sample`、运行时 MID 或材质交换，Static Switch 数不增加。
- Local 中心充分让开，默认半径 `200px @ 1080p Reference`、过渡 `8px @ 1080p Reference`，用共享 `MF_LR_CutawayRadialShape` 产生 reference-space `CutawayCoverage` 与 `SignedBoundaryDistanceRefPx`；Painterly cell/hash 只在该函数内执行，屏幕 Dither 只读取 Coverage，不再乘 raw Amount。Foreground 在 PixelDepth 100cm 内完全裁切、300cm 外完全可见。
- Art Outline 从 `/Game/LostRunic/Materials/Parameters/MPC_LR_CutawayView` 读取最多四个 `LocalCutawayState0..3`（RGBA 为 CenterUV.X/Y、RadiusRefPx、Amount），沿 signed distance `[-10,+2]`（8px transition + 2px margin）取四槽最大抑制环，约 1px 软化，并保持 `FinalArtEdge *= 1 - SuppressionRing`。状态由相机 sticky slots 发布；ActiveCount 只使用首个连续槽。四槽是明确的视觉缓存容量上限，不是 Cutaway 容量：超出四个的目标仍可裁切但不进入描边抑制，必须在测试关卡压力验证 1/2/4/5/6 个同时 Local Occluder，并在第 5 个开始出现人工黑边时另行评估扩容或机制调整。
- Root Preserve 由 MI Scalar `RootPreserveEnabled / DefaultRootHeightCm / DefaultRootFeatherCm` 配置。墙/树干生产起点 10/5cm；屋顶、叶簇默认 0。Target 的运行时 Override 只在明确启用时覆盖，显式 0 有效；其 Height/Feather 是世界厘米，归一化时计入 Primitive 的 Component World Scale Z。
- `DA_LRPresentationTuning.ApprovedCutawayMasterMaterials` 是材质准入唯一列表；Validator 读取最终 Blend Mode 与 Base Material，并按显式 `PersistentMaterialSlots` 排除，不递归猜测材质图。

## 13. 植被

植被以色块、AO、阴影和簇轮廓表达，不描每片叶子和每根草。

| 类型 | Normal Edge | Depth Silhouette | 资产策略 |
| --- | --- | --- | --- |
| 树干/粗枝 | 保留 | 保留 | 普通 Opaque 风格 |
| 大型叶簇 | 弱 | 保留 | Cluster 造型、平滑法线、大色块 |
| 小叶片 | 关闭/极弱 | 仅簇外轮廓 | Atlas、少量内部线 |
| 草 | 关闭 | 默认不强调 | Clump 而非单叶表达 |

优先通过平滑/簇法线、较高 Normal Threshold 和资产形体降低噪声。只有确有需要时才用保留的 Custom Stencil 类别屏蔽特定大型簇；不得让每根草为排除描边额外进入 CustomDepth Pass。

### 13.1 M1 块面化植被组织

LostRunic 的正式植被方向为 **Cluster-Based Stylized Foliage**：树冠首先读成少量连续的综合色团块，草地首先读成大片地表色块与草丛轮廓，而不是很多片叶子和很多根草。

- 中型树以树干/主枝真实 Geometry、少量 Canopy Lobes、每个 Lobe 内多张紧凑 Leaf-Cluster Card，以及少量外轮廓 Cluster 组成。
- `3–6` 个 Lobe、小树 `20–40` 个 Cluster、中型树 `40–80` 个 Cluster 只作为 M1 的 DCC 组织起点，不是画面计数或正式资产预算。
- 25% 缩略图下应读成少量清晰的大体积综合色块，而不是几十个独立叶簇；最终权威是 Gameplay Camera 下的 Silhouette、叶冠密度、Masked Overdraw、Shadow Cost 与 Temporal Stability。
- 一张 Cluster Card 表达一个连续叶簇。内部叶片身份主要由 BaseColor、SMK 与综合色表达；Alpha 以簇外轮廓为主，只保留少量较大的透空、枝条负空间和外轮廓缺口，不制作数十个单叶 Alpha Island。
- Card Geometry 应贴近 Cluster 外形；允许用少量额外 Triangle 换取更少的空透明区域，不以最少 Card 或最少 Triangle 作为单一优化目标。

草地固定为三层：

| 层级 | 表现职责 | 推荐实现 |
| --- | --- | --- |
| Ground Base | 大片综合色、泥土/草地冷暖变化 | Opaque Ground / Landscape Material |
| Grass Mass | 主要草量和连续综合色丛 | 宽 Blade/Fan 构成的低模 Clump，Static Mesh Foliage |
| Hero Tuft | 路边、石头边、窗下等少量突出草形 | 独立较大草簇、花或灌木 |

Grass Clump 的整体 Silhouette 优先于单根草叶细节；草根颜色接近地面暗色，向草尖过渡到略暖或略亮的绿色。密度按浓密区、中密度、稀疏过渡和路线空地组织，不使用均匀随机白噪声铺满地面。

### 13.2 Custom Vertex Normal 与导入契约

Custom Vertex Normal 是树冠主要体积受光的权威数据。M1 使用 `normalize(VertexPosition - LobeCenter)` 作为基准球面法线；正式资产允许根据 Lobe 比例使用椭球 Normal Field，或在 DCC 中将原始 Cluster Normal 与 Lobe Volume Normal 做少量混合。不得把该混合变成 M1 Shader 参数。

- Blender 导出必须包含 Custom Split Normals 与 Vertex Color。
- UE Static Mesh 使用 `Normal Import Method = Import Normals`、`Normal Generation Method = MikkTSpace`。
- Build 设置固定为 `Recompute Normals = Off`、`Recompute Tangents = On`、`Use MikkTSpace = On`；M1 不得在不同资产间混用 Tangent 策略。
- Import/Reimport 后必须用 WorldNormal 可视化确认每个 Lobe 仍呈连续的径向或椭球分布。若重新变成单张 Card 的平面法线，导入失败，不得据此否定风格方向。

M1 树的绕序是导出门禁的一部分，不以单个顶点法线代替面级验证。`generate_m1_foliage_meshes.py` 在导出前对每个 Trunk/Canopy 渲染面计算：

```text
Dot(FaceGeometricNormal,
    Normalize(Average(VertexCustomNormals))) > 0.5
```

平均 authored normal 小于等于 `1e-5` 或几何面法线接近零直接失败；失败行必须报告对象名、材质区、面索引、实际 dot 和原因，并在成功/失败前报告每个材质区的面数、`min_dot` 与 `avg_dot`。当前门禁输出为 Tree `Trunk`：`min=0.998836 / avg=0.999229`，`Canopy`：`min=0.655953 / avg=0.893891`。本轮脚本只对 Tree 的 Trunk/Branch 与 Canopy 执行该 dot assert；Grass/Hero 的根到尖倾斜 authored normal 不属于 Lobe 体积法线契约，须按独立草簇的轮廓、颜色与透明度门禁复核，不得把它们写成已通过本断言。

### 13.3 Foliage Vertex Color 契约

所有 Foliage 顶点 RGB 在艺术涂色前必须显式初始化为 `(0.5, 0.5, 0.5)`，不得依赖 DCC 默认白色或未初始化数据。M1 的 RGB 以 `0.5` 为中性，表示相对综合色变化：

```text
MassSigned = VertexColor.rgb × 2 - 1

MassMultiplier =
1 + MassSigned
    × FoliageMassTintRange
    × FoliageMassTintStrength

FinalBaseColor =
StylizedColor × MassMultiplier
→ PainterlyVariation
```

`FoliageMassTintRange = 0.2` 为 Candidate；此时 RGB 0/0.5/1 分别对应 0.8/1.0/1.2 倍。Master 的 `FoliageMassTintStrength` 默认保持 `0` 以兼容现有 H 资产，M1 实例使用 `1`。

Vertex Color A 保留为 `FutureBendWeight`。草地 M1 可以写入 Root 0 → Tip 1；树冠不将 A 定义为完整风层级语义，M1 材质不得读取 A，避免锁死未来 Pivot Painter 或分层树木风方案。

### 13.4 Foliage Alpha / Atlas 契约

- BaseColor Atlas 固定 `sRGB = On`、生成 Mips。当前项目未配置独立 Foliage Texture Group，M1 资产使用 `Texture Group = World`；若以后建立经验证的项目级 Foliage Group，再统一迁移，不得逐资产形成隐式配置分叉。
- `Do Scale Mips for Alpha Coverage = On`；`Alpha Coverage Thresholds` 只启用 `A Threshold = 0.4 Candidate`，R/G/B 不启用。
- Cluster 外轮廓必须有充分 Padding/Gutter；透明像素 RGB 必须做边缘 Dilation，不得用纯黑透明背景污染 Mip。
- Atlas 岛之间必须保留足够 Mip Gutter。Near/Mid/Far 检查中不得出现 Cluster 突然变瘦、消失、亮/黑边或跨岛串色。
- `Opacity Mask Clip Value = 0.4` 仅为 M1 Candidate，必须在真实 Mip、TSR 与 Screen Percentage 70%/100% 下验证。

### 13.5 Foliage Master 边界

`M_LR_StylizedFoliageMasked` 继续同时服务 Tree 与 Grass，并保持 `Masked / Default Lit / Two Sided / Decal Response None`、4 Texture Samples、0 Static Switch。M1 只增加 13.3 的综合色 Vertex Color 数据流，不增加 Detail、WPO、Wind、Subsurface、LOD Fade 或 Foliage Outline 分支。

若后续 Tree 与 Grass 在 WPO、Two-Sided Normal、Opacity、Shading Model、LOD Fade 或 Shadow 语义上出现结构性差异，则建立独立 Master；不得为避免拆分而持续向共享 Master 添加 Static Switch。

### 13.6 M1 视觉与时序门禁

- `Neutral Foliage Review Lighting`：使用固定中性 Key/Fill、固定 Manual Exposure 与 Neutral Color Grade，检查 Lobe Custom Normal 连续性、卡片正反面受光、体积和异常高光。
- `BaseColor Diagnostic`：使用 `Buffer Visualization > Base Color` 并关闭 Art Outline，独立检查 Vertex Color 大色块、Atlas、Cluster 内部综合色与草的 Root→Tip 色阶；不得用关闭主要灯光代替该诊断。
- Gameplay Camera 下不得出现视觉上占主导的单叶/草叶墨线。若通用 Art Outline 无法满足，则记录为后续 Foliage Outline Policy 输入，不在 M1 增加 Stencil 或特殊 Pass。
- 同一 Camera 下按 Tree 屏幕高度 Near 约 35%、Mid 约 15%、Far 约 7% 测试；覆盖 TSR、1080p/1440p 与 Screen Percentage 70%/100%。Near 读取 Cluster，Mid 读取 Lobe，Far 只读取整体 Silhouette 与综合色。只验证现有 mip 稳定性与 `1200–4500 cm` 内部 Normal Edge 淡出；远景保留 Depth Silhouette，不新增 Shader 分档。

法线必须按两步判定，禁止把 Normal、Painterly 与 Outline 同时打开后猜根因：

1. **Stage 1 / Custom Normal Gate**：`NormalStrength = 0`、`Default Lit`、`Neutral Lighting`、`Painterly Off`、`Art Outline Off`，用 `WorldNormal` 与 Lit 视图检查四个 Lobe 是否读成连续的大块受光。通过才表示 authored normal、绕序、导入和 tangent 链路通过；失败时只排查这些链路。
2. **Stage 2 / Normal Texture A/B**：保持 Stage 1 视图，只切 `NormalStrength = 0.25`。若贴图重新打碎大体积，成品候选保持 `0`，`0.25` 只作为失败对照，不能用贴图调参掩盖 Stage 1 失败。

Stage 1 通过后才执行 **Cluster Silhouette Gate**：在 `NormalStrength = 0`、`Art Outline Off` 的 Gameplay Camera 下看 25% 缩略图。若画面仍由细长条、接近 edge-on 的 Card、大空洞或纸片轮廓主导，归类为 `Cluster Geometry/Orientation/Alpha Density` 失败并停止本轮；不得修改 Cluster 数量、分布、尺寸、朝向策略或 Alpha 密度，也不得继续修改 Normal Shader。后续独立评审才可考虑簇间覆盖、减少内部空洞、调整 Card 朝向或让部分平面偏向树冠上方/Gameplay Camera。

法线与 silhouette 分类完成后才执行 **Outline Gate**：用 `ArtEdgeDebugView 1/3/4` 分别检查 Depth、Normal、Final；成品目标为树干轮廓清晰、树冠只保留弱外轮廓、草极弱或不强调。测试关卡的 `LR_PostProcess` 为全局体积，按 After DOF 顺序承载艺术描边和最终 `0.707` 交互描边；局部 `ArtBench_PPV` 不再重复添加 Blendable，`ArtBench_M1_NeutralPPV` 覆盖整个 `ArtBenchmark/Normal/FoliageM1` 区域。

### 13.7 M1 性能 Proxy 与已知 LOD 风险

性能比较固定为 1920×1080、Screen Percentage 100%、同一 Camera 与同一归一化 ROI，依次记录 No Foliage、Single Tree、Tree Density Cell、Grass、Dense Grass、Combined 的 Base Pass、VSM、Shader Complexity、Quad Overdraw 和 GPU 趋势。

- `Masked Screen Coverage Proxy`：对 No Foliage 与 Candidate 的同设置 Quad Overdraw 图，在 ROI 内计算逐像素 RGB 差；任一通道差异大于 `2/255` 记为 Foliage-Touched Pixel，Proxy 为该像素数除以 ROI 像素数。
- `Overlap Layers Proxy`：使用同一 Quad Overdraw 截图的可见 Legend Palette，将 Foliage-Touched Pixel 按最近颜色分入固定 Severity Class；记录 Mean Class、P95 Class 和 Class 4+ 的 ROI 占比。
- 两个值都只作为同一 Benchmark 内的趋势 Proxy，不解释为真实硬件 Fragment Coverage 或精确 Overdraw Layer Count；M1 不设正式硬预算。

M1 Tree 保留 `Trunk + Canopy` 双 Material Section。该结构会增加 Section/Draw Submission，并要求未来 Foliage LOD 保持 Section 与材质槽一致；M1 只验证 LOD0，不通过合并 Section、拆分 Mesh 或重构 Master 解决。正式 Foliage LOD 阶段必须重新评估 Section 数、LOD 材质一致性、实例批处理与 Draw Call 趋势。

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

## Perception 视觉与技术合同（2026-09-05）

本章是 Perception 的美术表现、运行时参数和验收边界的唯一合同；它补充而不改写 Normal 的生产基线。实现状态必须按下列词义记录：**已实现**表示 C++ 规则/API 已存在；**已装配**表示蓝图或资产引用已经配置并可编译/保存；**待验证**表示仍需在 UE Editor、PIE、材质编译或目标 GPU 上取得证据。本章不把存在于工作区的材质包、未装配的 Niagara、未执行的 PIE 或未采集的 GPU Profile 写成完成。

### 体验目标与状态边界

Perception 的情绪是“在不确定中得到近处、可信、可读的线索”：神秘，但让玩家感到安全和有掌控感。它不是高压恐怖、惊吓或剥夺控制的后处理；不以持续红色告警、强烈屏幕震动、随机闪烁或不可预测的噪声制造压力。显现、声源、残响和眼睑都必须有稳定的时间/空间语义。

`ULRStateComponent` 维护状态合法性，`ULRStatePresentationComponent` 维护表现锁，`ULRPerceptionPresentationComponent`/`ULRPerceptionEventSubsystem` 只负责表现适配和回放，不复制解锁规则。渲染器、HUD、眼睑 Widget、Niagara 均不得直接解锁状态。

### 玩家显影与 ValidSceneSurface

- 玩家位置使用世界空间厘米。`RevealEligibility` 只回答玩家能否获得空间信息：`0–400 cm` 完整，`400–450 cm` 以洗染（wash）羽化，`>450 cm` 不产生 Player Reveal。`PaintStrength` 独立决定显影完整度：综合色块在 `280 cm` 后先衰减，WorldNormal 内部结构线在 `340 cm` 后衰减，SceneDepth Silhouette 保持到 `400 cm` 后才在外带消散。权威顺序必须满足 `PerceptionColorFullRadius <= PerceptionInternalEdgeFullRadius <= PerceptionFullRevealRadius <= PerceptionRevealRadius`，默认 `280/340/400/450 cm`。
- 每像素只计算一次低频 `WorldWash`，并将同一扰动距离复用于 Eligibility、Color、InternalEdge 与 Silhouette；wash 可以在各自衰减带内改变消散形状，但不得把 Player Reveal 推到 `450 cm` 外。wash 绝不能成为全局屏幕 Clip，也不能以玩家最大半径限制远处 Echo：Echo 的资格、半径和抵达时间独立计算，远处 Echo 即使位于 `450 cm` 外仍可显现。
- `ValidSceneSurface` 是所有 Player、Echo、Wet、Narrative Accent Reveal 的共同安全门：必须确认像素对应有效场景表面，并排除天空、背景/无场景深度和无效深度样本。无效表面输出 `BlindColor`，不得因 Stencil、wash 或颜色优先级绕过该门。该模式也必须能在 Debug View 中独立查看。

### 声音、Echo 槽与空间语义

运行时视觉声源默认使用 `NoiseRevealRadius=200 cm`。`ULRPerceptionSoundSourceComponent` 可用 `VisualRadiusOverrideCm` 配置实例半径，并在事件请求中写入 `VisualRadiusCm`；两者为非正值时才回退到 200 cm。这个半径只属于 Perception 视觉事件，与 AI Hearing 完全独立。声源勾选 `bAlsoEmitToAI` 时，才以独立的 `AIHearingRadiusCm` 向 `UAISense_Hearing::ReportNoiseEvent` 报告；不得把 AI 半径反向写入视觉半径，也不得把视觉事件自动当成 AI 噪声。

运行时永远只有固定 **8 个 Echo 槽**（`MaxEchoSlots=8`），不是按事件动态扩容，也不是 8 个后处理 Pass。分配顺序为：第一个 inactive 槽、其次第一个已过期槽、最后选择最早 `ExpireTime` 的槽。槽结构至少包含 `Center`、`RadiusCm`、`FirstStartTime`、`LastPulseTime`、`ExpireTime`、`Intensity` 与 `SourceObject`。

声源事件语义如下：

| 事件 | 合同 |
| --- | --- |
| 一次性/Gameplay Pulse | 不刷新旧空间源；在当前位置申请槽。没有可刷新的槽时按固定 8 槽分配策略替换。 |
| Looping Pulse | 仅当 `bRefreshExistingSource=true`、`SourceObject` 相同、槽未过期且新位置距已存中心不超过 `EchoRefreshMergeDistance=25 cm` 时刷新。 |
| 合法刷新 | 保留 `Center` 与 `FirstStartTime`，只更新 `LastPulseTime`、`ExpireTime` 和强度；小范围移动不重启历史波。默认 Loop 间隔为 `3 s`，正的事件间隔可覆盖它（例如 Wind 使用 `6 s`）。 |
| 超过 25 cm 的移动 | 不把移动声源伪装成同一静态源；旧槽按原位置继续衰减，并为新位置申请新的槽。 |

`NoiseRevealDuration=5 s`。每个槽使用三项时间戳（合同中简称“三时钟”）：首次入槽时写入 `FirstStartTime`，每次接受 Pulse/刷新时写入 `LastPulseTime`，并令 `ExpireTime = LastPulseTime + 5 s`。三者和 `CurrentGameTime` 均来自 `UWorld::GetTimeSeconds()`；材质使用 View Game Time，`Ignore Pause=false`、`Override Period=false`。暂停时 Game Time 不前进，时间膨胀按同一 Game Time 语义影响抵达和过期，不能改用 Real Time 或另建 CurrentTime MPC。

Echo 的抵达必须是 **per-pixel arrival**，而非整圈同时显示。对每个像素计算：

```text
ArrivalDelayPx = saturate(distance(WorldPosition, Center) / max(RadiusCm, epsilon)) * EchoExpansionSeconds
HistoricalAgePx = CurrentGameTime - FirstStartTime - ArrivalDelayPx
WetAgePx       = CurrentGameTime - LastPulseTime  - ArrivalDelayPx
```

其中 `EchoExpansionSeconds=0.75 s`，`HistoricalAgePx < 0` 的像素尚未抵达；最新 Pulse 的 Wet 边只在抵达后的 `EchoWetSeconds=0.20 s` 内成立，`EchoWaveWidthCm=12.5 cm` 只控制边缘宽度，不改变槽资格。槽总寿命为 5 s；不另存 `EchoDryFadeStartSeconds`，而以 `ExpireTime - CurrentGameTime` 推导最后 `EchoDryFadeDurationSeconds=1.30 s` 的干退区，干退起点固定为 `5.00-1.30=3.70 s`。刷新不会把 `FirstStartTime` 重置为当前时间，因此历史残响不会被错误地变成新的整圈扩张。

### 单一 Composite、HDR 与 Palette

Perception 只允许一个 `M_PP_LR_PerceptionComposite`，挂在 **Before Tonemapping** 的单一 Composite/Blendable 入口。进入 Perception 时添加并逐步写入权重，退出时逐步归零后移除；不得用多个 Perception 后处理材质叠加同一责任。Normal 稳定态的 Perception Composite Pass 权重必须为 `0`（理想状态为已移除），并用 GPU Visualizer 验证“零权重/移除”没有残留成本或颜色影响。

Composite 的值链必须先压缩 HDR，再进行调色板和表面塑形，不得在后面重新引入未经压缩的 HDR：

```text
HdrValue        = luminance(SceneColorHDR)
CompressedValue = pow(saturate((HdrValue / (HdrValue + ValueShoulder)) * ValueScale + ValueBias), ValueGamma)
```

`ValueShoulder`、`ValueScale`、`ValueBias`、`ValueGamma` 来自 `ULRVisualStyleDefinition`。`NormalColorRetention=0.20` 表示 Player 基础色最多保留 20% 的压缩 Normal/场景色相；它不是恢复 Normal 全彩的旁路，也不能把原始 HDR 重新接回。`RevealedValueFloor=0.075` 只可施加于已经通过 `ValidSceneSurface` 且已经 Reveal 的像素；Blind/invalid 像素保持 `BlindColor`，不得被 Floor 提亮。

Palette 必须是一张 **256×4** 纹理，四行固定为：Row 0 Player、Row 1 Echo residue、Row 2 Wet wave、Row 3 Narrative Accent。压缩值在 X 轴取样，行使用固定行中心（0.125、0.375、0.625、0.875），X/Y 必须 Clamp，并禁止 Mip/过滤造成跨行 bleed（Point 或等效的显式行采样）。候选颜色的选择是优先级而不是最大值混合：`Narrative Accent > Wet > Echo residue > Player`；先由 `ValidSceneSurface` 和各候选 RevealMask 决定资格，再按该顺序选择唯一行。`RevealMask` 可以聚合候选的可见资格，但不能用“哪个强度最大”改变颜色语义。

所有已显现表面允许一个 8–15% 的暗面 ShapeLift，默认 `ShapeLiftStrength=0.10`，沿 `ShapeDirection` 的 Half-Lambert/等效方向项恢复形体；不得将其施加到盲区或 invalid surface，也不得提升到破坏安全感和调色板层级的高对比。`LR_Palette`、`LR_RevealedValueFloor`、`LR_NormalColorRetention`、`LR_ShapeLiftStrength` 等参数由 PP MID 写入；Perception 运行时位置和 8 槽数据由 `MPC_LR_PerceptionRuntime` 写入，状态门控由 `MPC_LR_VisualStyle` 写入。

### Outline、Stencil 与材质边界

艺术线必须 Reveal-gated，未知区域不能仅因几何边缘、Normal 或 Stencil 生成可见线。Perception Composite 使用相对 SceneDepth 梯度（Silhouette）与 WorldNormal 梯度（InternalEdge）检测边缘，但必须拆成两个视觉层：稳定 `InkEdge` 使用 `ULRVisualStyleDefinition::InkEdgeTint` 的深蓝紫/深青灰墨线，并随 340/400 cm 两级结构强度衰减；`WetEdge = ArtEdge × WetRevealMask` 使用浅青亮边，只存在最新声波抵达后的 `EchoWetSeconds=0.20 s`。普通 ArtEdge 不得借用 Wet Palette。Perception 进入期间 `NormalOutlineGate = 1 - StateBlend`，稳定 Perception 时为 0；因此 Perception 关闭 Normal Outline，不得把 Normal 外轮廓作为感知内容。交互表现由 `InteractionPresentationGate` 在 Perception 活跃期间抑制，退出后重新应用。

Stencil 值是互斥的固定语义，不使用位拼接：

| Stencil | 语义 | 可见性合同 |
| --- | --- | --- |
| 1 | `InteractionSelected` | Perception 活跃时由 Interaction Presentation 抑制；不转化为 Echo 或 Accent。 |
| 2 | `PlayerOccluded` | Player 的明确遮挡例外，可穿过预期遮挡物保留玩家轮廓/状态色；它不继承 Stencil 3 的同表面深度容差。 |
| 3 | `PerceptionNarrativeAccent` | 只有同时满足 `ValidSceneSurface` 与线性深度门控才可见；Stencil 值本身不是可见证明。 |

Stencil 3 的可见门控至少为：

```text
VisibleAccent = (CustomStencil == 3)
              * (abs(LinearCustomDepthCm - LinearSceneDepthCm) <= AccentDepthToleranceCm)
              * ValidSceneSurface
```

默认 `AccentDepthToleranceCm=3 cm`。Stencil 2 是唯一明确的遮挡例外；它不能被错误复用为 Narrative Accent，也不能让 Stencil 3 绕过深度比较。

透明、玻璃和 Decal 的边界必须显式记录：普通 Thin Translucent/玻璃不保证提供与 Opaque 相同的 CustomDepth/SceneDepth 证明，不能据此承诺独立 reveal、Stencil 3 深度门控或遮挡关系；若 Hero Glass 必须独立显现，应采用已批准的 Masked/Dithered 变体并单独验收，不在本章偷偷增加新的渲染路径。Color-only Decal 只改变有效 Receiver 的颜色，不写 Perception CustomDepth/Stencil，不得把颜色投射、DBuffer 或 Decal 响应误当成 Reveal，也不得向地板、角色、植被或玻璃产生 reveal bleed。

### UI 眼睑与唯一解锁入口

`/Game/LostRunic/UI/WBP_LRStateEyeOverlay` 是 `ULRStateEyeOverlayWidget` 的双眼睑 CanvasPanel：`TopBlock` 与 `BottomBlock` 分别锚定上下半屏，零偏移，Root/Images 为 `HitTestInvisible`。Widget 只表现 hold/成功/取消/拒绝，不查询世界、不判定状态、不报告解锁完成。稳定状态下黑块始终 `Opacity=0` 且 `Collapsed`；Perception 只保存“闭合”几何端点，不能把 0.8 透明度保存成稳定态。

- CloseEyes Hold 时上下眼睑从开放向闭合，使用 gameplay 提供的 hold 时长；成功后在闭合端快速透明并折叠，露出已经提交的 Perception。
- OpenEyes Hold 一开始，Renderer 仅把画面预览到 Normal 端点，眼睑滑开后露出的必须是 Normal；该预览不改变 `CurrentMode`、不关闭 Perception Event Bus，也不清空 Echo 槽。达到阈值后仍由正式状态事件执行退出与清理。
- 取消/拒绝时，Renderer 立即恢复 Perception 端点，眼睑按 `EyeOverlayCancelSeconds<=0.15 s` 回到已提交状态的几何端点并淡出；最终必须透明、`Collapsed`。达到阈值时眼睛视觉上已打开 `EyeOpenThresholdVisualProgress=0.90`，最后 10% 不改变玩法时机。
- Viewport Z-order 使用显式合同：StateOverlay `0`、HUD `5`、Narrative/Dialog/Menu `10–14`、Transition `30`。眼睑只遮挡世界，不得遮挡正常 HUD，也不得依赖 Widget 创建顺序。
- 状态表现窗口由同一权威调优控制：进入 Perception 为 `0.30 s`，退出 Perception 为 `0.20 s`。`BP_Ruth` 只能在 `OnStatePresentationRequested` 后选择相应延迟，并且只安排一次 `CompleteStatePresentation()`；`ULRStatePresentationComponent::CompleteStatePresentation()` 是状态表现锁唯一的正常解锁入口。Renderer、HUD、Eye Widget、Niagara 和 PP Composite 均不得调用它或复制它。

### Niagara 责任

`NS_LR_PerceptionPulse` 只能是事件的装饰性脉冲外壳：可读取位置、半径、强度和扩散时长来画波纹，但不得写 Echo 槽、RevealMask、AI Hearing、StateComponent 或解锁。当前候选资产已使用 CPU 单粒子、世界空间 Sprite，读取 `User.Radius`、`User.ExpansionSeconds`、`User.WaveWidthCm`、`User.Intensity` 与 `User.Tint`；装饰遵守普通场景深度，不写 CustomDepth/Stencil，不承担遮挡例外。Niagara 缺失、加载失败或被平台禁用时，只移除装饰，不得关闭 C++ 规则、槽时间、Per-pixel reveal 合同；其最终材质/粒子成本仍须单独 Profile。

### 实现状态（截至 2026-09-06）

| 范围 | 状态 | 证据与未完成项 |
| --- | --- | --- |
| C++ 状态边界、8 槽、三时间戳、刷新/移动声源、AI Hearing 分离、调优校验 | **已实现并通过定向测试** | `Source/LostRunic/Perception/`、`Source/LostRunic/Data/` 已提供运行时合同；2026-09-05 构建后 `LostRunic.Perception` 4 项与 `LostRunic.Tuning` 3 项自动化测试全部通过。 |
| 眼睑 Widget、HUD Screen Class、`BP_Ruth` 单一 Complete 调用装配 | **已修正候选** | 稳定遮罩透明/折叠、OpenEyes Normal 预览、取消恢复 Perception 与显式 HUD Z-order 已进入代码；Blueprint 已编译/保存，仍需在 `L_PIE_Test` 完成最终视觉验收。 |
| `MPC_LR_VisualStyle`、`MPC_LR_PerceptionRuntime` 文件 | **已装配候选资产** | Runtime MPC 为 `PlayerPosition + 8×(CenterRadius/Timing)` 共 17 个 Vector；VisualStyle MPC 为四个状态/门控 Scalar 与 `PlayerOcclusionColor`。`DA_LRPresentationTuning` 已绑定两套 MPC；实际 PIE 写值仍待验证。 |
| `M_PP_LR_PerceptionComposite`、256×4 Palette、Reveal/Outline 材质图 | **已修正候选资产** | Composite 为 UE 5.8 `MD_PostProcess / BL_SceneColorAfterDOF`（Before Tonemapping 当前枚举），单 Emissive 输出已连接；8 槽、HDR/Palette、Reveal、可见 Stencil 3 与 Stencil 2 例外已进入 Custom HLSL。玩家场已拆为 280/340/400/450 cm 的 Color/Internal/Silhouette/Eligibility 层级，稳定 ArtEdge 使用深色 InkEdge，浅青亮边只属于 Wet Wave；2026-09-06 已完成材质编译、`LostRunic.Perception` 4/4 定向自动化、完整 Editor 构建与 `L_PIE_Test` 稳定 Player Reveal 冒烟。Wet Wave 有声源主观观感及 GPU Profile 仍待签字。 |
| Niagara pulse | **已装配候选资产** | `NS_LR_PerceptionPulse` 与 `M_LR_PerceptionPulse` 已编译、保存并绑定 Presentation Tuning；Niagara Stack 为 0 error/0 warning。它仍是可选装饰，PIE 观感与 GPU Profile 待验证。 |
| PIE（距离、Echo、Pause/Time Dilation、Stencil、透明/Decal、唯一解锁） | **部分验证** | `L_PIE_Test` 已确认稳定 Perception 可进入、综合色按距离分级且普通边缘不再使用浅青 Wet 语言；完整 Echo、暂停/Time Dilation、Stencil/透明对照和 Wet Wave 动态签字仍待验证。 |
| GPU Visualizer / `stat GPU` | **待验证** | 尚未锁定目标最低配置 GPU，也未取得 Normal 零权重与 Perception Composite 分项 Profile。 |

### Candidate 性能预算与 Gate 0–3 验收

以下是 Vertical Slice 的候选预算，不是实测通过声明：

| 项目 | Candidate 目标 | 当前状态 |
| --- | --- | --- |
| Perception 单一 Composite（含其必要纹理/采样开销） | 1080p ≤ `1.5 ms`；1440p ≤ `2.5 ms` | **待验证**，需目标 GPU GPU Visualizer。 |
| Normal 稳定态 Perception Pass | Composite 权重 `0`/已移除，零残留颜色与可归因 GPU 增量 | **待验证**。 |
| Echo runtime | 固定 8 槽、事件/Timer 驱动；无永久 Perception Tick、无每槽独立 Pass | **已实现**，仍需 PIE 与 Profile 证明。 |
| Niagara | 仅装饰，按平台可禁用；不得成为规则依赖 | **已装配候选资产**，PIE 与 Profile 待验证。 |

| Gate | 验收内容 | 通过条件/当前状态 |
| --- | --- | --- |
| Gate 0 — 数据与资产完整性 | 校验 `400≤450`、`5/0.75/0.20/1.30`、200 cm 默认；确认 8 槽、三时间字段、两 MPC 参数名、单 Composite Before Tonemapping、Palette 256×4 Clamp/无跨行 bleed、Stencil 1/2/3、`ValidSceneSurface` | C++ 数值/API、MPC、Composite 编译、Palette 导入属性与 DataAsset 引用已取得 Editor 证据；实际运行写值仍 **待验证**。 |
| Gate 1 — 空间/时间语义 | 400/425/450/451 cm 玩家断点；450 cm 外的 Echo；1/8/9 Pulse 槽替换；同源 ≤25 cm 刷新与 >25 cm 空间分裂；5 s 过期、0.75 s per-pixel arrival、3.70 s dry 起点；暂停约 5 s 和 0.5×/2×时间膨胀 | 规则与测试定义 **已实现**；测试关卡运行与证据 **待验证**。 |
| Gate 2 — Composite/边缘/材质边界 | Reveal-gated Art Edge、稳定 Normal Outline=0、Stencil 3 线性深度门控、Stencil 2 遮挡例外、Player/Echo 优先级、HDR 压缩/Floor/ShapeLift、透明/玻璃/Decal 边界 | C++ 门控参数与 Composite 候选 **已实现**；既有 Style Outline、Interaction Outline 与 Player Occlusion 已接入 VisualStyle MPC 并重新编译。PIE 中的屏幕空间边缘、深度结果和透明边界仍 **待验证**。 |
| Gate 3 — 体验/整合/性能 | 眼睑 hold/0.12 tail/0.15 cancel、0.30/0.20 表现窗口、唯一 `CompleteStatePresentation`；Niagara 缺失不影响规则；Normal 零权重；1080p/1440p Candidate Profile；Output Log 无新增项目级 Warning/Error | UI 与 Niagara 候选资产 **已装配**；PIE、GPU 和最终体验签字 **待验证**。 |

所有 Gate 必须在 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test` 使用实际 Gameplay Camera 验收；记录静止/移动相机、暂停/时间膨胀、1/8/9 槽、深度遮挡和透明/Decal 对照。Gate 通过前，本章保持候选合同状态，不升级为已验证 Production Baseline。

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
- Exposure（旧 A–F 历史校准记录）：保留 `BenchmarkCalibratedEV = -2.14`；Gray18 ROI 为 `(0.43, 0.68)–(0.57, 0.75)`，线性中位亮度 `0.1774`，满足 0.18 目标与 ±0.03 容差。该历史数值不得直接作为新的 PPV `Exposure Compensation`；新 PPV 必须单独重新校准。
- Lighting：固定 `ArtBench_LightingROI = (0.02, 0.02)–(0.98, 0.98)`；Isolation 对照通过。截图/Profile PIE 临时禁用项为 `DirectionalLight_1`、`SkyLight_1`、`RectLight_0`、`RectLight_3–11`、`PostProcessVolume_1`，停止 PIE 后恢复。
- Art Outline：`M_PP_LR_StyleOutline` 使用 `After DOF / BlendablePriority = 0`；Raw Depth、Relative Depth、Normal 与 FinalArtEdge 调试输出均已分别验证，材质编译无错误。
- Temporal：TSR（`r.AntiAliasingMethod=4`）下完成固定相机连续平移、动态分辨率以及 1080p、1440p、4K 检查，未观察到明显闪烁、爬线、线宽跳变或远景爆线。证据位于 `Saved/ArtBenchmark/`。
- Interaction：`MF_LR_InteractionEdgeDetector`、8-tap `M_PP_InteractionOutline`、0.707/1.0 A/B 实例已编译；圆角/斜边方向选择 0.707。材质使用 `After DOF / BlendablePriority = 10`；无遮挡、部分遮挡、艺术线叠加和内部线检查通过。
- 自动化：`LostRunic.Interaction.OutlineStencilClearsAcrossTargets` 通过，覆盖 A 选中、A→B、B→None 与无残留 CustomDepth；无 Warning/Error。
- 构建：`LostRunicEditor Win64 Development` 完整构建通过。当前会话未发现 Shader/Material Error。
- 性能：本轮只记录实现趋势；最低 GPU 未锁定，GPU Visualizer 的硬预算验收仍是 Production Baseline 升级前置项。

### 19.2 Normal Material Coverage G–L 验证记录（2026-08-31）

- G：`M_LR_StylizedOpaque` 已接入独立 `SurfaceUTiling` / `SurfaceVTiling`；BaseColor、SMK、Normal 使用 SurfaceUV，Detail 使用 `SurfaceUV × DetailTiling`，Painterly 保持独立 UV0。Opaque 固定 5 samples、0 Static Switch。
- H：已创建 `M_LR_StylizedFoliageMasked`、`MI_LR_Benchmark_Foliage` 与确定性大叶簇 BC/SMK/Normal；Master 为 Masked、Default Lit、Two Sided、Decal Response None，4 samples、0 Static Switch。正反面基准观察未发现反转高光；密集正式 foliage 与最终 Overdraw 预算仍未宣称完成。
- I：已检查 Ruth 当前为四张独立 BaseColor/Roughness/Metallic/Normal，而非 packed map；创建 `M_LR_StylizedCharacter` 与 `MI_LR_Ruth_Child`，固定 Atlas UV0、Decal Response None、5 samples、0 Static Switch，并应用到 `BP_Ruth.CharacterMesh0`。
- J：已创建 `M_LR_StylizedGlass` 与基准实例；当前为 Thin Translucent、Surface ForwardShading、无 Refraction、Decal Response None，1 sample。Wash 只进入综合色，Opacity 只由常量与 Fresnel 控制。
- K：已创建 Color-only `M_D_LR_StylizedColor`、确定性裂纹 Mask 与 Benchmark Decal。Opaque Receiver 为 Color；Foliage、Character、Glass 为 None。固定相机未观察到向地板、Ruth、Foliage 或 Glass 的投影穿透。
- L：`L_PIE_Test` 的全局 `LR_PostProcess` 承载 Priority 0 Art Outline 与最终 0.707 Interaction Outline 实例；局部 `ArtBench_PPV` 不再包含 Blendable；`ArtBench_Door` 以 Stencil 1 作为 Combined Gate 目标。固定相机 PIE 已同时显示 Opaque、Foliage、Ruth、Glass、Decal 与双描边；`LostRunic.Interaction.OutlineStencilClearsAcrossTargets` 通过（1/1，0 Warning / 0 Error）。
- 编译：五个 Surface MF、Opaque/Foliage/Character/Glass/Decal 与两套 PP Material 已重新编译。当前结构统计为 Opaque 5、Foliage 4、Character 5、Glass 1、Decal 1 samples，均为 0 Static Switch；当前重新编译未产生新的 Material/Shader Error。构建阶段曾出现的 Thin Translucent output/Lighting Mode 与 Deferred Decal Blend Mode 警告属于节点尚未接完时的中间状态，最终状态已复编通过。
- 待升级项：MCP 当前未提供 CVar 写入、GPU Visualizer 或持久截图导出接口，因此 1080p/1440p、70%/100%、4K Smoke、Shader Complexity/Quad Overdraw、DBuffer 与分项 GPU 增量仍列为 Candidate Baseline 的人工 Profile 门禁，不在本记录中伪造通过。

### 19.3 M1 Cluster-Based Foliage Prototype 验证记录（2026-09-02）

- 确定性源：`Tools/ArtBenchmark/generate_m1_foliage_textures.py` 生成共享 BC/SMK/Normal；`Tools/ArtBenchmark/generate_m1_foliage_meshes.py` 生成中型树、三种 Grass Clump 与 Hero Tuft FBX。树冠使用 4 个 Lobe、80 个 Cluster Card 的 M1 组织起点；该数量不是生产预算。
- 绕序门禁：修复 Trunk/Branch cylinder 与 Canopy Card 绕序后，Blender 导出器逐面检查 `Dot(FaceGeometricNormal, Normalize(Average(VertexCustomNormals))) > 0.5`；平均 authored normal 近零直接失败，并输出对象/材质区/面索引/dot。当前 Tree `Trunk min/avg = 0.998836/0.999229`，`Canopy min/avg = 0.655953/0.893891`，门禁 PASS。
- 导入：树冠自定义法线与 Vertex Color 由 DCC 写入；UE 通过 `ImportAssets` 回退流程重导入至正式资产（MCP 仅提供新建导入，无法对现有包直接 Reimport），设置为 `Import Normals`、`Recompute Normals Off`、`Recompute Tangents On`、`MikkTSpace On`、weighted normals off、两槽保留。最新正式资产的 AssetImportData tag 为 `Timestamp=1788322961`（FBX mtime：`2026-09-02 04:22:41 UTC`）、`FileMD5=1492ae5d7b6ba9995ab93f6477323f9b`，与当前 FBX 一致。树 LOD0 为 1616 vertices / 568 triangles、1 LOD，Bounds 约 `4.83 m × 3.34 m × 5.60 m`；树干保留 1 个简单碰撞，Grass/Hero 不生成碰撞。TreeDensity `0_0..2_2` 九个 StaticMeshActor 均绑定该正式 Tree，变换保持原值。
- Atlas：`T_LR_M1_Foliage_BC` 为 sRGB/World/Default，生成 Mips，并启用 `Do Scale Mips for Alpha Coverage`，只启用 `A Threshold = 0.4`；SMK 为 Linear Masks，Normal 为 Normalmap。当前项目没有独立 Foliage Texture Group，因此本候选使用 World。
- 材质：`M_LR_StylizedFoliageMasked` 新增以 0.5 为中性的 Vertex Color RGB 相对综合色链路；`FoliageMassTintRange = 0.2`，Master 的 `FoliageMassTintStrength = 0`，M1 实例为 1。Vertex Color A 未接入。Master 重新编译成功，仍为 4 Texture Samples、0 Static Switch。
- Benchmark：`L_PIE_Test` 的 `ArtBenchmark/Normal/FoliageM1` 包含 NormalStrength 0/0.25 树木 A/B、Grass Band、Hero Tuft、Single/Density Cell 与固定 Neutral Key/Fill/PPV。已按 Stage 1（NormalStrength 0 + Default Lit + Neutral Lighting + Painterly/Art Outline Off）和 Stage 2（仅切 0.25）记录验证要求；当前 MCP 无法切换 WorldNormal/视口 Buffer Visualization 或持久截图，因此两阶段视觉连续性列为人工 Gate，不能伪造 PASS。Stage 1 通过后才允许执行 25% 缩略图 Cluster Silhouette Gate；若仍由条状/edge-on Card/空洞主导，应归类为 Geometry/Orientation/Alpha Density，不在本轮改 Shader 或 Cluster 分布。
- 后处理：`LR_PostProcess` 为全局 `Priority 0 / Unbound`，承载艺术描边与最终 `MI_PP_LR_InteractionOutline_Diag0707`；`ArtBench_PPV` 的重复 Blendable 已清理。`ArtBench_M1_NeutralPPV` 已移动/缩放至完整 FoliageM1 bounds 加约 100cm 边界（约 `X 4450–8100 / Y -1300–1300 / Z -102–660 cm`）。
- 构建：关闭 Live Coding 后，`LostRunicEditor Win64 Development` 完整构建通过；只有既有 UE/API 弃用警告。
- 未通过声明：当前 Density Cell 使用多个 StaticMeshActor 形成固定屏幕像素压力代理，不代表 Static Mesh Foliage/HISM 的实例提交或剔除成本。1080p/1440p、SP 70/100、Near/Mid/Far、Shader Complexity/Quad Overdraw Proxy、综合色缩略图与最终主观签字仍待人工 Profile；M1 不是 Production Baseline。
- 独立遗留：当前编辑器历史日志仍保留 G–L 阶段 `M_LR_StylizedGlass` 与 `M_D_LR_StylizedColor` 的旧编译失败记录。它们不是本次 M1 修改产生，但在重新形成无错误全局 Rendering Baseline 前必须单独复编并清除验证。

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
- [x] 艺术描边与交互描边统一使用 After DOF，并以 Priority 0/10 固定覆盖顺序；最低配置 GPU Profile 另列为 Production Baseline 前置项。
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
- Perception 的视觉与技术合同见本文专章；Courage、Memory 的最终视觉标准仍另行评审，且不得反向破坏本版 Normal 的候选生产基线。
