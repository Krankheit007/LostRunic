import unreal


SOURCE = "/Game/LostRunic/Materials/Master/M_LR_StylizedOpaque"
TARGET = "/Game/LostRunic/Materials/Benchmark/CutawayGate/M_Probe_StylizedCutaway"
SHARED_FUNCTION = "/Game/LostRunic/Materials/Functions/MF_LR_CutawayRadialShape"
CUTAWAY_VIEW_MPC = "/Game/LostRunic/Materials/Parameters/MPC_LR_CutawayView"


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)


def scalar(material, name, default, x, y, cpd_index=None):
    for candidate in unreal.MaterialEditingLibrary.get_material_expressions(material):
        if isinstance(candidate, unreal.MaterialExpressionScalarParameter) and str(
                candidate.get_editor_property("parameter_name")) == name:
            return candidate
    node = expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    node.set_editor_property("group", "Cutaway")
    if cpd_index is not None:
        node.set_editor_property("use_custom_primitive_data", True)
        node.set_editor_property("primitive_data_index", cpd_index)
    return node


def collection_scalar(material, collection, name, x, y):
    for candidate in unreal.MaterialEditingLibrary.get_material_expressions(material):
        if not isinstance(candidate, unreal.MaterialExpressionCollectionParameter):
            continue
        if (candidate.get_editor_property("collection") == collection
                and str(candidate.get_editor_property("parameter_name")) == name):
            return candidate
    node = expression(material, unreal.MaterialExpressionCollectionParameter, x, y)
    node.set_editor_property("collection", collection)
    node.set_editor_property("parameter_name", name)
    return node


def remove_local_transition_scalars(material):
    for candidate in unreal.MaterialEditingLibrary.get_material_expressions(material):
        if (isinstance(candidate, unreal.MaterialExpressionScalarParameter)
                and str(candidate.get_editor_property("parameter_name")) == "CutawayTransitionRefPx"):
            unreal.MaterialEditingLibrary.delete_material_expression(material, candidate)


def connect(source, source_output, target, target_input):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source, source_output, target, target_input):
        raise RuntimeError(f"Failed connection {source.get_name()}:{source_output} -> {target.get_name()}:{target_input}")


def reconnect(source, source_output, target, target_input):
    # MaterialEditingLibrary.connect_material_expressions does not replace an existing
    # link on this repeated Custom input, so clear only the requested target pin first.
    unreal.MaterialEditingLibrary.disconnect_material_expressions(target, target_input)
    connect(source, source_output, target, target_input)


def find_or_create_custom(material, x, y):
    for candidate in unreal.MaterialEditingLibrary.get_material_expressions(material):
        if not isinstance(candidate, unreal.MaterialExpressionCustom):
            continue
        if str(candidate.get_editor_property("description")) == "LR Cutaway Visibility (shared radial coverage + CPD 0-8)":
            return candidate
    return expression(material, unreal.MaterialExpressionCustom, x, y)


def install_cutaway(material, base_mask=None):
    if base_mask is None:
        base_mask = expression(material, unreal.MaterialExpressionConstant, -1400, 900)
        base_mask.set_editor_property("r", 1.0)

    cpd = [
        scalar(material, "CutawayLocalAmount", 0.0, -1400, 1000, 0),
        scalar(material, "CutawayGroupAmount", 0.0, -1400, 1080, 1),
        scalar(material, "CutawayForegroundAmount", 0.0, -1400, 1160, 2),
        scalar(material, "CutawayCenterUVX", 0.0, -1400, 1240, 3),
        scalar(material, "CutawayCenterUVY", 0.0, -1400, 1320, 4),
        scalar(material, "CutawayRadiusRefPx", 0.0, -1400, 1400, 5),
        scalar(material, "CutawayRootOverrideAmount", 0.0, -1400, 1480, 6),
        scalar(material, "CutawayRootHeight01", 0.0, -1400, 1560, 7),
        scalar(material, "CutawayRootFeather01", 0.0, -1400, 1640, 8),
    ]
    root_enabled = scalar(material, "RootPreserveEnabled", 0.0, -1050, 1480)
    root_height = scalar(material, "DefaultRootHeightCm", 0.0, -1050, 1560)
    root_feather = scalar(material, "DefaultRootFeatherCm", 0.0, -1050, 1640)
    cutaway_view = unreal.EditorAssetLibrary.load_asset(CUTAWAY_VIEW_MPC)
    if not cutaway_view:
        raise RuntimeError(f"Missing Cutaway view MPC: {CUTAWAY_VIEW_MPC}")
    remove_local_transition_scalars(material)
    edge = collection_scalar(material, cutaway_view, "CutawayTransitionRefPx", -1050, 1160)
    foreground_near = scalar(material, "ForegroundFullyCutDistanceCm", 100.0, -1050, 1240)
    foreground_far = scalar(material, "ForegroundVisibleDistanceCm", 300.0, -1050, 1320)

    center = expression(material, unreal.MaterialExpressionAppendVector, -800, 1270)
    connect(cpd[3], "", center, "A")
    connect(cpd[4], "", center, "B")
    screen = expression(material, unreal.MaterialExpressionScreenPosition, -800, 900)
    view_size = expression(material, unreal.MaterialExpressionViewSize, -800, 980)
    pixel_depth = expression(material, unreal.MaterialExpressionPixelDepth, -800, 1060)
    bounds = expression(material, unreal.MaterialExpressionObjectLocalBounds, -800, 1480)
    local_position = expression(material, unreal.MaterialExpressionLocalPosition, -800, 1640)

    custom = find_or_create_custom(material, -300, 1100)
    input_names = [
        "BaseOpacityMask", "LocalAmount", "GroupAmount", "ForegroundAmount", "CenterUV", "RadiusRefPx",
        "RootOverrideAmount", "RootHeight01", "RootFeather01", "RootPreserveEnabled", "DefaultRootHeightCm",
        "DefaultRootFeatherCm", "CutawayTransitionRefPx", "ForegroundNearCm", "ForegroundFarCm", "ScreenUV",
        "ViewSize", "PixelDepth", "BoundsMin", "BoundsMax", "LocalPosition"
    ]
    custom_inputs = []
    for name in input_names:
        custom_input = unreal.CustomInput()
        custom_input.set_editor_property("input_name", name)
        custom_inputs.append(custom_input)
    custom.set_editor_property("inputs", custom_inputs)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    custom.set_editor_property("description", "LR Cutaway Visibility (shared radial coverage + CPD 0-8)")
    custom.set_editor_property("code", r"""
float localCut = saturate(LocalAmount);
float foregroundRange = max(ForegroundFarCm - ForegroundNearCm, 0.01);
float foregroundShape = saturate((ForegroundFarCm - PixelDepth) / foregroundRange);
float foregroundCut = saturate(ForegroundAmount) * foregroundShape;
float combinedCut = max(max(localCut, saturate(GroupAmount)), foregroundCut);
float visibility = 1.0 - combinedCut;

float boundsHeight = max(BoundsMax.z - BoundsMin.z, 0.01);
float normalizedHeight = saturate((LocalPosition.z - BoundsMin.z) / boundsHeight);
float defaultRootHeight01 = saturate(DefaultRootHeightCm / boundsHeight);
float defaultRootFeather01 = saturate(DefaultRootFeatherCm / boundsHeight);
float rootHeight = lerp(defaultRootHeight01, saturate(RootHeight01), saturate(RootOverrideAmount));
float rootFeather = max(lerp(defaultRootFeather01, saturate(RootFeather01), saturate(RootOverrideAmount)), 0.0001);
float rootPreserve = saturate(RootPreserveEnabled) * (1.0 - smoothstep(rootHeight, rootHeight + rootFeather, normalizedHeight));
float finalVisibility = max(visibility, rootPreserve);

float2 pixelPosition = ScreenUV.xy * ViewSize.xy;
float dither = frac(52.9829189 * frac(dot(floor(pixelPosition), float2(0.06711056, 0.00583715))));
return (BaseOpacityMask.r > 0.3333 && finalVisibility >= dither) ? 1.0 : 0.0;
""")

    sources = [base_mask] + cpd[:3] + [center, cpd[5], cpd[6], cpd[7], cpd[8], root_enabled, root_height,
        root_feather, edge, foreground_near, foreground_far, screen, view_size, pixel_depth, bounds, bounds, local_position]
    outputs = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "ViewportUV", "", "", "Min", "Max", ""]
    for source, output, name in zip(sources, outputs, input_names):
        if name == "BoundsMax":
            reconnect(source, output, custom, name)
        else:
            connect(source, output, custom, name)

    function = unreal.EditorAssetLibrary.load_asset(SHARED_FUNCTION)
    if not function:
        raise RuntimeError(f"Missing shared cutaway function: {SHARED_FUNCTION}")
    call = None
    for candidate in unreal.MaterialEditingLibrary.get_material_expressions(material):
        if not isinstance(candidate, unreal.MaterialExpressionMaterialFunctionCall):
            continue
        if candidate.get_editor_property("material_function") == function:
            call = candidate
            break
    if call is None:
        call = expression(material, unreal.MaterialExpressionMaterialFunctionCall, -300, 850)
        call.set_editor_property("material_function", function)
    connect(screen, "ViewportUV", call, "ViewportUV")
    connect(center, "", call, "CenterUV")
    connect(cpd[5], "", call, "RadiusRefPx")
    connect(cpd[0], "", call, "Amount")
    connect(view_size, "", call, "ViewSize")
    connect(edge, "", call, "CutawayTransitionRefPx")
    connect(call, "CutawayCoverage", custom, "LocalAmount")
    unreal.MaterialEditingLibrary.connect_material_property(custom, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    unreal.MaterialEditingLibrary.recompile_material(material)


def create_template(name, parent, root_enabled, root_height, root_feather):
    path = f"/Game/LostRunic/Materials/Instances/Cutaway/{name}"
    instance = unreal.EditorAssetLibrary.load_asset(path)
    if not instance:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, "/Game/LostRunic/Materials/Instances/Cutaway", unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew()
        )
    instance.set_editor_property("parent", parent)
    overrides = instance.get_editor_property("base_property_overrides")
    overrides.set_editor_property("override_blend_mode", True)
    overrides.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    overrides.set_editor_property("override_opacity_mask_clip_value", True)
    overrides.set_editor_property("opacity_mask_clip_value", 0.3333)
    instance.set_editor_property("base_property_overrides", overrides)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "RootPreserveEnabled", root_enabled)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "DefaultRootHeightCm", root_height)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "DefaultRootFeatherCm", root_feather)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)


opaque = unreal.EditorAssetLibrary.load_asset("/Game/LostRunic/Materials/Master/M_LR_StylizedOpaque")
foliage = unreal.EditorAssetLibrary.load_asset("/Game/LostRunic/Materials/Master/M_LR_StylizedFoliageMasked")
if not opaque or not foliage:
    raise RuntimeError("Missing approved production masters")
foliage_alpha = unreal.MaterialEditingLibrary.get_material_property_input_node(foliage, unreal.MaterialProperty.MP_OPACITY_MASK)
if not foliage_alpha:
    raise RuntimeError("Foliage master must already have an Opacity Mask input")
install_cutaway(opaque)
install_cutaway(foliage, foliage_alpha)
unreal.EditorAssetLibrary.save_loaded_asset(opaque, only_if_is_dirty=False)
unreal.EditorAssetLibrary.save_loaded_asset(foliage, only_if_is_dirty=False)
create_template("MI_LR_CutawayWall_Template", opaque, 1.0, 10.0, 5.0)
create_template("MI_LR_CutawayRoof_Template", opaque, 0.0, 0.0, 0.0)
create_template("MI_LR_CutawayTrunk_Template", opaque, 1.0, 10.0, 5.0)
unreal.log("CUTAWAY_PRODUCTION_SUCCESS masters=Opaque,Foliage templates=Wall,Roof,Trunk transition=8 shared=MF_LR_CutawayRadialShape")
