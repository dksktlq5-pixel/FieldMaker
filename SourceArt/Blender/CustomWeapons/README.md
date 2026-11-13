# FieldMaker custom weapon set

This folder contains 17 newly modeled weapon meshes. The six existing entries from the requested catalog were intentionally excluded: AK-47, AR70, AR-15, Ghoul, AWP, and M249.

## New models

| Category | Models |
|---|---|
| Pistols | Glock-18, USP-S, P250, Five-SeveN, Desert Eagle |
| SMGs | MAC-10, UMP-45, MP5-SD, MP7, P90 |
| Shotguns / heavy | Nova, Negev |
| Throwables | HE grenade, smoke grenade, incendiary grenade, flashbang, decoy grenade |

## Files

- `Blend/`: editable Blender source files, one per model.
- `FBX/`: interchange exports for Unreal reimport.
- `Icons/`: transparent item renders and `WeaponSet_17_Preview.png` contact sheet.
- `build_custom_weapon_set.py`: regenerates the procedural models, exports, and renders with Blender 5.2+.
- `../weapon-visuals-fitted.json`: source data table mapping the weapon IDs to meshes and hand/muzzle offsets.

The models are original, stylized low-poly prototypes made from editable Blender geometry. They are scaled in centimeters for Unreal (X forward, Z up); the source meshes have separate named material slots. These are not scans or exact replicas of commercial game assets. Magazine animation geometry and weapon-specific hand animation clips are not included in this asset pass.

Unreal assets are under `/Game/FieldMaker/Weapons/BlenderModeled`, with item renders in its `Icons` subfolder. The shop and combat HUD look up those renders by weapon ID.
