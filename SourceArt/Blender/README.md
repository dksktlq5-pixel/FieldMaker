# FieldMaker weapon fit source (Blender)

- `FieldMaker_WeaponFit.blend`: editable 3D source scene for the weapon fit pass.
- `SourceFBX/`: FBX exports of the original Unreal static meshes; originals in `/Game/FieldMaker/Weapons/Realistic` are preserved.
- `FittedFBX/`: Blender re-exports imported by Unreal as `/Game/FieldMaker/Weapons/BlenderFitted`.
- `fit_weapons.py`: reruns the cleanup/export pass with Blender 5.2+ from this folder.

The pass retains each source mesh's dimensions, +X muzzle axis, and original pivot so the existing per-weapon magazine, muzzle, and hand contact coordinates remain valid. It removes UCX collision helper meshes from the render exports and joins split render pieces into one Static Mesh. Existing Unreal materials are reassigned after import. It does not sculpt a new firearm or author new firearm materials/textures.

All measurements are source dimensions in Unreal centimeters. Examples: AR-15 88 cm, MP5 68 cm, Police pistol 20 cm, AWP 120 cm, AK-47 88 cm, M249 101 cm. These are source-model measurements, not claims that every source model is dimensionally authentic.
