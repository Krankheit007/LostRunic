# 客厅资产候选交付

本目录包含 LivingRoom.blend / .blend1、81 个独立二进制 FBX、32 组 BaseColor/SMK 贴图、预览、Manifest.json、TextureManifest.json、Validation.json 和 Assumptions.md。

三级形体已获用户确认。三块地板已合为一块连续封闭网格，不含倒角，UV0 按米连续映射。家具和独立陈设分文件，墙体也分别输出；FBX 中不包含灯光或相机。

81 个文件全部回读检查：网格数、三角面数、材质槽、UV 存在、法线有限性、尺寸包络、局部变换、父子层级、明确声明闭合的网格非流形边检查通过。不能将“自动检查通过”理解为“UE 实际导入和视觉验收通过”。所有 mesh/triangle 统计在 Manifest 中，网格数只作诊断。

每个文件采用原资产局部原点；场景装配矩阵保存在 Manifest。FBX 使用米及单位元数据，-Y Forward / Z Up；UE 导入需转换场景单位为厘米。静态组合家具可按整个文件合并导入；如需利用门轴/抽屉层级，则保持组件分离并按轴心契约装配。未制作骨骼或烘焙动画；没有碰撞代理或 Lightmap UV。当前不修改 Unreal 关卡。

纹理绑定以 TextureManifest 为权威：BC 为 sRGB；SMK 为线性 Masks，RGB 分别为 DetailAmount、StructureCavity、MacroVariation；无 Alpha。SMK.G 仅地板板缝非零，其余为零，不烘焙 AO。未添加无明确依据的 Normal Map。Blender 内 SMK 作为命名贴图节点保留，实际通道接入由 UE 既有材质完成，不模拟描边/NPR。普通家具使用 UV0，地板按米平铺，地毯顶面为完整边框映射。

预览为简化表面候选，待用户颜色/表面验收；UE 实际镜头、贴图导入设置和动画运行未执行。原图画作内容仍为空白色块，壁炉火焰未作为实体或 VFX 实现；这些不属于已完成的贴图还原。地毯纹样为原图边带节奏的低频简化，不是精确花纹复刻。

生产脚本在 Tools/Blender/LivingRoom：generate_surface_textures.py → apply_surfaces.py → export_delivery.py → assemble_delivery.py。工程所需文件图像均已内嵌，无外部链接库。Blender 原先复制粘贴留下的 weak library provenance 不构成外部依赖。
