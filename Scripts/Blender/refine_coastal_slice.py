"""Refine the saved coastal scene without rebuilding its architecture or guards."""

import argparse
import json
import math
import random
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


def foliage():
    rng = random.Random(401)
    leaves = bpy.data.collections.get("Refinement | Cypress sprays")
    if leaves is None:
        leaves = bpy.data.collections.new("Refinement | Cypress sprays")
        bpy.context.scene.collection.children.link(leaves)
    for obj in list(leaves.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    verts, faces = [], []
    for core in list(bpy.context.scene.objects):
        if not core.name.startswith("Cypress foliage"):
            continue
        if "original_scale" not in core:
            core["original_scale"] = list(core.scale)
        sx, sy, sz = core["original_scale"]
        core.scale = (sx*.70, sy*.70, sz*.80)
        center = core.location
        for _ in range(400):
            # Overlapping narrow fronds break up the smooth ellipsoid outline.
            zz = rng.uniform(-1, 1)
            angle = rng.uniform(0, math.tau)
            radial = math.sqrt(1-zz*zz) * rng.uniform(.35, 1)
            point = center + Vector((math.cos(angle)*sx*radial,
                                     math.sin(angle)*sy*radial, zz*sz))
            direction = Vector((math.cos(angle)*.35, math.sin(angle)*.35, 1)).normalized()
            length = rng.uniform(.09, .23)
            width = rng.uniform(.012, .027)
            side = Vector((-math.sin(angle), math.cos(angle), 0))*width
            for cross in (side, direction.cross(side)):
                n = len(verts)
                verts.extend([point-direction*length*.25,
                              point+cross, point+direction*length,
                              point-cross])
                faces.extend([(n,n+1,n+2),(n,n+2,n+3)])
    mesh = bpy.data.meshes.new("Cypress needle sprays")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    mesh.materials.append(bpy.data.materials["GP_Foliage"])
    mesh.materials.append(bpy.data.materials["GP_FoliageTips"])
    for face in mesh.polygons:
        face.material_index = int(rng.random() < .18)
    obj = bpy.data.objects.new("REFINE_CypressSprays", mesh)
    leaves.objects.link(obj)
    return len(faces)


def fur():
    sys.path.insert(0, str(Path(__file__).parent))
    from bruno import _fur, _materials
    mats = _materials()
    root = bpy.data.objects.get("BRUNO_Viewmodel_Root")
    if root is None:
        return 0
    for name in ("BRUNO_Right_Coarse_Fur", "BRUNO_Left_Coarse_Fur",
                 "REFINE_Right_Fur", "REFINE_Left_Fur"):
        previous = bpy.data.objects.get(name)
        if previous:
            bpy.data.objects.remove(previous, do_unlink=True)
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for side, cutoff, seed in (("Right", -.605, 721), ("Left", -.890, 722)):
        skin = bpy.data.objects[f"BRUNO_{side}_Hand_And_Forearm"]
        # The evaluated mesh is in camera-local space; avoid sampling world-space
        # height after the camera has been positioned inside the environment.
        mesh = bpy.data.meshes.new_from_object(skin.evaluated_get(depsgraph))
        proxy = bpy.data.objects.new("Temporary local fur sampling surface", mesh)
        bpy.context.scene.collection.objects.link(proxy)
        hair = _fur(f"REFINE_{side}_Fur", proxy, [mats["fur"], mats["fur_tip"]],
                    26000, seed, lambda p: p.z > cutoff, .030)
        if hair is None:
            raise RuntimeError(f"No forearm surface found for {side} fur")
        hair.parent = root
        hair.matrix_parent_inverse = Matrix.Identity(4)
        bpy.data.objects.remove(proxy, do_unlink=True)
        bpy.data.meshes.remove(mesh)
    return 52000


def lighting():
    scene = bpy.context.scene
    nt = scene.world.node_tree
    bg = nt.nodes.get("Background")
    bg.inputs["Strength"].default_value = .42
    noise = next(n for n in nt.nodes if n.type == "TEX_NOISE")
    noise.inputs["Scale"].default_value = 2.3
    noise.inputs["Detail"].default_value = 3
    coord = next(n for n in nt.nodes if n.type == "TEX_COORD")
    stretch = nt.nodes.get("REFINE_CloudBands") or nt.nodes.new("ShaderNodeVectorMath")
    stretch.name = "REFINE_CloudBands"
    stretch.operation = "MULTIPLY"
    stretch.inputs[1].default_value = (1, 1, 3.5)
    nt.links.new(coord.outputs["Normal"], stretch.inputs[0])
    nt.links.new(stretch.outputs[0], noise.inputs["Vector"])
    ramp = next(n for n in nt.nodes if n.type == "VALTORGB")
    ramp.color_ramp.elements[0].position = .34
    ramp.color_ramp.elements[0].color = (.002, .008, .020, 1)
    ramp.color_ramp.elements[1].position = .70
    ramp.color_ramp.elements[1].color = (.085, .22, .39, 1)
    for name, energy, size in (("Moon through broken cloud", 2200, 5),
                                ("Soft coastal sky", 1150, 12),
                                ("Forecourt blue fill", 130, 8)):
        light = bpy.data.objects[name].data
        light.energy, light.size = energy, size
    for obj in scene.objects:
        if obj.type == "LIGHT" and obj.name.startswith("Operations tungsten"):
            obj.data.energy = 350
            obj.data.color = (1, .51, .22)
        if obj.type == "LIGHT" and obj.name.startswith("Canopy practical"):
            obj.data.color = (1, .48, .20)
    scene.view_settings.exposure = .05
    # Use this scene's illumination when opening Material Preview on the Mac.
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == "VIEW_3D":
                shade = area.spaces.active.shading
                shade.type = "MATERIAL"
                shade.use_scene_world = True
                shade.use_scene_lights = True


def refine():
    scene = bpy.context.scene
    print("Refining cypress silhouettes", flush=True)
    leaf_triangles = foliage()
    print("Building denser forearm fur", flush=True)
    hair_count = fur()
    lighting()
    # Bring the first guard into the open to make pose and scale reviewable.
    guard = bpy.data.objects.get("Guard Alfa_Root")
    if guard:
        guard.location = (-.6, -1.5, .045)
    scene["coastal_art_revision"] = 2
    return {"revision":2, "cypress_spray_triangles":leaf_triangles,
            "forearm_hair_clumps":hair_count, "gameplay_tested":False,
            "changes":["needle foliage", "denser fur", "storm light contrast", "guard framing"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--render", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    if not bpy.data.filepath:
        raise RuntimeError("Open the saved coastal .blend before running this script")
    result = refine()
    out = Path(bpy.data.filepath).parent
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 1120, 630
    scene.cycles.samples = 24
    scene.render.filepath = str(out/"encounter-refined.png")
    bpy.ops.wm.save_as_mainfile(filepath=bpy.data.filepath)
    (out/"refinement-report.json").write_text(json.dumps(result, indent=2)+"\n")
    if args.render:
        bpy.ops.render.render(write_still=True)
    print("COASTAL_REFINEMENT_COMPLETE", flush=True)


if __name__ == "__main__":
    main()
