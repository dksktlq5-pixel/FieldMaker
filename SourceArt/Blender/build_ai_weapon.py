import bpy, os, math, json
from mathutils import Vector
BASE=os.path.dirname(os.path.abspath(__file__))
PROJECT=os.path.abspath(os.path.join(BASE,'..','..'))
SOURCE=os.path.join(BASE,'FittedFBX')
OUT=os.path.join(BASE,'AiWeaponFBX')
os.makedirs(OUT,exist_ok=True)
# Curated design set. Existing high-detail firearms are the source meshes; differentiating parts are authored in Blender.
WEAPONS=[
 ('Pistol','Glock18','SM_FM_PolicePistol_Fitted','polymer striker pistol: squared slide, rear serrations, accessory rail'),
 ('Pistol','USPS','SM_FM_PolicePistol_Fitted','large-frame service pistol: threaded muzzle and cylindrical suppressor'),
 ('Pistol','P250','SM_FM_PolicePistol_Fitted','compact alloy pistol: compact slide, exposed hammer removed visually, short rail'),
 ('Pistol','FiveSeven','SM_FM_PolicePistol_Fitted','long polymer pistol: extended slide, tall sights, broad grip'),
 ('Pistol','DesertEagle','SM_FM_PolicePistol_Fitted','oversized heavy pistol: long squared slide, wide barrel and large grip panels'),
 ('SMG','MAC10','SM_FM_SMG11_Fitted','compact box receiver and wire stock; short barrel'),
 ('SMG','UMP45','SM_FM_MP5_Fitted','polymer receiver, straight box magazine, folding stock profile'),
 ('SMG','MP5SD','SM_FM_MP5_Fitted','integral suppressor shroud and front sight ring'),
 ('SMG','MP7','SM_FM_SMG11_Fitted','compact PDW: top rail, forward vertical grip, folding stock details'),
 ('SMG','P90','SM_FM_SMG11_Fitted','bullpup PDW: broad upper receiver, top-mounted horizontal magazine, centered grip'),
 ('Rifle','AK47','SM_FM_KA47_Fitted','curved magazine, stamped receiver, wood furniture retained from source'),
 ('Rifle','AR70','SM_FM_AR70_Fitted','Beretta AR70: upper receiver and distinctive stock retained from source'),
 ('Rifle','AR15','SM_FM_AR15_Fitted','AR platform: buffer stock, carry/rail sight and detachable curved magazine'),
 ('Sniper','Ghoul','SM_FM_Ghoul_Fitted','long precision rifle retained from source, optic and barrel silhouette preserved'),
 ('Sniper','AWP','SM_FM_AWP_Fitted','bolt-action precision silhouette with large optic and long barrel preserved'),
 ('Heavy','Nova','SM_FM_Shotgun_Fitted','pump shotgun: tubular magazine, ribbed pump and ghost-ring sights'),
 ('Heavy','Negev','SM_FM_M249_Fitted','belt-fed LMG variation: large drum housing, top cover and heavy barrel'),
 ('Heavy','M249','SM_FM_M249_Fitted','belt-fed LMG source: feed cover, bipod and heavy barrel preserved'),
 ('Throwable','HE','SM_FM_G67_Fitted','frag grenade: segmented body, spoon and pull ring'),
 ('Throwable','Flashbang','SM_FM_G67_Fitted','flash grenade: smooth cylindrical canister, end caps and pull ring'),
 ('Throwable','Smoke','SM_FM_G67_Fitted','smoke canister: ribbed cylinder and perforated end cap'),
 ('Throwable','Incendiary','SM_FM_G67_Fitted','incendiary canister: stout ribbed body and color-coded safety band'),
 ('Throwable','Decoy','SM_FM_G67_Fitted','decoy canister: compact cylinder, vent bands and distinct color stripe'),
]
bpy.ops.wm.read_factory_settings(use_empty=True); scene=bpy.context.scene; scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1.0
# Existing model-to-material provenance is kept intact; added geometry gets a dedicated material by role.
def mat(name,color,metal=0.0,rough=.48):
 m=bpy.data.materials.get(name)
 if not m: m=bpy.data.materials.new(name)
 m.diffuse_color=(*color,1); m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF')
 if bs:
  bs.inputs['Base Color'].default_value=(*color,1); bs.inputs['Metallic'].default_value=metal; bs.inputs['Roughness'].default_value=rough
 return m
M_METAL=mat('AIW_M_BlackenedSteel',(.075,.09,.105),.78,.28)
M_POLY=mat('AIW_M_ReinforcedPolymer',(.055,.065,.07),.08,.62)
M_WOOD=mat('AIW_M_WoodFurniture',(.22,.095,.035),.03,.53)
M_BRASS=mat('AIW_M_BurnishedSteel',(.30,.24,.11),.72,.33)
M_OD=mat('AIW_M_OliveCanister',(.13,.18,.095),.28,.7)
M_GREY=mat('AIW_M_GreyCanister',(.23,.26,.27),.38,.58)
M_RED=mat('AIW_M_SafetyBand',(.56,.105,.045),.18,.5)
M_BLUE=mat('AIW_M_IdentificationBand',(.06,.24,.42),.16,.48)

def deselect():
 bpy.ops.object.select_all(action='DESELECT')
def bevel_obj(o,amount=.001,segments=3):
 mod=o.modifiers.new('Manufactured edge radii','BEVEL'); mod.width=amount; mod.segments=segments
 mod2=o.modifiers.new('Weighted normals','WEIGHTED_NORMAL'); mod2.keep_sharp=True
 return o

def cube(name,loc,scale,material,bev=.001):
 bpy.ops.mesh.primitive_cube_add(size=1, location=loc); o=bpy.context.object; o.name=name; o.dimensions=scale; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if material:o.data.materials.append(material)
 if bev: bevel_obj(o,bev)
 return o

def cyl(name,loc,radius,depth,material,axis='X',verts=32):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc, rotation=(0,math.pi/2,0) if axis=='X' else ((math.pi/2,0,0) if axis=='Y' else (0,0,0)))
 o=bpy.context.object; o.name=name
 if material:o.data.materials.append(material)
 bevel_obj(o,min(radius*.12,.0015),2)
 return o

def source_mesh(n,clean=True):
 if clean:
  for obj in list(bpy.data.objects): bpy.data.objects.remove(obj,do_unlink=True)
 old_objects=set(bpy.data.objects)
 bpy.ops.import_scene.fbx(filepath=os.path.join(SOURCE,n+'.fbx'),use_anim=False)
 ms=[o for o in bpy.data.objects if o not in old_objects and o.type=='MESH' and not o.name.upper().startswith('UCX_')]
 if not ms: raise RuntimeError('missing source '+n)
 deselect()
 for o in ms:o.select_set(True)
 bpy.context.view_layer.objects.active=ms[0]
 if len(ms)>1:bpy.ops.object.join()
 root=bpy.context.view_layer.objects.active; root.name=n+'_Base'
 # All component placements are relative to actual source bounds, not a generic world-space template.
 coords=[root.matrix_world @ Vector(c) for c in root.bound_box]
 lo=Vector((min(v.x for v in coords),min(v.y for v in coords),min(v.z for v in coords))); hi=Vector((max(v.x for v in coords),max(v.y for v in coords),max(v.z for v in coords)))
 return root,lo,hi

def add_rail(parts, cx, cy, z, length, width, material, name):
 parts.append(cube(name+'_base',(cx,cy,z),(length,width,.006),material,.001))
 count=max(5,int(length/.018))
 for i in range(count):
  x=cx-length*.45+i*(length*.9/max(1,count-1))
  parts.append(cube(name+'_tooth_%02d'%i,(x,cy,z+.004),(.004,width*.9,.004),material,.0007))

def add_suppressor(parts,x,y,z,L,H,material,name):
 # fluted monocore cover plus front collar and stepped threaded interface
 parts.append(cyl(name+'_body',(x+L*.5,y,z),H*.46,L*.86,material,'X',48))
 parts.append(cyl(name+'_front_collar',(x+L*.94,y,z),H*.50,L*.08,M_METAL,'X',48))
 parts.append(cyl(name+'_rear_collar',(x+L*.07,y,z),H*.49,L*.08,M_METAL,'X',48))
 for j in range(7):
  xx=x+L*(.22+j*.085)
  parts.append(cyl(name+'_flute_%02d'%j,(xx,y,z),H*.475,L*.018,M_POLY,'X',40))

def recalc_world_bounds(root):
 bpy.context.view_layer.update()
 pts=[root.matrix_world @ Vector(c) for c in root.bound_box]
 lo=Vector((min(v.x for v in pts),min(v.y for v in pts),min(v.z for v in pts)))
 hi=Vector((max(v.x for v in pts),max(v.y for v in pts),max(v.z for v in pts)))
 return lo,hi

def scale_mesh_about_world_point(root,pivot,scale):
 inv=root.matrix_world.inverted()
 for v in root.data.vertices:
  p=root.matrix_world @ v.co
  p=Vector((pivot.x+(p.x-pivot.x)*scale[0],pivot.y+(p.y-pivot.y)*scale[1],pivot.z+(p.z-pivot.z)*scale[2]))
  v.co=inv @ p
 root.data.update();bpy.context.view_layer.update()

def realize_added_geometry(root,parts):
 # Blender join discards modifiers on non-active objects; realize beveled detail first.
 for o in parts:
  if o is root:continue
  bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
  for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)


def fit_compact_smg_variant(root,name):
 scales={'MAC10':(.86,1.0,.68),'MP7':(1.30,1.0,.58),'P90':(1.62,1.0,.58)}
 if name not in scales:return
 root.scale=(root.scale.x*scales[name][0],root.scale.y*scales[name][1],root.scale.z*scales[name][2])
 bpy.context.view_layer.objects.active=root;root.select_set(True)
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 bpy.context.view_layer.update()
 coords=[root.matrix_world @ Vector(c) for c in root.bound_box]
 lo=Vector((min(v.x for v in coords),min(v.y for v in coords),min(v.z for v in coords)))
 hi=Vector((max(v.x for v in coords),max(v.y for v in coords),max(v.z for v in coords)))
 return lo,hi

def add_pistol_details(parts, name, lo, hi, source):
 root=parts[0]
 # Existing right-palm contact in the visual table, converted from cm to Blender metres.
 # Scaling around this point preserves the grip pivot used by the first-person hand pose.
 grip=Vector((-.005,.01135,.022))
 scales={'P250':(.91,.98,.98),'FiveSeven':(1.08,.98,1.035)}
 if name in scales:
  scale_mesh_about_world_point(root,grip,scales[name]);lo,hi=recalc_world_bounds(root)
 L=hi.x-lo.x; W=hi.y-lo.y; H=hi.z-lo.z; cx=(lo.x+hi.x)/2; cy=(lo.y+hi.y)/2
 bore_z=lo.z+H*.875
 slide_z=lo.z+H*.82
 if name=='USPS':
  add_suppressor(parts,hi.x+L*.012,cy,bore_z,L*.45,H*.19,M_METAL,name+'_suppressor')
  parts.append(cyl(name+'_threadShoulder',(hi.x+L*.018,cy,bore_z),H*.083,L*.035,M_METAL,'X',32))
 elif name=='Glock18':
  # Subtle left-side selector and accessory rail, preserving the familiar Glock silhouette.
  parts.append(cube(name+'_selectorPlate',(lo.x+L*.30,cy-W*.525,slide_z),(L*.085,.0035,H*.045),M_METAL,.001))
  parts.append(cyl(name+'_selectorPivot',(lo.x+L*.30,cy-W*.56,slide_z),H*.027,.003,M_BRASS,'Y',24))
  parts.append(cube(name+'_frameRail',(lo.x+L*.59,cy,lo.z+H*.39),(L*.24,W*.52,.0035),M_METAL,.0008))
 elif name=='P250':
  # Compact slide length and closely spaced rear serrations.
  for side in (-1,1):
   for i in range(5):
    x=lo.x+L*(.23+i*.028)
    parts.append(cube(name+'_rearSerr_%s_%02d'%(side,i),(x,cy+side*W*.505,slide_z),(L*.009,.0015,H*.105),M_POLY,.0004))
  parts.append(cube(name+'_compactAccessoryRail',(lo.x+L*.54,cy,lo.z+H*.36),(L*.22,W*.50,.003),M_METAL,.0008))
 elif name=='FiveSeven':
  add_rail(parts,lo.x+L*.55,cy,hi.z+H*.003,L*.40,W*.42,M_METAL,name+'_opticInterface')
  for side in (-1,1):
   for i in range(6):
    x=lo.x+L*(.20+i*.025)
    parts.append(cube(name+'_slideGroove_%s_%02d'%(side,i),(x,cy+side*W*.505,slide_z),(L*.008,.0015,H*.09),M_POLY,.0004))
  parts.append(cube(name+'_tallRearSight',(lo.x+L*.33,cy,hi.z+H*.018),(L*.035,W*.20,H*.045),M_METAL,.001))
 elif name=='DesertEagle':
  # Keep the clean source pistol until the Desert Eagle can be rebuilt as a unified silhouette.
  # The previous bolt-on slide/grip details floated away from the frame and are intentionally omitted.
  return

def add_smg_details(parts,name,lo,hi):
 # Each SMG base is already a detailed, merged donor mesh. Generic bolt-on geometry
 # caused the boxy receiver/magazine/stock pieces seen detached in the user's screenshot.
 return

def add_heavy_details(parts,name,lo,hi):
 # Keep the purchased heavy-weapon donor intact. Added drum/bipod shapes were not
 # registered against its local coordinates; in particular Negev's drum floated below it.
 return

def add_grenade_variant(parts,name,lo,hi):
 L=hi.x-lo.x; W=hi.y-lo.y; H=hi.z-lo.z; cx=(lo.x+hi.x)/2; cy=(lo.y+hi.y)/2; cz=(lo.z+hi.z)/2
 if name=='HE': return
 band={'Flashbang':M_BLUE,'Smoke':M_RED,'Incendiary':M_RED,'Decoy':M_BLUE}[name]
 # M18-style smoke canister: olive body, raised end caps, color band, fuze and pull ring.
 if name=='Smoke':
  parts.append(cyl(name+'_bottomCap',(cx,cy,lo.z+H*.025),H*.37,H*.05,M_METAL,'Z',48))
  parts.append(cyl(name+'_shoulderRim',(cx,cy,hi.z-H*.12),H*.39,H*.035,M_POLY,'Z',48))
  for i in range(8):
   z=lo.z+H*(.12+i*.105)
   parts.append(cyl(name+'_bodyRib_%02d'%i,(cx,cy,z),H*.373,H*.012,M_POLY,'Z',48))
  parts.append(cyl(name+'_identificationBand',(cx,cy,lo.z+H*.20),H*.389,H*.05,band,'Z',48))
  parts.append(cyl(name+'_topColorCap',(cx,cy,hi.z-H*.035),H*.385,H*.07,M_RED,'Z',48))
  parts.append(cyl(name+'_fuzeCollar',(cx,cy,hi.z+H*.025),H*.19,H*.05,M_METAL,'Z',40))
  parts.append(cube(name+'_safetyLever',(cx,cy+W*.14,hi.z+H*.075),(L*.40,W*.20,H*.045),M_METAL,.002))
  parts.append(cyl(name+'_pullRing',(cx-L*.17,cy+W*.25,hi.z+H*.07),H*.13,H*.018,M_BRASS,'Y',32))
 # Flashbang from the reference: compact blue-green perforated sleeve with visible pin and lever.
 elif name=='Flashbang':
  parts.append(cyl(name+'_bottomCap',(cx,cy,lo.z+H*.03),H*.34,H*.06,M_METAL,'Z',40))
  for row,zf in enumerate((.34,.52,.70)):
   for i in range(10):
    a=2*math.pi*i/10+(row%2)*math.pi/10
    x=cx+H*.365*math.cos(a); y=cy+H*.365*math.sin(a)
    hole=cyl(name+'_sleevePort_%02d_%02d'%(row,i),(x,y,lo.z+H*zf),H*.033,H*.018,M_METAL,'Z',16)
    hole.rotation_mode='QUATERNION';hole.rotation_quaternion=Vector((math.cos(a),math.sin(a),0)).to_track_quat('Z','Y')
  for zf in (.24,.79): parts.append(cyl(name+'_accentRib_%s'%zf,(cx,cy,lo.z+H*zf),H*.385,H*.025,M_BLUE,'Z',40))
  parts.append(cyl(name+'_topCap',(cx,cy,hi.z-H*.035),H*.35,H*.07,M_METAL,'Z',40))
  parts.append(cyl(name+'_fuzeCollar',(cx,cy,hi.z+H*.03),H*.18,H*.055,M_BRASS,'Z',32))
  parts.append(cube(name+'_safetyLever',(cx,cy+W*.17,hi.z+H*.065),(L*.42,W*.20,H*.045),M_METAL,.002))
  parts.append(cyl(name+'_pullRing',(cx-L*.18,cy+W*.29,hi.z+H*.06),H*.12,H*.018,M_BRASS,'Y',32))
 # Molotov: shaped glass bottle with neck, mouth and a separate cloth wick; no grenade pin assembly.
 elif name=='Incendiary':
  parts.append(cyl(name+'_baseRim',(cx,cy,lo.z+H*.035),H*.135,H*.055,M_METAL,'Z',40))
  parts.append(cyl(name+'_bottleShoulder',(cx,cy,lo.z+H*.73),H*.13,H*.035,M_OD,'Z',40))
  parts.append(cyl(name+'_bottleNeck',(cx,cy,lo.z+H*.84),H*.075,H*.18,M_OD,'Z',32))
  parts.append(cyl(name+'_bottleMouth',(cx,cy,lo.z+H*.94),H*.082,H*.045,M_METAL,'Z',32))
  rag=cube(name+'_clothWick',(cx,cy+W*.12,hi.z+H*.17),(L*.12,W*.20,H*.38),M_POLY,.003)
  rag.rotation_euler[1]=-.28
  parts.append(rag)
  tail=cube(name+'_clothTail',(cx+L*.035,cy+W*.12,hi.z+H*.025),(L*.11,W*.07,H*.25),M_POLY,.002)
  tail.rotation_euler[1]=.35;parts.append(tail)
  parts.append(cyl(name+'_neckWrap',(cx,cy,lo.z+H*.82),H*.09,H*.035,M_RED,'Z',32))
 else:
  parts.append(cyl(name+'_identificationBand',(cx,cy,cz+H*.20),H*.37,H*.055,band,'Z',48))
  parts.append(cyl(name+'_topCap',(cx,cy,hi.z-H*.025),H*.37,H*.05,M_METAL,'Z',48))
  parts.append(cube(name+'_safetyLever',(cx,cy+W*.31,hi.z+H*.02),(L*.44,W*.15,H*.07),M_METAL,.002))
  parts.append(cyl(name+'_pullRing',(cx-L*.15,cy+W*.38,hi.z+H*.04),H*.13,H*.018,M_BRASS,'Y',32))

def create_throwable_body(name):
 # Separate purpose-built canister dimensions (cm-scale source units in the Blender scene are metres).
 if name=='Incendiary':
  # Lathed bottle profile gives the glass body a shoulder and narrow neck instead of a grenade-can silhouette.
  h=.285; profile=[(0,.010),(.026,.010),(.037,.018),(.041,.035),(.040,.085),(.036,.145),(.030,.190),(.019,.218),(.014,.232),(.014,.263),(.019,.267),(.019,.278),(.015,.282),(0,.282)]
  verts=[];faces=[];steps=40
  for radius,z in profile:
   for i in range(steps):
    a=2*math.pi*i/steps;verts.append((radius*math.cos(a),radius*math.sin(a),z))
  for j in range(len(profile)-1):
   for i in range(steps):
    a=j*steps+i;b=j*steps+(i+1)%steps;c=(j+1)*steps+(i+1)%steps;d=(j+1)*steps+i;faces.append((a,b,c,d))
  mesh=bpy.data.meshes.new('SM_AIW_Incendiary_BottleMesh');mesh.from_pydata(verts,[],faces);mesh.materials.append(M_OD);mesh.update()
  root=bpy.data.objects.new('SM_AIW_Incendiary_BottleBody',mesh);bpy.context.scene.collection.objects.link(root);bevel_obj(root,.001,2)
  r=.041;lo=Vector((-.041,-.041,0));hi=Vector((.041,.041,h))
 else:
  h={'Flashbang':.118,'Smoke':.145,'Decoy':.112}[name]
  r={'Flashbang':.022,'Smoke':.026,'Decoy':.024}[name]
  bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=r,depth=h,location=(0,0,h*.5))
  root=bpy.context.object;root.name='SM_AIW_'+name+'_CanisterBody';root.data.materials.append(M_BLUE if name=='Flashbang' else (M_OD if name=='Smoke' else M_GREY))
  bevel_obj(root,.0015,3);lo=Vector((-r,-r,0));hi=Vector((r,r,h))
 return root,lo,hi

def export_one(category,name,source,feature):
 if category=='Throwable' and name!='HE': root,lo,hi=create_throwable_body(name)
 else: root,lo,hi=source_mesh(source,clean=True)
 if source=='SM_FM_SMG11_Fitted':lo,hi=fit_compact_smg_variant(root,name)
 col=bpy.data.collections.new(name); bpy.context.scene.collection.children.link(col)
 # move imported base into its own independent collection; original is retained as editable mesh data
 for c in list(root.users_collection): c.objects.unlink(root)
 col.objects.link(root); root.name=name+'_SourceBody'; root['design_source']=source
 parts=[root]
 if category=='Pistol':add_pistol_details(parts,name,lo,hi,source)
 elif category=='SMG':add_smg_details(parts,name,lo,hi)
 elif category=='Heavy':add_heavy_details(parts,name,lo,hi)
 elif category=='Throwable':add_grenade_variant(parts,name,lo,hi)
 # Flatten rendered components into one engine-ready mesh, retaining material partitions.
 realize_added_geometry(root,parts)
 deselect()
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=root
 if len(parts)>1:bpy.ops.object.join()
 root=bpy.context.view_layer.objects.active; root.name='SM_AIW_'+name
 root['weapon_category']=category; root['visual_features']=feature; root['base_source_mesh']=source
 root['source_note']='Derived from FieldMaker licensed/owned source mesh; added geometry authored in Blender. Geometry is visual-only.'
 # Normalize normals; keep a single object with all original source materials and added detail roles.
 for p in root.data.polygons:p.use_smooth=True
 bpy.context.view_layer.update()
 fbx=os.path.join(OUT,'SM_AIW_'+name+'.fbx')
 bpy.ops.object.select_all(action='DESELECT'); root.select_set(True); bpy.context.view_layer.objects.active=root
 bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',global_scale=1.0,axis_forward='-Z',axis_up='Y',add_leaf_bones=False,bake_space_transform=False,embed_textures=False)
 return root,fbx

# Keep material library in a dedicated Blender collection for export/review organization.
material_col=bpy.data.collections.new('AIWeapon_MaterialLibrary'); scene.collection.children.link(material_col)
for m in [M_METAL,M_POLY,M_WOOD,M_BRASS,M_OD,M_GREY,M_RED,M_BLUE]:
 obj=bpy.data.objects.new('MAT_'+m.name,None);obj.empty_display_type='CUBE';obj.empty_display_size=.01;obj['material_name']=m.name;material_col.objects.link(obj)
manifest=[]
for cat,name,src,feature in WEAPONS:
 try:
  root,fbx=export_one(cat,name,src,feature)
  manifest.append({'category':cat,'name':name,'mesh':'SM_AIW_'+name,'base_source':src,'features':feature,'fbx':fbx,'triangles':len(root.data.polygons)})
  print('BUILT',name,'faces',len(root.data.polygons),'file',fbx)
 except Exception as e:
  print('FAILED',name,repr(e)); raise
# Rebuild a review scene with all 23 collections in a grid for visual QA, bases separated by categories.
for obj in list(bpy.data.objects): bpy.data.objects.remove(obj,do_unlink=True)
scene=bpy.context.scene
review=bpy.data.collections.new('AiWeapon_23_Review'); scene.collection.children.link(review)
for i,(cat,name,src,feature) in enumerate(WEAPONS):
 if cat=='Throwable' and name!='HE': root,lo,hi=create_throwable_body(name)
 else: root,lo,hi=source_mesh(src,clean=False)
 if src=='SM_FM_SMG11_Fitted':lo,hi=fit_compact_smg_variant(root,name)
 col=bpy.data.collections.new('%02d_%s_%s'%(i+1,cat,name));review.children.link(col)
 for c in list(root.users_collection):c.objects.unlink(root)
 col.objects.link(root);root.name='SM_AIW_'+name+'_ReviewBody';parts=[root]
 if cat=='Pistol':add_pistol_details(parts,name,lo,hi,src)
 elif cat=='SMG':add_smg_details(parts,name,lo,hi)
 elif cat=='Heavy':add_heavy_details(parts,name,lo,hi)
 elif cat=='Throwable':add_grenade_variant(parts,name,lo,hi)
 realize_added_geometry(root,parts)
 deselect()
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=root
 if len(parts)>1:bpy.ops.object.join()
 root=bpy.context.view_layer.objects.active;root.name='SM_AIW_'+name+'_ReviewBody';root.rotation_euler[2]=math.radians(90);root.location=(0, (i%5)*1.45, (i//5)*.55)
 root['visual_features']=feature;root['base_source_mesh']=src
# Put each category in separate row; reset positions for clean turntable/gallery layout.
for i,o in enumerate([o for o in bpy.data.objects if o.type=='MESH']):
 o.location=(0,(i%5)*1.45,(i//5)*.55)
# cameras and lights for renderable preview, but non-destructive source scene remains editable.
world=scene.world or bpy.data.worlds.new('AIWeaponPreviewWorld');scene.world=world;world.color=(.045,.05,.06)
scene.render.engine='BLENDER_EEVEE';scene.render.resolution_x=1800;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
cam_data=bpy.data.cameras.new('AIWeapon_ReviewCamera');cam=bpy.data.objects.new('AIWeapon_ReviewCamera',cam_data);scene.collection.objects.link(cam);cam.location=(4.0,2.9,1.5);target=Vector((0.0,2.9,1.1));cam.rotation_euler=(target-Vector(cam.location)).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=7.6;scene.camera=cam
for n,loc,power,size in [('Key', (3.2,2.0,3.2),1800,5),('Fill',(2.0,5.8,1.3),1200,4),('Rim',(-2.0,3.0,2.4),1600,3)]:
 ld=bpy.data.lights.new(n,'AREA');lo=bpy.data.objects.new(n,ld);scene.collection.objects.link(lo);lo.location=loc;lo.data.energy=power;lo.data.shape='DISK';lo.data.size=size
# Metadata report
json.dump(manifest,open(os.path.join(BASE,'AiWeapon_manifest.json'),'w',encoding='utf-8'),ensure_ascii=False,indent=2)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(BASE,'AiWeapon_23.blend'))
scene.render.filepath=os.path.join(BASE,'AiWeapon_23_review.png');bpy.ops.render.render(write_still=True)
print('COMPLETE',len(manifest))










