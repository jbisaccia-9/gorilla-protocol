import os
import unreal


ROOT = os.path.abspath(unreal.Paths.project_dir())
RAW = os.path.join(ROOT, "RawContent")
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EDITOR = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)


def log(message):
    unreal.log(f"[Gorilla Protocol] {message}")


def import_task(filename, destination, destination_name="", options=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    ASSET_TOOLS.import_asset_tasks([task])
    objects = list(task.get_objects())
    if not objects:
        raise RuntimeError(f"Import produced no assets: {filename}")
    return objects


def require_imported_type(objects, asset_type, target):
    existing = unreal.load_asset(target)
    if isinstance(existing, asset_type):
        return existing

    candidates = [asset for asset in objects if isinstance(asset, asset_type)]
    if not candidates:
        raise RuntimeError(f"Import did not produce {asset_type.__name__}")

    for asset in candidates:
        if asset.get_path_name().split(".")[0] == target:
            return asset

    # Interchange can replace the requested FBX name with its internal take name.
    # Prefer the actual clip over a one-frame targeting pose, then normalize its path.
    preferred = [
        asset for asset in candidates if "targeting_pose" not in asset.get_name().lower()
    ]
    if preferred:
        candidates = preferred

    def animation_length(asset):
        try:
            return float(asset.get_editor_property("play_length"))
        except Exception:
            return 0.0

    candidate = max(candidates, key=animation_length)
    imported_path = candidate.get_path_name().split(".")[0]
    if not unreal.EditorAssetLibrary.rename_loaded_asset(candidate, target):
        raise RuntimeError(f"Could not rename imported asset {imported_path} to {target}")

    normalized = unreal.load_asset(target)
    if not isinstance(normalized, asset_type):
        raise RuntimeError(f"Renamed asset is not available at {target}")
    return normalized


def import_guard():
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = True
    options.import_animations = False
    options.import_materials = True
    options.import_textures = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    paths = import_task(
        os.path.join(RAW, "Kenney", "Characters", "characterMedium.fbx"),
        "/Game/GorillaProtocol/Characters/Guards",
        "SK_Guard",
        options,
    )
    guard = require_imported_type(
        paths,
        unreal.SkeletalMesh,
        "/Game/GorillaProtocol/Characters/Guards/SK_Guard",
    )
    skeleton = guard.get_editor_property("skeleton")

    animation_names = {
        "idle.fbx": "AN_GuardIdle",
        "run.fbx": "AN_GuardRun",
        "jump.fbx": "AN_GuardJump",
    }
    for filename, target_name in animation_names.items():
        anim_options = unreal.FbxImportUI()
        anim_options.import_mesh = False
        anim_options.import_animations = True
        anim_options.import_materials = False
        anim_options.import_textures = False
        anim_options.skeleton = skeleton
        anim_options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        imported = import_task(
            os.path.join(RAW, "Kenney", "Characters", filename),
            "/Game/GorillaProtocol/Characters/Guards",
            target_name,
            anim_options,
        )
        require_imported_type(
            imported,
            unreal.AnimSequence,
            f"/Game/GorillaProtocol/Characters/Guards/{target_name}",
        )


def import_weapon():
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_animations = False
    options.import_materials = True
    options.import_textures = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    paths = import_task(
        os.path.join(RAW, "Kenney", "Weapons", "blaster-n.fbx"),
        "/Game/GorillaProtocol/Weapons/P9",
        "SM_P9",
        options,
    )
    require_imported_type(paths, unreal.StaticMesh, "/Game/GorillaProtocol/Weapons/P9/SM_P9")

    suppressor_paths = import_task(
        os.path.join(RAW, "Kenney", "Weapons", "silencer-small.fbx"),
        "/Game/GorillaProtocol/Weapons/P9",
        "SM_P9_Suppressor",
        options,
    )
    require_imported_type(
        suppressor_paths,
        unreal.StaticMesh,
        "/Game/GorillaProtocol/Weapons/P9/SM_P9_Suppressor",
    )


def import_environment():
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_animations = False
    options.import_materials = False
    options.import_textures = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH

    source_sets = {
        "Building": [
            "border-high.fbx", "column-wide.fbx", "detail-pipe.fbx", "floor.fbx",
            "plating-detailed-wide.fbx", "roof-flat-center.fbx", "stairs-open.fbx",
            "wall-doorway-wide-square.fbx", "wall-window-wide-square-detailed.fbx",
            "wall.fbx",
        ],
        "Industrial": [
            "building-q.fbx", "building-r.fbx", "chimney-large.fbx", "detail-tank.fbx",
        ],
    }
    for source_set, filenames in source_sets.items():
        for filename in filenames:
            asset_name = "SM_" + os.path.splitext(filename)[0].replace("-", "_")
            imported = import_task(
                os.path.join(RAW, "Kenney", "Environment", source_set, filename),
                "/Game/GorillaProtocol/Environment/Modular",
                asset_name,
                options,
            )
            require_imported_type(
                imported,
                unreal.StaticMesh,
                f"/Game/GorillaProtocol/Environment/Modular/{asset_name}",
            )


def import_audio_and_hdri():
    audio_dir = os.path.join(RAW, "Audio", "Italian")
    for filename in sorted(os.listdir(audio_dir)):
        if filename.endswith(".wav"):
            import_task(os.path.join(audio_dir, filename), "/Game/GorillaProtocol/Audio/Bruno")
    import_task(
        os.path.join(RAW, "PolyHaven", "limehouse_4k.exr"),
        "/Game/GorillaProtocol/Environment/HDRI",
    )
    concrete_dir = os.path.join(RAW, "PolyHaven", "Concrete")
    for filename in ("concrete_diff_1k.jpg", "concrete_nor_dx_1k.png", "concrete_arm_1k.png"):
        import_task(
            os.path.join(concrete_dir, filename),
            "/Game/GorillaProtocol/Environment/Textures",
        )


def create_material(name, color, roughness, metallic=0.0):
    path = f"/Game/GorillaProtocol/Materials/{name}"
    material = unreal.load_asset(path)
    if not material:
        material = ASSET_TOOLS.create_asset(
            name,
            "/Game/GorillaProtocol/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    base_color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -360, -60
    )
    base_color.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -360, 80
    )
    rough.set_editor_property("r", roughness)
    metal = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -360, 160
    )
    metal.set_editor_property("r", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(
        base_color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        metal, "", unreal.MaterialProperty.MP_METALLIC
    )
    errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if errors:
        raise RuntimeError(f"Material {material.get_path_name()} failed: {errors}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError(f"Could not save {material.get_path_name()}")


def create_concrete_material():
    path = "/Game/GorillaProtocol/Materials/M_YachtConcrete"
    material = unreal.load_asset(path)
    if not material:
        material = ASSET_TOOLS.create_asset(
            "M_YachtConcrete",
            "/Game/GorillaProtocol/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    texture_specs = (
        ("concrete_diff_1k", unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
        ("concrete_nor_dx_1k", unreal.MaterialProperty.MP_NORMAL, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
    )
    for row, (texture_name, property_name, sampler_type) in enumerate(texture_specs):
        sample = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -380, row * 180 - 80
        )
        sample.set_editor_property(
            "texture",
            unreal.load_asset(f"/Game/GorillaProtocol/Environment/Textures/{texture_name}"),
        )
        sample.set_editor_property("sampler_type", sampler_type)
        unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", property_name)

    arm = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -380, 300
    )
    arm.set_editor_property(
        "texture", unreal.load_asset("/Game/GorillaProtocol/Environment/Textures/concrete_arm_1k")
    )
    arm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    unreal.MaterialEditingLibrary.connect_material_property(arm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    unreal.MaterialEditingLibrary.connect_material_property(arm, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(arm, "B", unreal.MaterialProperty.MP_METALLIC)
    errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if errors:
        raise RuntimeError(f"Material {material.get_path_name()} failed: {errors}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError(f"Could not save {material.get_path_name()}")


def create_materials():
    create_material("M_BrunoFur", (0.028, 0.018, 0.012), 0.82, 0.0)
    create_material("M_WetDeck", (0.04, 0.055, 0.07), 0.16, 0.35)
    create_concrete_material()
    create_material("M_AlarmRed", (0.42, 0.006, 0.003), 0.26, 0.1)


def spawn_scaled_mesh(mesh_name, location, target_size, material_name="M_YachtConcrete", label="Facility"):
    mesh = unreal.load_asset(f"/Game/GorillaProtocol/Environment/Modular/{mesh_name}")
    if not mesh:
        raise RuntimeError(f"Missing environment mesh: {mesh_name}")
    actor = ACTORS.spawn_actor_from_object(
        mesh, unreal.Vector(*location), unreal.Rotator(), False
    )
    if not actor:
        raise RuntimeError(f"Could not place environment mesh: {mesh_name}")
    actor.set_actor_label(label)
    actor.set_editor_property("tags", [unreal.Name("GP_BOOTSTRAP")])
    origin, extent = actor.get_actor_bounds(False)
    current_size = (max(extent.x * 2.0, 1.0), max(extent.y * 2.0, 1.0), max(extent.z * 2.0, 1.0))
    actor.set_actor_scale3d(
        unreal.Vector(
            target_size[0] / current_size[0],
            target_size[1] / current_size[1],
            target_size[2] / current_size[2],
        )
    )
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    material = unreal.load_asset(f"/Game/GorillaProtocol/Materials/{material_name}")
    if component and material:
        component.set_material(0, material)
    return actor


def author_facility(world):
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if unreal.Name("GP_BOOTSTRAP") in actor.tags:
            ACTORS.destroy_actor(actor)

    pieces = [
        ("SM_floor", (1700, 0, -45), (4100, 1900, 90), "M_WetDeck", "Wet terrace"),
        ("SM_border_high", (1700, -980, 90), (4100, 45, 280), "M_YachtConcrete", "Port rail"),
        ("SM_border_high", (1700, 980, 90), (4100, 45, 280), "M_YachtConcrete", "Starboard rail"),
        ("SM_wall", (3750, 0, 220), (80, 1960, 520), "M_YachtConcrete", "Forward bulkhead"),
        ("SM_wall_window_wide_square_detailed", (2250, 0, 250), (900, 1000, 520), "M_YachtConcrete", "Operations lounge"),
        ("SM_wall_window_wide_square_detailed", (1790, 0, 250), (45, 920, 420), "M_YachtConcrete", "Lounge frontage"),
        ("SM_wall", (2500, -520, 185), (1350, 55, 390), "M_YachtConcrete", "North wall"),
        ("SM_wall_doorway_wide_square", (2500, 520, 185), (1350, 55, 390), "M_YachtConcrete", "South wall"),
        ("SM_plating_detailed_wide", (800, -430, 80), (330, 180, 170), "M_YachtConcrete", "Cover A"),
        ("SM_plating_detailed_wide", (1180, 360, 95), (260, 240, 200), "M_YachtConcrete", "Cover B"),
        ("SM_wall", (1670, -620, 70), (420, 150, 150), "M_YachtConcrete", "Planter cover"),
        ("SM_plating_detailed_wide", (3050, 390, 85), (300, 200, 180), "M_AlarmRed", "Ledger console"),
        ("SM_building_q", (3250, -1450, 500), (1400, 900, 1000), "M_YachtConcrete", "Cliff facility"),
        ("SM_building_r", (2200, 1500, 420), (1200, 800, 840), "M_YachtConcrete", "Communications annex"),
        ("SM_chimney_large", (3300, 650, 700), (190, 190, 1400), "M_YachtConcrete", "Radio mast"),
        ("SM_detail_tank", (500, 620, 130), (300, 300, 260), "M_YachtConcrete", "Fuel tank"),
        ("SM_detail_pipe", (2730, -530, 280), (900, 80, 130), "M_YachtConcrete", "Service pipe"),
    ]
    for mesh_name, location, size, material, label in pieces:
        spawn_scaled_mesh(mesh_name, location, size, material, label)

def create_map():
    map_path = "/Game/GorillaProtocol/Maps/L_ScimmiaDiMare"
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        if not LEVELS.load_level(map_path):
            raise RuntimeError(f"Could not load map {map_path}")
    else:
        if not LEVELS.new_level(map_path, False):
            raise RuntimeError(f"Could not create map {map_path}")

    world = EDITOR.get_editor_world()
    if not world:
        raise RuntimeError("Unreal did not provide an editor world")
    author_facility(world)
    starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    if not starts:
        start = ACTORS.spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(0.0, 0.0, 145.0), unreal.Rotator(), False
        )
        if not start:
            raise RuntimeError("Could not place Bruno's PlayerStart")
        start.set_actor_label("Bruno Start")

    world_settings = world.get_world_settings()
    game_mode = unreal.load_class(None, "/Script/GorillaProtocol.GPGameModeBase")
    if not game_mode:
        raise RuntimeError("GPGameModeBase is unavailable; compile the project module first")
    world_settings.set_editor_property("default_game_mode", game_mode)
    if world_settings.get_editor_property("default_game_mode") != game_mode:
        raise RuntimeError("GameMode override was not applied")
    if not LEVELS.save_current_level():
        raise RuntimeError("Could not save the playable map")
    if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
        raise RuntimeError("Could not save all generated packages")


def main():
    log("Importing guard and animation assets")
    import_guard()
    log("Importing weapon assets")
    import_weapon()
    log("Importing modular facility assets")
    import_environment()
    log("Importing Italian dialogue and maritime HDRI")
    import_audio_and_hdri()
    log("Creating project materials")
    create_materials()
    log("Creating playable mission map")
    create_map()
    log("PLAYABLE_BOOTSTRAP_COMPLETE")


main()
