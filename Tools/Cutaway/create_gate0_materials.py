import unreal


ROOT = "/Game/LostRunic/Materials/Benchmark/CutawayGate"
PARENT_PATH = f"{ROOT}/M_Test"
OPAQUE_INSTANCE_PATH = f"{ROOT}/MI_Test_Opaque"
MASKED_INSTANCE_PATH = f"{ROOT}/MI_Test_Masked"


def load_or_create(asset_name, asset_class, factory):
    asset_path = f"{ROOT}/{asset_name}"
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing:
        return existing
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    return asset_tools.create_asset(asset_name, ROOT, asset_class, factory)


def clear_material(material):
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)


def build_parent():
    material = load_or_create("M_Test", unreal.Material, unreal.MaterialFactoryNew())
    clear_material(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -500, -120
    )
    color.set_editor_property("constant", unreal.LinearColor(0.18, 0.18, 0.18, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -500, 20
    )
    roughness.set_editor_property("r", 0.65)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    gate_mask = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -500, 180
    )
    gate_mask.set_editor_property("parameter_name", "GateMask")
    gate_mask.set_editor_property("default_value", 0.0)
    unreal.MaterialEditingLibrary.connect_material_property(
        gate_mask, "", unreal.MaterialProperty.MP_OPACITY_MASK
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def build_instance(asset_name, parent, masked):
    instance = load_or_create(
        asset_name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew()
    )
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, "GateMask", 0.0
    )

    overrides = instance.get_editor_property("base_property_overrides")
    overrides.set_editor_property("override_blend_mode", masked)
    overrides.set_editor_property(
        "blend_mode", unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_OPAQUE
    )
    overrides.set_editor_property("override_opacity_mask_clip_value", masked)
    overrides.set_editor_property("opacity_mask_clip_value", 0.3333)
    instance.set_editor_property("base_property_overrides", overrides)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    return instance


def main():
    unreal.EditorAssetLibrary.make_directory(ROOT)
    parent = build_parent()
    opaque = build_instance("MI_Test_Opaque", parent, False)
    masked = build_instance("MI_Test_Masked", parent, True)

    opaque_overrides = opaque.get_editor_property("base_property_overrides")
    masked_overrides = masked.get_editor_property("base_property_overrides")
    unreal.log(
        "CUTAWAY_GATE0 parent={} opaque_override={} masked_override={} masked_mode={}".format(
            parent.get_editor_property("blend_mode"),
            opaque_overrides.get_editor_property("override_blend_mode"),
            masked_overrides.get_editor_property("override_blend_mode"),
            masked_overrides.get_editor_property("blend_mode"),
        )
    )


if __name__ == "__main__":
    main()
