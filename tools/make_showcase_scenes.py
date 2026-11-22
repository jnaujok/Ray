#!/usr/bin/env python3
"""Write the 2026-alpha-0.20.1 showcase and study scenes.

    make_showcase_scenes.py [scenes-dir]

  chess_dof_test.ray      A full chessboard in the Najdorf Sicilian, English
                          Attack, after 1.e4 c5 2.Nf3 d6 3.d4 cxd4 4.Nxd4 Nf6
                          5.Nc3 a6 6.Be3 e5 7.Nb3 Be6 8.f3 Be7 9.Qd2 O-O
                          10.O-O-O Nbd7 11.g4 b5, through the double Gauss wide
                          open (f/2) focused on one piece, with small bright
                          lights behind the board for bokeh. The position is
                          played out move by move with python-chess (pip
                          install chess), not placed by hand. The pieces are
                          Poly Haven's Chess Set (CC0), read from its glTF.
  as11_40_5903_test.ray   A recreation of NASA photograph AS11-40-5903: Buzz
                          Aldrin at Tranquility Base, with Neil Armstrong and
                          the lunar module reflected in his gold visor, on the
                          Data Camera's 60 mm Biogon, Reseau plate and
                          Ektachrome MS. NASA's astronaut and lunar module
                          models (public domain).
  studies/bounce_*.ray    Scenes for comparing the "objects" and "hemisphere"
                          indirect light methods (tools/bounce_sweep.py).

The test renders are small; the README's showcase images are rendered from the
same scenes at full size (see their render commands in README.md).
"""
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

MOVES = "e4 c5 Nf3 d6 d4 cxd4 Nxd4 Nf6 Nc3 a6 Be3 e5 Nb3 Be6 f3 Be7 Qd2 O-O O-O-O Nbd7 g4 b5"

# Poly Haven's pieces stand on their own origins, in meters; a square of its
# board is 0.0579 m, and a square here is one unit.
PIECE_SCALE = 1.0 / 0.05788
BOARD_TOP = 0.017392585054039955     # the board mesh's top, in the set's meters
BOARD_TURN = 0.0
PIECE_NODES = {"p": "piece_pawn_white_01", "r": "piece_rook_white_01", "n": "piece_knight_white_01",
               "b": "piece_bishop_white_01", "q": "piece_queen_white", "k": "piece_king_white"}


def najdorf_position():
    try:
        import chess
    except ImportError:
        sys.exit("needs python-chess: pip install chess")
    board = chess.Board()
    for move in MOVES.split():
        board.push_san(move)
    pieces = []
    for square, piece in board.piece_map().items():
        pieces.append((chess.square_file(square), chess.square_rank(square), piece.symbol()))
    return board.fen(), sorted(pieces)


def ambient(c):
    return {"type": "ambient", "influence": 1.0, "color": c}


def diffuse(c, influence=0.85, texture=None):
    m = {"type": "diffuse", "influence": influence, "color": c, "diffuseCast": False}
    if texture:
        m["texture"] = texture
    return m


def specular(influence, power, color=(1.0, 1.0, 1.0)):
    return {"type": "specular", "influence": influence, "color": list(color), "power": power,
            "indexOfRefraction": 1.5, "materialHighlight": False}


def reflective(influence, roughness=0.0):
    m = {"type": "reflective", "influence": influence, "color": [1.0, 1.0, 1.0], "diffuseReflectivity": 0.0,
         "filterReflection": False}
    if roughness:
        m["roughness"] = roughness
    return m


def scene(width, height, camera, objects, lights, background=(0.0, 0.0, 0.0), extra=None, name="scene"):
    s = {"format": "Ray scene", "version": 3,
         "settings": {"imageWidth": width, "imageHeight": height, "imageFile": name + ".out", "colorDepth": 8,
                      "maximumDepth": 8, "depthThreshold": 0.01},
         "background": list(background), "camera": camera, "objects": objects, "lights": lights}
    if extra:
        s.update(extra)
    return s


def chess_scene():
    fen, pieces = najdorf_position()
    # The set's own textures (Poly Haven's JPEGs), through the models' UVs.
    def texture(name):
        return {"type": "image", "file": "models/chess/textures/chess_set_%s_diff_1k.jpg" % name, "srgb": True}

    white = [ambient([0.06, 0.055, 0.045]), diffuse([1.0, 1.0, 1.0], 0.8, texture("pieces_white")), specular(0.55, 0.07),
             reflective(0.07)]
    black = [ambient([0.012, 0.01, 0.008]), diffuse([1.0, 1.0, 1.0], 0.85, texture("pieces_black")), specular(0.6, 0.06),
             reflective(0.12)]
    objects = [
        # The table, and the set's own board (its top at y = 0, a square 1 unit across).
        {"type": "plane", "position": [0.0, -0.35, 0.0], "normal": [0.0, 1.0, 0.0], "color": [0.5, 0.5, 0.5],
         "materials": [ambient([0.02, 0.012, 0.008]), diffuse([1, 1, 1], 0.8, {"preset": "walnut", "scale": 0.08})]},
        {"type": "mesh", "file": "models/chess/chess_set_1k.gltf", "node": "board", "scale": PIECE_SCALE,
         "rotation": [0.0, BOARD_TURN, 0.0], "align": "none", "translation": [0.0, -BOARD_TOP * PIECE_SCALE, 0.0],
         "smooth": True, "doubleSided": False, "color": [0.5, 0.5, 0.5],
         "materials": [ambient([0.03, 0.025, 0.02]), diffuse([1.0, 1.0, 1.0], 0.8, texture("board")), specular(0.35, 0.08),
                       reflective(0.1)]},
    ]
    for file_index, rank, symbol in pieces:
        x = file_index - 3.5
        z = rank - 3.5
        kind = symbol.lower()
        is_white = symbol.isupper()
        objects.append({"type": "mesh", "file": "models/chess/chess_set_1k.gltf", "node": PIECE_NODES[kind],
                        "scale": PIECE_SCALE, "rotation": [0.0, 90.0 if is_white else -90.0, 0.0], "align": "none",
                        "translation": [x, 0.0, z], "smooth": True, "doubleSided": False, "color": [0.5, 0.5, 0.5],
                        "materials": white if is_white else black})
    # Small bright lights far behind the board, for bokeh (out of focus they
    # spread into discs, cat's-eyes toward the corners).
    bokeh = [(-9.0, 1.2, 14.0), (-6.5, 2.4, 17.0), (-4.0, 0.6, 12.5), (-1.5, 3.2, 19.0), (1.0, 1.4, 15.5),
             (3.5, 2.8, 18.0), (6.0, 0.9, 13.5), (8.5, 2.1, 16.5), (11.0, 3.5, 20.0), (-11.5, 3.0, 19.5),
             (2.5, 4.4, 22.0), (-7.5, 4.6, 23.0)]
    warm = [(9.0, 6.5, 3.5), (8.0, 7.0, 5.0), (9.0, 7.5, 6.0), (7.0, 6.0, 4.0)]
    for i, (x, y, z) in enumerate(bokeh):
        objects.append({"type": "sphere", "center": [x, y, z], "radius": 0.14, "color": [0.5, 0.5, 0.5],
                        "materials": [ambient(list(warm[i % len(warm)]))]})
    lights = [
        {"type": "rectangleLight", "position": [-3.0, 9.0, -6.0], "color": [0.95, 0.9, 0.82], "edge1": [3.0, 0.0, 0.0],
         "edge2": [0.0, 0.6, 1.5], "samples": 2},
        {"type": "point", "position": [7.0, 5.0, -2.0], "color": [0.35, 0.32, 0.30]},
    ]
    # From behind White's queenside, low, down the board toward the black
    # king; focused on Black's pawn on e5 (file 4, rank 4), partway down.
    camera_at = [-4.4, 3.0, -9.2]
    target = [4 - 3.5, 0.45, 4 - 3.5]
    distance = math.sqrt(sum((a - b) ** 2 for a, b in zip(camera_at, target)))
    camera = {"position": camera_at, "lookAt": [0.4, 0.3, 0.9], "up": [0.0, 1.0, 0.0], "verticalAngle": 30.0,
              "horizontalAngle": 40.0, "aspectRatio": 1.333333,
              "sampler": {"type": "stratified", "grid": 3, "seed": 0}, "colorModel": "srgb", "clamp": 4.0,
              "lens": {"prescription": "double-gauss-50mm", "focusDistance": round(distance, 4),
                       "sensorWidth": 36.0, "millimetersPerUnit": 57.88, "seed": 0}}
    s = scene(160, 120, camera, objects, lights, background=(0.015, 0.013, 0.012), name="chess_dof_test")
    return s, fen


def as11_scene():
    suit = [ambient([0.01, 0.01, 0.01]), diffuse([0.8, 0.8, 0.77], 0.62), specular(0.1, 0.3)]
    visor = [{"type": "metal", "influence": 0.88, "metal": "gold"}, specular(0.25, 0.04, (1.0, 0.85, 0.5)),
             {"type": "transparent", "influence": 0.12, "color": [0.55, 0.45, 0.2], "indexOfRefraction": 1.0}]
    astronaut = "models/apollo/astronaut.glb"
    objects = [
        {"type": "plane", "position": [0.0, 0.0, 0.0], "normal": [0.0, 1.0, 0.0], "color": [0.5, 0.5, 0.5],
         "materials": [{"type": "regolith", "influence": 1.0, "color": [0.12, 0.115, 0.11], "surge": 1.0, "width": 0.06}],
         "bump": {"mode": "height", "texture": {"type": "noise", "scale": 0.9, "octaves": 6, "turbulence": 0.0, "seed": 11},
                  "depth": 0.05, "step": 0.005}},
        # Aldrin, facing the camera: the suit, and the gold visor apart.
        {"type": "mesh", "file": astronaut, "gltfMaterial": "astnt1_1", "scale": 0.025, "align": "none",
         "translation": [0.0, 0.93, 0.0], "rotation": [0.0, 8.0, 0.0], "smooth": True, "doubleSided": True,
         "color": [0.5, 0.5, 0.5], "materials": suit},
        {"type": "mesh", "file": astronaut, "gltfMaterial": "astnt1_2", "scale": 0.025, "align": "none",
         "translation": [0.0, 0.93, 0.0], "rotation": [0.0, 8.0, 0.0], "smooth": True, "doubleSided": True,
         "color": [0.5, 0.5, 0.5], "materials": visor},
        # Armstrong, the photographer, behind the camera, facing Aldrin.
        {"type": "mesh", "file": astronaut, "gltfMaterial": "astnt1_1", "scale": 0.025, "align": "none",
         "translation": [0.0, 0.93, -4.35], "rotation": [0.0, 180.0, 0.0], "smooth": True, "doubleSided": True,
         "color": [0.5, 0.5, 0.5], "materials": suit},
        # Eagle, behind Armstrong and to Aldrin's right.
        {"type": "mesh", "file": "models/apollo/apollo_lunar_module.glb", "fitSize": 9.4, "align": "bottom",
         "translation": [3.15, 0.0, -6.6], "rotation": [0.0, -20.0, 0.0], "smooth": True, "doubleSided": True,
         "color": [0.5, 0.5, 0.5],
         "materials": [ambient([0.01, 0.01, 0.01]), diffuse([1, 1, 1], 0.75, {"type": "faceColors"}), reflective(0.15, 0.3)]},
        # The Solar Wind Composition experiment: a pole with a foil sheet 30 x 140 cm.
        {"type": "cylinder", "base": [3.6, 0.0, -10.0], "top": [3.6, 1.55, -10.0], "radius": 0.015, "capped": True,
         "color": [0.5, 0.5, 0.5], "materials": [{"type": "metal", "influence": 1.0, "metal": "aluminum", "roughness": 0.3}]},
        {"type": "box", "center": [3.6, 0.85, -9.98], "size": [0.3, 1.4, 0.004], "rotation": [0.0, 15.0, 0.0],
         "color": [0.5, 0.5, 0.5], "materials": [{"type": "metal", "influence": 1.0, "metal": "aluminum", "roughness": 0.15}]},
        # The flag, 3 x 5 ft on a pole with a crossbar.
        {"type": "cylinder", "base": [-1.8, 0.0, -9.0], "top": [-1.8, 2.3, -9.0], "radius": 0.02, "capped": True,
         "color": [0.5, 0.5, 0.5], "materials": [{"type": "metal", "influence": 1.0, "metal": "aluminum", "roughness": 0.4}]},
        {"type": "box", "center": [-1.04, 1.78, -9.0], "size": [1.52, 0.91, 0.01], "rotation": [0.0, 0.0, 0.0],
         "color": [0.5, 0.5, 0.5], "materials": [ambient([0.02, 0.02, 0.02]),
                                                diffuse([1, 1, 1], 0.85, {"type": "checker", "colors": [[0.7, 0.08, 0.1], [0.92, 0.92, 0.9]],
                                                                          "size": 0.07, "solid": True})]},
        # Earth, small and bright high above.
        {"type": "sphere", "center": [-40.0, 140.0, -120.0], "radius": 2.2, "color": [0.5, 0.5, 0.5],
         "materials": [ambient([1.2, 1.4, 1.9])]},
    ]
    # The sun about 15 degrees up, behind Armstrong and to his right.
    elevation, azimuth = math.radians(15.0), math.radians(282.0)
    direction = [math.sin(azimuth) * math.cos(elevation), -math.sin(elevation), math.cos(azimuth) * math.cos(elevation)]
    lights = [{"type": "directional", "direction": [round(v, 6) for v in direction], "color": [2.2, 2.2, 2.15],
               "angle": 0.53, "samples": 2}]
    camera = {"position": [0.22, 1.45, -4.0], "lookAt": [0.05, 0.62, 0.0], "up": [0.09, 1.0, 0.0], "body": "hasselblad-hdc",
              "sampler": {"type": "stratified", "grid": 3, "seed": 0},
              "lens": {"focusDistance": 4.1, "millimetersPerUnit": 1000.0}}
    extra = {"indirect": None}
    s = scene(128, 128, camera, objects, lights, background=(0.0, 0.0, 0.0), name="as11_40_5903_test")
    s["settings"]["maximumDepth"] = 6
    # The hemisphere method: with "objects", the suit's own mesh holds every
    # point on it, so it was gathered over the whole half-sphere by one ray,
    # and a leg in shadow found the other, sunlit leg in a few rays only,
    # each counting heavily: bright specks all over the shadows.
    s["settings"]["indirect"] = {"method": "hemisphere", "bounces": 1, "samples": 4, "distance": 6.0, "strength": 1.0}
    del extra
    return s


def study_scenes():
    """Scenes where the two indirect light methods should differ."""
    white = [ambient([0.0, 0.0, 0.0]), diffuse([0.75, 0.75, 0.75], 0.9)]
    red = [ambient([0.0, 0.0, 0.0]), diffuse([0.8, 0.12, 0.1], 0.9)]
    green = [ambient([0.0, 0.0, 0.0]), diffuse([0.12, 0.7, 0.15], 0.9)]
    floor = {"type": "plane", "position": [0.0, 0.0, 0.0], "normal": [0.0, 1.0, 0.0], "color": [0.5, 0.5, 0.5], "materials": white}

    def cam(pos, look):
        return {"position": pos, "lookAt": look, "up": [0.0, 1.0, 0.0], "verticalAngle": 40.0, "horizontalAngle": 51.0,
                "aspectRatio": 1.333333, "sampler": {"type": "stratified", "grid": 2, "seed": 0}, "colorModel": "clip"}

    sun = {"type": "directional", "direction": [0.4, -0.75, 0.53], "color": [1.0, 0.97, 0.9], "angle": 1.0, "samples": 2}
    indirect = {"method": "objects", "bounces": 1, "samples": 4, "objects": 8, "distance": 10.0, "strength": 1.0}
    out = {}
    # Open outdoors: a few objects on an endless floor under a dim sky color.
    out["bounce_open"] = scene(160, 120, cam([0, 2.2, -8], [0, 1, 0]), [floor,
        {"type": "sphere", "center": [-1.6, 1.0, 0.0], "radius": 1.0, "color": [0.5, 0.5, 0.5], "materials": red},
        {"type": "box", "center": [1.4, 0.8, 0.6], "size": [1.6, 1.6, 1.6], "rotation": [0, 25, 0], "color": [0.5, 0.5, 0.5], "materials": white}],
        [sun], background=(0.25, 0.32, 0.45))
    # Sparse: small objects far apart.
    out["bounce_sparse"] = scene(160, 120, cam([0, 3.0, -10], [0, 0.5, 0]), [floor] + [
        {"type": "sphere", "center": [x, 0.35, z], "radius": 0.35, "color": [0.5, 0.5, 0.5], "materials": c}
        for (x, z, c) in [(-3, -1, red), (0, 2, green), (3, -0.5, white), (-1.5, 3.5, white), (2, 4, red)]],
        [sun], background=(0.25, 0.32, 0.45))
    # Cluttered room: a closed box room full of objects.
    room = [{"type": "box", "center": [0, 2.5, 0], "size": [8, 5, 8], "rotation": [0, 0, 0], "color": [0.5, 0.5, 0.5], "materials": white}]
    clutter = [{"type": "box" if i % 2 else "sphere", "color": [0.5, 0.5, 0.5], "materials": [red, green, white][i % 3]}
               for i in range(12)]
    for i, o in enumerate(clutter):
        x, z = -3 + (i % 4) * 2.0, -2.5 + (i // 4) * 2.2
        if o["type"] == "box":
            o.update({"center": [x, 0.4, z], "size": [0.8, 0.8, 0.8], "rotation": [0, 15 * i, 0]})
        else:
            o.update({"center": [x, 0.45, z], "radius": 0.45})
    out["bounce_cluttered"] = scene(160, 120, cam([0, 2.5, -3.6], [0, 1.0, 1.0]), room + clutter,
        [{"type": "rectangleLight", "position": [-0.6, 4.95, -0.6], "color": [1.0, 0.95, 0.85], "edge1": [1.2, 0, 0], "edge2": [0, 0, 1.2], "samples": 2}])
    # A large red wall close beside white objects.
    out["bounce_wall"] = scene(160, 120, cam([0, 2.2, -7], [0, 1, 0]), [floor,
        {"type": "box", "center": [-2.2, 2.0, 0.5], "size": [0.2, 4.0, 6.0], "rotation": [0, 0, 0], "color": [0.5, 0.5, 0.5], "materials": red},
        {"type": "sphere", "center": [-0.8, 0.9, 0.0], "radius": 0.9, "color": [0.5, 0.5, 0.5], "materials": white},
        {"type": "box", "center": [1.2, 0.7, 0.8], "size": [1.4, 1.4, 1.4], "rotation": [0, 30, 0], "color": [0.5, 0.5, 0.5], "materials": white}],
        [sun], background=(0.25, 0.32, 0.45))
    # A small bright object: a glowing ball beside white ones.
    out["bounce_bright"] = scene(160, 120, cam([0, 2.0, -7], [0, 0.8, 0]), [floor,
        {"type": "sphere", "center": [0.0, 0.3, 0.0], "radius": 0.3, "color": [0.5, 0.5, 0.5], "materials": [ambient([6.0, 4.5, 2.0])]},
        {"type": "sphere", "center": [-1.4, 0.8, 0.6], "radius": 0.8, "color": [0.5, 0.5, 0.5], "materials": white},
        {"type": "box", "center": [1.5, 0.7, 0.5], "size": [1.4, 1.4, 1.4], "rotation": [0, 20, 0], "color": [0.5, 0.5, 0.5], "materials": white}],
        [{"type": "point", "position": [3, 6, -5], "color": [0.25, 0.25, 0.25]}], background=(0.02, 0.02, 0.03))
    # Distance cutoff: a red wall 4 units away, just inside or outside the reach.
    out["bounce_cutoff"] = scene(160, 120, cam([0, 2.0, -6], [0, 0.8, 1]), [floor,
        {"type": "box", "center": [0.0, 1.5, 4.0], "size": [6.0, 3.0, 0.2], "rotation": [0, 0, 0], "color": [0.5, 0.5, 0.5], "materials": red},
        {"type": "sphere", "center": [-1.2, 0.7, 0.0], "radius": 0.7, "color": [0.5, 0.5, 0.5], "materials": white},
        {"type": "sphere", "center": [1.2, 0.7, 2.5], "radius": 0.7, "color": [0.5, 0.5, 0.5], "materials": white}],
        [sun], background=(0.25, 0.32, 0.45))
    for name, s in out.items():
        s["settings"]["indirect"] = dict(indirect)
        s["settings"]["imageFile"] = name + ".out"
    out["bounce_cutoff"]["settings"]["indirect"]["distance"] = 3.0
    return out


def main(argv):
    root = argv[0] if argv else os.path.join(HERE, "..", "scenes")
    s, fen = chess_scene()
    with open(os.path.join(root, "chess_dof_test.ray"), "w") as f:
        json.dump(s, f, indent=2)
        f.write("\n")
    print("chess_dof_test.ray:", fen)
    with open(os.path.join(root, "as11_40_5903_test.ray"), "w") as f:
        json.dump(as11_scene(), f, indent=2)
        f.write("\n")
    os.makedirs(os.path.join(root, "studies"), exist_ok=True)
    for name, s in study_scenes().items():
        with open(os.path.join(root, "studies", name + ".ray"), "w") as f:
            json.dump(s, f, indent=2)
            f.write("\n")
    print("studies:", ", ".join(sorted(study_scenes())))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
