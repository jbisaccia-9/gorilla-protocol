from pathlib import Path

import unreal

MAP_PATH = "/Game/GorillaProtocol/Maps/L_Boot"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    level_subsystem.load_level(MAP_PATH)
else:
    if not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError(f"Unable to create {MAP_PATH}")

if not any(isinstance(actor, unreal.PlayerStart) for actor in actor_subsystem.get_all_level_actors()):
    actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(0.0, 0.0, 110.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )

if not level_subsystem.save_current_level():
    raise RuntimeError(f"Unable to save {MAP_PATH}")

config_path = Path(unreal.Paths.project_config_dir()) / "DefaultEngine.ini"
config_text = config_path.read_text(encoding="utf-8")
config_text = config_text.replace("/Engine/Maps/Entry", f"{MAP_PATH}.L_Boot")
config_path.write_text(config_text, encoding="utf-8")

unreal.log(f"Created and saved {MAP_PATH}")
unreal.SystemLibrary.quit_editor()
