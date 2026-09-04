import unreal


ASSET_PATH = "/Game/LostRunic/Materials/PostProcess/M_PP_LR_PlayerOcclusion"


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)


def connect(source, source_output, target, target_input):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source, source_output, target, target_input):
        raise RuntimeError(f"Failed connection {source.get_name()}:{source_output} -> {target.get_name()}:{target_input}")


material = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not material:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_PP_LR_PlayerOcclusion", "/Game/LostRunic/Materials/PostProcess", unreal.Material,
        unreal.MaterialFactoryNew()
    )
if not material:
    raise RuntimeError("Could not create player occlusion post process material")

unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
material.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_DOF)
material.set_editor_property("blendable_priority", 20)

screen = expression(material, unreal.MaterialExpressionScreenPosition, -900, 0)
view_size = expression(material, unreal.MaterialExpressionViewSize, -900, 100)

scene = expression(material, unreal.MaterialExpressionSceneTexture, -900, 220)
scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
scene_depth = expression(material, unreal.MaterialExpressionSceneTexture, -900, 320)
scene_depth.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_SCENE_DEPTH)
custom_depth = expression(material, unreal.MaterialExpressionSceneTexture, -900, 420)
custom_depth.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_DEPTH)
stencil = expression(material, unreal.MaterialExpressionSceneTexture, -900, 520)
stencil.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_STENCIL)

width = expression(material, unreal.MaterialExpressionScalarParameter, -900, 650)
width.set_editor_property("parameter_name", "PlayerOutlineWidthRefPx")
width.set_editor_property("default_value", 1.0)
width.set_editor_property("group", "Player Occlusion")
bias = expression(material, unreal.MaterialExpressionScalarParameter, -900, 730)
bias.set_editor_property("parameter_name", "OcclusionDepthBiasCm")
bias.set_editor_property("default_value", 1.0)
bias.set_editor_property("group", "Player Occlusion")
tint = expression(material, unreal.MaterialExpressionVectorParameter, -900, 810)
tint.set_editor_property("parameter_name", "PlayerOcclusionTint")
tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
tint.set_editor_property("group", "Player Occlusion")

custom = expression(material, unreal.MaterialExpressionCustom, -350, 250)
names = ["SceneColor", "SceneDepthCenter", "CustomDepthCenter", "StencilCenter", "ScreenUV", "ViewSize", "WidthRefPx", "DepthBiasCm", "Tint"]
inputs = []
for name in names:
    custom_input = unreal.CustomInput()
    custom_input.set_editor_property("input_name", name)
    inputs.append(custom_input)
custom.set_editor_property("inputs", inputs)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "Stencil 2 occluded player fill + 1px@1080p outline")
custom.set_editor_property("code", r"""
float2 uv = ScreenUV.xy;
float referenceScale = ViewSize.y / 1080.0;
float2 texel = (max(WidthRefPx * referenceScale, 0.5) / ViewSize.xy);

#define LR_SCENE_UV(U, ID) ViewportUVToSceneTextureUV((U), (ID))
#define LR_OCC(U) ((abs(SceneTextureLookup(LR_SCENE_UV((U), 25), 25, false).r - 2.0) < 0.5 && SceneTextureLookup(LR_SCENE_UV((U), 13), 13, false).r > SceneTextureLookup(LR_SCENE_UV((U), 1), 1, false).r + DepthBiasCm) ? 1.0 : 0.0)
float mask = LR_OCC(uv);
mask = max(mask, LR_OCC(uv + float2(texel.x, 0.0)));
mask = max(mask, LR_OCC(uv - float2(texel.x, 0.0)));
mask = max(mask, LR_OCC(uv + float2(0.0, texel.y)));
mask = max(mask, LR_OCC(uv - float2(0.0, texel.y)));
mask = max(mask, LR_OCC(uv + texel));
mask = max(mask, LR_OCC(uv - texel));
mask = max(mask, LR_OCC(uv + float2(texel.x, -texel.y)));
mask = max(mask, LR_OCC(uv + float2(-texel.x, texel.y)));
#undef LR_OCC
#undef LR_SCENE_UV
return lerp(SceneColor.rgb, Tint.rgb, saturate(mask));
""")

sources = [scene, scene_depth, custom_depth, stencil, screen, view_size, width, bias, tint]
outputs = ["Color", "", "", "", "ViewportUV", "", "", "", ""]
for source, output, name in zip(sources, outputs, names):
    connect(source, output, custom, name)
unreal.MaterialEditingLibrary.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
unreal.log(f"CUTAWAY_POSTPROCESS_SUCCESS asset={ASSET_PATH} priority=20 location=AfterDOF")
