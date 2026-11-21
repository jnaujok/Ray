# Models

Model files the sample scenes load into meshes (see the `mesh` object's `file` in
[`../README.md`](../README.md)), and patch files for `bezier` objects.

| File | What it is |
|------|------------|
| `3DBenchy.stl` | [#3DBenchy](https://www.3dbenchy.com), the 3D-printer test boat by Creative Tools, designed by Daniel Norée: the single-part binary STL from [CreativeTools/3DBenchy](https://github.com/CreativeTools/3DBenchy), 225,706 triangles, 60 × 31 × 48 mm standing on z. Included unmodified. It is dedicated to the public domain under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/), not under this project's Apache License; see [`../../NOTICE`](../../NOTICE). |
| `color_cube.stl` | A puzzle cube, colored stickers with black gaps, as a binary STL colored the VisCAM/SolidView way. |
| `magics_gem.stl` | An octahedron colored the Materialise Magics way: four faces have their own colors, four the model's. |
| `ring.obj` | A torus of quads with texture coordinates and normals, written with negative indices. |
| `teapot.bpt` | The Utah teapot as Martin Newell measured it in 1975: 32 bicubic Bezier patches, z up, the spout along +x, the base at z = 0 and the knob at 3.15, without Jim Blinn's later vertical squash. Newell's data is in the public domain. Written by [`tools/make_teapot_bpt.py`](../../tools/make_teapot_bpt.py) from the copy of the data in three.js (`examples/jsm/geometries/TeapotGeometry.js`, release r160). |

The last three are made by [`tools/make_models.py`](../../tools/make_models.py), which writes them the
same every time.
