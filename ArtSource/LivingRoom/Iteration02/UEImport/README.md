# LivingRoom UE 导入与场景

目标关卡：`/Game/LostRunic/Levels/PIE_Test/L_Art_Demo`，用户明确指定用于本次美术场景布置。

- Mesh：`/Game/LostRunic/Scene/Meshes`，81 件独立 Static Mesh。
- Texture：`/Game/LostRunic/Scene/Textures`，31 张颜色图和 1 张共享 SMK。
- Material Instance：`/Game/LostRunic/Materials/Instances/Scenes`，32 个表面实例及独立艺术描边实例。
- 母材质沿用 M_LR_StylizedOpaque、M_LR_StylizedFoliageMasked、M_LR_StylizedGlass。没有修改共享母材质。
- 颜色图启用 sRGB / World / Mips；SMK 使用 Linear / Masks。共享 SMK 为 R DetailAmount、G=0 StructureCavity、B MacroVariation，未用 AO 替代结构遮罩。
- 当前 NormalStrength=0；不将旧 BC 派生的法线图误接到已修订纹理。布料采用家具组专属 Atlas UV0。Surface U/V=1，不叠加平铺在 Atlas 上。

## 导入尺度

当前 UE MCP import_file 不读取原 FBX 米制单位的预期比例。使用 Blender MCP 从交付 FBX 重新输出厘米数值版至本目录 FBX/，参数 global_scale=1、apply_unit_scale=False、apply_scale_options=FBX_SCALE_NONE（原 FBX 重新导入 Blender 后已应用单位换算）。首件门高由错误的 2.39cm 修正为 239cm。UE Actor 不使用 100 倍补偿缩放。

Assembly.json 保留布局契约。Blender → UE 使用 (X,-Y,Z) 坐标转换和米到厘米换算；旋转从反射转换后的矩阵分解。每件家具的内部零件合并为 Static Mesh，材质槽保留；本次是静态展示，未创建门/抽屉动画蓝图。

## 场景调参

在 World Outliner 的 LivingRoom/Geometry、LivingRoom/Lighting、LivingRoom/Camera 中查看本次资产。LRScene_ConceptCamera 是正交参考相机，Ortho Width=1324cm、Aspect Ratio=4:3。原有关卡 Benchmark 内容位于远处，保留。

LRScene_PostProcess 是此独立 Art Demo 的曝光权威：Manual、Physical Camera Exposure Off、Exposure Compensation=-4、Priority=20、Unbound。该展示校准值不写入正式章节 DataAsset，也不作为生产曝光基线。

DirectionalLight 为暖色窗光，Intensity=100、Temperature=4800K、Pitch=-34、Yaw=155、Source Angle=0.8。WindowFill Rect Light=1100lm，CoolFill=1000lm；装饰灯=220lm，FireGlow=350lm，所有新增局部灯关闭动态阴影。窗格投影来自 DirectionalLight。展示天空使用 Engine BP_Sky_Sphere，冷色 Zenith、暖灰 Horizon、关闭云和星点。

描边使用 MI_PP_LR_LivingRoomOutline，父材质为项目已有 M_PP_LR_StyleOutline；Width=1px，沿用项目已配置的深度/法线边缘参数。

## 验证边界

UEValidation.json 记录实际导入后的网格 Bounds、Triangle Count、材质引用与保存包清单。最终预览为 UE 实际视口输出。尚未完成目标 GPU 性能验收、碰撞/导航游戏验收和最终视觉签字。源模型仍缺少原图完整火焰、自然褶皱和更丰富植被，不能把资产导入成功等同于原图完全还原。

最终核对：81 个布置 Actor，位置相对 Assembly.json 的最大误差 0cm；180 个材质槽均引用指定 Scenes 目录。UE 总计 175600 triangles，10 个资产的源/UE 面数差额全部等于源文件零面积三角形数，见 TriangleDiagnostics.json。147 个包保存成功，目标关卡再次保存成功。原有 ArtBench_RoomShell 设置 Hidden In Game 以避免它出现在展示镜头右上角；其他原有 Benchmark 几何未移动。Preview.png 为 UE 视口原始截图，非 AI 修图。
