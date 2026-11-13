import bpy, glob, os
files=glob.glob(r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\Blend\*.blend')
results=[]
for f in files:
    bpy.ops.wm.open_mainfile(filepath=f)
    o=next((x for x in bpy.context.scene.objects if x.type=='MESH'),None)
    before=(len(o.data.vertices),len(o.data.polygons)) if o else (0,0)
    changed=o.data.validate(verbose=False,clean_customdata=True) if o else False
    after=(len(o.data.vertices),len(o.data.polygons)) if o else (0,0)
    results.append((os.path.basename(f),before,changed,after))
print(results)
