# Ray release gallery

Every release, rendered: what each one changed in the pictures Ray makes, from the 1997
code to today. This page is written by `tools/make_gallery_md.py` from
[`index.html`](index.html), the same gallery as an interactive page with a slider that
compares any two versions of the torture test (open it in a browser, or through GitHub
Pages).

## The torture test, version by version

| Version | Image | What changed |
|---|---|---|
| alpha 0.4 | <img src="images/a0.4-torture.jpg" width="320" alt="Torture test at 0.4"> | First torture test: every shape, mirrors, glass and textures through a real lens. |
| alpha 0.4.1 | <img src="images/a0.4.1-torture.jpg" width="320" alt="Torture test at 0.4.1"> | Blocked lens rays are cast again: the lens grain is gone. (0.5 renders it byte for byte the same, only faster.) |
| alpha 0.6 | <img src="images/a0.6-torture.jpg" width="320" alt="Torture test at 0.6"> | A marble teapot, a wooden mirror frame, and a glass cube that absorbs by thickness. |
| alpha 0.7 | <img src="images/a0.7-torture.jpg" width="320" alt="Torture test at 0.7"> | Frosted glass ball, brushed gold tube, and soft shadows from sphere and rectangle lights. |
| alpha 0.8 | <img src="images/a0.8-torture.jpg" width="320" alt="Torture test at 0.8"> | Bricks from a PNG texture and normal map, a hammered gold tube and cone, and a frame bumped by its own grain. |
| alpha 0.9 | <img src="images/a0.9-torture.jpg" width="320" alt="Torture test at 0.9"> | Under an HDR sky: clouds in the mirror ball, sky light on the floor, a soft spot on the teapot and a low sun. |
| alpha 0.10 | <img src="images/a0.10-torture.jpg" width="320" alt="Torture test at 0.10"> | A flint glass prism splitting colors, a die drilled with CSG, and the sample clamp clearing the frosted ball's specks. |
| alpha 0.11 | <img src="images/a0.11-torture.jpg" width="320" alt="Torture test at 0.11"> | A layer-lined orange 3DBenchy from STL, a colored-STL puzzle cube and an OBJ ring on the floor. |
| alpha 0.11.1 | <img src="images/a0.11.1-torture.jpg" width="320" alt="Torture test at 0.11.1"> | Spectral dispersion: the prism and the lens split light wavelength by wavelength. Same picture otherwise, at a fifth of the memory. |
| alpha 0.12.1 | <img src="images/a0.12.1-torture.jpg" width="320" alt="Torture test at 0.12.1"> | Stratified wavelengths: the prism's rainbow is smoother, with less grain for the same rays. |
| alpha 0.13 | <img src="images/a0.13-torture.jpg" width="320" alt="Torture test at 0.13"> | Light bounced between nearby objects, and a low haze that fades the horizon band. |
| alpha 0.14 | <img src="images/a0.14-torture.jpg" width="320" alt="Torture test at 0.14"> | A walnut frame and a malachite cone from the material library, and dappled caustic light from the teapot's spot. |
| alpha 0.15 | <img src="images/a0.15-torture.jpg" width="320" alt="Torture test at 0.15"> | A mirror frosted in patches by a cells map, marble swirls inside the glass cube, and cloud shadows from a noise-driven sun. |
| alpha 0.16 | <img src="images/a0.16-torture.jpg" width="320" alt="Torture test at 0.16"> | A gold torus and beveled Lato “RAY” letters, and the first full-size render with the adaptive sampler (25 to 256 rays a pixel). |
| alpha 0.17 | <img src="images/a0.17-torture.jpg" width="320" alt="Torture test at 0.17"> | The teapot is now Martin Newell's 32 Bézier patches, traced exactly, with the same marble, spot and caustics. |
| alpha 0.18 | <img src="images/a0.18-torture.jpg" width="320" alt="Torture test at 0.18"> | A glass goblet and a black queen turned on the lathe, a tapered twisted star, and a steel spring swept along a path. |
| alpha 0.19 | <img src="images/a0.19-torture.jpg" width="320" alt="Torture test at 0.19"> | A glossy purple cluster of four metaballs behind the mirror sphere. |
| alpha 0.20 | <img src="images/a0.20-torture.jpg" width="320" alt="Torture test at 0.20"> | A dark lump of lunar regolith at the front right. |
| alpha 0.20.1 | <img src="images/a0.20.1-torture.jpg" width="320" alt="Torture test at 0.20.1"> | Stopped down to f/8 and refocused, so everything from the front row to the mirror is sharp; a true gold torus from the new metal material; rendered with the tuned adaptive sampler at under a third of the cost. |
| alpha 0.21 | <img src="images/a0.21-torture.jpg" width="320" alt="Torture test at 0.21"> | Smoother bounce light in shadow, from four times the rays where a point sits inside an object's bounds; frames finish 30% sooner on 16 threads. |

## Releases, newest first

### alpha 0.21.1 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.21.1) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.21.1/CHANGELOG.md)

JPEG and WebP decoders written from scratch (matching libjpeg-turbo and libwebp pixel for pixel on the test files), rays counted by kind and reported by raythrower, the chess set's own lacquer and marble textures, and the first downloadable builds for Linux, macOS and Windows.

<img src="images/a0.21.1-chess_dof.jpg" width="720" alt="The chessboard with its own textures">

**The chessboard with its own textures.** Poly Haven's chess set with its lacquered pieces and marble board, now decoded from its JPEG textures.

<img src="../images/changelog/chess-textures.png" width="720" alt="Before and after textures">

**Before and after textures.** The chessboard with flat colors and with the set's own textures.

<img src="../images/changelog/jpeg-webp.png" width="720" alt="PNG, JPEG and WebP">

**PNG, JPEG and WebP.** The same brick texture decoded from PNG, JPEG and WebP.

### alpha 0.21.4 (2026-11-22)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.21.4) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.21.4/CHANGELOG.md)

Two add-ons: an equirectangular 360°×180° panorama camera that renders straight to HDR, and a Python tool that turns the LROC NAC elevation model of the Apollo 11 site into terrain meshes. With them, an LM-free environment of Tranquility Base baked from the AS11-40-5927 camera station; and the AS11-40-5927 recreation on the real ground: the view solved from all four footpads and the LM's shadow, the Eagle model scaled to the real LM's proportions (height ×0.948), the true f/11 at 1/250 s exposure, gold-wrapped secondary struts, crinkled silver foil and a lighter engine bell.

<img src="../images/changelog/as11-40-5927-terrain.png" width="720" alt="AS11-40-5927 on the LROC terrain">

**AS11-40-5927 on the LROC terrain.** NASA's frame beside Ray's. The pads fit to 17.7 px on the 1950 px scan; ground within 1%, Kapton within 10% (still too orange), the box face deliberately 9% under because NASA's print looks dodged on the ascent stage.

<img src="../images/changelog/panorama.png" width="720" alt="Panoramas">

**Panoramas.** A test panorama, and Tranquility Base out to 2.4 km baked from the camera station with no LM in it.

<img src="../images/changelog/terrain.png" width="720" alt="LROC terrain">

**LROC terrain.** The meshes made from the LROC NAC elevation model.

<img src="../images/changelog/lm5-eagle-proportions.png" width="720" alt="Real LM proportions">

**Real LM proportions.** The Eagle model before and after scaling its height to the real LM's 22 ft 11 in over a 31 ft footpad span.

### alpha 0.21.3 (2026-11-22)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.21.3) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.21.3/CHANGELOG.md)

Eagle as our own model: NASA's LM rebuilt into lm5_eagle.glb with 16 named parts, its ascent skin split into 8,922 bellied-out, riveted, individually tinted panels; saved objects, an include object, materialMap, creaseAngle and model textures sampled through their UVs; and a recreation of AS11-40-5927, tuned by measurement against NASA's frame (ground, box face and Kapton within a few percent).

<img src="../images/changelog/as11-40-5927.png" width="720" alt="AS11-40-5927">

**AS11-40-5927.** NASA's frame beside Ray's. Aldrin, the PSEP and the flag are left out. Still to fix in 0.21.4: the yaw (the far legs and the tank side), gold-wrapped struts, the silver foil, the engine bell's gray, the exposure calibration and the near-black shadows.

<img src="../images/changelog/lm5-eagle.png" width="720" alt="The LM-5 Eagle model">

**The LM-5 Eagle model.** Far and near views of the rebuilt model.

<img src="../images/changelog/facecolors-uv.png" width="720" alt="Model textures through UVs">

**Model textures through UVs.** Per-face color against the model's own image sampled through its UVs: UNITED STATES becomes readable.

<img src="../images/changelog/lens-fans.png" width="720" alt="Softer ray fans">

**Softer ray fans.** The lens cutaway fans at half brightness.

### alpha 0.21.2 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.21.2) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.21.2/CHANGELOG.md)

Every lens as glass: each built-in prescription built in 3D and cut away, with ray fans traced through its real surfaces. Plus .deb, .rpm and Flatpak packages, a Linux editor launcher that names missing GTK libraries, and the MFC editor archived at the tag mfc-editor-final.

<img src="../images/changelog/lens-cutaways.png" width="720" alt="Lens cutaways with ray fans">

**Lens cutaways with ray fans.** The double Gauss and the seven Apollo lenses (the 60 mm also with its Réseau plate), cut in half, with an on-axis fan in red, green and blue light and a 0.7-field fan in orange, traced through each prescription to the film.

### Experiment (2026-11-21, Eagle materials)

NASA's LM with materials mapped part by part: amber Kapton foil with a crinkle bump and sun highlights, the MESA bay, a dark niobium-titanium engine bell, white-painted dishes, Pyromark black tank fairings, and the ascent skin as gray brushed anodized aluminum (mostly diffuse, reflection under 10%, grain from noise stretched along one axis). 512×512, 16 hemisphere rays a point.

<img src="images/exp-eagle-near.png" width="360" alt="Near side">

**Near side.** From the AS11-40-5903 side.

<img src="images/exp-eagle-far.png" width="360" alt="Far side">

**Far side.** The model turned 180° so the sun lights its far side.

### Experiment (2026-11-21, foil lunar module)

NASA's public-domain Lunar Module (Draco glTF, decoded by Ray) with its gold Kapton turned into true gold metal under a crinkled-foil bump map (turbulent Worley cell edges), coated-quartz windows over a dark cabin, a smooth dark-metal descent engine bell and normals recomputed with an experimental creaseAngle field, on regolith under a 15° sun, shot with AS11-40-5903's Data Camera, 60 mm Biogon, Réseau plate and Ektachrome MS. 1024×1024, adaptive sampling, 88 seconds on 16 threads.

<img src="images/exp-lm-foil.png" width="720" alt="The foil Lunar Module">

**The foil Lunar Module.** Full frame. The Réseau crosses print over the scene as on the Apollo frames.

<img src="images/exp-lm-crops.png" width="720" alt="Foil and engine bell, full resolution">

**Foil and engine bell, full resolution.** Left, the crinkled gold foil catching the sun on the descent stage. Right, the engine bell: smooth dark metal under the struts' shadows.

### Experiment (2026-11-21, 2026-alpha-0.21.1 build)

The torture test at 320×200 with the full-size adaptive settings (16 to 100 rays a pixel, 68.5 on average): 360 million rays, about 5,600 a pixel. On one 16-thread machine (an i7-11800H) 157 s; with 32 rayengines on a second machine (a 32-thread Ryzen 9 5950X) joining 16 on the first, 66 s, 2.4 times faster, with a byte-identical image and identical ray counts.

<img src="images/exp-heatmap.png" width="720" alt="Render and rays-per-pixel heat map">

**Render and rays-per-pixel heat map.** Left, the render; right, camera rays per pixel from dark blue (16, the minimum) to white (100, the maximum). The bright floor squares and the sky take the most rays because the stopping threshold is absolute; the dark squares stop early.

### alpha 0.21 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.21) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.21/CHANGELOG.md)

Optimization: frames finish sooner by splitting the last tiles (torture test 153 s to 108 s wall on 16 threads), decoded model files are shared between threads (chessboard CPU down 21%), the scene is encoded once for every networked engine (33% faster over three engines), and lathe shadows and the root finder do less work. Every image is byte for byte the same, except that bounce light from inside an object's bounds now uses four times the rays.

<img src="../images/changelog/bounce-fallback.png" width="720" alt="Bounce light from inside an object's bounds">

**Bounce light from inside an object's bounds.** Before, one ray over the half-sphere; after, four. The speckles in the suit's shadows smooth out.

<img src="../images/changelog/torture-fallback.png" width="720" alt="Torture test, 0.20.2 and 0.21">

**Torture test, 0.20.2 and 0.21.** The same change on the torture test, whose long boxes contain much of the scene.

### alpha 0.20.2 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.20.2) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.20.2/CHANGELOG.md)

Blender files and Draco: meshes read straight from .blend files saved by Blender 2.8 to 4.x, plain, gzip or Zstandard compressed, with Ray's own Zstandard decoder; Draco-compressed glTF decoded inside Ray, so NASA's original Lunar Module loads unchanged; and the speckles on the AS11-40-5903 suit traced to the object bounce method and fixed.

<img src="../images/changelog/blend-draco.png" width="720" alt=".blend and Draco">

**.blend and Draco.** The .blend test fixtures, Suzanne read from a Blender 2.93 file, and NASA's Lunar Module decoded from its original Draco-compressed glTF (97,588 triangles).

<img src="../images/changelog/as11-speckles.png" width="720" alt="The suit speckles">

**The suit speckles.** One bounce ray per object, from inside the suit's own bounds, against four rays over the half-sphere.

### alpha 0.20.1 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.20.1) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.20.1/CHANGELOG.md)

Studies and showcases: the torture test sharpened and taken out of CI; a confidence-aware, per-channel adaptive sampler with a rays-per-pixel heat map, cutting the full-size torture render from 67,301 to 19,329 CPU-seconds; studies of the two bounce-light methods; a glTF reader and a metal material; a Najdorf chessboard showing off depth of field; and a first recreation of AS11-40-5903.

<img src="images/a0.20.1-chess_dof.jpg" width="720" alt="Depth of field: the Najdorf English Attack">

**Depth of field: the Najdorf English Attack.** Poly Haven's CC0 chess set in the position after 11...b5, through the double Gauss wide open at f/2.02, focused on Black's e5 pawn, with bokeh from lights behind the board.

<img src="../images/changelog/torture-sharp.png" width="720" alt="The sharper torture test">

**The sharper torture test.** 0.20 against 0.20.1: stopped down to f/8 and focused at 18 units, with the torus in true gold.

<img src="../images/changelog/adaptive-rays.png" width="720" alt="Where the adaptive sampler spends its rays">

**Where the adaptive sampler spends its rays.** Heat maps of rays per pixel: edges, glass, rough surfaces and the frosted mirror take the most; flat floor and sky the fewest.

<img src="../images/changelog/bounce-studies.png" width="720" alt="Objects against hemisphere">

**Objects against hemisphere.** Six scenes at matched rays per point. The hemisphere is smoother in five; the object method wins with a small bright source.

<img src="images/a0.20.1-as11-40-5903.png" width="720" alt="AS11-40-5903, first attempt">

**AS11-40-5903, first attempt.** Ray's recreation beside NASA's frame: NASA's generic astronaut and Lunar Module, a gold metal visor, the Data Camera with its Réseau and Ektachrome. A first pass: the pose, framing, visor reflection and ground detail still differ from the original.

### alpha 0.20 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.20) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.20/CHANGELOG.md)

The Apollo cameras: the Hasselblads flown to the Moon as camera bodies, seven of their Carl Zeiss lenses, Kodak film with its curve, grain and spread of light, the Data Camera's Réseau plate whose crosses bloom away behind bright subjects, and lunar regolith, shown together at Tranquility Base.

<img src="../images/changelog/apollo-tranquility.png" width="720" alt="Tranquility Base">

**Tranquility Base.** Left, plain diffuse soil, a pinhole camera and sRGB. Right, the regolith material with its opposition surge glowing opposite the sun, shot on the Data Camera with the 60 mm Biogon, its Réseau plate and Ektachrome MS. The lunar module is built from primitives.

<img src="../images/changelog/apollo-reseau.png" width="720" alt="The Réseau plate">

**The Réseau plate.** The crosses print black on the soil and the gray ball, and fade on the bright white ball, where the film's spread of light fills them in, as on the Apollo frames.

<img src="../images/changelog/apollo-lenses.png" width="720" alt="Seven Apollo lenses">

**Seven Apollo lenses.** One subject through a pinhole and through the Biogon 38 and 60, Planar 80 and 100, UV-Sonnar 105, Sonnar 250 and Tele-Tessar 500: the 38's wide field and corner vignetting through to the 500's few degrees.

<img src="../images/changelog/apollo-film.png" width="720" alt="Kodak film stocks">

**Kodak film stocks.** Gray steps a stop apart, colored balls and an overexposed lamp in sRGB and on Ektachrome MS and EF, Panatomic-X and High Speed 2485: each stock's curve, color, grain and glow around the lamp.

<img src="../images/changelog/torture-regolith.png" width="720" alt="Torture test, 0.19 and 0.20">

**Torture test, 0.19 and 0.20.** The regolith lump at the front right, beside the cone.

### alpha 0.19 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.19) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.19/CHANGELOG.md)

Shapes from mathematics: implicit surfaces from a formula in x, y and z with exact gradients, metaballs solved exactly as polynomials, and Mandelbulb, Menger sponge and Sierpinski fractals traced by their distance estimates.

<img src="../images/changelog/math-solids.png" width="720" alt="Implicit surfaces, metaballs and fractals">

**Implicit surfaces, metaballs and fractals.** Left, an implicit sphere beside the real one, a gyroid ball and a heart. Middle, merging metaballs and a dented ball. Right, a Mandelbulb, a Menger sponge and a Sierpinski tetrahedron.

<img src="../images/changelog/torture-blob.png" width="720" alt="Torture test, 0.18 and 0.19">

**Torture test, 0.18 and 0.19.** Whole frame and an enlarged crop of the new metaball cluster.

### alpha 0.18 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.18) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.18/CHANGELOG.md)

Turned and pushed solids: lathes that spin a profile of lines and curves around an axis, traced exactly, and extrusions and sweeps that push outlines back into solids or carry them along a path.

<img src="../images/changelog/lathe-extrusion.png" width="720" alt="Lathes, extrusions and sweeps">

**Lathes, extrusions and sweeps.** Left, chess pieces, a glass goblet, a vase and a partial lathe. Right, a tapered star, a twisted column, a beveled frame with a hole, a spring, a zigzag rail and a closed figure-eight tube.

<img src="../images/changelog/torture-lathe.png" width="720" alt="Torture test, 0.17 and 0.18">

**Torture test, 0.17 and 0.18.** Whole frame and a full-size crop of the new front row.

### alpha 0.17 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.17) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.17/CHANGELOG.md)

Curved surfaces: Bézier patches and B-spline and NURBS surfaces, traced exactly rather than cut into triangles, and the Utah teapot as Martin Newell's own 32 patches, now the teapot in the torture test.

<img src="../images/changelog/bezier-teapot.png" width="720" alt="Mesh against patches">

**Mesh against patches.** The teapot's lid rim and spout as 6,320 triangles and as 32 Bézier patches, then the spout's lip enlarged: the facets and jagged silhouette are gone.

<img src="../images/changelog/torture-patches.png" width="720" alt="Torture test, 0.16 and 0.17">

**Torture test, 0.16 and 0.17.** Whole frame and a full-size crop with the patch teapot.

<img src="images/a0.17-patches_test.png" width="360" alt="patches_test">

**patches_test.** The patch teapot beside the mesh teapot in the same pose and material.

<img src="images/a0.17-nurbs_test.png" width="360" alt="nurbs_test">

**nurbs_test.** A NURBS torus beside the exact torus, and a waved bicubic sheet with an image texture and a noise bump.

### alpha 0.16 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.16) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.16/CHANGELOG.md)

More shapes, and letters: tori, discs and annuli, paraboloids, hyperboloids, superquadrics and rounded boxes, and a text object that sets a string in a TrueType font and extrudes it into a closed, beveled solid. Plus a lint test for the Windows min/max mistake, and the full-size torture image rendered with the adaptive sampler.

<img src="../images/changelog/shapes-text.png" width="720" alt="New shapes and text">

**New shapes and text.** Left, a torus, rounded boxes, a paraboloid bowl, a hyperboloid, a superquadric star and an annulus. Right, text in Lato Bold, extruded and beveled, and CSG carved into a slab.

<img src="../images/changelog/torture-shapes-text.png" width="720" alt="Torture test, 0.15 and 0.16">

**Torture test, 0.15 and 0.16.** The 5×5 stratified render against the adaptive one, whole frame and a full-size crop: the gold torus and RAY letters, and far less grain.

### alpha 0.15 (2026-11-21)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.15) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.15/CHANGELOG.md)

Textures everywhere: every number and color of a material, a bump map's depth and strength, and a light's tint and intensity can be a texture read where it shades, so glass can be frosted in places, a mirror polished in patches and sunlight broken by cloud shadows; and glass whose absorption is a texture adds it up along the path inside, coloring it from within.

<img src="../images/changelog/maps-volumes.png" width="720" alt="Maps and volumes">

**Maps and volumes.** Top, the maps test with constants and with texture-driven roughness, transparency, index and a light tinted by a 3D texture at the lit point. Middle, the volume test: glass absorbing evenly, then by a solid texture marched along the path inside. Bottom, the torture test at 0.14 and 0.15.

<img src="images/a0.15-maps_test.png" width="360" alt="maps_test">

**maps_test.** Roughness, transparency and index of refraction driven by textures, and a light colored by a 3D texture.

<img src="images/a0.15-volume_test.png" width="360" alt="volume_test">

**volume_test.** Art glass, smoky quartz, amber and veins floating in a clear block.

### alpha 0.14.1 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.14.1) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.14.1/CHANGELOG.md)

Faster tests and measured bounce light: the full suite drops from 89 s to 42 s (Release) and 263 s to 139 s (Debug); a sweep of bounce light from 1 to 64 rays per object sets the bounce test at 9; and a hemisphere test at the same rays per point for contrast.

<img src="../images/changelog/bounce-sweep.png" width="720" alt="Bounce light from 1 to 64 rays per object">

**Bounce light from 1 to 64 rays per object.** The bounce test at 1, 4, 9, 16, 25, 36, 49 and 64 rays per object, each labelled with its noise against a 256-ray reference. Noise halves with every fourfold increase in rays; 9 is where the grain stops showing at test size.

<img src="../images/changelog/bounce-rays.png" width="720" alt="Old, new and hemisphere">

**Old, new and hemisphere.** The old 2 rays per object, the new 9, and the hemisphere method at the same 54 rays a point, which is about 3.4 times smoother in this closed room for twice the CPU.

<img src="images/a0.14.1-bounce_hemisphere_test.png" width="360" alt="bounce_hemisphere_test">

**bounce_hemisphere_test.** The bounce room with the hemisphere method, kept as a reference render for contrast.

### alpha 0.14 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.14) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.14/CHANGELOG.md)

Natural materials and shaped light: a library of thirty-one stones, woods, gems and metals that a scene pulls in by name and then changes, cellular and caustics textures, beam profiles that shape a spot light like real fixtures with lenses, distance attenuation, and filters a light shines through, projecting slides, gobos and caustics.

<img src="../images/changelog/presets-lights.png" width="720" alt="Beam profiles, filters and caustics">

**Beam profiles, filters and caustics.** Top, six spots with the old smoothstep edge, then smoothstep, linear, cosine, gaussian, fresnel and a table profile, a projected slide and an attenuated light. Middle, the underwater scene without and with caustics projected from the sun through the haze. Bottom, the torture test at 0.13 and 0.14: malachite cone and dappled light on the teapot.

<img src="images/a0.14-underwater_test.png" width="360" alt="underwater_test">

**underwater_test.** Blue-green haze, a sandy floor and caustic light rippling over everything.

<img src="images/a0.14-materials_test.png" width="360" alt="materials_test">

**materials_test.** All thirty-one presets: marbles, granites, slate, sandstone, woods, jade, amber, obsidian, malachite, lapis, brushed metals, rust and terracotta.

<img src="images/a0.14-projector_test.png" width="360" alt="projector_test">

**projector_test.** Beam profiles side by side, a slide projected by a spot, and distance attenuation.

### alpha 0.13 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.13) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.13/CHANGELOG.md)

Light between surfaces, and air: matte surfaces gather light bounced off the objects near them (rays aimed at each nearby object, no cache), so colors bleed and shadows get fill light; and a haze that far things fade into dims light on its way to a surface, the same everywhere or lying low as ground mist.

<img src="../images/changelog/bounce-haze.png" width="720" alt="Bounce light and haze">

**Bounce light and haze.** Top, a red and green room without and with one bounce: the box and ball pick up red and green, the ceiling and shadows fill in. Middle, the haze test clear and with ground mist. Bottom, the torture test at 0.12.1 and 0.13.

<img src="../images/changelog/bounce-methods.png" width="720" alt="Two ways to gather">

**Two ways to gather.** No indirect light, rays aimed at nearby objects (the default), and rays spread over the half above each point.

<img src="images/a0.13-bounce_test.png" width="360" alt="bounce_test">

**bounce_test.** A red and green room lit by one light, one bounce.

<img src="images/a0.13-haze_test.png" width="360" alt="haze_test">

**haze_test.** Columns fading into haze, with a mist lying low over the floor.

### alpha 0.12.1 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.12.1) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.12.1/CHANGELOG.md)

Smoother spectral dispersion: a pixel's samples now take one each of as many wavelength strata, shuffled per pixel, instead of wavelengths tied to where each sample falls in the pixel. 21% less error at 4 rays a pixel and 27% less at 64. Plus the first screenshot of ray-editor.

<img src="../images/changelog/spectral-strata.png" width="720" alt="Stratified wavelengths">

**Stratified wavelengths.** The prism test at 4 and 64 rays a pixel and the torture test's prism, 0.11.1 against 0.12.1. At 64 rays the bars spread into smooth overlapping spectra; at 4 rays they are still grainy, but less so.

### alpha 0.12 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.12) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.12/CHANGELOG.md)

A cross-platform scene editor, ray-editor, built on wxWidgets for Windows, Linux and macOS: a scene tree, typed property grids and JSON editing checked by the scene's own reader, a shaded OpenGL view with a wireframe overlay and click-to-pick, and a Render button that ray-traces the current view progressively in place.

<img src="images/a0.12-editor.png" width="720" alt="ray-editor on Linux">

**ray-editor on Linux.** The textured Utah teapot after pressing Render: the ray-traced view drawn over the OpenGL preview, with the scene tree on the left and the settings' property grid on the right.

### alpha 0.11.1 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.11.1) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.11.1/CHANGELOG.md)

Rainbows and lean meshes: dispersion can be traced wavelength by wavelength, so glass and a chromatic lens smear colors into a continuous rainbow instead of three bars, and meshes keep their faces as indices in one geometry every thread shares, so a large model costs its memory once (Benchy at 16 threads: 4.3 GB down to 0.8 GB).

<img src="../images/changelog/spectral.png" width="720" alt="RGB against spectral dispersion">

**RGB against spectral dispersion.** Top, the prism test at 64 rays a pixel; middle, an enlarged crop of the chromatic lens test; bottom, the torture test's prism at full size. Left, three color bars; right, four wavelengths a split weighted by the CIE curves.

### alpha 0.11 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.11) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.11/CHANGELOG.md)

Models and prints: meshes load STL (binary, ASCII and colored) and OBJ files into the scene and its archive, with 3DBenchy as the sample; a layer-lines texture makes them look 3D-printed; the mesh tree is built by the surface area heuristic and walked nearest first; and the scene editor can edit, add, delete and import anything as JSON.

<img src="../images/changelog/models-layers.png" width="720" alt="Models and layer lines">

**Models and layer lines.** Top, Benchy's bow plain and with 0.1 mm layer lines as both color and bump map. Bottom, the torture test before and after: a printed Benchy and an OBJ ring colored per face.

<img src="images/a0.11-benchy_test.png" width="360" alt="benchy_test">

**benchy_test.** 3DBenchy (225,706 corners) in orange PLA with layer lines.

<img src="images/a0.11-models_test.png" width="360" alt="models_test">

**models_test.** Colored STL in both informal conventions, and an OBJ model.

### alpha 0.10 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.10) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.10/CHANGELOG.md)

Solids and spectra: constructive solid geometry, which makes lens blanks, bowls and drilled dice from spheres, boxes, cylinders, cones and meshes, and glass that splits light into colors by its Abbe number. Plus a sample clamp that takes the bright specks out of rough glass and area lights.

<img src="../images/changelog/csg-dispersion.png" width="720" alt="CSG, dispersion and the clamp">

**CSG, dispersion and the clamp.** Top, the prism test without and with its Abbe number: color fringes on everything seen through the flint glass. Middle, the torture test at 0.9 and 0.10, with the new prism. Bottom, the frosted ball without and with the clamp: the white specks are gone.

<img src="images/a0.10-csg_test.png" width="360" alt="csg_test">

**csg_test.** A glass lens blank, a bowl blue outside and white inside, a die with drilled pips, and a hammered capsule.

<img src="images/a0.10-prism_test.png" width="360" alt="prism_test">

**prism_test.** A flint prism and ball (index 1.62, Abbe number 20) before white bars.

### alpha 0.9 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.9) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.9/CHANGELOG.md)

Light and sky: spot lights with soft edges, directional sunlight whose shadows blur with distance, and environment maps, a sky gradient or an HDR picture of the surroundings that shows behind the scene, in mirrors and through glass, and can light it too.

<img src="../images/changelog/lights-environment.png" width="720" alt="Spot lights, sunlight and the sky">

**Spot lights, sunlight and the sky.** Top, point lights against spot lights and a soft sun: pools of light with hard and soft edges and long shadows blurring away from the posts. Middle, a flat background against the HDR sky: clouds in the mirror ball, the sky bent through the glass, the floor lit by the sky and darker under each ball. Bottom, the torture test at 0.8 and 0.9.

<img src="../images/lights.png" width="360" alt="lights_test">

**lights_test.** Spot lights with hard and soft edges and a sun with an angular size.

<img src="../images/environment.png" width="360" alt="environment_test">

**environment_test.** A generated HDR sky in reflections, through glass, and lighting the floor.

### alpha 0.8 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.8) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.8/CHANGELOG.md)

Surface detail: bump and normal maps that tilt the shading normal so surfaces look dented, grooved or rough while their shapes stay smooth, and PNG image textures of every kind, read by Ray's own decoder.

<img src="../images/changelog/bump-maps.png" width="720" alt="Bump and normal maps">

**Bump and normal maps.** Above, the new bump test without and with its bump maps: a hammered copper ball, an orange with its peel, a box embossed with bricks, and the floor's grain. Below, the torture test: the gold tube is hammered and the box behind the teapot is bricks, a PNG texture with a PNG normal map.

<img src="images/a0.8-bump_test.png" width="360" alt="bump_test">

**bump_test.** Height maps from solid noise and from a 16-bit PNG, and a tangent-space normal map.

### alpha 0.7 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.7) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.7/CHANGELOG.md)

Softness: frosted glass and brushed mirrors, and area lights that cast soft shadows, all picking their directions and points from where each ray falls in its pixel, so a stratified sampler smooths them and every render is the same each time.

<img src="../images/changelog/soft-lights.png" width="720" alt="Rough glass, brushed mirrors, area lights">

**Rough glass, brushed mirrors, area lights.** Above, the soft test with smooth surfaces and point lights, then as it is now: frosted panes blur the stripes behind them, the gold ball's reflection is brushed, and the shadows soften at their edges. Below, the torture test: a frosted glass ball, a brushed gold tube, and softer shadows.

<img src="../images/soft.png" width="360" alt="soft_test">

**soft_test.** Glass panes at three roughnesses, a brushed gold ball, and shadows from sphere and rectangle lights.

### alpha 0.6 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.6) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.6/CHANGELOG.md)

Solid noise textures (marble, wood, and noise for granite and stone) and glass that absorbs light by how far it travels inside, shown in a new test scene and worked into the torture test.

<img src="../images/changelog/torture-materials.png" width="720" alt="Torture test materials">

**Torture test materials.** Before: a copper teapot, a thin black frame, a flat green cube. After: polished white marble with gray veins, a wooden frame, and a cube that is deeper green where light crosses more glass.

<img src="../images/changelog/absorption.png" width="720" alt="Absorbing glass">

**Absorbing glass.** Left, the old flat tint: three slabs 0.15, 0.5 and 1.2 thick, all the same green. Right, Beer–Lambert absorption: the thin slab is pale, the thick one dark, and the amber ball deepest through its middle.

<img src="../images/solid_textures.png" width="360" alt="solid_textures_test">

**solid_textures_test.** White and green marble, a wooden log and floor, a granite wall, the glass slabs and the amber ball.

### alpha 0.5 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.5) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.5/CHANGELOG.md)

Faster: plain value types for vectors and colors, a bounding volume hierarchy over the scene, shadow rays that stop at the first hit, and lens tables worked out once and saved with the scene. Every render is byte for byte what it was. Plus the adaptive sampler.

*No picture changed: this release made the same images faster. The full-size torture test takes about a minute on a Release build.*

### alpha 0.4.1 (2026-11-20)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.4.1) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.4.1/CHANGELOG.md)

Real lenses cast blocked rays again, so lens renders have almost no grain from the lens.

<img src="../images/changelog/recast.png" width="720" alt="Casting blocked rays again">

**Casting blocked rays again.** lens_test, the torture test, and a full-size crop of it. On a plain gray background the difference between neighboring pixels falls from 0.022 to 0.002.

### alpha 0.4 (2026-11-19)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.4) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.4/CHANGELOG.md)

New shapes and a new way to share out the work: cylinders, cones and boxes; meshes with a bounding volume hierarchy; a torture test of everything through a real lens; PNG and high dynamic range output; and tasks that pull tiles of the image.

<img src="../images/changelog/exit-pupil.png" width="720" alt="Exit-pupil lens sampling">

**Exit-pupil lens sampling.** lens_test, chromatic_test, the torture test and a full-size crop: rays aimed only where the lens lets light through. lens_test is about 37% smoother and the torture test about 27%, with the same brightness.

<img src="images/a0.4-primitives.png" width="360" alt="primitives_test">

**primitives_test.** A capped cylinder, an open tube, a cone, a textured truncated cone, a textured cube and a glass box.

### alpha 0.3 (2026-11-19)

[Code at this tag](https://github.com/jnaujok/Ray/tree/2026-alpha-0.3) · [Changelog](https://github.com/jnaujok/Ray/blob/2026-alpha-0.3/CHANGELOG.md)

The ray tracing resumed: a corrected camera, anti-aliasing, color models, real lenses with chromatic aberration, meshes and the Utah teapot, textures, corrected materials, and scenes saved as JSON inside resource archives.

<img src="../images/changelog/camera-fix.png" width="360" alt="The camera fix">

**The camera fix.** Before: axis_test's spheres sheared into ellipses, xxx mirrored, reflect_me smeared. After: round spheres, the right way round.

<img src="../images/changelog/material-fixes.png" width="360" alt="Material fixes">

**Material fixes.** Before: the 1997 refraction and no specular highlight on the teapot. After: Snell's law, glass at index 1.5, and the highlight.

<img src="../images/lens.png" width="360" alt="Real lenses">

**Real lenses.** scan_test through a pinhole, and through a double Gauss 50 mm at f/2 focused on the glass sphere.

<img src="../images/textures.png" width="360" alt="Textures">

**Textures.** A checkered floor, an image on a wall and a sphere, and a solid checker sphere.

<img src="../images/teapot_textured.png" width="360" alt="The Utah teapot">

**The Utah teapot.** Spherically UV-mapped with an image texture.

<img src="../images/gallery.png" width="720" alt="The sample scenes">

**The sample scenes.** Anti-aliased renders of six sample scenes, the Utah teapot among them, after the camera and material fixes.

### 0.2.0 (2026-11-19)

[Code at this tag](https://github.com/jnaujok/Ray/tree/v0.2.0) · [Changelog](https://github.com/jnaujok/Ray/blob/v0.2.0/CHANGELOG.md)

Where it started: the 1997 code building again on Windows, Linux and macOS on top of IASLib, with the ray tracing logic unchanged. These are the pictures it made.

<img src="images/v0.2-gallery.png" width="720" alt="The 1997 renders">

**The 1997 renders.** Six sample scenes as the original code drew them, with its sheared camera and 1997 refraction.

<img src="images/v0.2-scan_test.png" width="360" alt="scan_test, 1997">

**scan_test, 1997.** Reflective and transparent spheres over a plane. Its glass sphere had an index of 1.0, which looked like glass only because of a refraction bug fixed in alpha 0.3.

## Coming releases

| Version | Plan | State |
|---|---|---|
| alpha 0.22 | Glowing objects like neon lights: any shape as a light source, neon tubes, halos in haze, and bloom | Planned |
