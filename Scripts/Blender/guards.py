"""Original procedural guard for Blender 5.2; meters, Z up, local -Y facing.

    root, children = build_guard('Guard_A', (0, 0, 0), facing=0.0, pose='aim')

`facing` is a Z rotation in radians. `children` is a flat list of every mesh
and pivot under the root, excluding the root itself. Shared Guard_* materials
are reused. No selection, camera, frame, or existing objects are cleared by
build_guard. Mesh modifiers are baked; the hierarchy is NOT a skinned rig.
Local animation ranges: idle 1-40, aim 41-80, recoil 81-96 (24 fps suggested).
`pose` sets the initial transform without changing the scene frame; playback
evaluates the keyed timeline. Recoil is presentation animation, not gameplay.

Run an isolated, destructive-to-the-current-scene preview only via CLI:
  blender -b --python guards.py -- --preview --output /absolute/Art/Guards
"""

import argparse
import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


MATERIALS = {
    'Guard_NavyFabric': ((0.023, 0.040, 0.069, 1), 0.0, 0.88),
    'Guard_FabricSeams': ((0.054, 0.074, 0.100, 1), 0.0, 0.85),
    'Guard_Armor': ((0.042, 0.058, 0.065, 1), 0.10, 0.64),
    'Guard_Webbing': ((0.091, 0.109, 0.108, 1), 0.0, 0.90),
    'Guard_Rubber': ((0.014, 0.019, 0.023, 1), 0.0, 0.75),
    'Guard_Gunmetal': ((0.050, 0.061, 0.073, 1), 0.70, 0.36),
    'Guard_Steel': ((0.18, 0.21, 0.23, 1), 0.80, 0.30),
    'Guard_Visor': ((0.055, 0.115, 0.145, 1), 0.62, 0.18),
    'Guard_Skin': ((0.37, 0.215, 0.140, 1), 0.0, 0.66),
    'Guard_Identifier': ((0.61, 0.48, 0.24, 1), 0.1, 0.63),
    'Guard_Lens': ((0.05, 0.28, 0.26, 1), 0.45, 0.15),
}


def _materials():
    result = {}
    for name, (color, metallic, roughness) in MATERIALS.items():
        material = bpy.data.materials.get(name)
        if material is None:
            material = bpy.data.materials.new(name)
            material.diffuse_color = color
            material.use_nodes = True
            shader = material.node_tree.nodes.get('Principled BSDF')
            shader.inputs['Base Color'].default_value = color
            shader.inputs['Metallic'].default_value = metallic
            shader.inputs['Roughness'].default_value = roughness
            if name in ('Guard_NavyFabric', 'Guard_Webbing'):
                noise = material.node_tree.nodes.new('ShaderNodeTexNoise')
                noise.inputs['Scale'].default_value = 185.0
                noise.inputs['Detail'].default_value = 2.0
                bump = material.node_tree.nodes.new('ShaderNodeBump')
                bump.inputs['Strength'].default_value = 0.14
                bump.inputs['Distance'].default_value = 0.0012
                material.node_tree.links.new(noise.outputs['Fac'], bump.inputs['Height'])
                material.node_tree.links.new(bump.outputs['Normal'], shader.inputs['Normal'])
        result[name.removeprefix('Guard_')] = material
    return result


class _Builder:
    def __init__(self, name):
        self.name = name
        self.materials = _materials()
        self.children = []
        self.collection = bpy.data.collections.new(name + '_Collection')
        bpy.context.scene.collection.children.link(self.collection)
        self.root = self.pivot('Root', (0, 0, 0), None)

    def attach(self, obj, label, parent):
        obj.name = self.name + '_' + label
        for collection in list(obj.users_collection):
            collection.objects.unlink(obj)
        self.collection.objects.link(obj)
        if parent is not None:
            # Everything is authored in canonical guard space before placement.
            world = obj.matrix_world.copy()
            obj.parent = parent
            obj.matrix_world = world
            self.children.append(obj)
        return obj

    def pivot(self, label, location, parent):
        obj = bpy.data.objects.new(self.name + '_' + label, None)
        obj.empty_display_type = 'PLAIN_AXES'
        obj.empty_display_size = 0.08
        obj.location = location
        self.collection.objects.link(obj)
        bpy.context.view_layer.update()
        return self.attach(obj, label, parent)

    def finish(self, obj, label, material, parent, smooth=False):
        obj.data.materials.append(self.materials[material])
        for poly in obj.data.polygons:
            poly.use_smooth = smooth
        bpy.context.view_layer.update()
        return self.attach(obj, label, parent)

    def bake(self, obj, modifier):
        with bpy.context.temp_override(object=obj, active_object=obj,
                                       selected_objects=[obj], selected_editable_objects=[obj]):
            bpy.ops.object.modifier_apply(modifier=modifier.name)

    def box(self, label, center, size, material, parent, bevel=0.01, rotation=None):
        bpy.ops.mesh.primitive_cube_add(size=1, location=center)
        obj = bpy.context.object
        # Bake size directly into vertices, avoiding operator selection side effects.
        for vertex in obj.data.vertices:
            vertex.co.x *= size[0]
            vertex.co.y *= size[1]
            vertex.co.z *= size[2]
        if bevel:
            mod = obj.modifiers.new('Rounded manufactured edges', 'BEVEL')
            mod.width = min(bevel, min(size) * 0.42)
            mod.segments = 2
            self.bake(obj, mod)
        if rotation:
            obj.rotation_euler = rotation
        return self.finish(obj, label, material, parent)

    def ellipsoid(self, label, center, size, material, parent, segments=32, rings=16):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings,
                                           radius=1, location=center)
        obj = bpy.context.object
        for vertex in obj.data.vertices:
            vertex.co.x *= size[0]
            vertex.co.y *= size[1]
            vertex.co.z *= size[2]
        return self.finish(obj, label, material, parent, True)

    def loft(self, label, profiles, material, parent, sides=24, subdiv=1):
        """Horizontal elliptical rings: (x, y, z, radius_x, radius_y)."""
        vertices = []
        for x, y, z, rx, ry in profiles:
            for j in range(sides):
                angle = j * math.tau / sides
                vertices.append((x + rx * math.cos(angle), y + ry * math.sin(angle), z))
        faces = [tuple(reversed(range(sides)))]
        for ring in range(len(profiles) - 1):
            for j in range(sides):
                a = ring * sides + j
                b = ring * sides + (j + 1) % sides
                faces.append((a, b, b + sides, a + sides))
        faces.append(tuple((len(profiles) - 1) * sides + j for j in range(sides)))
        mesh = bpy.data.meshes.new(self.name + '_' + label + '_Mesh')
        mesh.from_pydata(vertices, [], faces)
        mesh.update()
        obj = bpy.data.objects.new(label, mesh)
        self.collection.objects.link(obj)
        if subdiv:
            mod = obj.modifiers.new('Tailored contour', 'SUBSURF')
            mod.levels = subdiv
            self.bake(obj, mod)
        return self.finish(obj, label, material, parent, True)

    def limb(self, label, start, end, radii, material, parent):
        start, end = Vector(start), Vector(end)
        length = (end - start).length
        profiles = [(0, 0, t * length, radius, radius * 0.88)
                    for t, radius in radii]
        obj = self.loft(label, profiles, material, parent, sides=20)
        world = (end - start).to_track_quat('Z', 'Y').to_matrix().to_4x4()
        world.translation = start
        obj.matrix_world = world
        return obj

    def cylinder(self, label, start, end, radius, material, parent, vertices=24):
        start, end = Vector(start), Vector(end)
        bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius,
                                           depth=(end - start).length,
                                           location=(start + end) * 0.5)
        obj = bpy.context.object
        obj.rotation_euler = (end - start).to_track_quat('Z', 'Y').to_euler()
        mod = obj.modifiers.new('Machined edge', 'BEVEL')
        mod.width = min(radius * 0.15, 0.003)
        mod.segments = 2
        self.bake(obj, mod)
        return self.finish(obj, label, material, parent, True)


def _animate(upper, head, pose):
    samples = [(1, 0.024, 0.0), (20, 0.031, 0.004), (40, 0.024, 0.0),
               (41, 0.0, 0.0), (60, -0.004, 0.002), (80, 0.0, 0.0),
               (81, 0.0, 0.0), (84, -0.045, -0.003),
               (89, 0.011, 0.001), (96, 0.0, 0.0)]
    base_z = upper.location.z
    for frame, pitch, breath in samples:
        upper.rotation_euler.x = pitch
        upper.location.z = base_z + breath
        upper.keyframe_insert(data_path='rotation_euler', frame=frame, group='Posture')
        upper.keyframe_insert(data_path='location', frame=frame, group='Breathing')
        head.rotation_euler.z = 0.012 if frame == 20 else 0.0
        head.keyframe_insert(data_path='rotation_euler', frame=frame, group='Attention')
    upper.animation_data.action.name = upper.name + '_Idle_Aim_Recoil'
    head.animation_data.action.name = head.name + '_Attention'
    upper.location.z = base_z
    upper.rotation_euler.x = {'aim': 0.0, 'idle': 0.024, 'recoil': -0.045}[pose]
    head.rotation_euler.z = 0.0


def build_guard(name, location, facing=0.0, pose='aim'):
    """Return (root Empty, flat list of descendants); facing is radians about Z."""
    if pose not in {'aim', 'idle', 'recoil'}:
        raise ValueError("pose must be 'aim', 'idle', or 'recoil'")
    if len(location) != 3 or not all(math.isfinite(float(v)) for v in location):
        raise ValueError('location must contain three finite coordinates')
    if not math.isfinite(facing):
        raise ValueError('facing must be finite')
    selected = list(bpy.context.selected_objects)
    active = bpy.context.view_layer.objects.active
    b = _Builder(name)
    root = b.root
    hips = b.pivot('Hips', (0, 0.015, 0.91), root)
    upper = b.pivot('UpperBody', (0, 0.01, 1.05), hips)
    head = b.pivot('Head', (0, -0.015, 1.54), upper)
    weapon = b.pivot('Weapon', (0.14, -0.34, 1.43), upper)

    b.loft('Pelvis', [(0, .01, .82, .13, .10), (0, .01, .87, .185, .125),
                     (0, .015, .98, .18, .12), (0, .01, 1.06, .15, .10)],
           'NavyFabric', hips)
    for side, x, y in [('L', -.135, -.065), ('R', .145, .065)]:
        leg = b.pivot(side + '_Leg', (x, y, .92), hips)
        b.loft(side + '_Trouser', [(x, y, .17, .064, .066), (x, y, .21, .075, .075),
               (x, y, .32, .084, .085), (x, y-.018, .45, .079, .081),
               (x, y-.026, .51, .089, .087), (x, y, .59, .10, .096),
               (x*.86, y+.01, .75, .112, .115), (x*.82, y+.01, .91, .111, .12),
               (x*.80, y+.01, .96, .095, .10)], 'NavyFabric', leg)
        b.box(side+'_BootSole', (x, y-.04, .034), (.161, .30, .068), 'Rubber', leg, .023)
        b.box(side+'_BootToe', (x, y-.088, .099), (.153, .21, .104), 'Rubber', leg, .041)
        b.loft(side+'_BootUpper', [(x,y,.055,.073,.104), (x,y,.10,.076,.105),
               (x,y+.01,.17,.067,.077), (x,y+.01,.245,.065,.067),
               (x,y+.01,.25,.061,.065)], 'Rubber', leg)
        b.box(side+'_BootTongue', (x,y-.066,.188), (.071,.022,.105), 'Armor', leg, .012)
        for i in range(4):
            b.box(side+f'_Lace{i}', (x,y-.081,.152+i*.021), (.067,.009,.007), 'Webbing', leg, .002)
        b.box(side+'_KneeBacking', (x,y-.091,.50), (.157,.039,.19), 'Webbing', leg, .026)
        b.box(side+'_KneePad', (x,y-.12,.50), (.137,.047,.146), 'Armor', leg, .025)
        b.box(side+'_KneeInset', (x,y-.148,.515), (.09,.013,.067), 'Rubber', leg, .012)
        for z in (.436,.56):
            b.box(side+'_KneeStrap', (x,y-.001,z), (.163,.18,.024), 'Webbing', leg, .009)
        outer_x = x + (-.07 if side == 'L' else .07)
        b.box(side+'_CargoPocket', (outer_x,y-.034,.723), (.098,.158,.176), 'NavyFabric', leg, .018)
        b.box(side+'_CargoFlap', (outer_x,y-.121,.781), (.10,.02,.048), 'FabricSeams', leg, .008)
        for z in (.295,.615):
            b.box(side+'_FabricFold', (x,y-.074,z), (.12,.017,.014), 'FabricSeams', leg, .006,
                  rotation=(0,.09 if side=='L' else -.09,0))

    b.loft('Torso', [(0,.025,.99,.147,.105), (0,.02,1.05,.16,.115),
           (0,.018,1.17,.18,.12), (0,.008,1.33,.218,.128),
           (0,.006,1.44,.235,.122), (0,.005,1.49,.205,.11),
           (0,.005,1.515,.105,.077)], 'NavyFabric', upper)
    b.box('DutyBelt', (0,.01,1.008), (.36,.27,.065), 'Rubber', hips, .025)
    b.box('BeltBuckle', (0,-.134,1.013), (.068,.024,.049), 'Gunmetal', hips, .008)
    for x in (-.125,.125):
        b.box('BeltKeeper', (x,-.13,1.012), (.021,.02,.065), 'Webbing', hips, .005)
    b.box('FrontPlateCarrier', (0,-.13,1.295), (.365,.11,.355), 'Webbing', upper, .037)
    b.box('FrontArmorPlate', (0,-.19,1.335), (.30,.038,.258), 'Armor', upper, .028)
    b.box('BackPlateCarrier', (0,.137,1.30), (.34,.085,.34), 'Webbing', upper, .035)
    b.box('BackArmorPlate', (0,.186,1.33), (.282,.034,.245), 'Armor', upper, .025)
    for side, sign in [('L',-1),('R',1)]:
        b.box(side+'_ShoulderStrap', (sign*.142,-.015,1.486), (.067,.285,.033), 'Webbing', upper, .012)
        b.box(side+'_StrapBuckle', (sign*.14,-.139,1.459), (.061,.023,.044), 'Gunmetal', upper, .008)
        b.box(side+'_Cummerbund', (sign*.179,.009,1.19), (.042,.266,.136), 'Webbing', upper, .015)
        for z in (1.162,1.204,1.246):
            b.box(side+'_SideWebbing', (sign*.204,.012,z), (.014,.23,.018), 'FabricSeams', upper, .004)
    for i,x in enumerate((-.111,0,.111)):
        b.box(f'MagazinePouch{i}', (x,-.224,1.183), (.093,.087,.162), 'Webbing', upper, .013)
        b.box(f'PouchFlap{i}', (x,-.273,1.235), (.089,.017,.055), 'Armor', upper, .007)
        b.box(f'PouchTab{i}', (x,-.285,1.209), (.022,.009,.036), 'FabricSeams', upper, .003)
    for z in (1.30,1.342):
        for x in (-.099,-.033,.033,.099):
            b.box('MolleLoop', (x,-.214,z), (.052,.012,.017), 'Webbing', upper, .003)
    b.box('CallsignPatch', (-.07,-.217,1.405), (.12,.012,.042), 'Rubber', upper, .005)
    for i,height in enumerate((.019,.027,.019)):
        b.box('PatchMark', (-.103+i*.027,-.225,1.405), (.009,.003,height), 'Identifier', upper, .001)
    b.box('Radio', (-.216,-.033,1.265), (.054,.07,.16), 'Rubber', upper, .01)
    b.cylinder('RadioAntenna', (-.224,-.018,1.336), (-.224,-.018,1.514), .005, 'Rubber', upper, 12)
    b.box('MedicalPouch', (.209,.054,1.045), (.085,.155,.12), 'Webbing', hips, .016)

    # Cloth limbs overlap at elbows and shoulders; gloves close the wrist seams.
    arms = [('R',(.222,.0,1.455),(.375,-.128,1.235),(.164,-.340,1.31)),
            ('L',(-.222,.0,1.455),(-.285,-.285,1.295),(-.027,-.536,1.372))]
    for side, shoulder, elbow, wrist in arms:
        b.ellipsoid(side+'_Deltoid', shoulder, (.102,.108,.112), 'NavyFabric', upper, 24, 12)
        b.limb(side+'_UpperSleeve', shoulder, elbow,
               [(0,.086),(.08,.096),(.36,.091),(.70,.077),(.94,.073),(1,.071)], 'NavyFabric', upper)
        b.ellipsoid(side+'_ElbowJoint', elbow, (.076,.071,.072), 'NavyFabric', upper, 24, 12)
        b.limb(side+'_ForearmSleeve', elbow, wrist,
               [(0,.071),(.12,.075),(.35,.071),(.68,.058),(.92,.048),(1,.048)], 'NavyFabric', upper)
        e = Vector(elbow)
        b.ellipsoid(side+'_ElbowArmor', (e.x+(.045 if side=='R' else -.045),e.y+.02,e.z),
                    (.046,.065,.063), 'Armor', upper, 24, 12)
        w = Vector(wrist)
        direction = (w-e).normalized()
        b.limb(side+'_WristCuff', w-direction*.035, w+direction*.020,
               [(0,.05),(.15,.053),(.85,.053),(1,.05)], 'Rubber', upper)
    b.ellipsoid('R_GlovePalm', (.15,-.378,1.319), (.048,.061,.051), 'Rubber', upper, 24, 12)
    for i in range(3):
        b.box('R_GripFinger', (.139,-.404,1.284+i*.018), (.063,.026,.017), 'Armor', upper, .007)
    b.limb('R_TriggerFinger', (.185,-.374,1.344), (.175,-.442,1.353),
           [(0,.013),(.2,.015),(.8,.013),(1,.009)], 'Rubber', upper)
    b.ellipsoid('L_GlovePalm', (.047,-.568,1.379), (.080,.043,.039), 'Rubber', upper, 24, 12)
    for i in range(4):
        b.box('L_SupportFinger', (.111,-.601+i*.019,1.388), (.066,.016,.047), 'Armor', upper, .007)
    b.ellipsoid('L_Thumb', (.092,-.538,1.419), (.043,.018,.020), 'Rubber', upper, 20, 10)

    b.cylinder('Neck', (0,0,1.48), (0,-.018,1.60), .067, 'Rubber', head)
    b.loft('BalaclavaHead', [(0,-.014,1.535,.048,.054),(0,-.025,1.566,.068,.075),
           (0,-.021,1.62,.082,.084),(0,-.014,1.69,.090,.091),
           (0,-.005,1.755,.088,.087),(0,.003,1.794,.069,.061),
           (0,.003,1.81,.025,.025)], 'NavyFabric', head)
    b.ellipsoid('ExposedBrow', (0,-.093,1.714), (.074,.017,.027), 'Skin', head, 24, 12)
    b.box('GoggleSeal', (0,-.103,1.717), (.186,.044,.073), 'Rubber', head, .025)
    b.box('VisorLens', (0,-.13,1.719), (.165,.02,.048), 'Visor', head, .018)
    b.ellipsoid('NoseBridgeGlimpse', (0,-.111,1.677), (.016,.018,.017), 'Skin', head, 20, 10)
    b.ellipsoid('FaceCover', (0,-.092,1.631), (.070,.036,.050), 'Rubber', head, 24, 12)
    for z in (1.617,1.633,1.649):
        b.box('MaskSeam', (0,-.129,z), (.086,.006,.006), 'FabricSeams', head, .002)
    b.loft('HelmetShell', [(0,.003,1.735,.105,.11),(0,.003,1.75,.110,.115),
           (0,.005,1.79,.106,.111),(0,.009,1.825,.086,.091),
           (0,.012,1.846,.048,.053),(0,.012,1.85,.012,.014)], 'Armor', head, sides=32)
    b.box('HelmetBrow', (0,-.106,1.758), (.189,.052,.026), 'Armor', head, .011)
    b.box('HelmetMount', (0,-.112,1.795), (.044,.022,.053), 'Gunmetal', head, .005)
    for side, sign in [('L',-1),('R',1)]:
        b.box(side+'_HelmetRail', (sign*.108,.015,1.766), (.017,.12,.029), 'Gunmetal', head, .005)
        b.ellipsoid(side+'_EarProtection', (sign*.106,.012,1.679), (.032,.053,.064), 'Rubber', head, 24, 12)
        b.box(side+'_EarCup', (sign*.129,.012,1.680), (.022,.073,.081), 'Armor', head, .009)
        b.limb(side+'_ChinStrap', (sign*.08,-.002,1.716), (sign*.042,-.055,1.565),
               [(0,.012),(.1,.013),(.9,.013),(1,.012)], 'Webbing', head)

    # Compact fictional carbine: stock, receiver, magwell, vented handguard,
    # grip, open trigger guard, rail, optic, barrel, and recessed muzzle.
    x = .14
    b.box('RifleReceiver', (x,-.402,1.434), (.069,.29,.075), 'Gunmetal', weapon, .009)
    b.box('RifleUpper', (x,-.401,1.476), (.058,.30,.025), 'Gunmetal', weapon, .005)
    b.box('RifleStockSpine', (x,-.193,1.449), (.041,.19,.042), 'Gunmetal', weapon, .005)
    b.box('RifleStock', (x,-.122,1.424), (.062,.138,.099), 'Armor', weapon, .016)
    b.box('RifleButtPad', (x,-.045,1.416), (.070,.022,.116), 'Rubber', weapon, .009)
    b.box('RifleCheekRest', (x,-.154,1.471), (.065,.099,.030), 'Rubber', weapon, .01)
    b.box('RiflePistolGrip', (x,-.364,1.321), (.044,.060,.139), 'Rubber', weapon, .012,
          rotation=(math.radians(-16),0,0))
    b.box('RifleMagazineWell', (x,-.486,1.388), (.056,.079,.050), 'Gunmetal', weapon, .006)
    b.box('RifleMagazine', (x,-.487,1.293), (.046,.076,.175), 'Armor', weapon, .012,
          rotation=(math.radians(10),0,0))
    for z in (1.249,1.284,1.319):
        b.box('MagazineRib', (x+.026,-.487,z), (.005,.059,.008), 'Gunmetal', weapon, .002)
    for label, center, size in [('Front', (x,-.442,1.349),(.018,.012,.063)),
                                ('Bottom',(x,-.411,1.321),(.018,.073,.012))]:
        b.box('TriggerGuard'+label,center,size,'Gunmetal',weapon,.004)
    b.box('Trigger', (x,-.405,1.362), (.012,.014,.034), 'Steel', weapon, .004,
          rotation=(.22,0,0))
    b.box('RifleHandguard', (x,-.618,1.436), (.068,.188,.081), 'Armor', weapon, .012)
    for sign in (-1,1):
        for i in range(5):
            b.box('HandguardVent', (x+sign*.035,-.550-i*.032,1.445), (.004,.020,.016), 'Rubber', weapon, .002)
        b.box('HandguardSideRail', (x+sign*.038,-.635,1.414), (.012,.118,.016), 'Gunmetal', weapon, .003)
    for i in range(16):
        b.box('TopRailTooth', (x,-.285-i*.026,1.499), (.072,.014,.012), 'Gunmetal', weapon, .002)
    b.box('EjectionPort', (x+.036,-.408,1.451), (.006,.084,.026), 'Rubber', weapon, .002)
    b.box('Bolt', (x+.040,-.418,1.451), (.004,.060,.010), 'Steel', weapon, .001)
    b.cylinder('Selector', (x+.034,-.326,1.418), (x+.045,-.326,1.418), .010, 'Steel', weapon, 16)
    for y in (-.28,-.495):
        b.cylinder('ReceiverPin', (x+.033,y,1.429), (x+.038,y,1.429), .005, 'Steel', weapon, 12)
    b.box('OpticMount', (x,-.373,1.518), (.055,.065,.03), 'Gunmetal', weapon, .004)
    b.cylinder('OpticHousing', (x,-.330,1.552), (x,-.423,1.552), .031, 'Armor', weapon)
    b.cylinder('OpticGlass', (x,-.424,1.552), (x,-.426,1.552), .024, 'Lens', weapon)
    b.cylinder('Barrel', (x,-.70,1.444), (x,-.892,1.444), .014, 'Gunmetal', weapon)
    b.cylinder('MuzzleBrake', (x,-.868,1.444), (x,-.935,1.444), .022, 'Gunmetal', weapon)
    b.cylinder('MuzzleRecess', (x,-.936,1.444), (x,-.938,1.444), .013, 'Rubber', weapon)
    for y in (-.889,-.912):
        b.box('MuzzleSidePort', (x+.022,y,1.444), (.002,.013,.018), 'Rubber', weapon, .001)
    b.box('FrontSightBase', (x,-.687,1.508), (.038,.025,.020), 'Gunmetal', weapon, .003)
    b.box('FrontSightPost', (x,-.687,1.531), (.009,.012,.033), 'Gunmetal', weapon, .002)

    _animate(upper, head, pose)
    root.location = location
    root.rotation_euler.z = facing
    root['asset_type'] = 'Original tactical guard; transform-rigged prop character'
    root['forward_axis'] = '-Y'
    root['height_m'] = 1.85
    root['animation_ranges'] = 'idle:1-40; aim:41-80; recoil:81-96'
    root['pose'] = pose
    bpy.context.view_layer.update()
    for obj in bpy.context.selected_objects:
        obj.select_set(False)
    for obj in selected:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = active
    return root, b.children


def _preview(output):
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.render.fps = 24
    root, children = build_guard('Sentinel', (0,0,0))
    scene.frame_start, scene.frame_end = 1, 96
    scene.frame_set(41)
    for name, frame in [('IDLE',1),('AIM',41),('RECOIL',81)]:
        scene.timeline_markers.new(name, frame=frame)
    meshes = [obj for obj in children if obj.type == 'MESH']
    triangles = 0
    for obj in meshes:
        obj.data.calc_loop_triangles()
        triangles += len(obj.data.loop_triangles)
    corners = [obj.matrix_world @ Vector(corner) for obj in meshes for corner in obj.bound_box]
    bounds = {'min':[min(v[i] for v in corners) for i in range(3)],
              'max':[max(v[i] for v in corners) for i in range(3)]}
    assert 20000 <= triangles <= 50000, f'Triangle budget: {triangles}'
    assert 1.82 < bounds['max'][2] - bounds['min'][2] <= 1.86, bounds
    assert all(obj.parent is not None for obj in children)
    scene.world.color = (.15,.15,.15)
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value = (.075,.10,.14,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value = .45
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0,0,-.004))
    floor = bpy.context.object
    floor.name = 'Preview_Ground'
    mat = bpy.data.materials.new('Preview_Ground')
    mat.diffuse_color = (.12,.15,.19,1)
    floor.data.materials.append(mat)
    for name, loc, power, size, color in [
        ('Key',(-3,-4,5),650,3,(.80,.88,1)),
        ('Fill',(3,-2,2.8),450,2.5,(1,.87,.71)),
        ('Rim',(1,2.4,3.4),900,2,(.53,.73,1))]:
        data = bpy.data.lights.new('Preview_'+name,'AREA')
        data.energy, data.shape, data.size, data.color = power,'DISK',size,color
        light = bpy.data.objects.new('Preview_'+name,data)
        scene.collection.objects.link(light)
        light.location = loc
        light.rotation_euler = (Vector((0,0,1))-light.location).to_track_quat('-Z','Y').to_euler()
    camera_data = bpy.data.cameras.new('Preview_Camera')
    camera = bpy.data.objects.new('Preview_Camera',camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera_data.type = 'ORTHO'
    camera_data.ortho_scale = 2.40
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 48
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1000
    scene.render.resolution_y = 1200
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'AgX'
    scene.render.image_settings.file_format = 'PNG'
    for label, loc in [('three_quarter',(3,-5,2.65)), ('front',(0,-6,2.1)), ('rear',(-3,5,2.6))]:
        camera.location = loc
        camera.rotation_euler = (Vector((0,-.16,.99))-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath = str(output / ('guard_'+label+'.png'))
        bpy.ops.render.render(write_still=True)
    camera.location = (3,-5,2.65)
    camera.rotation_euler = (Vector((0,-.16,.99))-camera.location).to_track_quat('-Z','Y').to_euler()
    bpy.ops.wm.save_as_mainfile(filepath=str(output/'guard_preview.blend'))
    report = {'blender':bpy.app.version_string,'triangles':triangles,'mesh_objects':len(meshes),
              'descendants':len(children),'bounds_m':bounds,'materials':list(MATERIALS),
              'animation':'transform hierarchy; idle 1-40, aim 41-80, recoil 81-96',
              'caveats':['No armature, skin weights, UV unwrap, LODs, collision, or gameplay AI.',
                         'Rigid hierarchy; animation is subtle presentation motion, not locomotion.',
                         'Fabric bump uses procedural nodes; bake for engine export.']}
    (output/'guard_report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('GUARD_REPORT '+json.dumps(report))


if __name__ == '__main__':
    import sys
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preview', action='store_true')
    parser.add_argument('--output', default=str(Path(__file__).resolve().parents[2]/'Art'/'Guards'))
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    if args.preview:
        _preview(args.output)
