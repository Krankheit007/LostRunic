# P2.5 Asset Authoring Standard

状态：**Candidate / P2.5 实施标准**（2026-09-08）
适用范围：LostRunic Normal 状态、Windows PC、UE 5.8、俯视 Gameplay Camera 的墙、地板、木件、织物、地毯和玻璃资产。

本轮最新范围：用户否决新贴图候选，要求保留现有贴图，直接调整 UE 光照、后处理和描边。本文保留为未来制作标准，不要求按这些规则重制或替换当前首批贴图；WatercolorSamples 不接入 Content。

本文档把 `Docs/Art/01_LostRunicArtBibleAndTechnicalArtStandard.md` 的材质、UV、SMK 和导入约束落成一条可执行的资产交付流程。它是生产门禁，不替代 Art Bible、`TextureManifest.json`、`ClothAtlasManifest.json` 或 UE 实际资产记录。本文档本轮只新增文档；没有运行 UE/Blender，也没有把尚未完成的生成器当作依赖。

## 1. 来源、状态和术语

### 1.1 权威来源

- 视觉语言、Master 划分、SMK RGBA 语义和颜色空间：[`01_LostRunicArtBibleAndTechnicalArtStandard.md`](01_LostRunicArtBibleAndTechnicalArtStandard.md)。
- LivingRoom 交付链：`ArtSource/LivingRoom/Delivery/README.md`、`Manifest.json`、`TextureManifest.json`、`Validation.json`。
- 当前 UE 导入记录：`ArtSource/LivingRoom/Iteration02/UEImport/README.md`、`Assembly.json`、`UEValidation.json`、`Materials.json`、`TriangleDiagnostics.json`。
- 当前 Iteration02 的材质来源和限制：`ArtSource/LivingRoom/Iteration02/Analysis.md`、`ClothAtlasManifest.json`。

### 1.2 证据标签

文档、Manifest 或日志已经记录的结果写作“已验证记录”；通过规则推导但尚未在目标相机/UE 中检查的内容写作“候选”；没有证据的内容不得写成 PASS。自动回读通过只说明文件结构和数值契约通过，不等于 Gameplay Camera 的视觉通过。

### 1.3 六类表面共同原则

资产先由轮廓、结构面和材质大色块成立，再用有限方向性细节补充识别。所有材质默认保持干净的低频组织：

- BaseColor 不铺满面随机噪声，不烘焙 AO、镜面高光、Lumen/VSM 阴影、接触阴影或其他灯影。
- 不默认添加灰尘、旧化、脏边、随机污点或重复磨损。需要故事痕迹时，使用有明确位置和理由的局部绘制或 Decal，并记录其来源；不能用全表面旧化填空。
- 允许低频、方向明确的手绘笔触，例如沿木纹、板缝、墙纸节奏或地毯边带组织颜色。笔触不能变成另一层高频噪声，也不能替代真实几何、实时灯光或结构线。
- 关闭 Art Outline 后，轮廓、材质和结构仍应可辨认。不能通过加黑线挽救 BaseColor、法线或形体问题。

## 2. 颜色、SMK 和法线契约

### 2.1 SMK 的固定 RGBA 语义

所有使用 SMK 的不透明表面都沿用 Art Bible 的语义；资产不能重新解释通道。

| 通道 | 固定语义 | 允许内容 | 禁止内容 |
| --- | --- | --- | --- |
| R | `DetailAmount` | 控制木纹、墙纸、织物方向等选择性细节 | 玩法状态、曝光或灯光遮罩 |
| G | `StructureCavity` | 艺术家明确画出的门缝、面板/柜门缝、地板板缝和少量装饰结构线 | 整模型 AO、自动 Cavity、全表面曲率暗化、光照渐变 |
| B | `MacroVariation` | 大块的颜色、粗糙度或故事变化 | 高频颗粒、随机灰尘密度 |
| A | `SecondaryMaterial` | 明确存在的第二材质区域；不需要时省略 Alpha | 临时 Detail、AO、磨损或自定义资产私有语义 |

`SMK.G` 必须是清楚的结构线而不是“凹处都变黑”。门缝、板缝和装饰结构线已经由几何或实时光照充分表达时，G 应减弱或归零。Delivery 候选的 `TextureManifest`/README 记录 G 只对地板板缝使用；当前 `Iteration02/UEImport/README.md` 记录的是一张共享 `Neutral SMK`，其 G 为 0，但 R/B 仍保留 `DetailAmount`/`MacroVariation` 语义，不能把它称为全零贴图。每次交付都要单独查看 `SMK_G_StructureCavity` Debug View，确认没有 AO 化。

推荐的输出关系仍为：

```text
资产 SMK → Material Instance 强度 → 允许的区域覆盖 → 章节 Global 参数
```

不得在某个资产中改变通道含义，也不得把 Vertex Color 的 `G WearOverride` 当作 SMK.G 的替代品来存放灯影。

### 2.2 颜色空间和贴图导入

| 资源 | UE 色彩空间 | 压缩/设置 | 规则 |
| --- | --- | --- | --- |
| BaseColor (`*_BC`) | sRGB **On** | Color/Default，生成 Mip | 只保存材质颜色和允许的低频方向性笔触 |
| SMK (`*_SMK`) | sRGB **Off**，Linear/Non-Color | Masks，生成 Mip；不用 Alpha 时移除 | 按固定 RGBA 语义采样 |
| 切线空间 Normal (`*_Normal`/`*_NRM`) | sRGB **Off** | Normalmap/BC5，生成 Mip | 只表达中尺度结构，不把灰度高度图直接接 Normal |
| Roughness（若单独提供） | sRGB **Off** | Linear/Non-Color | 材质类别和低频块面变化；不放进 SMK 未声明通道 |

普通 Streaming 材质源交付必须使用 POT 尺寸，并在 UE 实际回读中确认 `Power Of Two Mode`、Mip 生成设置和完整 Mip 链；仅勾选 Generate Mips 不算通过。当前 imagegen 原图墙/地/木约 `1254×1254`、地毯 `1086×1448` 均为 NPOT，应先制作保持 UV 与图案比例的 POT 候选；原 PNG 分辨率不是物理尺度依据。

提供 Normal 的静态网格，本轮候选配置为：`Normal Import Method = Import Normals`、`Recompute Normals = Off`、`Recompute Tangents = On`、`Use MikkTSpace = On`；这些字段必须在实际 UE 导入后回读确认，不能据此宣称现有六类资产已经达标。导出时保留 authored/custom normals；`export_delivery.py` 的三角化使用 `keep_custom_normals=True`。没有合理中尺度结构时可以不提供 Normal，保持材质实例的 `NormalStrength = 0`，不得把旧 BC 派生法线强行接入新 BC。

## 3. 六类表面标准

前五类表面默认使用 `M_LR_StylizedOpaque`，玻璃走独立的 `M_LR_StylizedGlass`。下表的 Slot 是新增资产的语义角色建议；已有 R02 资产若使用带 `ROOT_*` 语义和导入后缀的槽名，必须保留现有槽名、顺序和绑定，不为追求命名统一而重命名。新建槽禁止 `Material_0`、按文件随机生成的数字槽名或同一槽在不同资产中改变职责。

| 类别 | Master / Slot | BaseColor 起点 | SMK 使用 | Normal 与 Roughness | UV0 / 物理尺度 |
| --- | --- | --- | --- | --- | --- |
| 墙（Wall/Plaster/Wallpaper） | `M_LR_StylizedOpaque` / `Wall` 或 `Plaster` | 大块墙色、少量墙纸节奏或低频定向笔触；默认无灰尘旧化 | R 只开必要细节；G 只开门缝、墙板/装饰接缝；B 为低频大块变化 | Normal 可省略或保持弱中尺度；粗糙度偏哑光，不做颗粒镜面 | UV0 沿建筑方向展开；`SurfaceUTiling/SurfaceVTiling` 默认 `1.0`，尺度由 UV0/Trim UV 负责 |
| 地板（Floor） | `M_LR_StylizedOpaque` / `Floor` | 保留木板/石板方向和大块色差；不烘焙窗光、接触影或 AO | G 的非零区域只允许真实板缝/装饰结构线；不是整地面 AO | Normal 可省略；粗糙度使用材质级低频变化 | 连续米制 UV0，板缝方向稳定；不要为墙长创建不同 MI |
| 木（Wood/Trim/Furniture） | `M_LR_StylizedOpaque` / `Wood` | 宽而清楚的木纹方向和色块；不画毛刺、细孔、随机划痕满版 | G 只画门缝、面板缝、抽屉缝和装饰结构线；B 负责宏观色差 | 只有能改变中形的结构才用 Normal；木材保持受控哑光高光 | UV0 沿木纹/Trim 方向；材质族可调 Tiling，但不能替代实体 UV |
| 织物 Atlas（Cloth Atlas） | `M_LR_StylizedOpaque` / `Cloth` | 以家具组的低频色块和有限方向性笔触为主；无默认织物噪声/旧化 | G 通常为零；只有明确缝线等结构线才写入 | 当前 R02 织物不加附加 Normal；需要时使用弱中尺度 Normal，粗糙度偏哑光 | 精确使用家具组 Atlas UV0；`Surface U/V = 1`，不得在 Atlas 上叠加平铺 |
| 地毯（Rug） | `M_LR_StylizedOpaque` / `Rug` | 低频中心色块和克制的边带节奏；不默认脏污、磨损或满版细纹 | G 只可表达明确边框/缝接等装饰结构线，不能模拟绒毛 AO | Normal 通常省略；粗糙度偏哑光，可用低频整块变化 | 顶面保留完整矩形边框映射（通常为 `0..1`）；不能为方便打包破坏边带比例 |
| 玻璃（Glass） | **独立** `M_LR_StylizedGlass` / `Glass` | 不强制提供 BC/SMK/Normal；由 Tint、Transmittance 和 Fresnel 表达存在感 | 不把 Opaque 的 SMK 契约强塞进玻璃；若以后确有贴图需求，另行记录其输入语义 | `Translucent / Thin Translucent / Surface ForwardShading / Two Sided Off`；无 Refraction、Scene Color、WPO 或完整 Opaque 功能 | UV0 只有在显式贴图需要时才要求；不以任何 Actor/贴图缩放补偿透明度 |

玻璃的默认透明度仍由 `Opacity = Saturate(OpacityBase + Fresnel × EdgeOpacityStrength)` 控制；Fresnel 只帮助识别玻璃，不能形成比 Art Outline 更醒目的第二圈轮廓。玻璃没有“必须凑齐三张贴图”的要求，也不得用假噪声贴图掩盖材质设置错误。

## 4. UV、Atlas、Pivot 和材质槽

### 4.1 UV 尺度权威

UV0 是颜色、SMK、Normal 和方向性结构的主要输入。建筑材质的物理尺度由 Mesh UV/Trim UV 决定，Material Instance 的 `SurfaceUTiling` / `SurfaceVTiling` 只用于材质族调整或有记录的例外；不能按 2m、4m、6m 墙长各建一份实例，也不能使用 Everywhere Triplanar。

不透明 Master 的默认关系为：

```text
SurfaceUV   = UV0 × float2(SurfaceUTiling, SurfaceVTiling)
BaseColor   → SurfaceUV
SMK         → SurfaceUV
Normal      → SurfaceUV
DetailUV    = SurfaceUV × DetailTiling
PainterlyUV = UV0 × PainterlyTiling
```

方向性表面要在 DCC 里保持真实朝向：地板板缝沿板方向，木件沿木纹，墙纸沿图案方向。布料 Atlas 和地毯的完整边框是图案契约，不能以统一的世界平铺覆盖。

### 4.2 布料 Atlas 不可破坏

当前 `ClothAtlasManifest.json` 记录了七组 2048 Atlas：`ROOT_Sofa`、`ROOT_ArmchairOchre`、`ROOT_ArmchairRusset`、`ROOT_Ottoman`、`ROOT_WoodChair`、`ROOT_CoatStand`、`ROOT_Curtains`。记录中的 `packed group atlas; baked box mapping` 是现有 R02 候选的 UV 描述，不是任意新资产的免检许可。

- Atlas 岛、旋转、镜像、比例、边框和 Padding/Gutter 一经交付即视为接口；不得在 UE 或后续导出时重排、裁切或统一缩放。
- 任何 Atlas 变更都必须同时重新导出所有依赖该 Atlas 的 Mesh，并更新 `ClothAtlasManifest.json`、贴图名和版本记录；只替换图片会造成 UV 语义漂移。
- UE Material Instance 的 Surface U/V 固定为 `1.0/1.0`。发现布料侧面拉伸时先回到 UV0/Atlas 岛修正，不用 MI 平铺掩盖。
- Mip 采样必须有足够 Gutter，透明或空白边缘不得用会污染 Mip 的纯黑颜色；这一条在目标相机 Near/Mid/Far 回读时检查。

地毯顶面遵循同样的接口思路，但保持完整边框映射而不是家具组 Atlas；当前 `apply_surfaces.py` 对 `ROOT_Rug` 顶面按局部包围盒写入 UV0，交付时不得再把它打散成独立小岛。

### 4.3 Pivot、父子层级和槽位

- 每个独立资产以带 `independent_asset` 的根节点归组；导出名由根节点去除 `ROOT_`/`PROP_` 后形成 `SM_LR_*`。根节点是局部原点，不能为“看起来居中”而重置。
- DCC 源和 FBX Manifest 保留静态家具的原资产局部原点与父子关系；门、抽屉等未来要动的部件保留 `PIVOT_*` 局部轴心。当前 LivingRoom 导出是静态 FBX，不烘焙 Actions，也不因此伪造动画已完成。
- 导出 Manifest 中的 `assembly_world_matrix` 是场景装配契约；组件局部变换不能在导出后被隐式应用、镜像或归零。
- 新增 Material Slot 最少化但语义稳定。若一个新资产同时有木、布和玻璃，保留三个明确职责的槽并分别指向对应 Master；不要为了减少槽而把玻璃接入 Opaque。已有槽位按现有 R02 绑定继续使用。

## 5. 当前 LivingRoom 交付链和单位门禁

### 5.1 已存在的 Blender 交付脚本

`ArtSource/LivingRoom/Delivery/README.md` 记录的生产脚本顺序为：

```text
Tools/Blender/LivingRoom/generate_surface_textures.py
→ Tools/Blender/LivingRoom/apply_surfaces.py
→ Tools/Blender/LivingRoom/export_delivery.py
→ Tools/Blender/LivingRoom/assemble_delivery.py
```

它们的职责是：

1. `generate_surface_textures.py` 生成 Delivery 阶段的低频 BC/SMK 候选，当前脚本使用确定性程序变化并把 Floor 的 G 约束在板缝；它不是 Iteration02 当前四张修订 BC 的来源。
2. `apply_surfaces.py` 把 BC/SMK 以 sRGB/Non-Color 节点接入 Blender 材质，并为 `ROOT_Rug` 顶面保留完整 UV 映射。
3. `export_delivery.py` 按独立资产根复制 Mesh/Empty，保留局部层级和 authored normals，导出二进制 FBX，随后在 Blender 内重新导入检查网格、三角面、Bounds、材质槽、UV、有限法线、局部变换、父级和声明为 closed 的网格。
4. `assemble_delivery.py` 只生成预览和 `SMK.G` Debug 图；预览不是 UE 视觉证据。

Iteration02 的记录另有 `iterate_reference02.py → refine_iteration02_cloth.py → export_delivery.py（输出目录切到 Iteration02）→ assemble_iteration02.py`。`Analysis.md` 明确说明四张修订 BC 由 built-in imagegen 生成后复制进当前 `Textures`，因此不能把 `generate_surface_textures.py` 或历史 `revise_texture_library.py` 的程序噪声描述成当前修订 BC 的生产来源。新生成器和 `ArtSource/LivingRoom/WatercolorSamples` 仍按各自落地时的 README 命令协同；本标准不绑定未完成接口，只接收满足本章契约且有 Manifest 的输出。

### 5.2 米到厘米的单位门禁

Blender 源文件和 Delivery FBX 使用米制；UE MCP 当前不会按原 FBX 的米制单位元数据得到期望比例。下图是当前工具已验证的桥接路径；未来若有已经验证的厘米值导出器，也可以直接交给 UE。这里的“唯一”只指每条资产交付链只执行一次 m→cm 换算，不能在 DCC、导出器、UE Import 和 Actor 多处重复换算。

```text
Blender 源场景（Metric，scale_length = 1，坐标单位 m）
  ↓ export_delivery.py：global_scale=1，apply_unit_scale=True，FBX_SCALE_UNITS
Delivery FBX（带米制单位元数据）
  ↓ 在 Blender 重新导入，并在 DCC 中应用单位换算
UEImport/FBX（顶点数值已为 cm）
  ↓ Blender 导出：global_scale=1，apply_unit_scale=False，FBX_SCALE_NONE
UE Static Mesh / Actor（导入与放置 Scale 均为 1,1,1）
```

轴向也只有一套：FBX 使用 `-Y Forward / Z Up`；Blender→UE 采用 `(X, -Y, Z)`，旋转由反射后的矩阵分解，布局数值以 `Assembly.json` 为准。一个 1m 的 DCC 尺寸必须在 UE 读回为约 100cm；不要在 UE Import、Static Mesh、Actor、父级或蓝图里再乘 100，也不要用非均匀缩放修复尺寸、透明度、UV 或描边。

## 6. UE 导入后回读验收

回读必须以目标 Static Mesh 和实际 Actor 为对象，逐项把结果写入 `UEValidation.json` 或同等可追溯记录。没有记录就保持 Candidate。

### 6.1 结构和变换

1. 在 `/Game/LostRunic/Scene/Meshes` 导入目标 FBX；确认 Mesh 数量、Bounds、三角面数与该批次 `Manifest.json` 对应。允许的差异必须能解释为导出三角化或源文件零面积面，不能用 Actor 缩放掩盖。
2. 在 Static Mesh Editor 的 Details/Build Settings 回读本标准第 2.2 节的候选法线配置；有 authored normals 的资产在中性光下检查没有翻面、黑面和突变高光。该回读是候选配置核对，不是对现有六类资产的预先 PASS 声明。
3. 在 Static Mesh Editor 的 Material Slots 回读槽位数量、现有语义名/导入后缀、Material Interface 和顺序。新资产槽位职责必须与第 3 节一致，已有 R02 槽位不得为此重命名。
4. 在 World Outliner/Details 回读 `Location`、`Rotation` 和 `Scale`。位置/旋转与 `Assembly.json` 相符，Actor 和父级 Scale 为 `1,1,1`；尺寸错误应回到 DCC 单位桥，不允许 100x 补偿。
5. 对门/抽屉等未来可能拆件的资产，在 DCC 源文件和 FBX Manifest 中回读 `PIVOT_*` 的局部轴向与父子关系。当前 UE 导入会把家具内部零件合并为 Static Mesh，合并后不要求也不应假定 UE 仍暴露运行时 Pivot；若将来需要动画，再以拆分交付和独立动画验收为准。

### 6.2 材质、UV 和视觉诊断

1. 用 Buffer Visualization 的 Base Color 在关闭 Outline、Bloom 和额外收束时查看：没有全表面噪声、烘焙灯影、AO 黑边或默认灰尘旧化；允许的低频方向性笔触应与表面方向一致。
2. 独立查看 `SMK_R_DetailAmount`、`SMK_G_StructureCavity`、`SMK_B_MacroVariation` 和需要时的 `SMK_A_SecondaryMaterial`。G 只应出现门缝、板缝或明确装饰结构线，不得覆盖整片凹凸或受光区域。
3. 在 Static Mesh Editor 查看 UV Channel 0：墙/地/木的方向和尺度稳定；布料 Atlas 岛、Padding、边框未被重排；地毯顶面仍有完整边框映射；玻璃不因缺少贴图而创建假 UV/假纹理。
4. 使用 World Normal/Lit 视图在固定中性灯下检查大面连续性。没有 Normal 的资产确认未误接旧图；有 Normal 的资产确认法线只表达中尺度结构。
5. 玻璃单独回读 `M_LR_StylizedGlass` 的 Thin Translucent/Surface ForwardShading、Two Sided Off、无 Refraction/Scene Color/WPO；确认没有把 Opaque 的 SMK/Normal 采样强行叠加。

### 6.3 相机和性能记录

P2.5 authoring review 的相机候选起点固定为 **SpringArm Target Arm Length = 500cm**、向下俯角 **52°**（UE Pitch 记录为 `-52°`）、**FOV 68°**。500cm 是 SpringArm 目标长度，不是相机高度，也不是到任意兴趣点的临时距离。同一批次比较六类表面时不更换这三个值。`/Game/LostRunic/Levels/PIE_Test/L_Art_Demo` 是本轮 LivingRoom 美术验收场景；其中已记录的 `LRScene_ConceptCamera` 仍为正交展示相机（Ortho Width 1324cm、4:3），与本候选 Gameplay Camera 不等价。

开发机趋势记录使用 1920×1080、Screen Percentage 100%、目标 60fps；至少记录材质、Base Pass、Translucency 和 Post Processing 的趋势。最低配置机型和硬预算尚未确定，只有完成目标 GPU Profile 后才能把 Candidate 升级为 Production Baseline。当前文档不声称相机或性能门禁已通过。

通用功能 PIE 冒烟使用 `/Game/LostRunic/Levels/PIE_Test/L_PIE_Test`，检查 Output Log 中项目级 Warning/Error；本轮美术验收使用 `L_Art_Demo`。未执行的 UE/PIE/Profile 项目必须保持“待验证”。

## 7. 目前证据边界与签字顺序

现有文件已经记录的事实（本轮未重跑）包括：Delivery 的 81 件独立二进制 FBX Blender 回读 PASS；`Iteration02/UEImport/README.md` 的 81 个 Static Mesh、Assembly 最大误差 0cm、180 个材质槽和 UE 保存记录；米制 FBX 经 Blender 应用单位换算后输出厘米值、Actor 不使用 100 倍缩放；七组 2048 布料 Atlas、R02 布料粗糙度约 0.91、无附加 Normal，以及四张修订 BC 的 imagegen 来源。

仍为 Candidate 的是：六类新资产在 `L_Art_Demo` 的固定 Gameplay Camera 视觉、BaseColor/SMK Debug、法线配置实际回读、UV/Atlas Mip 稳定性、开发机 1080p/SP100/60fps 趋势、最低配置 GPU 硬预算，以及新生成器输出的 Manifest/UE 回读。通用功能 PIE 仍按 `L_PIE_Test` 执行；未执行项目不得写成 PASS。

签字顺序固定为：**DCC 结构/单位/Pivot → UV/Atlas → BC/SMK/Normal → FBX Blender 回读 → UE 尺寸/Slot/Transform/法线回读 → 固定相机视觉 → 性能与 PIE**。任一环节失败时回到对应来源修复，不在下游用 Actor 缩放、假灯影贴图或额外后处理补偿。
