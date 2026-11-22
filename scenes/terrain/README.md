# Terrain

The ground around Tranquility Base, from the Lunar Reconnaissance Orbiter Camera's Narrow Angle
Camera digital terrain model of the Apollo 11 site (product `NAC_DTM_APOLLO11_E008N0234`, 2 m
posting, NASA/GSFC/Arizona State University, public domain; see [`../../NOTICE`](../../NOTICE)),
and its color from the orthoimages that come with it (`NAC_DTM_APOLLO11_M150361817_50CM` and
`_2M`, made from NAC frame M150361817). The model (118 MB) and the orthoimages (944 MB and 59 MB)
aren't in the repository; download them from the [LROC PDS archive](https://pds.lroc.im-ldi.com)
(`NAC_DTM_APOLLO11.IMG`, `NAC_DTM_APOLLO11_M150361817_50CM.IMG` and
`NAC_DTM_APOLLO11_M150361817_2M.IMG` under `RDR/NAC_DTM/APOLLO11/`) to make the files again. Only
rows 33300 to 34900 of the 50 cm orthoimage are needed; `tools/geotiff_albedo.py` reads only the
rows it uses, so a file with just those rows in place (the rest zeros) works.

Meshes are in local meters around the LM's position (0.67416° N, 23.47314° E): x east, y up, z
north, the ground under the LM at y = 0, the Moon's curvature included (the ground drops d²/2R:
1.66 m at 2.4 km). The middle 9 m, where the DTM shows the LM itself as a bump, is replaced by the
plane fitted to the ground from 9 to 18 m out (3.2° down toward azimuth 85).

| File | What it is |
|------|------------|
| `apollo11_inner.glb` | Out to 300 m at the DTM's 2 m posting (70,661 vertices, 140,128 triangles, smooth normals, texture coordinates for ±300 m), regolith material; the ground of `as11_40_5927_test.ray` |
| `apollo11_outer.glb` | From 300 m to 2.4 km, past the horizon (2.2 km away for an eye 1.4 m up), at 16 m (54,643 vertices, 108,112 triangles, texture coordinates for ±2.4 km), for the bake |
| `apollo11_inner_albedo.png` | The albedo of the inner square (±300 m, 1024 × 1024, 59 cm a pixel) from the 50 cm orthoimage: 8-bit linear gray, the albedo over its mean, halved (mean 128) |
| `apollo11_outer_albedo.png` | The same for ±2.4 km (2048 × 2048, 2.3 m a pixel) from the 2 m orthoimage, which covers 73% of it; the rest fades to the mean |
| `apollo11_inner_crop.tif` | The inner area's heights as a float32 GeoTIFF in local meters, which `tools/solve_5927_view.py` reads to put the camera 1.03 m above the ground |
| `apollo11_bake.ray` | Both meshes, the sun of AS11-40-5927 (14.96° up at azimuth 90.2°) and a black sky, seen by an equirectangular camera from the photograph's camera station, 1.4 m up, without the LM |
| `apollo11_surroundings.hdr` | That bake, 2048 × 1024, linear, run-length encoded: the `environment` of `as11_40_5927_test.ray`, so the horizon and the far terrain show past the inner mesh and in the LM's metals |

## The albedo textures

An orthoimage is the ground photographed in sunlight, so every slope in it is brighter or darker
than its albedo. `tools/geotiff_albedo.py` takes that out:

- It samples the orthoimage and the DTM on the texture's grid (north up, exactly the square the
  meshes' texture coordinates span).
- It divides by the Lommel-Seeliger shading of the DTM under the orthoimage's own sun. That sun
  was fitted (`--fit-sun`): the shading of the slopes correlates best with the image's detail for
  the sun 23° up in the west (azimuth 270; correlation 0.56, against −0.52 for the east). Pixels
  whose slopes face nearly away from it (shadowed crater walls, and spikes where the DTM's strips
  meet) are filled in from around them.
- It divides by a broad Gaussian blur (80 m for the inner texture, 300 m for the outer), which
  removes what slow brightness changes remain and keeps the mottling: the bright blast zone round
  the LM, ejecta halos, dark patches.
- It paints over the LM and its shadow (within 6 m, the shadow 17 m long to the east) by smoothing
  in the pixels around them; the scene has its own LM there. The outer texture is also destriped
  (a seam between NAC frames runs north-south 1.15 km west of the LM).

In the scenes (`tools/make_eagle_scenes.py`) the regolith's `texture` is a `combine` that
multiplies the albedo image by three noise layers (2 m, 20 cm and 1.5 cm across, darker patches a
little warmer, brighter ones cooler) and by a color that turns the image's mean into an albedo of
0.11 in Mare Tranquillitatis' warm gray (linear 0.135 : 0.125 : 0.112). That mean is set so that
the sunlit ground in front of the LM, which lies in the bright blast zone, measures as the
photograph does at f/11 and 1/250 s. The inner ground's bump adds pebbles 2 to 10 cm across
(`cells`) and fine grain to the gentle undulation; the regolith blends in Oren-Nayar's rough
diffuse term (`orenNayar` 0.9 rad, 35%). The far ground leaves out the finer layers.

## Making them again

With the DTM and orthoimages downloaded:

```sh
python3 tools/geotiff_terrain.py NAC_DTM_APOLLO11.IMG --center 0.67416 23.47314 --radius 300 \
    --fill-center 9 --curvature --name apollo11_inner --uv-extent 300 \
    --out scenes/terrain/apollo11_inner.glb --crop scenes/terrain/apollo11_inner_crop.tif
python3 tools/geotiff_terrain.py NAC_DTM_APOLLO11.IMG --center 0.67416 23.47314 --radius 2400 \
    --inner 300 --step 8 --fill-center 9 --curvature --name apollo11_outer --uv-extent 2400 \
    --out scenes/terrain/apollo11_outer.glb
python3 tools/geotiff_albedo.py NAC_DTM_APOLLO11_M150361817_50CM.IMG --dtm NAC_DTM_APOLLO11.IMG \
    --center 0.67416 23.47314 --extent 300 --size 1024 --sun 270 23 --highpass 80 --mask-lander 6 \
    --out scenes/terrain/apollo11_inner_albedo.png
python3 tools/geotiff_albedo.py NAC_DTM_APOLLO11_M150361817_2M.IMG --dtm NAC_DTM_APOLLO11.IMG \
    --center 0.67416 23.47314 --extent 2400 --size 2048 --sun 270 23 --highpass 300 --mask-lander 6 \
    --destripe --out scenes/terrain/apollo11_outer_albedo.png
```

To bake the surroundings again (after changing the sun, the regolith or the meshes in
`tools/make_eagle_scenes.py`, which writes `apollo11_bake.ray`):

```sh
python3 tools/make_eagle_scenes.py
raythrower scenes/terrain/apollo11_bake.ray -o /tmp/bake.raw --hdr scenes/terrain/apollo11_surroundings.hdr
python3 tools/hdr_rle.py scenes/terrain/apollo11_surroundings.hdr
```
