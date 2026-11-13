import json, shutil
from pathlib import Path
root=Path(r'D:\UnReal\FieldMaker')
source=root/'SourceArt/Blender/weapon-visuals-fitted.json'
rows=json.loads(source.read_text(encoding='utf-8-sig'))
by={r['Name']:r for r in rows}
map_data={
 'Glock':('Glock18','pistol',17.3,5.3), 'Pistol':('USPS','pistol',28.5,5.3), 'P250':('P250','pistol',17.3,5.3),
 'FiveSeven':('FiveSeven','pistol',17.3,5.3), 'Deagle':('DesertEagle','pistol',19.8,5.3),
 'MAC10':('MAC10','long',34,4.8), 'UMP45':('UMP45','long',56,5.1), 'MP5SD':('MP5SD','long',68.5,5.2),
 'MP7':('MP7','long',49,4.2), 'P90':('P90','long',43,5.5), 'Nova':('Nova','long',64.5,4.8),
 'Negev':('Negev','long',85.5,8.0), 'Grenade':('HEGrenade','grenade',0,0),
 'Smoke':('SmokeGrenade','grenade',0,0), 'Fire':('IncendiaryGrenade','grenade',0,0),
 'Flash':('Flashbang','grenade',0,0), 'Decoy':('DecoyGrenade','grenade',0,0)
}
# Use a model-specific grip root aligned with the generated +X muzzle / +Z up meshes.
grips={
 'Glock':(0,0,-4,8,0,2), 'Pistol':(0,0,-4,8,0,2), 'P250':(0,0,-4,8,0,2), 'FiveSeven':(0,0,-4,8,0,2), 'Deagle':(0,0,-4,8,0,2),
 'MAC10':(-1,0,-3,20,0,4), 'UMP45':(-2,0,-5,26,0,4), 'MP5SD':(-1,0,-5,27,0,4), 'MP7':(-1,0,-4,25,0,4),
 'P90':(2,0,-7,20,0,8), 'Nova':(0,0,-7,25,0,5), 'Negev':(-4,0,-4,26,0,7)
}
for rowname,(model,kind,muzzle_x,muzzle_z) in map_data.items():
    template_name = ('Grenade' if kind=='grenade' else 'Pistol' if kind=='pistol' else ('Nova' if rowname=='Nova' else 'Negev' if rowname=='Negev' else 'MP5SD'))
    template=by.get(template_name) or by['Grenade']
    row=dict(template)
    if rowname not in by:
        row['Name']=rowname
        rows.append(row); by[rowname]=row
    else:
        row=by[rowname]
        row.update(template)
        row['Name']=rowname
    path=f'/Game/FieldMaker/Weapons/BlenderModeled/SM_FM_{model}_Custom.SM_FM_{model}_Custom'
    row['Mesh']=path
    row['MagazineMesh']='None'
    row['AccessoryMesh']='None'
    row['MagazineLocation']={'X':0,'Y':0,'Z':0}
    row['MagazineRotation']={'Pitch':0,'Yaw':0,'Roll':0}
    row['AccessoryLocation']={'X':0,'Y':0,'Z':0}
    row['AccessoryScale']={'X':1,'Y':1,'Z':1}
    row['ModelLabel']=f'NSLOCTEXT("DT_FMWeaponVisuals", "{rowname}_ModelLabel", "Blender · 오리지널 모델")'
    row['bAvailable']=True; row['bRepresentative']=False
    if kind=='grenade':
        row['bUseCustomHands']=False
        row['Muzzle']={'X':0,'Y':0,'Z':0}
        row['ViewOffset']={'X':0,'Y':0,'Z':0}; row['ADSOffset']={'X':0,'Y':0,'Z':0}
        row['ArmOffset']={'X':0,'Y':0,'Z':0}; row['ADSArmOffset']={'X':0,'Y':0,'Z':0}
    else:
        rx,ry,rz,lx,ly,lz=grips[rowname]
        row['bUseCustomHands']=True
        row['RightPalm']={'X':rx,'Y':ry,'Z':rz}
        row['LeftPalm']={'X':lx,'Y':ly,'Z':lz}
        row['Muzzle']={'X':muzzle_x-9,'Y':0,'Z':muzzle_z}
# Save back to the source table JSON, retain a synchronized docs copy.
text=json.dumps(rows,ensure_ascii=False,indent=2)
source.write_text(text+'\n',encoding='utf-8')
(root/'Docs/Data/weapon-visuals-current.json').write_text(text+'\n',encoding='utf-8')
print('updated',len(map_data),'new items; rows total',len(rows))
