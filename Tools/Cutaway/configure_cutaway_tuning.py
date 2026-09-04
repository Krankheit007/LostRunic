import unreal


tuning = unreal.EditorAssetLibrary.load_asset("/Game/LostRunic/Data/Tuning/DA_LRPresentationTuning")
silhouette = unreal.EditorAssetLibrary.load_asset("/Game/LostRunic/Materials/PostProcess/M_PP_LR_PlayerOcclusion")
opaque = unreal.EditorAssetLibrary.load_asset("/Game/LostRunic/Materials/Master/M_LR_StylizedOpaque")
foliage = unreal.EditorAssetLibrary.load_asset("/Game/LostRunic/Materials/Master/M_LR_StylizedFoliageMasked")
cutaway_view = unreal.EditorAssetLibrary.load_asset(
    "/Game/LostRunic/Materials/Parameters/MPC_LR_CutawayView"
)
if not tuning or not silhouette or not opaque or not foliage or not cutaway_view:
    raise RuntimeError("Missing presentation tuning or player occlusion material")
tuning.set_editor_property("player_occlusion_post_process_material", silhouette)
tuning.set_editor_property("cutaway_view_parameter_collection", cutaway_view)
tuning.set_editor_property("approved_cutaway_master_materials", [opaque, foliage])
unreal.EditorAssetLibrary.save_loaded_asset(tuning, only_if_is_dirty=False)
unreal.log("CUTAWAY_TUNING_SUCCESS PlayerOcclusionPostProcessMaterial and MPC_LR_CutawayView configured")
