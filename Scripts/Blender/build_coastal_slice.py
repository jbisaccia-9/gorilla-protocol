"""Author the coastal encounter in Blender. Units are metres; +Z is up.

Run: blender -b --python Scripts/Blender/build_coastal_slice.py -- --preview
The .blend is the source deliverable. Renders are art validation, not gameplay.
"""

import argparse
import json
import math
import random
import sys
from pathlib import Path

import bpy
import bmesh
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Art" / "CoastalSlice"
RNG = random.Random(1949)
MATERIALS = {}
COLLECTION = None


def collection(name):
    global COLLECTION
    result = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(result)
    COLLECTION = result
    return result


def register(obj, name, material=None):
    obj.name = name
    for old in list(obj.users_collection):
        old.objects.unlink(obj)
    COLLECTION.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    return obj


def principled(name, color, roughness=0.5, metal=0.0, noise=0.0, emission=0.0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    nt = mat.node_tree
    shader = nt.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1)
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metal
    if emission:
        shader.inputs["Emission Color"].default_value = (*color, 1)
        shader.inputs["Emission Strength"].default_value = emission
    if noise:
        coord = nt.nodes.new("ShaderNodeTexCoord")
        texture = nt.nodes.new("ShaderNodeTexNoise")
        texture.inputs["Scale"].default_value = 6
        texture.inputs["Detail"].default_value = 4
        nt.links.new(coord.outputs["Object"], texture.inputs["Vector"])
        ramp = nt.nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].position = .18
        ramp.color_ramp.elements[0].color = (*(v * .4 for v in color), 1)
        ramp.color_ramp.elements[1].position = .83
        ramp.color_ramp.elements[1].color = (*(min(v * 1.5, 1) for v in color), 1)
        nt.links.new(texture.outputs["Fac"], ramp.inputs[0])
        nt.links.new(ramp.outputs[0], shader.inputs["Base Color"])
        fine = nt.nodes.new("ShaderNodeTexNoise")
        fine.inputs["Scale"].default_value = 95
        fine.inputs["Detail"].default_value = 3
        nt.links.new(coord.outputs["Object"], fine.inputs["Vector"])
        bump = nt.nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = noise
        bump.inputs["Distance"].default_value = .027
        nt.links.new(fine.outputs["Fac"], bump.inputs["Height"])
        nt.links.new(bump.outputs[0], shader.inputs["Normal"])
    MATERIALS[name] = mat
    return mat


def materials():
    principled("GP_WetLimestone", (.27, .285, .27), .42, noise=.6)
    principled("GP_Coping", (.32, .34, .32), .28, noise=.42)
    principled("GP_Mortar", (.095, .11, .105), .85, noise=.7)
    principled("GP_CharcoalSteel", (.055, .068, .073), .3, .8, .15)
    principled("GP_OxidizedBrass", (.20, .12, .04), .34, .8, .2)
    principled("GP_Foliage", (.035, .07, .028), .62, noise=.2)
    principled("GP_FoliageTips", (.065, .105, .035), .58, noise=.2)
    principled("GP_Bark", (.085, .065, .039), .8, noise=.8)
    principled("GP_Soil", (.025, .025, .018), .97, noise=.8)
    principled("GP_Tungsten", (1, .53, .19), .3, emission=5)
    principled("GP_Alarm", (1, .008, .003), .25, emission=5)
    principled("GP_Paper", (.55, .56, .46), .8)
    principled("GP_Screen", (.1, .55, .5), .2, emission=1.3)
    principled("GP_InteriorPlaster", (.38, .33, .24), .8, noise=.1)
    principled("GP_Rock", (.10, .125, .135), .65, noise=.9)
    principled("GP_Foam", (.28, .44, .48), .35)
    mat = principled("GP_Window", (.12, .22, .23), .12, .15)
    mat.node_tree.nodes.get("Principled BSDF").inputs["Transmission Weight"].default_value = .65
    mat = principled("GP_Puddle", (.10, .14, .16), .09, .45)
    mat.node_tree.nodes.get("Principled BSDF").inputs["Coat Weight"].default_value = .7
    mat = principled("GP_Paving", (.12, .15, .16), .34, .13, .38)
    shader = mat.node_tree.nodes.get("Principled BSDF")
    nt = mat.node_tree
    wet = nt.nodes.new("ShaderNodeTexNoise")
    wet.inputs["Scale"].default_value = 2.5
    wet.inputs["Detail"].default_value = 2
    coord = nt.nodes.new("ShaderNodeTexCoord")
    nt.links.new(coord.outputs["Object"], wet.inputs["Vector"])
    ramp = nt.nodes.new("ShaderNodeMapRange")
    ramp.inputs["From Min"].default_value = .28
    ramp.inputs["From Max"].default_value = .66
    ramp.inputs["To Min"].default_value = .09
    ramp.inputs["To Max"].default_value = .5
    nt.links.new(wet.outputs["Fac"], ramp.inputs[0])
    nt.links.new(ramp.outputs[0], shader.inputs["Roughness"])
    shader.inputs["Coat Weight"].default_value = .4


def cube(name, loc, size, mat, bevel=.025):
    # Build at size instead of applying object transforms for each masonry block.
    x, y, z = (v / 2 for v in size)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([(-x,-y,-z),(-x,-y,z),(-x,y,-z),(-x,y,z),
                     (x,-y,-z),(x,-y,z),(x,y,-z),(x,y,z)], [],
                    [(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    COLLECTION.objects.link(obj)
    obj.location = loc
    mesh.materials.append(MATERIALS[mat])
    if bevel:
        mod = obj.modifiers.new("Worn edges", "BEVEL")
        mod.width = bevel
        mod.segments = 2
    return obj


def rod(name, a, b, radius, mat, vertices=12):
    a, b = Vector(a), Vector(b)
    depth = (b-a).length
    verts = [(radius*math.cos(math.tau*i/vertices), radius*math.sin(math.tau*i/vertices), z)
             for z in (-depth/2, depth/2) for i in range(vertices)]
    faces = [tuple(reversed(range(vertices))), tuple(range(vertices, 2*vertices))]
    faces += [(i, (i+1)%vertices, (i+1)%vertices+vertices, i+vertices) for i in range(vertices)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    obj = bpy.data.objects.new(name, mesh)
    COLLECTION.objects.link(obj)
    obj.location = (a+b)/2
    mesh.materials.append(MATERIALS[mat])
    obj.rotation_euler = (b-a).to_track_quat("Z", "Y").to_euler()
    for face in obj.data.polygons:
        face.use_smooth = True
    return obj


def ico(name, center, radius, material, subdivisions=1):
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=radius)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    COLLECTION.objects.link(obj)
    obj.location = center
    mesh.materials.append(MATERIALS[material])
    return obj


def light(name, loc, color, power, size=1, target=None, kind="AREA"):
    data = bpy.data.lights.new(name, kind)
    data.energy = power
    data.color = color
    if kind == "AREA":
        data.shape = "DISK"
        data.size = size
    elif kind == "POINT":
        data.shadow_soft_size = size
    obj = bpy.data.objects.new(name, data)
    COLLECTION.objects.link(obj)
    obj.location = loc
    if target:
        obj.rotation_euler = (Vector(target)-obj.location).to_track_quat("-Z", "Y").to_euler()
    return obj


def masonry(name, loc, size):
    cube(name+" mortar", loc, size, "GP_Mortar")
    x, y, z = loc
    w, d, h = size
    # The visible front and end faces have proper joints and varied stone sizes.
    rows = max(1, round(h/.42))
    for row in range(rows):
        hh = h/rows
        cols = max(1, round(w/1.02))
        for col in range(cols):
            ww = w/cols
            cube(name+" face stone", (x-w/2+ww*(col+.5), y-d/2-.028, z-h/2+hh*(row+.5)),
                 (ww-.025, .10, hh-.025), "GP_WetLimestone", .025)
        if d > .5:
            cols = max(1, round(d/.9))
            for col in range(cols):
                dd = d/cols
                cube(name+" end stone", (x-w/2-.028, y-d/2+dd*(col+.5), z-h/2+hh*(row+.5)),
                     (.1, dd-.025, hh-.025), "GP_WetLimestone", .024)
    cube(name+" coping", (x, y, z+h/2+.05), (w+.12, d+.12, .14), "GP_Coping", .04)


def paving(xmin, xmax, ymin, ymax, z):
    cube("Terrace structure", ((xmin+xmax)/2,(ymin+ymax)/2,z-.2),
         (xmax-xmin,ymax-ymin,.4), "GP_Mortar", .03)
    nx, ny = math.ceil((xmax-xmin)/1.05), math.ceil((ymax-ymin)/1.25)
    dx, dy = (xmax-xmin)/nx, (ymax-ymin)/ny
    for ix in range(nx):
        for iy in range(ny):
            cube("Wet paving slab", (xmin+(ix+.5)*dx, ymin+(iy+.5)*dy,z+.012),
                 (dx-.02,dy-.02,.045), "GP_Paving", .012)
    for _ in range(int(nx*ny*.10)):
        x, y = RNG.uniform(xmin+.5,xmax-.5), RNG.uniform(ymin+.5,ymax-.5)
        verts = [(x,y,z+.040)]
        rx, ry = RNG.uniform(.15,.55), RNG.uniform(.25,.7)
        for i in range(18):
            t=math.tau*i/18
            r=RNG.uniform(.75,1.0)
            verts.append((x+math.cos(t)*rx*r, y+math.sin(t)*ry*r, z+.040))
        faces=[(0,i+1,((i+1)%18)+1) for i in range(18)]
        mesh=bpy.data.meshes.new("Puddle surface")
        mesh.from_pydata(verts,[],faces)
        obj=bpy.data.objects.new("Rain pooling",mesh)
        COLLECTION.objects.link(obj)
        obj.data.materials.append(MATERIALS["GP_Puddle"])


def cypress(loc, height=8):
    x,y,z=loc
    rod("Cypress trunk", (x,y,z),(x,y,z+height*.88),.12,"GP_Bark")
    for i in range(44):
        t=i/44
        r=(math.sin(math.pi*t)**.55)*.55+.09
        angle=RNG.uniform(0,math.tau)
        center=(x+math.cos(angle)*r*.45,y+math.sin(angle)*r*.45,z+height*(.15+.84*t))
        obj=ico("Cypress foliage",center,1,"GP_FoliageTips" if i%5==0 else "GP_Foliage")
        obj.scale=(r,r,height*.12)
        for vertex in obj.data.vertices:
            vertex.co *= RNG.uniform(.75,1.2)
        for face in obj.data.polygons:
            face.use_smooth=True
    for i in range(22):
        a=RNG.uniform(0,math.tau)
        zz=z+RNG.uniform(height*.25,height*.85)
        rod("Cypress branch",(x,y,zz),(x+.5*math.cos(a),y+.5*math.sin(a),zz+.45),.018,"GP_Bark",6)


def shrub(loc, radius=.65):
    x,y,z=loc
    for i in range(30):
        a=RNG.uniform(0,math.tau)
        l=RNG.uniform(.2,radius)
        end=(x+math.cos(a)*l,y+math.sin(a)*l,z+RNG.uniform(.15,.6))
        rod("Rosemary stem", (x,y,z),end,.009,"GP_Bark",5)
        for j in range(3):
            p=Vector((x,y,z)).lerp(Vector(end),.4+j*.22)
            obj=ico("Rosemary leaves",p,.1,"GP_FoliageTips")
            obj.scale=(.5,1.2,.45)
            obj.rotation_euler[2]=a


def planter(loc, size, tree=False):
    x,y,z=loc
    w,d,h=size
    masonry("Limestone planter", (x,y,z+h/2),(w,d,h))
    cube("Planter soil",(x,y,z+h+.12),(w-.22,d-.22,.07),"GP_Soil",0)
    if tree:
        cypress((x,y,z+h),RNG.uniform(7.5,9))
    else:
        for i in range(max(2,round(w/.55))):
            shrub((x-w/2+.3+i*(w-.6)/max(1,round(w/.55)-1),y,z+h+.12))


def sconce(loc, alarm=False):
    x,y,z=loc
    mat="GP_Alarm" if alarm else "GP_Tungsten"
    cube("Wall luminaire plate",(x,y,z),(.28,.10,.52),"GP_CharcoalSteel")
    cube("Protected lamp glass",(x,y-.072,z),(.18,.075,.37),mat,.035)
    for zz in [-.13,0,.13]:
        cube("Luminaire protective bar",(x,y-.12,z+zz),(.24,.035,.02),"GP_CharcoalSteel",.006)
    light("Red alarm spill" if alarm else "Warm wall spill",(x,y-.32,z),
          (1,.012,.003) if alarm else (1,.57,.27),90 if alarm else 85,.15,kind="POINT")


def window_bay(xmin,xmax,y,zmin,zmax):
    w,h=xmax-xmin,zmax-zmin
    cube("Window glazing",((xmin+xmax)/2,y,(zmin+zmax)/2),(w,.018,h),"GP_Window",0)
    for x in [xmin,xmin+w/3,xmin+w*2/3,xmax]:
        cube("Window mullion",(x,y-.02,(zmin+zmax)/2),(.06,.1,h+.10),"GP_CharcoalSteel",.008)
    for z in [zmin,zmin+h*.65,zmax]:
        cube("Window crossbar",((xmin+xmax)/2,y-.03,z),(w,.1,.055),"GP_CharcoalSteel",.008)


def lettering(text,loc,size=.2,material="GP_Paper",rotation=(math.pi/2,0,0)):
    curve=bpy.data.curves.new(text,"FONT")
    curve.body=text
    curve.size=size
    curve.extrude=.001
    curve.align_x="CENTER"
    obj=bpy.data.objects.new(text,curve)
    COLLECTION.objects.link(obj)
    obj.location=loc
    obj.rotation_euler=rotation
    curve.materials.append(MATERIALS[material])
    return obj


def facility():
    collection("01 | Walkable terraces and cover")
    paving(-6,14,-11,6,0)
    paving(-6,14,10,23,2.4)
    # Broad central stair and a low exterior bypass ramp meet the upper landing.
    for i in range(12):
        cube("Stair tread",(0,6+(i+.5)/3,(i+1)*.2-.08),(4,.355,.16),"GP_Coping",.014)
        cube("Stair riser",(0,6+(i+.5)/3,(i+1)*.1-.09),(4,.34,(i+1)*.2),"GP_WetLimestone",.01)
    masonry("Stair cheek left",(-2.2,8,1.1),(.38,4,2.2))
    masonry("Stair cheek right",(2.2,8,1.1),(.38,4,2.2))
    planter((-3.3,.0,0),(3.9,1.25,.92))
    planter((3.5,5.3,0),(2.2,1.05,1.0))
    planter((-4.8,6.0,0),(1.8,1.8,1.15),True)
    planter((-4.6,12,2.4),(1.8,1.8,.7),True)
    planter((3.5,11,2.4),(2.0,1.6,.6))
    masonry("Cliff-edge balustrade",(-6,-2.5,.35),(.38,16.5,.7))
    for yy in [-7,-3,1,5]:
        cube("Drainage grate",(-5.5,yy,.05),(.24,.7,.025),"GP_CharcoalSteel",.003)
        for n in range(8):
            cube("Drain slot",(-5.5,yy-.3+n*.08,.067),(.19,.022,.005),"GP_Mortar",0)

    collection("02 | Intelligence station architecture")
    # Connected stone wings frame the approach; all walls meet the floor.
    masonry("Operations foundation",(7.8,15,1.1),(11,12,2.2))
    masonry("Operations left pier",(3.2,15,4.25),(1.2,10,3.7))
    masonry("Operations right pier",(12.8,15,4.25),(1.2,10,3.7))
    masonry("Operations back wall",(8,20,4.25),(10.5,.5,3.7))
    cube("Operations ceiling",(8,15,6.2),(11.4,11,.28),"GP_Coping",.045)
    cube("Operations interior floor",(8,15,2.45),(10,10,.12),"GP_InteriorPlaster",.02)
    window_bay(3.85,12.15,9.93,3.2,5.95)
    masonry("Front sill",(8,10,2.75),(8.3,.4,.5))
    masonry("Communications tower",(5,18,7.5),(3.2,4,2.3))
    cube("Tower rooftop",(5,18,8.72),(3.45,4.3,.2),"GP_Coping")
    # Walk-in equipment bay to the right of the stairs, warm light behind cover.
    masonry("Arcade end wall",(13.7,1.5,2.25),(.6,10,4.5))
    cube("Arcade canopy",(10.6,2.0,4.55),(6.9,11.8,.32),"GP_Coping",.05)
    for y in [-3,1,5,7]:
        cube("Canopy steel post",(7.6,y,2.2),(.17,.17,4.4),"GP_CharcoalSteel",.018)
        rod("Canopy brace",(7.6,y,3.4),(7.6,y+.8,4.4),.045,"GP_CharcoalSteel")
        cube("Ceiling beam",(10.6,y,4.25),(6.0,.13,.22),"GP_OxidizedBrass",.015)
    masonry("Security partition",(10.8,7.6,1.65),(6.0,.5,3.3))
    cube("Recessed blast door",(10.7,7.28,1.55),(1.65,.18,2.9),"GP_CharcoalSteel",.045)
    for x in [10.25,11.1]:
        cube("Door inset",(x,7.175,1.8),(.65,.018,1.9),"GP_Mortar",.01)
    cube("Access keypad",(11.8,7.25,1.5),(.18,.07,.28),"GP_CharcoalSteel",.012)
    cube("Access status",(11.8,7.205,1.57),(.12,.015,.06),"GP_Screen",0)
    lettering("ARCHIVIO / 04",(10.7,7.26,3.08),.16)
    sconce((9.3,7.25,2.5),True)
    sconce((12.3,7.25,2.5),False)
    sconce((3.25,9.27,4.15),True)
    sconce((12.8,9.2,4.5),False)
    for y in [-1.5,3.8]:
        cube("Pendant housing",(10.4,y,4.06),(.45,.45,.12),"GP_CharcoalSteel")
        cube("Pendant diffuser",(10.4,y,3.985),(.35,.35,.035),"GP_Tungsten",.015)
        light("Canopy practical",(10.4,y,3.85),(1,.57,.26),220,1.0,(10.4,y,0))
    for xx in [5.3,9.6]:
        light("Operations tungsten",(xx,12.4,5.65),(1,.64,.34),280,2.0,(xx,12.5,2.4))
        cube("Operations ceiling strip",(xx,12.4,5.9),(2.1,.22,.03),"GP_Tungsten")
        cube("Analyst desk",(xx,12.3,3.25),(2.3,.85,.12),"GP_OxidizedBrass")
        for lx in [-.85,.85]:
            cube("Desk support",(xx+lx,12.3,2.85),(.05,.6,.8),"GP_CharcoalSteel",.006)
        cube("Monitor bezel",(xx,12.45,3.65),(.65,.09,.45),"GP_CharcoalSteel",.012)
        cube("Monitor display",(xx,12.394,3.65),(.59,.016,.37),"GP_Screen",.002)
        cube("Desk paper",(xx+.75,12.1,3.32),(.25,.3,.005),"GP_Paper",0)
    # Rail on roof provides the horizontal detail visible against the sky.
    for x in range(3,14):
        rod("Roof rail post",(x,9.7,6.35),(x,9.7,7.3),.023,"GP_CharcoalSteel")
    for z in [6.6,6.95,7.3]:
        rod("Roof guard rail",(3,9.7,z),(13,9.7,z),.022,"GP_CharcoalSteel")
    for yy in [10,12,14,16,18,20]:
        rod("Roof side post",(13,yy,6.35),(13,yy,7.3),.022,"GP_CharcoalSteel")
    for z in [6.6,6.95,7.3]:
        rod("Roof side rail",(13,10,z),(13,20,z),.022,"GP_CharcoalSteel")


def dish():
    collection("03 | Communications equipment")
    rod("Dish pedestal",(5,18,8.8),(5,18,9.65),.16,"GP_CharcoalSteel",20)
    center=Vector((5,18,10.25))
    quat=Vector((-.2,-.78,.62)).normalized().to_track_quat("Z","Y")
    verts=[(0,0,0)]
    rings,segments=12,64
    for ring in range(1,rings+1):
        r=1.65*ring/rings
        for s in range(segments):
            t=math.tau*s/segments
            verts.append((r*math.cos(t),r*math.sin(t),.22*r*r))
    faces=[]
    for s in range(segments):
        faces.append((0,1+s,1+(s+1)%segments))
    for ring in range(rings-1):
        for s in range(segments):
            a=1+ring*segments+s
            b=1+ring*segments+(s+1)%segments
            faces.append((a,a+segments,b+segments,b))
    mesh=bpy.data.meshes.new("Parabolic dish")
    mesh.from_pydata(verts,[],faces)
    obj=bpy.data.objects.new("Parabolic microwave antenna",mesh)
    COLLECTION.objects.link(obj)
    obj.location=center
    obj.rotation_euler=quat.to_euler()
    mesh.materials.append(MATERIALS["GP_Coping"])
    solid=obj.modifiers.new("Dish thickness","SOLIDIFY")
    solid.thickness=.025
    for poly in mesh.polygons:
        poly.use_smooth=True
    for i in range(12):
        t=math.tau*i/12
        points=[center+quat@Vector((1.65*j/8*math.cos(t),1.65*j/8*math.sin(t),.22*(1.65*j/8)**2+.012)) for j in range(9)]
        for a,b in zip(points,points[1:]):
            rod("Dish structural rib",a,b,.015,"GP_CharcoalSteel",6)
    focus=center+quat@Vector((0,0,1.2))
    for t in [0,math.tau/3,2*math.tau/3]:
        rod("Antenna feed support",center+quat@Vector((1.5*math.cos(t),1.5*math.sin(t),.50)),focus,.032,"GP_CharcoalSteel")
    rod("Feed receiver",focus,focus+quat@Vector((0,0,.22)),.11,"GP_CharcoalSteel")
    # Lattice mast offshore to the left of the facility.
    for dx,dy in [(-.4,0),(.4,0),(0,.65)]:
        rod("Radio mast upright",(-10+dx,21+dy,-2),(-10+dx,21+dy,11),.05,"GP_CharcoalSteel")
    for n in range(12):
        z=n-1
        rod("Radio mast lattice",(-10-.4,21,z),(-10+.4,21,z+1),.025,"GP_CharcoalSteel",6)
        rod("Radio mast lattice",(-10+.4,21,z),(-10-.4,21,z+1),.025,"GP_CharcoalSteel",6)
    for z in [7,9,10.5]:
        cube("Radio panel antenna",(-10.6,21,z),(.3,.18,.8),"GP_Coping",.02)
    cube("Mast obstruction beacon",(-10,21,10.8),(.18,.18,.25),"GP_Alarm",.03)
    light("Radio beacon",(-10,20.8,10.8),(1,.01,.002),45,.15,kind="POINT")


def landscape():
    collection("04 | Sea cliffs and planting")
    for i in range(24):
        x=RNG.uniform(-11,-7) if i<14 else RNG.uniform(-6,14)
        y=RNG.uniform(-14,30) if i<14 else RNG.uniform(19,30)
        obj=ico("Stratified sea cliff",(x,y,-4.5),1,"GP_Rock",subdivisions=2)
        obj.scale=(RNG.uniform(2,4),RNG.uniform(2,5),RNG.uniform(3,7))
        for v in obj.data.vertices:
            v.co*=RNG.uniform(.83,1.17)
    for x,y in [(14,22),(11,22),(0,23),(-4,25)]:
        cypress((x,y,2.4),RNG.uniform(6,9))
    water=principled("GP_Sea",(.028,.092,.125),.17,.45,.12)
    nt=water.node_tree
    bs=nt.nodes.get("Principled BSDF")
    wave=nt.nodes.new("ShaderNodeTexNoise")
    wave.inputs["Scale"].default_value=1.6
    wave.inputs["Detail"].default_value=5
    coord=nt.nodes.new("ShaderNodeTexCoord")
    nt.links.new(coord.outputs["Object"],wave.inputs["Vector"])
    bump=nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value=.7
    bump.inputs["Distance"].default_value=.18
    nt.links.new(wave.outputs["Fac"],bump.inputs["Height"])
    nt.links.new(bump.outputs[0],bs.inputs["Normal"])
    n=100
    verts=[]
    for i in range(n+1):
        for j in range(n+1):
            x,y=-180+i*2.0,-130+j*3.2
            z=-8+.14*math.sin(x*.8+y*.26)+.06*math.sin(y*1.6+x)
            verts.append((x,y,z))
    faces=[(i*(n+1)+j,i*(n+1)+j+1,(i+1)*(n+1)+j+1,(i+1)*(n+1)+j) for i in range(n) for j in range(n)]
    mesh=bpy.data.meshes.new("Ocean waves")
    mesh.from_pydata(verts,[],faces)
    mesh.materials.append(water)
    obj=bpy.data.objects.new("Tyrrhenian Sea",mesh)
    COLLECTION.objects.link(obj)
    for face in mesh.polygons:
        face.use_smooth=True


def atmosphere():
    collection("05 | Storm lighting")
    world=bpy.data.worlds.new("Storm over the Tyrrhenian")
    bpy.context.scene.world=world
    world.use_nodes=True
    nt=world.node_tree
    bg=nt.nodes.get("Background")
    bg.inputs["Strength"].default_value=.65
    coord=nt.nodes.new("ShaderNodeTexCoord")
    noise=nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value=3.0
    noise.inputs["Detail"].default_value=5
    noise.inputs["Roughness"].default_value=.75
    nt.links.new(coord.outputs["Normal"],noise.inputs["Vector"])
    ramp=nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position=.25
    ramp.color_ramp.elements[0].color=(.006,.014,.028,1)
    ramp.color_ramp.elements[1].position=.77
    ramp.color_ramp.elements[1].color=(.21,.37,.49,1)
    nt.links.new(noise.outputs["Fac"],ramp.inputs[0])
    nt.links.new(ramp.outputs[0],bg.inputs["Color"])
    light("Moon through broken cloud",(-10,-2,18),(.38,.65,1),2800,12,(0,8,0))
    light("Soft coastal sky",(-18,12,12),(.35,.6,.9),1800,15,(4,8,2))
    light("Forecourt blue fill",(-3,-7,7),(.42,.64,.8),400,8,(1,3,1))
    # Local fog behind the playable terrace leaves the weapon and guards readable.
    fog=bpy.data.materials.new("GP_SeaMist")
    fog.use_nodes=True
    nt=fog.node_tree
    nt.nodes.clear()
    output=nt.nodes.new("ShaderNodeOutputMaterial")
    volume=nt.nodes.new("ShaderNodeVolumePrincipled")
    volume.inputs["Density"].default_value=.011
    volume.inputs["Color"].default_value=(.27,.42,.55,1)
    nt.links.new(volume.outputs[0],output.inputs["Volume"])
    MATERIALS["GP_SeaMist"]=fog
    cube("Sea mist bank",(-25,25,-1),(36,40,12),"GP_SeaMist",0)


def camera(name,loc,target,lens=26):
    data=bpy.data.cameras.new(name)
    data.lens=lens
    data.clip_start=.03
    data.clip_end=500
    obj=bpy.data.objects.new(name,data)
    COLLECTION.objects.link(obj)
    obj.location=loc
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat("-Z","Y").to_euler()
    return obj


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--preview",action="store_true")
    parser.add_argument("--environment-only",action="store_true")
    parser.add_argument("--no-render",action="store_true")
    parser.add_argument("--engine",choices=["CYCLES","BLENDER_EEVEE"],default="CYCLES")
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    OUT.mkdir(parents=True,exist_ok=True)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    scene=bpy.context.scene
    scene.unit_settings.system="METRIC"
    scene.unit_settings.scale_length=1.0
    materials()
    print("Building terraces and architecture",flush=True)
    facility()
    print("Building communications equipment",flush=True)
    dish()
    print("Building cliffs, sea and foliage",flush=True)
    landscape()
    atmosphere()
    print("Environment built",flush=True)
    collection("06 | Cameras")
    hero=camera("PLAYER | Lower terrace",(-3,-8,1.85),(2,9,3.15),24)
    camera("ARCHITECTURE | Overview",(-17,-18,12),(4,9,2),32)
    camera("PLAYER | Stair approach",(0,3,1.85),(3,14,4.2),24)
    scene.camera=hero
    sys.path.insert(0,str(Path(__file__).parent))
    if not args.environment_only:
        from bruno import build_viewmodel
        from guards import build_guard
        collection("07 | Bruno first-person model")
        print("Building gorilla hands and weapon",flush=True)
        build_viewmodel(hero)
        collection("08 | Security detail")
        print("Building security characters",flush=True)
        build_guard("Guard Alfa",(-.9,1.8,.05),facing=-.28,pose="aim")
        build_guard("Guard Bravo",(1.0,9.8,2.35),facing=-.1,pose="aim")
    scene.render.engine=args.engine
    scene.render.resolution_x=1120 if args.preview else 1920
    scene.render.resolution_y=630 if args.preview else 1080
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format="PNG"
    scene.render.film_transparent=False
    scene.view_settings.view_transform="AgX"
    scene.view_settings.exposure=.4
    scene.render.fps=30
    scene.frame_start=1
    scene.frame_end=120
    if args.engine=="CYCLES":
        scene.cycles.samples=16 if args.preview else 64
        scene.cycles.use_denoising=True
        scene.cycles.max_bounces=6
        scene.cycles.volume_bounces=0
    scene.frame_set(41)
    # Opening the file immediately presents the art camera in material preview.
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=="VIEW_3D":
                area.spaces.active.region_3d.view_perspective="CAMERA"
                area.spaces.active.shading.type="MATERIAL"
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"Gorilla_Coastal_Encounter.blend"))
    manifest={"status":"Blender art build; gameplay and Unreal integration unverified",
              "units":"meters", "coordinates":"Blender X right, Y forward, Z up",
              "objects":len(scene.objects),"materials":len(bpy.data.materials),
              "camera":"PLAYER | Lower terrace", "map_extent_m":[20,34],
              "routes":["Lower cliff terrace to central stairs","Sheltered arcade to archive door"],
              "required_next":["Bake and export materials","Import into Lyra","Test collision and navigation","Profile packaged gameplay"]}
    (OUT/"build-manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    if not args.no_render:
        scene.render.filepath=str(OUT/("environment-preview.png" if args.environment_only else "encounter-preview.png"))
        bpy.ops.render.render(write_still=True)
    print("COASTAL_ART_BUILD_COMPLETE",flush=True)


if __name__=="__main__":
    main()
