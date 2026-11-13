import bpy
blend=r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\Blend\USPS.blend'
out=r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\FBX\SM_FM_USPS_TestNative.obj'
bpy.ops.wm.open_mainfile(filepath=blend)
o=next(x for x in bpy.context.scene.objects if x.type=='MESH')
bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.mesh.remove_doubles(threshold=0.00001); bpy.ops.mesh.dissolve_degenerate(threshold=0.00001); bpy.ops.mesh.normals_make_consistent(inside=False); bpy.ops.mesh.quads_convert_to_tris(quad_method='BEAUTY', ngon_method='BEAUTY'); bpy.ops.object.mode_set(mode='OBJECT')
o.data.validate(verbose=True,clean_customdata=True); o.data.update()
bpy.ops.wm.obj_export(filepath=out,export_selected_objects=True,apply_modifiers=True,export_materials=True,export_normals=True,export_uv=True,global_scale=100.0,forward_axis='NEGATIVE_Y',up_axis='Z')
print('wrote',out)


