# Sample scenes

The `.ray` files here are the test scenes from the original development in 1996–1997.
Every scene that can be read is rendered by the regression tests and compared with the
reference renders in [`tests/reference/`](../tests/reference).

| Scene | Format | Notes |
|-------|--------|-------|
| `axis_test.ray` | current | Six colored spheres along the axes |
| `inside_sphere.ray` | current | Camera inside a sphere |
| `mat_test.ray` | current | Material test |
| `reflect_me.ray`, `reflect_test2.ray`, `reflect_test3.ray`, `reflection_test.ray` | current | Reflection tests |
| `scan_test.ray` | current | Reflective and transparent spheres on a plane |
| `ser_test.ray`, `ser_test2.ray` | current | Serialization tests (`ser_test2` renders blank: nothing is in view) |
| `spec_test.ray` | current | Specular highlights |
| `test.ray` | current | |
| `triangle.ray`, `triangle2.ray` | current | Triangle primitives |
| `xxx.ray` | version 1 | Read with the legacy number format |
| `huh.ray` | transitional | Version-2 header with legacy numbers; renders blank |
| `yyy.ray` | **unreadable** | Saved by an in-between build that wrote an extra, undefined field |
| `stream_test1.ray` | **unreadable** | An early stream test: debug-heap fill bytes, not a scene |

## Format history

- **Version 1** (before April 1997) stored every floating-point value as two 4-byte integers,
  a whole part and a fraction of 2³¹, and read objects until the end of the file.
- **Version 2** stores floating-point values as `%.18e` text and writes an object count.
  For a few weeks files had the version-2 header but still used the old numbers;
  `CScene::Load` tries both.

`renders/` holds output files from 1997. `scan_test.raw` there is a partial trace from
that time (a few scan lines over black), not a reference image.
