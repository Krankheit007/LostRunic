# 参考图材质修订

此版本替代用户否决的 Delivery 浅色表面。模型顶点、面索引与世界变换保持，地板仍为单一连续无倒角网格；仅修改材质、UV 和预览光照。

已调整深色旧木、灰蓝墙面、橄榄绿与赭黄/红褐软包、陶器和旧黄铜；生成 BC、SMK、独立 Roughness 和弱 DirectX Normal。Blender 内反转 Normal 绿色通道以匹配预览，UE 导入按 DirectX 使用。SMK 为线性 RGB（DetailAmount / 指定板缝 StructureCavity / MacroVariation），无 AO。BC 不含灯影。

地毯使用内置 imagegen 依照参考图生成的旧织锦纹样；可见画作通过原图内部区域 UV 取样复用，因此仅有源图有限像素，部分遮挡及源画面明暗仍保留，不宣称获得完整高分辨率画作。窗侧日光与暖灯仅用于 Blender 预览，不进入家具 FBX，不复制 UE NPR。

产物：LivingRoom_MaterialRevision.blend、FBX/ 每件资产独立文件、Textures/、TextureManifest.json、Manifest.json、Validation.json、Preservation.json、Previews/。原 Delivery 作为被否决旧版本保留，不应继续使用其纹理。

自动回读通过不代表原图视觉完全还原。该版本仍待用户材质视觉验收；几何是先前获准形体，不能仅以材质消除全部轮廓差异。无真实 UE Gameplay Camera 或运行时材质验证。没有烘焙动画，保留源静态组件层级。

重建顺序：从旧 Delivery 工程打开 → revise_texture_library.py（需保留生成的 T_LR_Rug_BC.png）→ apply_surface_revision.py → export_delivery.py（输出路径替换为 SurfaceRevision）→ 预览。脚本在 Tools/Blender/LivingRoom。

地毯生成使用 built-in image_gen，不使用 CLI；参考图为 D:/GameDesign/DontForgetAdele/livingroom.png。生成提示词见 ImageGenerationPrompt.txt。
