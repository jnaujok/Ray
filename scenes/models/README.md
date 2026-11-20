# Models

Model files the sample scenes load into meshes (see the `mesh` object's `file` in
[`../README.md`](../README.md)).

| File | What it is |
|------|------------|
| `3DBenchy.stl` | [#3DBenchy](https://www.3dbenchy.com), the 3D-printer test boat by Creative Tools, designed by Daniel Norée: the single-part binary STL from [CreativeTools/3DBenchy](https://github.com/CreativeTools/3DBenchy), 225,706 triangles, 60 × 31 × 48 mm standing on z. Included unmodified. It is dedicated to the public domain under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/), not under this project's Apache License; see [`../../NOTICE`](../../NOTICE). |
| `color_cube.stl` | A puzzle cube, colored stickers with black gaps, as a binary STL colored the VisCAM/SolidView way. |
| `magics_gem.stl` | An octahedron colored the Materialise Magics way: four faces have their own colors, four the model's. |
| `ring.obj` | A torus of quads with texture coordinates and normals, written with negative indices. |

The last three are made by [`tools/make_models.py`](../../tools/make_models.py), which writes them the
same every time.
