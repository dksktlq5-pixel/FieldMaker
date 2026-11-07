"""Run in an existing 2-player CombatTest PIE through Unreal Python / VibeUE.
No assets or saved appearance are modified. The client fires into the sky.
"""
import unreal, json, math
from pathlib import Path

def world(i):
    result=json.loads(unreal.PIEActorService.resolve_world('instance:'+str(i)))
    return unreal.find_object(None,result['world_path'])
def pawn(i): return unreal.GameplayStatics.get_player_pawn(world(i),0)
def pc(i): return unreal.GameplayStatics.get_player_controller(world(i),0)
def part(a,n): return next(c for c in a.get_components_by_class(unreal.SceneComponent) if c.get_name()==n)
def call(i,obj,name,args=None):
    result=json.loads(unreal.PIEActorService.call_function('instance:'+str(i),obj.get_path_name(),name,json.dumps(args or {})))
    return result['success']
def remote_client():
    return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world(0),unreal.FMCharacter) if a!=pawn(0) and a.get_editor_property('player_state'))
refs=json.loads(Path(unreal.Paths.project_dir()+'Tools/QA/finger-helper-reference.json').read_text())
records=[]
def pose_stats(a,first_person=True):
    data=a.get_weapon_visual_data();mesh=part(a,'FirstPersonArms' if first_person else 'CharacterMesh0');weapon=part(a,'ViewWeapon' if first_person else 'WorldWeapon');wt=weapon.get_socket_transform('None');out={}
    for side,target,offset in [('r',data.right_palm,unreal.Vector(-6,1.6,0)),('l',data.left_palm,unreal.Vector(6,-1.6,0))]:
        hand=mesh.get_socket_transform('hand_'+side)
        actual=unreal.MathLibrary.transform_location(hand,offset);expected=unreal.MathLibrary.transform_location(wt,target)
        out[side+'_palm_cm']=(actual-expected).length()
        axis=unreal.MathLibrary.transform_direction(hand,unreal.Vector(1,0,0)).to_tuple();fore=(hand.translation-mesh.get_socket_location('lowerarm_'+side)).to_tuple();length=math.sqrt(sum(x*x for x in fore));dot=sum(axis[k]*fore[k]/length for k in range(3))*(-1 if side=='r' else 1)
        out[side+'_bend_deg']=math.degrees(math.acos(max(-1,min(1,dot))))
    errors=[]
    for ref in refs:
        local=unreal.MathLibrary.inverse_transform_location(mesh.get_socket_transform(ref['parent']),mesh.get_socket_location(ref['name']))
        errors.append((local-unreal.Vector(*ref['local'])).length())
    out['helper_cm']=max(errors)
    return out

def select_client(id):
    return call(1,pawn(1),'ServerSelectCatalogWeapon',{'Id':id})
def selected_and_pose(id):
    global before_ammo
    a=pawn(1);b=remote_client();data=a.get_catalog_weapon_data(id);slot=unreal.FMWeaponSlot.SECONDARY if data.category==unreal.FMWeaponCategory.PISTOL else unreal.FMWeaponSlot.PRIMARY
    local=pose_stats(a);remote=pose_stats(b,False);before_ammo=a.get_loaded_ammo()
    record={'id':id,'local':local,'remote':remote,'model':part(a,'ViewWeapon').static_mesh.get_path_name()};records.append(record)
    return str(a.get_weapon_id(slot))==id and str(b.get_weapon_id(slot))==id and part(a,'ViewWeapon').static_mesh==part(b,'WorldWeapon').static_mesh and max(local['r_palm_cm'],local['l_palm_cm'])<.25 and max(remote['r_palm_cm'],remote['l_palm_cm'])<2 and local['helper_cm']<.02 and remote['helper_cm']<.02

def ammo_ok(): return pawn(1).get_loaded_ammo()==before_ammo-1 and remote_client().get_loaded_ammo()==before_ammo-1

def mid_reload(id):
    a=pawn(1);data=a.get_weapon_visual_data();mag=part(a,'ViewMagazine');separate=bool(mag.static_mesh);mesh=part(a,'FirstPersonArms');target=data.magazine_hand if separate else data.reload_palm;space=mag.get_socket_transform('None') if separate else part(a,'ViewWeapon').get_socket_transform('None')
    expected=unreal.MathLibrary.transform_location(space,target);actual=unreal.MathLibrary.transform_location(mesh.get_socket_transform('hand_l'),unreal.Vector(6,-1.6,0));gap=(actual-expected).length();records[-1]['reload_palm_cm']=gap;records[-1]['separate_magazine']=separate
    return a.is_reloading() and gap<2

def reload_done(id):
    d=pawn(1).get_catalog_weapon_data(id);return not pawn(1).is_reloading() and pawn(1).get_loaded_ammo()==d.magazine_size and remote_client().get_loaded_ammo()==d.magazine_size

def ads_ok(id):
    a=pawn(1);expected=a.get_catalog_weapon_data(id).category==unreal.FMWeaponCategory.SNIPER or a.get_weapon_visual_data().scope_overlay
    records[-1]['scope']=a.is_scope_view_active();records[-1]['ads']=pose_stats(a)
    return a.is_aiming() and remote_client().is_aiming() and a.is_scope_view_active()==expected and part(a,'ViewWeapon').is_visible()!=expected and part(a,'FirstPersonArms').is_visible()!=expected

pc(1).set_control_rotation(unreal.Rotator(pitch=78,yaw=90,roll=0))
ids=[str(x) for x in pawn(1).get_catalog_weapon_ids() if str(x)!='Grenade']
assert len(ids)==18

def c(e): return {'action':'python_assert','expression':e,'expected':'True'}
def w(t): return {'action':'wait','seconds':t}
steps=[c("call(1,pawn(1),'ServerSetAiming',{'bNewAiming':False})"),w(.4)]
for id in ids:
    d=pawn(1).get_catalog_weapon_data(id);t=d.reload_time
    steps += [c(f'select_client({id!r})'),w(.75),c(f'selected_and_pose({id!r})'),c("call(1,pawn(1),'ServerUseWeapon')"),w(.3),c('ammo_ok()'),c("call(1,pawn(1),'ServerReload')"),w(t*.43),c(f'mid_reload({id!r})'),w(t*.62+.15),c(f'reload_done({id!r})'),c("call(1,pawn(1),'ServerSetAiming',{'bNewAiming':True})"),w(.65),c(f'ads_ok({id!r})'),c("call(1,pawn(1),'ServerSetAiming',{'bNewAiming':False})"),w(.3)]
final_grip_scenario=json.loads(unreal.WorkflowService.run_scenario(json.dumps({'name':'18 models client pose corrective bones fire reload scope replication','steps':steps,'dependencies':['Source/FieldMaker/FMWeaponPoseMesh.cpp','Source/FieldMaker/FMWeaponPresentation.cpp','Source/FieldMaker/FMWeaponVisualData.h','Source/FieldMaker/FMCharacter.cpp','Source/FieldMaker/FMCombatHUD.cpp','Content/FieldMaker/Data/DT_FMWeapons.uasset','Content/FieldMaker/Data/DT_FMWeaponVisuals.uasset','Tools/QA/verify_weapon_grips.py'],'teardown':{'stop_pie':False}})))
print(final_grip_scenario)

