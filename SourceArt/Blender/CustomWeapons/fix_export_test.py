import bpy
blend=r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\Blend\USPS.blend'
out=r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\FBX\SM_FM_USPS_FixTest.fbx'
bpy.ops.wm.open_mainfile(filepath=blend)
o=next(x for x in bpy.context.scene.objects if x.type=='MESH')
bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.mesh.remove_doubles(threshold=0.00001); bpy.ops.mesh.dissolve_degenerate(threshold=0.00001); bpy.ops.mesh.normals_make_consistent(inside=False); bpy.ops.mesh.quads_convert_to_tris(quad_method='BEAUTY', ngon_method='BEAUTY'); bpy.ops.object.mode_set(mode='OBJECT')
o.data.validate(verbose=True, clean_customdata=True); o.data.update()
bpy.ops.export_scene.fbx(filepath=out,use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',global_scale=1.0,axis_forward='-Y',axis_up='Z',add_leaf_bones=False,bake_space_transform=False,embed_textures=False,mesh_smooth_type='FACE',use_mesh_modifiers=True)
print('wrote',out,len(o.data.vertices),len(o.data.polygons))
