import unreal


def log(message):
    unreal.log(f"CUTAWAY_INSPECT {message}")


def inspect_blueprint(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        log(f"missing_blueprint={path}")
        return
    generated_class = asset.generated_class()
    cdo = unreal.get_default_object(generated_class)
    log(f"blueprint={path} class={generated_class.get_name()}")
    for component in cdo.get_components_by_class(unreal.ActorComponent):
        tags = list(component.get_editor_property("component_tags"))
        log(f"component={component.get_name()} type={component.get_class().get_name()} tags={tags}")


def inspect_material(path):
    material = unreal.EditorAssetLibrary.load_asset(path)
    if not material:
        log(f"missing_material={path}")
        return
    if isinstance(material, unreal.MaterialInstanceConstant):
        overrides = material.get_editor_property("base_property_overrides")
        log(f"material={path} class=MaterialInstanceConstant parent={material.get_editor_property('parent').get_name()} overrides={overrides}")
    else:
        mask_input = unreal.MaterialEditingLibrary.get_material_property_input_node(
            material, unreal.MaterialProperty.MP_OPACITY_MASK
        )
        log(
            f"material={path} class={material.get_class().get_name()} "
            f"blend={material.get_editor_property('blend_mode')} "
            f"domain={material.get_editor_property('material_domain')} "
            f"shading={material.get_editor_property('shading_model')} "
            f"opacity_mask_input={mask_input.get_name() if mask_input else 'None'}"
        )


inspect_blueprint("/Game/LostRunic/Blueprints/Character/BP_Ruth")
for material_path in (
    "/Game/LostRunic/Materials/Master/M_LR_StylizedOpaque",
    "/Game/LostRunic/Materials/Master/M_LR_StylizedFoliageMasked",
    "/Game/LostRunic/Materials/Master/M_LR_StylizedCharacter",
    "/Game/LostRunic/Materials/Benchmark/CutawayGate/M_Test",
    "/Game/LostRunic/Materials/Benchmark/CutawayGate/MI_Test_Opaque",
    "/Game/LostRunic/Materials/Benchmark/CutawayGate/MI_Test_Masked",
):
    inspect_material(material_path)
