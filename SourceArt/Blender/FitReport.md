# Blender weapon fit pass

| Source mesh | Measured size (X × Y × Z, cm) | Notes |
|---|---:|---|
| SM_FM_AR15 | 88.00 × 8.83 × 30.67 | muzzle +X |
| SM_FM_AR4 | 91.03 × 6.46 × 26.11 | muzzle +X |
| SM_FM_AR70 | 92.05 × 4.82 × 19.65 | merged split render mesh; 0.41 cm bounds delta |
| SM_FM_ASVAL | 95.22 × 6.78 × 22.47 | muzzle +X |
| SM_FM_AWP | 120.00 × 12.97 × 36.72 | muzzle +X |
| SM_FM_G67 | 7.03 × 6.23 × 9.00 | grenade |
| SM_FM_Ghoul | 139.87 × 26.65 × 34.64 | source model length retained |
| SM_FM_KA47 | 87.92 × 6.67 × 19.65 | muzzle +X |
| SM_FM_KA74U | 73.79 × 7.53 × 27.09 | muzzle +X |
| SM_FM_M107 | 132.95 × 9.36 × 23.15 | muzzle +X |
| SM_FM_M249 | 100.73 × 17.00 × 26.40 | muzzle +X |
| SM_FM_M9 | 31.44 × 3.11 × 7.79 | source model length retained |
| SM_FM_MP5 | 68.00 × 6.11 × 22.02 | muzzle +X |
| SM_FM_PolicePistol | 19.96 × 3.34 × 12.48 | muzzle +X |
| SM_FM_Shotgun | 90.02 × 5.06 × 17.73 | muzzle +X |
| SM_FM_SMG11 | 32.42 × 5.22 × 31.61 | source model length retained |

## What changed

- Exported the existing 16 unique weapon static meshes through Blender 5.2.4 LTS.
- Kept source pivots, local axes, dimensions, hand contact data, and magazine/muzzle coordinates intact.
- Removed UCX collision helper meshes from the render exports and joined split render pieces, so each weapon imports as one Static Mesh.
- Reimported to `/Game/FieldMaker/Weapons/BlenderFitted`; original `/Game/FieldMaker/Weapons/Realistic` meshes were not edited or deleted.
- Reassigned original Unreal material slots and updated all 39 weapon visual rows to the fitted mesh set.
- Added Blender empties for the right-hand grip, support-hand grip, and muzzle in the editable review file.

## Verification

- Unreal FBX round-trip bounds match the source exactly for 15 of 16 models; AR70 differs by 0.41 cm due to consolidating its split render pieces.
- All 39 visual table rows resolve to a valid fitted mesh.
- PIE capture checked the primary rifle and secondary pistol after remapping.
- Blender file and source/output FBXs are under `SourceArt/Blender` so Unreal's source-file references remain available after clearing `Saved`.

This is a cleanup and hand-contact-preserving fit pass on the project's real Fab weapon geometry. It does not claim newly sculpted firearm designs or physically measured authentic dimensions; the listed values are the dimensions of the imported source meshes.
