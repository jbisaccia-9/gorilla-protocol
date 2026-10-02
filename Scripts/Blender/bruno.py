"""Bruno's camera-local gorilla viewmodel, in meters. Blender 5.2+.

Public API: build_viewmodel(camera) -> list[Object], build_bruno(location,
rotation) -> list[Object]. Builders do not change cameras, lights or render
settings. Run this file with Blender --python bruno.py -- --test for isolated
QA, or --source-test to render the optional source-based third-person hero.
Camera convention: +X right, +Y up, -Z forward. No external texture dependency.
"""

import argparse
import math
from pathlib import Path
import random

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Art" / "Bruno"


def _material(name, color, roughness=0.45, metallic=0.0, texture=None):
    name = "BRUNO_" + name
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    if texture:
        coord = nodes.new("ShaderNodeTexCoord")
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 220 if texture == "skin" else 370
        noise.inputs["Detail"].default_value = 3.0
        links.new(coord.outputs["Object"], noise.inputs["Vector"])
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.32 if texture == "skin" else 0.13
        bump.inputs["Distance"].default_value = 0.0012 if texture == "skin" else 0.00025
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        ramp = nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].position = 0.15
        ramp.color_ramp.elements[0].color = (*(v * 0.60 for v in color), 1)
        ramp.color_ramp.elements[1].position = 0.85
        ramp.color_ramp.elements[1].color = (*(v * 1.3 for v in color), 1)
        links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
        links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def _materials():
    return {
        "skin": _material("Black_Leather_Skin", (0.039, 0.033, 0.030), 0.48, texture="skin"),
        "fur": _material("Coarse_Charcoal_Fur", (0.014, 0.011, 0.009), 0.77),
        "fur_tip": _material("Warm_Guard_Hairs", (0.042, 0.032, 0.021), 0.66),
        "nail": _material("Worn_Black_Keratin", (0.052, 0.047, 0.039), 0.39),
        "metal": _material("Graphite_Anodized_Aluminum", (0.065, 0.079, 0.083), 0.30, 0.82, "metal"),
        "steel": _material("Machined_Gunmetal", (0.115, 0.125, 0.127), 0.27, 0.86),
        "dark": _material("Recess_Oxidized_Steel", (0.009, 0.013, 0.015), 0.47, 0.6),
        "polymer": _material("Olive_Black_Polymer", (0.049, 0.056, 0.045), 0.70, texture="polymer"),
        "rubber": _material("Stock_Rubber", (0.014, 0.018, 0.016), 0.86),
        "yellow": _material("Banana_Enamel_Yellow", (0.94, 0.53, 0.035), 0.29),
        "stem": _material("Banana_Brown_Tip", (0.14, 0.058, 0.012), 0.55),
        "gold": _material("Charm_Brass", (0.51, 0.30, 0.071), 0.25, 0.8),
        "green": _material("Italian_Green", (0.015, 0.22, 0.095), 0.7),
        "white": _material("Italian_Ivory", (0.74, 0.75, 0.65), 0.7),
        "red": _material("Italian_Red", (0.42, 0.018, 0.019), 0.7),
    }


def _mesh(name, vertices, faces, mat, smooth=True):
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(mat)
    if smooth:
        for polygon in mesh.polygons:
            polygon.use_smooth = True
    return obj


def _bevel(obj, width=0.003, segments=3):
    mod = obj.modifiers.new("Manufactured_Edge_Radii", "BEVEL")
    mod.width = width
    mod.segments = segments
    mod = obj.modifiers.new("Face_Weighted_Normals", "WEIGHTED_NORMAL")
    mod.keep_sharp = True
    return obj


def _box(name, center, size, mat, bevel=0.003, rotation=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    if bevel:
        _bevel(obj, bevel)
    if rotation:
        obj.rotation_euler = rotation
    return obj


def _cylinder(name, a, b, radius, mat, vertices=32):
    a, b = Vector(a), Vector(b)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius,
                                      depth=(b - a).length, location=(a + b) * 0.5)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    obj.data.materials.append(mat)
    for poly in obj.data.polygons:
        poly.use_smooth = len(poly.vertices) == 4
    return _bevel(obj, min(radius * 0.13, 0.0015), 2)


def _tube(name, points, radii, mat, sides=12, subdivision=2):
    """Ring loft with a stable frame; elliptical rings give anatomical mass."""
    points = [Vector(p) for p in points]
    verts, faces = [], []
    for i, (point, radius) in enumerate(zip(points, radii)):
        tangent = (points[min(i + 1, len(points) - 1)] - points[max(0, i - 1)]).normalized()
        ref = Vector((0, 1, 0)) if abs(tangent.y) < 0.90 else Vector((1, 0, 0))
        u = tangent.cross(ref).normalized()
        v = tangent.cross(u).normalized()
        rx, ry = radius if isinstance(radius, (tuple, list)) else (radius, radius)
        for k in range(sides):
            theta = 2 * math.pi * k / sides
            verts.append(point + rx * math.cos(theta) * u + ry * math.sin(theta) * v)
    for j in range(len(points) - 1):
        for k in range(sides):
            a, b = j * sides + k, j * sides + (k + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    faces.extend([tuple(reversed(range(sides))), tuple((len(points) - 1) * sides + k for k in range(sides))])
    obj = _mesh(name, verts, faces, mat)
    if subdivision:
        sub = obj.modifiers.new("Organic_Surface", "SUBSURF")
        sub.levels = subdivision
        sub.render_levels = subdivision
    return obj


def _curve(name, points, radius, mat):
    data = bpy.data.curves.new(name, "CURVE")
    data.dimensions = "3D"
    data.resolution_u = 16
    data.bevel_depth = radius
    data.bevel_resolution = 3
    spline = data.splines.new("BEZIER")
    spline.bezier_points.add(len(points) - 1)
    for p, co in zip(spline.bezier_points, points):
        p.co = co
        p.handle_left_type = p.handle_right_type = "AUTO"
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    data.materials.append(mat)
    return obj


def _ring(name, center, outer, inner, depth, mat):
    verts, faces = [], []
    for z, r in [(-depth / 2, outer), (depth / 2, outer),
                 (-depth / 2, inner), (depth / 2, inner)]:
        for i in range(48):
            a = i * math.tau / 48
            verts.append((center[0] + r * math.cos(a), center[1] + r * math.sin(a), center[2] + z))
    for i in range(48):
        j = (i + 1) % 48
        faces.extend([(i, j, 48 + j, 48 + i), (96 + j, 96 + i, 144 + i, 144 + j),
                      (j, i, 96 + i, 96 + j), (48 + i, 48 + j, 144 + j, 144 + i)])
    return _bevel(_mesh(name, verts, faces, mat), 0.0007, 2)


def _fuse(name, objects, mat):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        for mod in list(obj.modifiers):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    remesh = obj.modifiers.new("Continuous_Wrist_And_Knuckle_Skin", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = 0.0023
    bpy.ops.object.modifier_apply(modifier=remesh.name)
    smooth = obj.modifiers.new("Skin_Relax", "SMOOTH")
    smooth.factor = 0.7
    smooth.iterations = 3
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    for face in obj.data.polygons:
        face.use_smooth = True
    return obj


def _fur(name, surface, mats, count, seed, predicate=None, length=0.022):
    """Deterministic bent, tapered crossed ribbons sampled on evaluated skin."""
    rng = random.Random(seed)
    bpy.context.view_layer.update()
    evaluated = surface.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    mesh.calc_loop_triangles()
    triangles, areas = [], []
    for tri in mesh.loop_triangles:
        coords = [surface.matrix_world @ mesh.vertices[i].co for i in tri.vertices]
        midpoint = sum(coords, Vector()) / 3
        if predicate and not predicate(midpoint):
            continue
        triangles.append(coords)
        areas.append(tri.area)
    if not triangles:
        evaluated.to_mesh_clear()
        return None
    verts, faces = [], []
    for triangle in rng.choices(triangles, weights=areas, k=count):
        a, b, c = triangle
        u, v = rng.random(), rng.random()
        if u + v > 1:
            u, v = 1 - u, 1 - v
        point = a + u * (b - a) + v * (c - a)
        normal = (b - a).cross(c - a).normalized()
        direction = (normal * 0.6 + Vector((0.1, -0.45, 0.55))).normalized()
        extent = length * rng.uniform(0.45, 1.4)
        side = normal.cross(Vector((0.17, 0.91, 0.2))).normalized()
        width = rng.uniform(0.00040, 0.00090)
        point -= normal * 0.0005
        for axis in [side, normal.cross(side).normalized()]:
            n = len(verts)
            mid = point + direction * extent * 0.52 + normal * extent * 0.12
            tip = point + direction * extent
            verts.extend([point - axis * width, point + axis * width,
                          mid - axis * width * 0.45, mid + axis * width * 0.45, tip])
            faces.extend([(n, n + 1, n + 3, n + 2), (n + 2, n + 3, n + 4)])
    evaluated.to_mesh_clear()
    obj = _mesh(name, verts, faces, mats[0])
    obj.data.materials.append(mats[1])
    for face in obj.data.polygons:
        face.material_index = int(rng.random() < 0.15)
    return obj


def _weapon(m):
    x = 0.215
    _box("BRUNO_Carbine_Upper_Receiver", (x, -0.205, -0.748), (0.054, 0.064, 0.244), m["metal"], 0.007)
    _box("BRUNO_Lower_Receiver", (x, -0.251, -0.715), (0.048, 0.043, 0.135), m["metal"], 0.004)
    _box("BRUNO_Handguard_MLOK", (x, -0.202, -0.939), (0.052, 0.058, 0.166), m["metal"], 0.008)
    _cylinder("BRUNO_Barrel_Cold_Hammered", (x, -0.19, -1.012), (x, -0.19, -1.104), 0.011, m["steel"])
    _cylinder("BRUNO_Suppressor_Collar", (x, -0.19, -1.07), (x, -0.19, -1.103), 0.021, m["steel"])
    _cylinder("BRUNO_Suppressor_Titanium", (x, -0.19, -1.093), (x, -0.19, -1.267), 0.024, m["metal"], 64)
    _ring("BRUNO_Suppressor_Endcap_Real_Bore", (x, -0.19, -1.27), 0.024, 0.006, 0.009, m["steel"])
    _cylinder("BRUNO_Bore_Shadow", (x, -0.19, -1.252), (x, -0.19, -1.253), 0.006, m["dark"])
    for z in [-1.104, -1.115, -1.248, -1.258]:
        _ring("BRUNO_Suppressor_Turning_Line", (x, -0.19, z), 0.0246, 0.0234, 0.0018, m["dark"])
    _box("BRUNO_Rail_Spine", (x, -0.168, -0.829), (0.024, 0.009, 0.365), m["dark"], 0.001)
    for i in range(28):
        _box("BRUNO_Picatinny_Tooth_%02d" % i, (x, -0.162, -0.65 - i * 0.0128), (0.034, 0.01, 0.0068), m["steel"], 0.0011)
    for side in [-1, 1]:
        for i in range(5):
            _box("BRUNO_MLOK_Recess", (x + side * 0.0261, -0.204, -0.88 - i * 0.027), (0.0008, 0.010, 0.018), m["dark"], 0.0003)
        for z in [-0.665, -0.745, -0.833, -0.866, -1.002]:
            _cylinder("BRUNO_Captive_Receiver_Pin", (x + side * 0.025, -0.229, z),
                      (x + side * 0.029, -0.229, z), 0.003, m["steel"], 16)
    _box("BRUNO_Ejection_Port_Recess", (x + 0.0273, -0.198, -0.746), (0.0015, 0.024, 0.086), m["dark"], 0.0005)
    _box("BRUNO_Bolt_Carrier_Visible", (x + 0.0282, -0.198, -0.756), (0.0012, 0.014, 0.050), m["steel"], 0.0004)
    _box("BRUNO_Ejection_Port_Dust_Cover", (x + 0.032, -0.218, -0.746), (0.012, 0.004, 0.084), m["metal"], 0.001)
    _box("BRUNO_Charging_Handle", (x, -0.178, -0.635), (0.081, 0.013, 0.012), m["steel"], 0.003)
    _cylinder("BRUNO_Buffer_Tube", (x, -0.215, -0.632), (x, -0.215, -0.433), 0.017, m["dark"])
    _box("BRUNO_Stock_Cheek_Rest", (x, -0.215, -0.492), (0.060, 0.058, 0.143), m["polymer"], 0.011)
    _box("BRUNO_Stock_Butt_Pad", (x, -0.245, -0.416), (0.067, 0.109, 0.021), m["rubber"], 0.007)
    for i in range(7):
        _box("BRUNO_Stock_Traction_Rib", (x, -0.29 + i * 0.014, -0.404), (0.054, 0.004, 0.003), m["dark"], 0.001)
    _box("BRUNO_Pistol_Grip", (x, -0.321, -0.656), (0.038, 0.112, 0.045), m["polymer"], 0.009, (math.radians(-17), 0, 0))
    for i in range(6):
        _box("BRUNO_Grip_Texture_Rib", (x + 0.020, -0.289 - i * 0.014, -0.656 + i * 0.004), (0.002, 0.004, 0.035), m["rubber"], 0.001)
    _curve("BRUNO_Trigger_Guard", [(x, -0.267, -0.739), (x, -0.305, -0.73), (x, -0.310, -0.696), (x, -0.276, -0.677)], 0.004, m["metal"])
    _curve("BRUNO_Curved_Trigger", [(x, -0.265, -0.710), (x, -0.285, -0.716), (x, -0.295, -0.705)], 0.0028, m["steel"])
    # Rectangular loft rather than an oversized cylindrical magazine.
    verts, faces = [], []
    for y, z in [(-0.268, -0.786), (-0.29, -0.787), (-0.36, -0.802), (-0.426, -0.835)]:
        verts.extend([(x + dx, y, z + dz) for dx, dz in [(-0.016, -0.034), (0.016, -0.034), (0.016, 0.034), (-0.016, 0.034)]])
    faces.extend([(0, 3, 2, 1), (12, 13, 14, 15)])
    for j in range(3):
        for k in range(4):
            faces.append((j * 4 + k, j * 4 + (k + 1) % 4, (j + 1) * 4 + (k + 1) % 4, (j + 1) * 4 + k))
    _bevel(_mesh("BRUNO_Curved_Box_Magazine", verts, faces, m["polymer"], False), 0.004)
    for side in [-1, 1]:
        for dz in [-0.022, 0, 0.022]:
            _curve("BRUNO_Magazine_Molded_Flute", [(x + side * 0.0165, -0.290, -0.787 + dz), (x + side * 0.0165, -0.350, -0.8 + dz), (x + side * 0.0165, -0.413, -0.83 + dz)], 0.0018, m["dark"])
    _box("BRUNO_Magazine_Floorplate", (x, -0.428, -0.835), (0.040, 0.014, 0.079), m["rubber"], 0.003)
    _box("BRUNO_Optic_Riser", (x, -0.137, -0.738), (0.033, 0.034, 0.052), m["metal"], 0.003)
    _ring("BRUNO_Micro_Optic_Housing", (x, -0.105, -0.741), 0.025, 0.020, 0.066, m["metal"])
    for z in [-0.777, -0.705]:
        _ring("BRUNO_Optic_Rim", (x, -0.105, z), 0.026, 0.020, 0.008, m["dark"])
    glass = _material("Optic_Coated_Glass", (0.055, 0.19, 0.17), 0.10, 0.1)
    shader = glass.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Transmission Weight"].default_value = 0.94
    shader.inputs["IOR"].default_value = 1.46
    _cylinder("BRUNO_Optic_Lens", (x, -0.105, -0.760), (x, -0.105, -0.761), 0.0198, glass, 48)
    _cylinder("BRUNO_Optic_Windage_Dial", (x + 0.019, -0.105, -0.741), (x + 0.037, -0.105, -0.741), 0.009, m["dark"])
    _box("BRUNO_Front_Folding_Sight", (x, -0.153, -0.990), (0.031, 0.008, 0.029), m["dark"], 0.002)
    # Three sewn color bars identify the Italian hero without billboard text.
    for i, color in enumerate(["green", "white", "red"]):
        _box("BRUNO_Italian_Stock_Inlay_" + color, (x + 0.0304, -0.211, -0.47 - i * 0.009), (0.001, 0.017, 0.008), m[color], 0.0004)
    _curve("BRUNO_Banana_Charm_Chain", [(0.245, -0.227, -0.629), (0.277, -0.246, -0.625), (0.276, -0.277, -0.625)], 0.001, m["gold"])
    _ring("BRUNO_Banana_Split_Ring", (0.276, -0.28, -0.625), 0.005, 0.0038, 0.001, m["gold"])
    _tube("BRUNO_Little_Curved_Banana_Charm", [(0.276, -0.283, -0.625), (0.278, -0.298, -0.625), (0.289, -0.311, -0.625), (0.306, -0.310, -0.625), (0.317, -0.30, -0.625)], [0.0018, 0.005, 0.0062, 0.0045, 0.0009], m["yellow"], 12, 2)
    _cylinder("BRUNO_Banana_Stem", (0.276, -0.279, -0.625), (0.276, -0.286, -0.625), 0.0018, m["stem"], 12)


def _arms(m):
    right = [_tube("BRUNO_Right_Forearm_Base", [(0.35, -0.40, -0.32), (0.337, -0.374, -0.39), (0.305, -0.348, -0.47), (0.277, -0.329, -0.55), (0.269, -0.317, -0.604), (0.266, -0.307, -0.641)], [(0.09, 0.076), (0.085, 0.071), (0.072, 0.058), (0.051, 0.044), (0.040, 0.037), (0.032, 0.03)], m["skin"])]
    right.append(_tube("BRUNO_Right_Palm_Base", [(0.264, -0.322, -0.601), (0.268, -0.314, -0.634), (0.267, -0.309, -0.661), (0.263, -0.306, -0.685)], [(0.023, 0.028), (0.034, 0.051), (0.032, 0.052), (0.02, 0.042)], m["skin"]))
    for i in range(3):
        y = -0.312 - i * 0.024
        points = [(0.27, y, -0.671), (0.25, y - 0.004, -0.685), (0.23, y - 0.005, -0.687), (0.199, y - 0.003, -0.678), (0.194, y + 0.001, -0.653)]
        right.append(_tube("BRUNO_Right_Grip_Finger_%d" % i, points, [0.015, 0.016, 0.014, 0.012, 0.007], m["skin"]))
        _curve("BRUNO_Right_Knuckle_Crease_%d" % i, [(0.254, y + 0.009, -0.674), (0.261, y + 0.007, -0.680), (0.268, y + 0.004, -0.679)], 0.00065, m["dark"])
    # Trigger discipline: index rests on the receiver, not through the trigger.
    right.append(_tube("BRUNO_Right_Index_Along_Receiver", [(0.267, -0.28, -0.661), (0.263, -0.267, -0.690), (0.253, -0.264, -0.72), (0.251, -0.261, -0.75)], [0.015, 0.014, 0.012, 0.007], m["skin"]))
    right.append(_tube("BRUNO_Right_Opposing_Thumb", [(0.264, -0.293, -0.624), (0.254, -0.27, -0.630), (0.223, -0.27, -0.637), (0.187, -0.282, -0.647)], [0.021, 0.021, 0.017, 0.009], m["skin"]))
    skin_r = _fuse("BRUNO_Right_Hand_And_Forearm", right, m["skin"])
    left = [_tube("BRUNO_Left_Forearm_Base", [(-0.125, -0.45, -0.36), (-0.094, -0.399, -0.46), (-0.041, -0.356, -0.60), (0.035, -0.318, -0.77), (0.105, -0.288, -0.898), (0.145, -0.268, -0.938)], [(0.076, 0.063), (0.073, 0.059), (0.062, 0.05), (0.047, 0.041), (0.034, 0.029), (0.03, 0.025)], m["skin"])]
    left.append(_tube("BRUNO_Left_Support_Palm", [(0.109, -0.287, -0.9), (0.151, -0.271, -0.932), (0.185, -0.259, -0.948), (0.209, -0.247, -0.949)], [(0.025, 0.025), (0.042, 0.027), (0.046, 0.022), (0.033, 0.018)], m["skin"]))
    for i in range(4):
        z = -0.904 - i * 0.022
        y = -0.246 + (0.003 if i in [1, 2] else 0)
        points = [(0.17, -0.263, z), (0.205, -0.256, z - 0.003), (0.239, y - 0.004, z - 0.004), (0.250, -0.227, z - 0.005), (0.245, -0.212, z - 0.006)]
        left.append(_tube("BRUNO_Left_Curled_Finger_%d" % i, points, [0.014, 0.014, 0.013, 0.011, 0.0065], m["skin"]))
        _tube("BRUNO_Left_Flat_Keratin_Nail_%d" % i, [(0.254, -0.228, z - 0.005), (0.253, -0.222, z - 0.006), (0.249, -0.216, z - 0.006)], [(0.0044, 0.0011), (0.005, 0.0013), (0.003, 0.0007)], m["nail"], 8, 2)
        _curve("BRUNO_Left_Knuckle_Crease_%d" % i, [(0.243, -0.249, z + 0.006), (0.250, -0.245, z), (0.249, -0.242, z - 0.006)], 0.0006, m["dark"])
    left.append(_tube("BRUNO_Left_Opposing_Thumb", [(0.135, -0.26, -0.919), (0.157, -0.23, -0.902), (0.182, -0.211, -0.886), (0.189, -0.193, -0.901)], [0.021, 0.020, 0.017, 0.009], m["skin"]))
    skin_l = _fuse("BRUNO_Left_Hand_And_Forearm", left, m["skin"])
    _fur("BRUNO_Right_Coarse_Fur", skin_r, [m["fur"], m["fur_tip"]], 4300, 721, lambda p: p.z > -0.605, 0.020)
    _fur("BRUNO_Left_Coarse_Fur", skin_l, [m["fur"], m["fur_tip"]], 4600, 722, lambda p: p.z > -0.890, 0.019)
    _fur("BRUNO_Right_Wrist_Sparse_Hair", skin_r, [m["fur"], m["fur_tip"]], 320, 723, lambda p: -0.641 < p.z < -0.597, 0.009)
    _fur("BRUNO_Left_Wrist_Sparse_Hair", skin_l, [m["fur"], m["fur_tip"]], 320, 724, lambda p: -0.930 < p.z < -0.878, 0.008)


def build_viewmodel(camera):
    """Create a 0.87 m compact carbine and gorilla arms, parented to camera.

    All geometry is expressed in camera-local meters. Returns all created
    objects including BRUNO_Viewmodel_Root. No animation is enabled by default.
    A 26-30 mm camera (36 mm sensor) is recommended for the intended framing.
    """
    if camera is None or camera.type != "CAMERA":
        raise ValueError("build_viewmodel requires a Blender camera object")
    before = set(bpy.data.objects)
    m = _materials()
    _weapon(m)
    _arms(m)
    root = bpy.data.objects.new("BRUNO_Viewmodel_Root", None)
    bpy.context.collection.objects.link(root)
    root.empty_display_size = 0.08
    root.parent = camera
    for obj in set(bpy.data.objects) - before - {root}:
        obj.parent = root
    root["coordinate_system"] = "+X right; +Y up; -Z forward; meters"
    root["hero"] = "Bruno / Italian coastal intelligence"
    root["weapon_length_m"] = 0.87
    root["grip_pose"] = "low ready; four-finger support; trigger finger indexed"
    return sorted(set(bpy.data.objects) - before, key=lambda o: o.name)


def add_idle_animation(root, start=1, end=121):
    """Optional subtle 120-frame breath loop. Does not alter scene timing."""
    base = root.location.copy()
    rotation = root.rotation_euler.copy()
    for frame, rise in [(start, 0), ((start + end) // 2, 1), (end, 0)]:
        root.location = base + Vector((0.0015 * rise, 0.0025 * rise, 0))
        root.rotation_euler = (rotation.x + 0.002 * rise, rotation.y, rotation.z + 0.0015 * rise)
        root.keyframe_insert(data_path="location", frame=frame)
        root.keyframe_insert(data_path="rotation_euler", frame=frame)
    root.animation_data.action.name = "BRUNO_Idle_Slow_Controlled_Breath"
    root.location = base
    root.rotation_euler = rotation


def build_bruno(location=(0, 0, 0), rotation=(0, 0, 0)):
    """Optional unrigged 1.95 m hero using the supplied CC0 gorilla mesh.

    rotation is an XYZ Euler in radians, or a scalar Z heading. This is a
    source-based third-person placeholder, not the detailed viewmodel hands.
    """
    source = ROOT / "RawContent" / "Gorilla" / "gorilla_male.blend"
    if not source.exists():
        raise FileNotFoundError(source)
    before = set(bpy.data.objects)
    m = _materials()
    with bpy.data.libraries.load(str(source), link=False) as (available, chosen):
        chosen.objects = ["Crocodile"] if "Crocodile" in available.objects else []
    if not chosen.objects:
        raise ValueError("Expected CC0 gorilla source mesh 'Crocodile'")
    body = chosen.objects[0]
    bpy.context.collection.objects.link(body)
    body.name = "BRUNO_CC0_Third_Person_Body"
    for mod in list(body.modifiers):
        if mod.type == "EDGE_SPLIT":
            body.modifiers.remove(mod)
    body.data.materials.clear()
    body.data.materials.append(m["fur"])
    sub = body.modifiers.new("Hero_Silhouette_Subdivision", "SUBSURF")
    sub.levels = sub.render_levels = 2
    for face in body.data.polygons:
        face.use_smooth = True
    bpy.context.view_layer.update()
    scale = 1.95 / body.dimensions.z
    body.scale = (scale,) * 3
    bpy.context.view_layer.update()
    lowest = min((body.matrix_world @ Vector(corner)).z for corner in body.bound_box)
    body.location.z -= lowest
    _fur("BRUNO_Third_Person_Guard_Fur", body, [m["fur"], m["fur_tip"]], 22000, 933, lambda p: p.y > -0.30, 0.033)
    root = bpy.data.objects.new("BRUNO_Third_Person_Root", None)
    bpy.context.collection.objects.link(root)
    for obj in set(bpy.data.objects) - before - {root}:
        obj.parent = root
    root.location = location
    root.rotation_euler = (0, 0, rotation) if isinstance(rotation, (int, float)) else rotation
    root["source"] = "RawContent/Gorilla/gorilla_male.blend (user-supplied CC0)"
    root["status"] = "Unrigged optional silhouette; detailed hands are viewmodel-only"
    return sorted(set(bpy.data.objects) - before, key=lambda o: o.name)


def _test(source=False):
    """Isolated QA only. Never called by import or either public builder."""
    import json
    from bpy_extras.object_utils import world_to_camera_view

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 16
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1440
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = "AgX"
    world = bpy.data.worlds.new("BRUNO_QA_Storm_Ambient")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.13, 0.19, 0.25, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.38
    scene.world = world
    bpy.ops.object.camera_add(location=(0, 0, 0))
    camera = bpy.context.object
    camera.name = "BRUNO_QA_Camera"
    camera.data.lens = 27
    camera.data.clip_start = 0.025
    scene.camera = camera
    if source:
        created = build_bruno()
        camera.location = (3.0, -4.5, 2.45)
        camera.rotation_euler = (Vector((0, 0, 1)) - camera.location).to_track_quat("-Z", "Y").to_euler()
        camera.data.lens = 49
    else:
        created = build_viewmodel(camera)
    for name, position, power, color, size in [
        ("Cold_Sky_Softbox", (-1.5, 1.8, 0.6), 150, (0.64, 0.78, 1.0), 2.3),
        ("Facility_Warm_Rim", (1.5, 0.45, -1.8), 180, (1.0, 0.74, 0.46), 1.3),
        ("Front_Shape_Fill", (0.0, -0.3, 1.7), 60, (0.80, 0.91, 1.0), 1.7),
    ]:
        bpy.ops.object.light_add(type="AREA", location=position)
        light = bpy.context.object
        light.name = "BRUNO_QA_" + name
        light.data.energy = power * (8 if source else 1)
        light.data.color = color
        light.data.shape = "DISK"
        light.data.size = size
        target = Vector((0, 0, 1) if source else (0.12, -0.25, -0.7))
        light.rotation_euler = (target - light.location).to_track_quat("-Z", "Y").to_euler()
    OUT.mkdir(parents=True, exist_ok=True)
    stem = "bruno_source_hero" if source else "bruno_viewmodel"
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.filepath = str(OUT / (stem + ".png"))
    bpy.context.view_layer.update()
    stats = {"created_objects": len(created), "camera_lens_mm": camera.data.lens,
             "render_samples": 16, "source_hero": source, "objects": {}}
    for obj in created:
        if obj.type != "MESH":
            continue
        projected = [world_to_camera_view(scene, camera, obj.matrix_world @ Vector(c)) for c in obj.bound_box]
        stats["objects"][obj.name] = {
            "dimensions_m": [round(v, 4) for v in obj.dimensions],
            "screen_bounds_xy": [[round(min(p[i] for p in projected), 4), round(max(p[i] for p in projected), 4)] for i in range(2)],
        }
    (OUT / (stem + "_qa.json")).write_text(json.dumps(stats, indent=2))
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / (stem + "_qa.blend")))
    bpy.ops.render.render(write_still=True)
    print("BRUNO_QA_COMPLETE", str(OUT / (stem + ".png")), len(created))


if __name__ == "__main__":
    import sys
    parser = argparse.ArgumentParser()
    parser.add_argument("--test", action="store_true")
    parser.add_argument("--source-test", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    if args.test or args.source_test:
        _test(args.source_test)
