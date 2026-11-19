#!/usr/bin/env python3
"""Convert a Wavefront OBJ model into a Ray mesh object (JSON).

    obj_to_ray.py model.obj [--flip] [--smooth] [--double-sided] [--weld]
                            [--scale S] [--translate X Y Z] > mesh.json

Only vertices ("v") and faces ("f") are read; texture coordinates, normals
and materials in the OBJ are ignored (Ray works out its own normals).
Polygons with more than three corners are split into a fan of triangles.

Ray takes a face's front to be the side (V2 - V1) x (V3 - V2) points to,
the same rule OBJ files are made for, so faces come out facing outward.
--flip is for the occasional model wound the other way. The output is one JSON object, ready to put
in a scene's "objects" list; raythrower --save tidies a scene's layout.
"""
import argparse
import json
import sys


def read_obj(path):
    vertices = []
    faces = []
    with open(path, encoding="utf-8", errors="replace") as obj:
        for line_number, line in enumerate(obj, 1):
            parts = line.split()
            if not parts:
                continue
            if parts[0] == "v":
                vertices.append([float(value) for value in parts[1:4]])
            elif parts[0] == "f":
                corners = []
                for corner in parts[1:]:
                    index = int(corner.split("/")[0])
                    # OBJ indices count from 1, or back from the end if negative.
                    corners.append(index - 1 if index > 0 else len(vertices) + index)
                if len(corners) < 3:
                    sys.exit(f"{path}:{line_number}: a face needs three corners")
                for k in range(1, len(corners) - 1):
                    faces.append([corners[0], corners[k], corners[k + 1]])
    for face in faces:
        if any(index < 0 or index >= len(vertices) for index in face):
            sys.exit(f"{path}: a face uses a vertex that doesn't exist")
    return vertices, faces


def weld(vertices, faces):
    """Merges vertices with the same (rounded) position, and drops faces
    that lose a corner to the merge."""
    first = {}
    remap = []
    kept = []
    for vertex in vertices:
        key = tuple(vertex)
        if key not in first:
            first[key] = len(kept)
            kept.append(vertex)
        remap.append(first[key])
    welded = []
    for face in faces:
        face = [remap[index] for index in face]
        if len(set(face)) == 3:
            welded.append(face)
    return kept, welded


def main():
    parser = argparse.ArgumentParser(description="Convert an OBJ model into a Ray mesh object.")
    parser.add_argument("obj")
    parser.add_argument("--flip", action="store_true", help="reverse every face's corners, for a model wound the other way")
    parser.add_argument("--smooth", action="store_true", help="shade as a smooth surface")
    parser.add_argument("--double-sided", action="store_true", help="faces can be seen from behind")
    parser.add_argument("--weld", action="store_true",
                        help="merge vertices at the same position, so faces on either side of a seam share "
                             "a smooth normal (many models repeat vertices along the seams between parts)")
    parser.add_argument("--scale", type=float, default=1.0)
    parser.add_argument("--translate", type=float, nargs=3, default=[0.0, 0.0, 0.0], metavar=("X", "Y", "Z"))
    args = parser.parse_args()

    vertices, faces = read_obj(args.obj)
    vertices = [[round(v[i] * args.scale + args.translate[i], 6) for i in range(3)] for v in vertices]
    if args.weld:
        vertices, faces = weld(vertices, faces)
    if args.flip:
        faces = [[a, c, b] for a, b, c in faces]

    mesh = {
        "type": "mesh",
        "vertices": vertices,
        "faces": faces,
        "smooth": args.smooth,
        "doubleSided": args.double_sided,
    }
    json.dump(mesh, sys.stdout)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
