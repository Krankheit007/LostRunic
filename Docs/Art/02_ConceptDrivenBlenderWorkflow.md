# 场景概念图驱动的 Blender 资产工作流

版本：2；更新：2026-09-06。用户本轮明确修订的流程替代先前客厅任务中的流程描述。

## 来源与职责

- 场景概念图是造型依据，不等同于多视角正交设计图。明确区分可见证据与不可见面的推断。
- 智能体通过 Blender MCP 创建几何、材质、层级与轴心，维护构建脚本，完成自动验证和交付。用户不需要提供零件名称、层级或操作步骤。
- 用户确认范围、尺度、复杂度、静态/动画方向，并负责各级视觉与最终动画验收。
- 本文定义生产流程；模型和贴图技术语义遵循 `01_LostRunicArtBibleAndTechnicalArtStandard.md`。Blender 预览不复制 UE Outline/NPR。
- Blender 内的概念构图装配用于验收；未经明确要求不在 Unreal 正式关卡中摆放资产。

## 范围确认

场景概念图 → 识别独立家具 → 标记 Hero / Reusable / Decoration → 建立 Proposed Asset List → 分析可见结构与不可见推断 → 提出尺寸、资产复杂度、静态/动画分类 → 用户确认范围与尺度。

Hero / Reusable / Decoration 是制作角色标签，不等同于网格类型，必要时可兼有标签。清单须记录稳定资产 ID、预计独立导出单元、尺寸、角色标签、复杂度、静态或可动分类、证据和假设；尚未制作的项目不能伪报完成。

**Decoration 不等同于 LEVEL 3。** 独立花瓶、花束、烛台、杯碟、书籍等同样需要自己的一级轮廓和二级结构。明显影响构图的陈设主体必须进入一级占位，并在二级补齐基本结构；只有器物雕饰、花纹、细小花瓣、压纹等选择性细节留到三级或贴图阶段。二级 Gate 应按 Proposed Asset List 核查独立道具是否漏项，不能把“装饰物主体未建”记录为“三级待办”。

## 分级形体验收

| 阶段 | 允许制作与检查 | Gate | 不通过时 |
| --- | --- | --- | --- |
| LEVEL 1 — SILHOUETTE | 主体包络、比例、占地、负空间；概念构图的位置和遮挡 | Concept View + Multi-view + Black Silhouette Gate | 只修一级形体，保持本级 |
| LEVEL 2 — STRUCTURE | 框架、扶手、腿、抽屉、柜门、Cushion、大结构 | Multi-view Structure Gate | 只修二级结构；如诊断为一级错误，明确回退一级 |
| LEVEL 3 — SELECTIVE DETAIL | 时代特征、大线脚、把手、少量来源明确的装饰 | Gameplay Camera Detail Gate | 删除或修正三级细节；不能加细节掩盖一级或二级问题 |

Gate 必须保存可复查的视图和用户结论。未通过不得升级。缺少真实 Gameplay Camera 验证能力时明确写“待验证”，不得把 Blender 检查相机冒充 Gameplay Camera。

一级允许临时圆角来表达软体包络，但不代表正式 Hero Edge、法线或细节已验收。黑色剪影必须是黑色物体配纯色背景，禁用纹理、光照线索和描边；提供孤立资产视图以检查轮廓及负空间，房间整体剪影不能替代单件家具检查。

## 表面制作顺序

LEVEL 3 通过 → BEVEL + NORMAL（Hero Edge / major plane validation）→ UV / TRIM / ATLAS → TEXTURE（BaseColor、SMK、确有依据的 Weak Normal）→ Surface Validation → BLENDER PREVIEW。

Blender Preview 仅验证颜色、材质分区及贴图作用，不复制 UE Outline / NPR，也不把概念图光影烘入 BaseColor。正式表面验证遵循项目 BC / SMK / Normal 通道及分辨率要求。

## 导出与回读

EXPORT：每件家具独立 Binary FBX → FBX ROUND-TRIP IMPORT → 自动检查：Geometry、Normals、UV、Materials、Pivot、Scale、Hierarchy、Bounds、Triangle diagnostics。

回读须在独立验证场景中检查，不能只验证导出前的 Blender 源网格。网格数和三角面数是诊断，不通过盲目拆网格、堆零件或提高细分接近推荐数量。仅对 Manifest 中声明为封闭外壳的部分检查封闭性；透明薄片、叶片等必须明确声明例外。

## 最终视觉对照与回退

回读后将模型渲染和原概念图并列：

- FAIL：定位 Level 1 / Level 2 / Level 3 / Surface → 修改对应资产专属 Blender 构建脚本 → 重新生成受影响资产及依赖产物 → 重新验证相关 Gate、回读检查与视觉对照。
- PASS：进入 FINAL DELIVERY。

不得让 Blender 场景里的手工临时变换成为唯一事实来源。修正必须落入脚本或脚本使用的资产数据，旧截图不得混入当前验收集。

## FINAL DELIVERY

`.blend`、每件家具独立 FBX、Textures、Previews、Manifest、Validation Report、Assumptions。`.blend1` 可作为备份附带，不替代主工程。

Manifest 必须记录文件与资产的对应关系、单位/轴向、原点、包络、层级、材质槽、统计及静态/动画契约。Validation Report 区分自动检查通过、人工视觉通过、未验证和不适用。Assumptions 独立记录看不到的背面、连接和运动机制的推断。

如交付需要蓝图装配或配置，同步维护 `Docs/Technical/06_BlueprintConfigurationGuide.md` 的实际接口和配置步骤；不得声称未执行的 UE 导入、PIE、动画或 GPU 验收已通过。
