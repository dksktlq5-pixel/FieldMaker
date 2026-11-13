import bpy, os, json
base=os.path.dirname(os.path.abspath(__file__)); project=os.path.abspath(os.path.join(base,'..','..')); src=os.path.join(base,'SourceFBX'); out=os.path.join(base,'FittedFBX'); os.makedirs(out,exist_ok=True)
data=json.load(open(os.path.join(project,'Docs','Data','weapon-visuals-current.json'),encoding='utf-8'))
lookup={r['Mesh'].split('/')[-1].split('.')[0].removesuffix('_Fitted'):r for r in data if r.get('Mesh','').startswith('/Game/')}
files=sorted([f for f in os.listdir(src) if f.lower().endswith('.fbx')])
for filename in files:
 bpy.ops.wm.read_factory_settings(use_empty=True); scene=bpy.context.scene; scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1.0
 name=os.path.splitext(filename)[0]; bpy.ops.import_scene.fbx(filepath=os.path.join(src,filename),use_anim=False)
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']; render=[o for o in meshes if not o.name.upper().startswith('UCX_')]
 for o in meshes:
  if o not in render: bpy.data.objects.remove(o,do_unlink=True)
 if not render: print('SKIP',name); continue
 bpy.ops.object.select_all(action='DESELECT')
 for o in render: o.select_set(True)
 bpy.context.view_layer.objects.active=render[0]
 if len(render)>1: bpy.ops.object.join()
 model=bpy.context.view_layer.objects.active; model.name=name+'_Fitted'
 row=lookup.get(name)
 if row:
  for key,label in [('RightPalm','GRIP_RIGHT'),('LeftPalm','GRIP_SUPPORT'),('Muzzle','MUZZLE')]:
   v=row.get(key)
   if v:
    e=bpy.data.objects.new(name+'_'+label,None); e.empty_display_type='SPHERE'; e.empty_display_size=0.012; scene.collection.objects.link(e); e.parent=model; e.location=(v['X'],v['Y'],v['Z'])
 model['source_asset']=name; model['pivot_policy']='Preserved source pivot; grip coordinates remain valid'
 model['coordinate_system']='Unreal local axes: +X muzzle, +Z up; centimeter scale'
 bpy.context.view_layer.update(); dims=tuple(round(float(x),4) for x in model.dimensions)
 bpy.ops.object.select_all(action='DESELECT'); model.select_set(True); bpy.context.view_layer.objects.active=model
 filepath=os.path.join(out,name+'_Fitted.fbx')
 bpy.ops.export_scene.fbx(filepath=filepath,use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',global_scale=1.0,axis_forward='-Z',axis_up='Y',add_leaf_bones=False,bake_space_transform=False)
 print('EXPORTED',name,'pieces',len(render),'dims_m',dims)
# Separate editable Blender workfile with independent weapon collections, marks, and original pivots.
bpy.ops.wm.read_factory_settings(use_empty=True); scene=bpy.context.scene; scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1.0
for filename in files:
 name=os.path.splitext(filename)[0]; bpy.ops.import_scene.fbx(filepath=os.path.join(src,filename),use_anim=False)
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']; render=[o for o in meshes if not o.name.upper().startswith('UCX_')]
 for o in meshes:
  if o not in render: bpy.data.objects.remove(o,do_unlink=True)
 if not render: continue
 coll=bpy.data.collections.new(name); scene.collection.children.link(coll)
 for o in render:
  for c in list(o.users_collection): c.objects.unlink(o)
  coll.objects.link(o)
 row=lookup.get(name)
 if row:
  for key,label in [('RightPalm','GRIP_RIGHT'),('LeftPalm','GRIP_SUPPORT'),('Muzzle','MUZZLE')]:
   v=row.get(key)
   if v:
    e=bpy.data.objects.new(name+'_'+label,None); e.empty_display_type='SPHERE'; e.empty_display_size=0.012; coll.objects.link(e); e.parent=render[0]; e.location=(v['X'],v['Y'],v['Z'])
 coll['source_asset']=name; coll['pivot_policy']='Source pivot preserved; grip data landmarks shown as empties'
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(base,'FieldMaker_WeaponFit.blend'))
print('SAVED_BLEND',os.path.join(base,'FieldMaker_WeaponFit.blend'))

