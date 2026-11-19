# Tools

## `render_previews.py`

Writes a PNG preview of every reference render in `tests/reference/renders.tar.gz` to
`docs/images/tests/`, shrunk to at most 320 pixels wide, for the README's "Expected output"
section. It needs only Python 3. Run it from the repository root whenever the references change:

```sh
tools/render_previews.py
```

## `obj_to_ray.py`

Turns a Wavefront OBJ model into a Ray mesh object (JSON), ready to put in a scene's `objects`
list. It needs only Python 3.

```sh
tools/obj_to_ray.py model.obj [--flip] [--smooth] [--double-sided] [--weld] \
                              [--scale S] [--translate X Y Z] > mesh.json
```

| Option | Meaning |
|--------|---------|
| `--flip` | Reverse each face's corners. Ray takes a face's front to be the side (V2 − V1) × (V3 − V2) points to, the rule OBJ files are made for, so this is only for the occasional model wound the other way. |
| `--smooth` | Shade as a smooth surface (per-vertex normals) rather than flat facets |
| `--double-sided` | Let faces be hit from behind, for open models you can see into |
| `--weld` | Merge vertices at the same position. Many models repeat vertices along the seams between their parts, which leaves visible seams in smooth shading. |
| `--scale`, `--translate` | Move the model into place |

Only vertices and faces are read; polygons are split into triangles. `raythrower --save` tidies the
layout of a scene the mesh has been pasted into. `scenes/teapot.ray` was made with
`--smooth --double-sided --weld`.
