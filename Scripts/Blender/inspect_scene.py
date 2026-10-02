"""Verify the saved art scene and render another camera without rebuilding it."""

import argparse
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--camera", default="PLAYER | Lower terrace")
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--full", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    scene = bpy.context.scene
    camera = bpy.data.objects.get(args.camera)
    if camera is None or camera.type != "CAMERA":
        raise RuntimeError(f"Camera missing: {args.camera}")
    scene.camera = camera
    out = Path(bpy.data.filepath).parent
    checks = {
        "camera_count": len([obj for obj in scene.objects if obj.type == "CAMERA"]),
        "light_count": len([obj for obj in scene.objects if obj.type == "LIGHT"]),
        "has_gorilla_hands": all(bpy.data.objects.get(name) is not None for name in
                                 ("BRUNO_Left_Hand_And_Forearm", "BRUNO_Right_Hand_And_Forearm")),
        "has_banana_charm": bpy.data.objects.get("BRUNO_Little_Curved_Banana_Charm") is not None,
        "has_guard_alfa": bpy.data.objects.get("Guard Alfa_Root") is not None,
        "has_guard_bravo": bpy.data.objects.get("Guard Bravo_Root") is not None,
        "external_images": [image.name for image in bpy.data.images
                            if image.source == "FILE" and not image.packed_file],
    }
    assert checks["has_gorilla_hands"] and checks["has_banana_charm"], checks
    assert checks["has_guard_alfa"] and checks["has_guard_bravo"], checks
    assert not checks["external_images"], checks
    depsgraph = bpy.context.evaluated_depsgraph_get()
    triangles = 0
    for obj in scene.objects:
        if obj.type != "MESH":
            continue
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        mesh.calc_loop_triangles()
        triangles += len(mesh.loop_triangles)
        evaluated.to_mesh_clear()
    checks["evaluated_triangles"] = triangles
    checks["gameplay_tested"] = False
    checks["unreal_tested"] = False
    checks["art_revision"] = int(scene.get("coastal_art_revision", 1))
    manifest_path = out / "build-manifest.json"
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        manifest.update(objects=len(scene.objects), materials=len(bpy.data.materials),
                        art_revision=checks["art_revision"])
        manifest_path.write_text(json.dumps(manifest, indent=2)+"\n")
    (out / "scene-checks.json").write_text(json.dumps(checks, indent=2)+"\n")
    print("SCENE_CHECKS", json.dumps(checks), flush=True)
    if args.render:
        scene.render.resolution_x = 1920 if args.full else 1120
        scene.render.resolution_y = 1080 if args.full else 630
        scene.cycles.samples = 64 if args.full else 16
        scene.render.filepath = str(out / ("architecture.png" if args.camera.startswith("ARCHITECTURE") else "encounter.png"))
        bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    main()
