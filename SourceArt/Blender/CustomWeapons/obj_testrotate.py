import bpy, math
blend=r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\Blend\USPS.blend'; out=r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\FBX\SM_FM_USPS_TestRotate.obj'
bpy.ops.wm.open_mainfile(filepath=blend); o=next(x for x in bpy.context.scene.objects if x.type=='MESH'); bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.remove_doubles(threshold=.00001);bpy.ops.mesh.dissolve_degenerate(threshold=.00001);bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.mesh.quads_convert_to_tris();bpy.ops.object.mode_set(mode='OBJECT')
o.rotation_euler.x=math.radians(-90); bpy.ops.object.transform_apply(location=False,rotation=True,scale=False)
bpy.ops.wm.obj_export(filepath=out,export_selected_objects=True,apply_modifiers=True,export_materials=True,export_normals=True,export_uv=True,global_scale=100.0,forward_axis='NEGATIVE_Z',up_axis='Y')
