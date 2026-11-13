import bpy
out=r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\SM_FM_WeaponMaterialLibrary.obj'
materials=[('M_Gunmetal',(0.055,0.073,0.082),0.72,0.29),('M_BlackPolymer',(0.025,0.035,0.039),0.12,0.43),('M_Steel',(0.19,0.22,0.23),0.82,0.24),('M_TanPolymer',(0.31,0.24,0.15),0.08,0.48),('M_OlivePolymer',(0.15,0.18,0.12),0.05,0.52),('M_Wood',(0.22,0.095,0.035),0.02,0.48),('M_Rubber',(0.012,0.017,0.019),0.0,0.8),('M_Brass',(0.48,0.30,0.075),0.65,0.24),('M_Green',(0.16,0.23,0.12),0.07,0.6),('M_Orange',(0.7,0.22,0.04),0.06,0.44)]
for name,color,metal,rough in materials:
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*color,1); p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
    bpy.ops.mesh.primitive_cube_add(size=0.10,location=(materials.index((name,color,metal,rough))*0.15,0,0))
    obj=bpy.context.object; obj.name=name+'_MaterialSample'; obj.data.materials.append(m)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.wm.obj_export(filepath=out,export_selected_objects=True,apply_modifiers=True,export_materials=True,export_normals=True,export_uv=True,global_scale=100.0,forward_axis='NEGATIVE_Z',up_axis='Y')
print('wrote',out,len(materials))
