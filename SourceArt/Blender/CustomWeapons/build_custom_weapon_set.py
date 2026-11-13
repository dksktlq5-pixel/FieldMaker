"""Create a set of original, game-ready weapon meshes and matching catalog renders.

All gun axes use +X for muzzle and +Z for up. Units are meters in Blender and are
exported to Unreal centimeters. These are original stylized game assets, not copies
of commercial game meshes. Existing AK-47, AR-70, AR-15, Ghoul, AWP and M249 assets
are intentionally excluded from this set.
"""
import bpy, math, os
from mathutils import Vector

ROOT = os.path.dirname(os.path.abspath(__file__))
OUT_FBX = os.path.join(ROOT, "FBX")
OUT_ICON = os.path.join(ROOT, "Icons")
OUT_BLEND = os.path.join(ROOT, "Blend")
for folder in (OUT_FBX, OUT_ICON, OUT_BLEND): os.makedirs(folder, exist_ok=True)

SCALE = 0.01  # author dimensions in cm, Blender stores meters
MAT = {}

def material(name, color, metallic=0.0, roughness=0.55):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1.0)
    m.use_nodes = True
    bs = m.node_tree.nodes.get("Principled BSDF")
    bs.inputs["Base Color"].default_value = (*color, 1.0)
    bs.inputs["Metallic"].default_value = metallic
    bs.inputs["Roughness"].default_value = roughness
    bs.inputs["Specular IOR Level"].default_value = 0.24 if "Polymer" in name or name=="M_Rubber" else 0.36
    MAT[name] = m
    return m

material("M_Gunmetal", (0.055, 0.073, 0.082), 0.72, 0.29)
material("M_BlackPolymer", (0.025, 0.035, 0.039), 0.12, 0.43)
material("M_Steel", (0.19, 0.22, 0.23), 0.82, 0.24)
material("M_TanPolymer", (0.31, 0.24, 0.15), 0.08, 0.48)
material("M_OlivePolymer", (0.15, 0.18, 0.12), 0.05, 0.52)
material("M_Wood", (0.22, 0.095, 0.035), 0.02, 0.48)
material("M_Rubber", (0.012, 0.017, 0.019), 0.0, 0.8)
material("M_Brass", (0.48, 0.30, 0.075), 0.65, 0.24)
material("M_Green", (0.16, 0.23, 0.12), 0.07, 0.6)
material("M_Orange", (0.7, 0.22, 0.04), 0.06, 0.44)

def _cm(v): return v * SCALE

def cube(name, loc, size, mat="M_Gunmetal", bevel=0.8, rot_y=0.0, rot_x=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=tuple(_cm(v) for v in loc))
    o = bpy.context.object
    o.name = name
    o.dimensions = tuple(_cm(v) for v in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.rotation_euler = (rot_x, rot_y, 0.0)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    if mat: o.data.materials.append(MAT[mat])
    if bevel > 0:
        mod = o.modifiers.new("Machined edge radii", "BEVEL")
        mod.width = _cm(bevel)
        mod.segments = 2
        mod.profile = 0.5
        norm = o.modifiers.new("Weighted hard-surface normals", "WEIGHTED_NORMAL")
        norm.keep_sharp = True
    return o

def cylinder(name, loc, radius, depth, axis="X", mat="M_Steel", vertices=24):
    rot = (0, math.pi/2, 0) if axis == "X" else (math.pi/2, 0, 0) if axis == "Y" else (0, 0, 0)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=_cm(radius), depth=_cm(depth),
        location=tuple(_cm(v) for v in loc), rotation=rot)
    o = bpy.context.object
    o.name = name
    o.data.materials.append(MAT[mat])
    bevel = o.modifiers.new("Edge break", "BEVEL")
    bevel.width = _cm(min(0.18, radius * 0.14))
    bevel.segments = 2
    o.modifiers.new("Weighted normals", "WEIGHTED_NORMAL")
    return o

def sphere(name, loc, scale, mat="M_Steel"):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8,
        location=tuple(_cm(v) for v in loc))
    o=bpy.context.object; o.name=name; o.scale=tuple(_cm(v) for v in scale)
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(MAT[mat]); bpy.ops.object.shade_smooth(); return o

def profile(name, points_xz, depth, mat="M_BlackPolymer", y=0.0, bevel=0.35):
    # Extruded X/Z profile, useful for grips, magazines and tapered stocks.
    n=len(points_xz); half=_cm(depth)*0.5
    verts=[]
    for yy in (-half, half):
        for x,z in points_xz: verts.append((_cm(x), yy+_cm(y), _cm(z)))
    faces=[tuple(range(n)), tuple(range(n,2*n))]
    for i in range(n): faces.append((i,(i+1)%n,(i+1)%n+n,i+n))
    mesh=bpy.data.meshes.new(name+"_Mesh"); mesh.from_pydata(verts,[],faces); mesh.materials.append(MAT[mat]); mesh.update()
    obj=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(obj)
    if bevel:
        b=obj.modifiers.new("Rounded profile edges","BEVEL"); b.width=_cm(bevel); b.segments=2
        obj.modifiers.new("Weighted normals","WEIGHTED_NORMAL")
    return obj

def line(name, points, radius=0.25, mat="M_Steel"):
    curve=bpy.data.curves.new(name,"CURVE"); curve.dimensions="3D"; curve.resolution_u=1
    curve.bevel_depth=_cm(radius); curve.bevel_resolution=2
    spline=curve.splines.new("POLY"); spline.points.add(len(points)-1)
    for p,co in zip(spline.points,points): p.co=(*tuple(_cm(v) for v in co),1)
    ob=bpy.data.objects.new(name,curve); bpy.context.collection.objects.link(ob); ob.data.materials.append(MAT[mat])
    bpy.context.view_layer.objects.active=ob; ob.select_set(True); bpy.ops.object.convert(target="MESH"); ob=bpy.context.object; ob.select_set(False)
    return ob

def picrail(x0,x1,z,y=0.0,step=2.0,mat="M_Steel"):
    cube("Continuous optic rail",((x0+x1)/2,y,z),(x1-x0,2.0,0.75),mat,0.18)
    x=x0+1.0
    while x < x1-0.5:
        cube("Rail cross slot",(x,y,z+0.42),(0.55,2.3,0.22),"M_BlackPolymer",0.06)
        x+=step

def screws(xs, z, y=0.0, mat="M_Steel"):
    for x in xs:
        cylinder("Receiver pin",(x,y,z),0.6,0.35,"Y",mat,12)

def pistol(model, style):
    # Pivot at the web of the firing hand; +X points down the bore.
    slide_mat="M_Steel" if style=="DEAGLE" else "M_Gunmetal"
    frame_mat="M_TanPolymer" if style=="FIVESEVEN" else "M_BlackPolymer"
    cube(model+" lower frame",(1,0,1.7),(12.5,3.3,5.0),frame_mat,1.05)
    cube(model+" slide",(7.0,0,5.2),(15.3,3.0,3.0),slide_mat,0.8)
    cylinder(model+" barrel",(14.2,0,5.3),0.82,5.8,"X","M_Steel")
    cylinder(model+" recessed muzzle",(17.3,0,5.3),0.62,0.18,"X","M_BlackPolymer")
    # Serrations, ejection port, sights and grip panels.
    for x in (2.1,3.2,4.3,10.6,11.7):
        cube(model+" slide serration",(x,0,6.77),(0.35,3.08,1.0),"M_Steel",0.08)
    cube(model+" ejection port",(8.2,-1.57,5.45),(3.1,0.08,1.15),"M_BlackPolymer",0.22)
    profile(model+" grip",[(-3.5,0.0),(1.0,0.0),(2.8,-10.0),(-1.0,-12.0),(-4.6,-6.0)],4.3,frame_mat,bevel=0.55)
    cube(model+" grip side panel",(-1.2,-2.25,-5.1),(5.0,0.28,5.2),"M_Rubber",0.35,rot_y=-0.13)
    cube(model+" grip side panel",(-1.2,2.25,-5.1),(5.0,0.28,5.2),frame_mat,0.35,rot_y=-0.13)
    line(model+" trigger guard",[(2.1,0,1.7),(3.3,0,-2.8),(7.2,0,-2.8),(8.0,0,1.5)],0.48,"M_Steel")
    cube(model+" trigger",(5.4,-0.1,-0.5),(0.6,0.45,2.2),"M_Steel",0.2)
    cube(model+" front sight",(16.2,0,7.05),(1.0,1.1,0.9),"M_BlackPolymer",0.18)
    cube(model+" rear sight",(1.6,0,7.1),(1.0,1.5,0.9),"M_BlackPolymer",0.16)
    if style=="USP-S":
        cube(model+" threaded suppressor",(22.5,0,5.3),(12.0,2.6,2.5),"M_BlackPolymer",0.65)
        for x in (18,20,22,24,26): cylinder(model+" suppressor port",(x,-1.33,5.3),0.2,0.12,"Y","M_Steel",12)
        picrail(0,10,2.6,step=2.0)
    elif style=="GLOCK":
        cube(model+" accessory rail",(5.8,0,0.1),(7.2,2.1,0.55),"M_BlackPolymer",0.15)
    elif style=="P250":
        cube(model+" compact optic plate",(7,-0.05,6.85),(3.2,2.8,0.25),"M_Steel",0.1)
    elif style=="FIVESEVEN":
        cube(model+" long slide accent",(7,0,6.85),(9.0,3.1,0.35),"M_TanPolymer",0.1)
        cylinder(model+" extended mag base",(-1.8,0,-10.8),1.45,4.4,"Z","M_TanPolymer")
    elif style=="DEAGLE":
        cube(model+" heavy slide top rib",(7,0,6.9),(13,3.05,0.52),"M_Steel",0.2)
        cube(model+" muzzle brake",(18.6,0,5.3),(2.4,3.4,3.2),"M_Steel",0.55)
        for x in (-2,0): screws([x],1.7,2.32)
    screws([0,4,9,13],4.2,1.65)

def compact_smg(model, style):
    # Long-gun convention: right-hand grip at x=0, support hand at x=22–30 cm.
    if style=="MAC10":
        cube(model+" sheet receiver",(2,0,4),(24,5.4,8.3),"M_BlackPolymer",1.15)
        cube(model+" receiver cover",(4,0,8.3),(18,5.2,1.0),"M_Steel",0.35)
        cylinder(model+" barrel",(23,0,4.8),1.2,22,"X","M_Steel")
        cube(model+" barrel shroud",(18,0,4.8),(9,5.5,4.3),"M_BlackPolymer",1.0)
        profile(model+" box magazine",[(0,0),(7,0),(7,-16),(2,-18),(-1,-16)],4.0,"M_BlackPolymer")
        cube(model+" pistol grip",(-1,0,-3),(5,3.8,11),"M_Rubber",0.9,rot_y=-0.12)
        line(model+" folding wire stock",[(-10,-1,8),(-24,-1,8),(-28,-1,4),(-28,-1,-1),(-12,-1,-1)],0.7,"M_Steel")
        cube(model+" top sight",(6,0,10),(2.4,1.8,1.7),"M_BlackPolymer",0.25)
        for x in (-5,0,5,10): cube(model+" vent slot",(x,-2.76,4),(2.2,0.12,0.45),"M_Steel",0.08)
        picrail(-6,13,9.0,step=2.1)
    elif style=="UMP45":
        profile(model+" polymer receiver",[(-10,1),(-7,8),(15,8),(22,5),(24,1),(19,-3),(-7,-3)],6.2,"M_BlackPolymer")
        cube(model+" upper receiver",(3,0,8.3),(25,5.8,2.5),"M_Gunmetal",0.7)
        cube(model+" handguard",(22,0,5.0),(18,6.4,6.6),"M_BlackPolymer",1.0)
        cylinder(model+" barrel",(42,0,5.1),1.05,28,"X","M_Steel")
        profile(model+" curved magazine",[(2,0),(10,0),(13,-14),(10,-19),(4,-17)],4.2,"M_BlackPolymer")
        profile(model+" side-fold stock",[(-7,8),(-20,7),(-25,4),(-24,1),(-8,2)],4.2,"M_BlackPolymer")
        cube(model+" pistol grip",(-2,0,-5),(5.5,4.4,12),"M_Rubber",0.85,rot_y=-0.14)
        picrail(-7,18,10.1,step=2.1)
        for x in (16,20,24,28): cube(model+" handguard vent",(x,-3.25,5.3),(1.6,0.12,2.8),"M_Steel",0.3)
    elif style=="MP5SD":
        cube(model+" receiver",(1,0,5.0),(26,5.8,8.1),"M_Gunmetal",1.3)
        cube(model+" upper receiver",(-1,0,9.2),(17,5.0,1.2),"M_Steel",0.3)
        cube(model+" trigger housing",(-1,0,-0.3),(10,5.3,3.4),"M_BlackPolymer",0.6)
        cube(model+" handguard",(21,0,4.0),(17,6.8,7.2),"M_BlackPolymer",1.2)
        cylinder(model+" integral suppressor",(48,0,5.2),2.0,40,"X","M_BlackPolymer",32)
        cylinder(model+" bore",(67.5,0,5.2),0.55,2.0,"X","M_Steel")
        profile(model+" curved magazine",[(3,0),(9,0),(11,-14),(8,-18),(2,-16)],4.0,"M_BlackPolymer")
        profile(model+" retractable stock",[(-10,8),(-22,6),(-28,3),(-27,-1),(-12,1)],4.0,"M_BlackPolymer")
        cube(model+" pistol grip",(-1,0,-5),(5,4.2,12),"M_Rubber",0.8,rot_y=-0.16)
        picrail(-8,17,10.1,step=2.0)
        for x in (17,21,25): cube(model+" suppressor vent",(x,-3.4,4),(2,0.1,1.2),"M_Steel",0.35)
    elif style=="MP7":
        profile(model+" compact receiver",[(-11,0),(-8,9),(13,10),(20,7),(22,2),(14,-2),(-8,-2)],5.6,"M_BlackPolymer")
        cube(model+" upper rail",(2,0,10),(24,2.1,1.1),"M_Steel",0.25)
        cube(model+" forend",(23,0,4),(15,5.6,6),"M_BlackPolymer",1.0)
        cylinder(model+" barrel",(39,0,4.2),0.9,20,"X","M_Steel")
        profile(model+" vertical magazine",[(4,0),(10,0),(9,-13),(6,-16),(2,-14)],4.3,"M_BlackPolymer")
        cube(model+" grip",(-1,0,-4),(5.3,4.1,10),"M_Rubber",0.75,rot_y=-0.1)
        profile(model+" adjustable stock",[(-10,7),(-21,5),(-26,3),(-24,0),(-10,1)],3.7,"M_BlackPolymer")
        cube(model+" front sight",(29,0,8),(1.2,1.6,3.0),"M_Steel",0.2)
        for x in (18,21,24,27): cube(model+" side rail slot",(x,-2.9,4),(0.7,0.15,0.5),"M_Steel",0.08)
    elif style=="P90":
        profile(model+" bullpup receiver",[(-23,-3),(-21,10),(-13,13),(13,11),(23,7),(25,0),(16,-6),(-13,-7)],7.4,"M_TanPolymer")
        cube(model+" receiver top",(-1,0,12.0),(38,6.8,2.0),"M_BlackPolymer",0.8)
        cube(model+" carry handle",(2,0,16.0),(28,3.8,4.0),"M_BlackPolymer",1.2)
        cube(model+" sight channel",(2,0,18.3),(22,1.1,0.45),"M_Steel",0.1)
        cylinder(model+" barrel",(32,0,5.5),1.1,22,"X","M_Steel")
        profile(model+" horizontal translucent magazine",[(-11,0),(18,0),(16,3),(-10,3)],3.8,"M_Steel",y=0)
        for i in range(9): cylinder(model+" visible cartridge",(-9+i*2.7,-1.65,4.0),0.65,3.0,"Y","M_Brass",12)
        profile(model+" pistol grip",[(-1,1),(7,1),(8,-12),(2,-14),(-2,-7)],4.8,"M_BlackPolymer")
        cube(model+" butt pad",(-25,0,1),(3.2,7.5,12),"M_Rubber",1.0)
        picrail(-12,12,13.2,step=2.2)
    picrail(-12,18,10.8,step=2.2) if style in ("UMP45","MP5SD") else None
    screws([-8,-2,5,11,18],5,3.05)

def nova_shotgun():
    n="Nova"
    cube(n+" receiver",(-1,0,3.5),(21,5.6,8.2),"M_Gunmetal",1.0)
    cube(n+" top cover",(0,0,8.0),(15,5.9,1.4),"M_Steel",0.45)
    cylinder(n+" barrel",(36,0,4.8),1.2,57,"X","M_Steel",28)
    cylinder(n+" magazine tube",(32,0,1.5),1.65,48,"X","M_Gunmetal",24)
    cube(n+" pump forend",(22,0,0.5),(17,6.6,6.2),"M_BlackPolymer",1.25)
    for x in (17,20,23,26,29): cube(n+" pump groove",(x,-3.35,0.5),(0.42,0.12,4.1),"M_Steel",0.12)
    profile(n+" stock",[(-9,7),(-25,5),(-30,1),(-29,-4),(-22,-7),(-12,-5)],5.2,"M_Wood")
    cube(n+" rubber butt",(-29,0,0),(2.8,5.6,11),"M_Rubber",0.7)
    profile(n+" grip",[(-6,0),(1,0),(4,-11),(0,-14),(-5,-9)],4.6,"M_Wood")
    cube(n+" shell port",(1,-2.9,3.5),(5.5,0.16,2.0),"M_BlackPolymer",0.3)
    cube(n+" rear sight",(3,0,9.4),(1.4,1.6,1.4),"M_Steel",0.2)
    cylinder(n+" front bead",(60,0,6.4),0.58,1.4,"Z","M_Brass",12)
    for x in (12,14,16,18): cube(n+" shell stop",(x,-3.4,2),(0.65,0.25,2.2),"M_Steel",0.14)
    screws([-8,1,8,15],3,2.92)

def negev_lmg():
    n="Negev"
    cube(n+" receiver",(0,0,7),(31,7.2,11.8),"M_Gunmetal",1.25)
    cube(n+" top feed cover",(1,0,13.2),(25,7.5,1.8),"M_Steel",0.45)
    cube(n+" handguard",(28,0,7),(23,8.0,9.0),"M_BlackPolymer",1.2)
    cylinder(n+" heavy barrel",(60,0,8),1.6,45,"X","M_Steel",28)
    cylinder(n+" flash hider",(83,0,8),2.4,5,"X","M_Gunmetal",24)
    cube(n+" feed tray",(3,0,-1.0),(19,6.7,3.6),"M_Steel",0.65)
    cube(n+" box magazine",(10,-4,-7.5),(19,7.5,12.5),"M_OlivePolymer",1.1)
    profile(n+" pistol grip",[(-8,3),(-1,3),(2,-8),(-2,-13),(-9,-10)],5.0,"M_Rubber")
    profile(n+" buttstock",[(-15,11),(-32,8),(-36,3),(-34,-3),(-25,-4),(-15,0)],5.8,"M_OlivePolymer")
    cube(n+" butt pad",(-34,0,2),(3,6.2,12),"M_Rubber",0.8)
    # Ammo belt guide, carrying handle, sights and folded bipod.
    line(n+" feed belt guide",[(0,-3,14),(5,-4,18),(13,-4,18),(16,-3,14)],0.7,"M_Steel")
    for i in range(10):
        x=2+i*1.25
        cylinder(n+" belt cartridge",(x,-4.8,16.8),0.55,2.2,"Z","M_Brass",12)
    line(n+" bipod left",[(49,-3.8,3),(57,-5,-13),(58,-5,-14)],0.7,"M_Steel")
    line(n+" bipod right",[(49,3.8,3),(57,5,-13),(58,5,-14)],0.7,"M_Steel")
    cube(n+" carry handle",(22,0,17),(19,2.0,1.0),"M_BlackPolymer",0.3)
    cube(n+" rear sight",(8,0,15.1),(1.4,2,2.1),"M_Steel",0.22)
    picrail(-4,22,14.4,step=2.2)
    screws([-11,-5,1,7,13,19,25],7,3.75)

def grenade(model, style):
    # Upright, pin at +Z. Components are separately modeled and joined at export.
    zbody=0
    if style=="HE":
        cylinder(model+" segmented steel body",(0,0,zbody),3.5,8.0,"Z","M_OlivePolymer",20)
        for z in (-3,-1,1,3): cylinder(model+" fragmentation band",(0,0,z),3.58,0.42,"Z","M_Gunmetal",24)
        cube(model+" fuse collar",(0,0,4.3),(5.2,5.2,1.8),"M_Steel",0.3)
        line(model+" spoon",[(0,-2.5,5.1),(0,-3.5,7.4),(2,-3.8,8.1)],0.5,"M_Steel")
        cylinder(model+" pin",(0,-3.7,5.7),0.42,3.8,"Y","M_Steel",12)
        line(model+" pull ring",[(0,-3.8,5.7),(2.5,-3.8,5.7),(3.1,-3.8,4.6),(2.5,-3.8,3.7),(1.4,-3.8,3.7)],0.42,"M_Steel")
    elif style=="SMOKE":
        cylinder(model+" smoke canister",(0,0,0),3.2,10.2,"Z","M_OlivePolymer",24)
        cylinder(model+" top cap",(0,0,5.4),3.32,1.3,"Z","M_Steel",24)
        cylinder(model+" base cap",(0,0,-5.4),3.35,1.2,"Z","M_Gunmetal",24)
        for z in (-3.3,-2.0,2.0,3.3): cylinder(model+" reinforcement rib",(0,0,z),3.35,0.35,"Z","M_BlackPolymer",24)
        line(model+" safety lever",[(0,-2.8,5.3),(0,-3.2,7.3),(2.4,-3.2,7.5)],0.45,"M_Steel")
        line(model+" safety ring",[(0,-3.4,5.6),(2.5,-3.4,5.6),(3.0,-3.4,4.5),(2.4,-3.4,3.6),(1.2,-3.4,3.6)],0.4,"M_Steel")
    elif style=="FIRE":
        cylinder(model+" incendiary can body",(0,0,0),3.0,9.0,"Z","M_TanPolymer",18)
        cylinder(model+" top fuse",(0,0,5.0),2.7,1.5,"Z","M_Steel",20)
        cylinder(model+" bottom collar",(0,0,-5.0),3.15,1.0,"Z","M_Gunmetal",20)
        for z in (-3,0,3): cylinder(model+" grip rib",(0,0,z),3.15,0.48,"Z","M_Orange",20)
        cube(model+" safety block",(0,-2.6,6.0),(2.1,1.0,2.0),"M_Steel",0.3)
        line(model+" pin and ring",[(0,-3.3,6.0),(2.5,-3.3,6.0),(3.1,-3.3,4.9),(2.5,-3.3,4.0),(1.3,-3.3,4.0)],0.42,"M_Steel")
    elif style=="FLASH":
        cylinder(model+" flash body",(0,0,0),2.85,9.4,"Z","M_Steel",24)
        cylinder(model+" top cap",(0,0,5.0),3.0,1.2,"Z","M_Gunmetal",24)
        cylinder(model+" base cap",(0,0,-5.0),3.05,1.0,"Z","M_BlackPolymer",24)
        for z in (-3.2,-1.8,1.8,3.2): cylinder(model+" traction ring",(0,0,z),3.0,0.3,"Z","M_BlackPolymer",24)
        line(model+" spoon",[(0,-2.5,5.4),(0,-3.0,7.4),(2.0,-3.0,7.6)],0.45,"M_Gunmetal")
        line(model+" pin loop",[(0,-3.4,5.3),(2.2,-3.4,5.3),(2.8,-3.4,4.2),(2.2,-3.4,3.3),(1.1,-3.4,3.3)],0.38,"M_Steel")
    elif style=="DECOY":
        cylinder(model+" decoy can",(0,0,0),3.1,10.0,"Z","M_Gunmetal",20)
        cylinder(model+" top cap",(0,0,5.25),3.2,1.2,"Z","M_Steel",20)
        cylinder(model+" base cap",(0,0,-5.25),3.25,1.2,"Z","M_BlackPolymer",20)
        for z in (-3,-1,1,3): cylinder(model+" sound vent band",(0,0,z),3.22,0.36,"Z","M_OlivePolymer",20)
        for a in range(8):
            ang=a*math.tau/8
            cylinder(model+" vent port",(3.16*math.cos(ang),3.16*math.sin(ang),0),0.36,0.15,"Z","M_BlackPolymer",12)
        line(model+" safety ring",[(0,-3.3,5.0),(2.3,-3.3,5.0),(2.8,-3.3,4.0),(2.2,-3.3,3.0),(1.0,-3.3,3.0)],0.38,"M_Steel")
    cylinder(model+" fuse cap",(0,0,6.0),1.5,1.8,"Z","M_Brass",16)

ASSETS = [
 ("Glock18","Pistol","GLOCK"), ("USPS","Pistol","USP-S"), ("P250","Pistol","P250"),
 ("FiveSeven","Pistol","FIVESEVEN"), ("DesertEagle","Pistol","DEAGLE"),
 ("MAC10","SMG","MAC10"), ("UMP45","SMG","UMP45"), ("MP5SD","SMG","MP5SD"),
 ("MP7","SMG","MP7"), ("P90","SMG","P90"), ("Nova","Shotgun","NOVA"),
 ("Negev","MachineGun","NEGEV"), ("HEGrenade","Grenade","HE"),
 ("SmokeGrenade","Grenade","SMOKE"), ("IncendiaryGrenade","Grenade","FIRE"),
 ("Flashbang","Grenade","FLASH"), ("DecoyGrenade","Grenade","DECOY"),
]

def clear_scene():
    bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes,bpy.data.curves,bpy.data.cameras,bpy.data.lights):
        for d in list(datablocks):
            if d.users==0: datablocks.remove(d)

def setup_render(center, length, output):
    scene=bpy.context.scene
    scene.render.engine="BLENDER_EEVEE"
    scene.eevee.taa_render_samples=48
    scene.render.resolution_x=768; scene.render.resolution_y=512; scene.render.resolution_percentage=100
    scene.render.image_settings.file_format="PNG"; scene.render.image_settings.color_mode="RGBA"
    scene.render.film_transparent=True
    scene.world.color=(0.04,0.04,0.04)
    scene.view_settings.view_transform="AgX"
    scene.camera=None
    bpy.ops.object.camera_add(location=(_cm(center[0]-length*0.1),_cm(-length*2.7),_cm(length*1.0)))
    cam=bpy.context.object; cam.name="Catalog_Render_Camera"; cam.data.type="ORTHO"
    cam.data.ortho_scale=_cm(length*1.18)
    target=Vector(tuple(_cm(v) for v in center)); direction=target-cam.location
    cam.rotation_euler=direction.to_track_quat("-Z","Y").to_euler(); scene.camera=cam
    bpy.ops.object.light_add(type="AREA", location=(_cm(center[0]-length*.3),_cm(-length*.8),_cm(length*1.7)))
    key=bpy.context.object; key.name="Key_Softbox"; key.data.energy=24; key.data.shape="DISK"; key.data.size=_cm(length*.95)
    key.rotation_euler=(target-key.location).to_track_quat("-Z","Y").to_euler()
    bpy.ops.object.light_add(type="AREA", location=(_cm(center[0]+length*.45),_cm(length*.55),_cm(length*.6)))
    rim=bpy.context.object; rim.name="Rim_Softbox"; rim.data.energy=42; rim.data.size=_cm(length*.75)
    rim.rotation_euler=(target-rim.location).to_track_quat("-Z","Y").to_euler()
    scene.render.filepath=output

def build_one(item):
    name, category, style=item
    clear_scene()
    scene=bpy.context.scene; scene.unit_settings.system="METRIC"; scene.unit_settings.scale_length=1.0
    if category=="Pistol": pistol(name,style)
    elif category=="SMG": compact_smg(name,style)
    elif style=="NOVA": nova_shotgun()
    elif style=="NEGEV": negev_lmg()
    else: grenade(name,style)
    meshes=[o for o in scene.objects if o.type=="MESH"]
    for mesh in meshes:
        bpy.ops.object.select_all(action="DESELECT"); mesh.select_set(True); bpy.context.view_layer.objects.active=mesh
        for modifier in list(mesh.modifiers): bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes: o.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0]
    if len(meshes)>1: bpy.ops.object.join()
    obj=bpy.context.object; obj.name="SM_FM_"+name+"_Custom"
    # Remove coincident/zero-area edges left where the authored hard-surface parts overlap.
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=0.00001)
    bpy.ops.mesh.dissolve_degenerate(threshold=0.00001)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.mesh.quads_convert_to_tris(quad_method="BEAUTY", ngon_method="BEAUTY")
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.data.validate(verbose=True, clean_customdata=True)
    obj.data.update()
    mins=[min(c[i] for c in obj.bound_box) for i in range(3)]
    maxs=[max(c[i] for c in obj.bound_box) for i in range(3)]
    center=tuple((mins[i]+maxs[i])*50.0 for i in range(3))
    length=max((maxs[i]-mins[i])*100.0 for i in range(3))
    obj["asset_authoring"]="Original procedural hard-surface model authored in Blender"
    obj["weapon_id"]=name; obj["weapon_category"]=category; obj["muzzle_axis"]="+X" if category!="Grenade" else "+Z"
    # Keep a clean pivot at the firing-hand grip for firearms; center of body for grenades.
    icon=os.path.join(OUT_ICON,"T_UIWeapon_"+name+".png")
    setup_render(center,length*1.34,icon); bpy.ops.render.render(write_still=True)
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active=obj; obj.select_set(True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_BLEND,name+".blend"))
    # Restore only the weapon mesh after rendering so cameras/lights never enter the FBX.
    bpy.ops.object.select_all(action="DESELECT"); obj.select_set(True); bpy.context.view_layer.objects.active=obj
    fbx=os.path.join(OUT_FBX,"SM_FM_"+name+"_Custom.fbx")
    bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={"MESH"},apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_NONE",global_scale=1.0,axis_forward="-Y",axis_up="Z",
        add_leaf_bones=False,bake_space_transform=False,embed_textures=False,mesh_smooth_type="FACE",use_mesh_modifiers=True)
    # OBJ is used for Unreal import because its FBX MeshDescription path rejected some
    # Blender meshes after conversion. OBJ coordinates are centimeters in Unreal.
    obj_path=os.path.join(OUT_FBX,"SM_FM_"+name+"_Custom.obj")
    obj.rotation_euler.x=math.radians(-90)
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=False)
    bpy.ops.wm.obj_export(filepath=obj_path,export_selected_objects=True,apply_modifiers=True,export_materials=True,
        export_normals=True,export_uv=True,global_scale=100.0,forward_axis="NEGATIVE_Z",up_axis="Y")
    print("BUILT",name,category,"tris",len(obj.data.polygons),"icon",icon,"fbx",fbx,"obj",obj_path,flush=True)

for asset in ASSETS: build_one(asset)
print("COMPLETE",len(ASSETS),flush=True)
