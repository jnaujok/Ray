# Terrain

The ground around Tranquility Base, from the Lunar Reconnaissance Orbiter Camera's Narrow Angle
Camera digital terrain model of the Apollo 11 site (product `NAC_DTM_APOLLO11_E008N0234`, 2 m
posting, NASA/GSFC/Arizona State University, public domain; see [`../../NOTICE`](../../NOTICE)).
The model itself (118 MB) isn't in the repository; download it from the
[LROC PDS archive](https://pds.lroc.im-ldi.com) (`NAC_DTM_APOLLO11.IMG` under
`RDR/NAC_DTM/APOLLO11/`) to make the files again.

Meshes are in local meters around the LM's position (0.67416° N, 23.47314° E): x east, y up, z
north, the ground under the LM at y = 0, the Moon's curvature included (the ground drops d²/2R:
1.66 m at 2.4 km). The middle 6 m, where the DTM shows the LM itself as a 40 cm bump, is
replaced by the plane fitted to the ground from 6 to 12 m out (2.8° down toward azimuth 70).

| File | What it is |
|------|------------|
| `apollo11_inner.glb` | Out to 300 m at the DTM's 2 m posting (70,661 vertices, 140,128 triangles, smooth normals), regolith material; the ground of `as11_40_5927_test.ray` |
| `apollo11_outer.glb` | From 300 m to 2.4 km, past the horizon (2.2 km away for an eye 1.4 m up), at 16 m (54,643 vertices, 108,112 triangles), for the bake |
| `apollo11_inner_crop.tif` | The inner area's heights as a float32 GeoTIFF in local meters, which `tools/solve_5927_view.py` reads to put the camera 1.03 m above the ground |
| `apollo11_bake.ray` | Both meshes, the sun of AS11-40-5927 (14.96° up at azimuth 90.2°) and a black sky, seen by an equirectangular camera from the photograph's camera station, 1.4 m up, without the LM |
| `apollo11_surroundings.hdr` | That bake, 2048 × 1024, linear, run-length encoded: the `environment` of `as11_40_5927_test.ray`, so the horizon and the far terrain show past the inner mesh and in the LM's metals |

To make them again (with `NAC_DTM_APOLLO11.IMG` downloaded):

```sh
python3 tools/geotiff_terrain.py NAC_DTM_APOLLO11.IMG --center 0.67416 23.47314 --radius 300 \
    --fill-center 6 --curvature --name apollo11_inner \
    --out scenes/terrain/apollo11_inner.glb --crop scenes/terrain/apollo11_inner_crop.tif
python3 tools/geotiff_terrain.py NAC_DTM_APOLLO11.IMG --center 0.67416 23.47314 --radius 2400 \
    --inner 300 --step 8 --fill-center 6 --curvature --name apollo11_outer \
    --out scenes/terrain/apollo11_outer.glb
```

To bake the surroundings again (after changing the sun, the regolith or the meshes in
`tools/make_eagle_scenes.py`, which writes `apollo11_bake.ray`):

```sh
python3 tools/make_eagle_scenes.py
raythrower scenes/terrain/apollo11_bake.ray -o /tmp/bake.raw --hdr scenes/terrain/apollo11_surroundings.hdr
python3 tools/hdr_rle.py scenes/terrain/apollo11_surroundings.hdr
```
