# Models

Model files the sample scenes load into meshes (see the `mesh` object's `file` in
[`../README.md`](../README.md)), and patch files for `bezier` objects.

| File | What it is |
|------|------------|
| `3DBenchy.stl` | [#3DBenchy](https://www.3dbenchy.com), the 3D-printer test boat by Creative Tools, designed by Daniel Norée: the single-part binary STL from [CreativeTools/3DBenchy](https://github.com/CreativeTools/3DBenchy), 225,706 triangles, 60 × 31 × 48 mm standing on z. Included unmodified. It is dedicated to the public domain under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/), not under this project's Apache License; see [`../../NOTICE`](../../NOTICE). |
| `color_cube.stl` | A puzzle cube, colored stickers with black gaps, as a binary STL colored the VisCAM/SolidView way. |
| `magics_gem.stl` | An octahedron colored the Materialise Magics way: four faces have their own colors, four the model's. |
| `ring.obj` | A torus of quads with texture coordinates and normals, written with negative indices. |
| `chess/chess_set_1k.gltf`, `chess/chess_set.bin` | The [Chess Set](https://polyhaven.com/a/chess_set) by Riley Queen from Poly Haven: a full Staunton set and board as glTF 2.0 (76,920 triangles in all; a pawn is 1,640, a king 4,172), in meters, y up. Included unmodified, without its JPEG textures (which Ray doesn't read; a mesh's `node` picks one piece). Dedicated to the public domain under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/), not under this project's Apache License. |
| `apollo/apollo_lunar_module.glb` | NASA's Apollo Lunar Module from [NASA 3D Resources](https://github.com/nasa/NASA-3D-Resources) (157 primitives, about 100,000 triangles, 12 materials whose base colors come through `faceColors`), free and without copyright. NASA ships it Draco-compressed, which Ray doesn't decode, so this copy was written back out plain by [`tools/gltf_decompress.py`](../../tools/gltf_decompress.py); nothing else changed. |
| `apollo/astronaut.glb` | NASA's Astronaut from the same collection (6,381 triangles; the suit is material `astnt1_1` and the visor `astnt1_2`), decompressed the same way and with its arms lowered from a T-pose by [`tools/pose_astronaut.py`](../../tools/pose_astronaut.py), for the AS11-40-5903 recreation. |
| `teapot.bpt` | The Utah teapot as Martin Newell measured it in 1975: 32 bicubic Bezier patches, z up, the spout along +x, the base at z = 0 and the knob at 3.15, without Jim Blinn's later vertical squash. Newell's data is in the public domain. Written by [`tools/make_teapot_bpt.py`](../../tools/make_teapot_bpt.py) from the copy of the data in three.js (`examples/jsm/geometries/TeapotGeometry.js`, release r160). |

The last three are made by [`tools/make_models.py`](../../tools/make_models.py), which writes them the
same every time.
