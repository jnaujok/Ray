#!/usr/bin/env python3
"""Measure the adaptive sampler's noise against its cost, for picking its settings.

    adaptive_sweep.py RAYTHROWER [--scene FILE]... [--size WxH] [--threads N]
                      [--reference SAMPLER] [--sampler SPEC]... [--out DIR]

Renders each scene (the torture test by default) at --size with a reference
sampler (stratified:grid=16, 256 rays a pixel, by default) and with each
--sampler (raythrower --aa specs; a set comparing the original rule with the
confidence-aware, per-channel one by default), timing each (wall and CPU time)
and measuring its noise: the root mean square difference from the reference on
a 0-255 scale, over the whole image and over its worst tenth of pixels. With the
adaptive sampler it also records the rays each pixel took (raythrower
--ray-count) and reports their average. Prints a Markdown table a scene; the
renders, and a heat map for each adaptive one, are kept in --out, and
references are reused on a second run. Nothing here is random.
"""
import argparse
import os
import re
import resource
import struct
import subprocess
import sys
import time

SAMPLERS = [
    "adaptive:min=25,max=256,threshold=0.004",
    "adaptive:min=4,max=256,threshold=0.004,confidence=0.95,channels=rgb",
    "adaptive:min=4,max=100,threshold=0.004,confidence=0.95,channels=rgb",
    "adaptive:min=4,max=100,threshold=0.006,confidence=0.95,channels=rgb",
    "adaptive:min=4,max=64,threshold=0.006,confidence=0.9,channels=rgb",
    "adaptive:min=9,max=100,threshold=0.004",
]


def read_raw(path):
    with open(path, "rb") as f:
        data = f.read()
    width, height = struct.unpack(">HH", data[:4])
    return width, height, data[4:4 + width * height * 3]


def render(raythrower, scene, sampler, out, size, threads, heat=None):
    args = [raythrower, scene, "-o", out, "-N%d" % threads, "--aa", sampler]
    if size:
        args += ["--size", size]
    if heat:
        args += ["--ray-count", heat]
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    start = time.time()
    result = subprocess.run(args, cwd=os.path.dirname(os.path.abspath(scene)), capture_output=True, text=True)
    wall = time.time() - start
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    if result.returncode:
        sys.exit("raythrower failed: %s" % result.stderr)
    cpu = (after.ru_utime - before.ru_utime) + (after.ru_stime - before.ru_stime)
    found = re.search(r"Rays per pixel: (\d+) to (\d+), ([\d.]+) on average", result.stdout)
    rays = float(found.group(3)) if found else None
    if rays is None:
        found = re.search(r"Rays Traced: \d+ \(([\d.]+) per pixel\)", result.stdout)
        rays = float(found.group(1)) if found else None
    return wall, cpu, rays


def noise(pixels, reference):
    errors = []
    for i in range(0, len(pixels), 3):
        errors.append(sum((pixels[i + c] - reference[i + c]) ** 2 for c in range(3)) / 3.0)
    whole = (sum(errors) / len(errors)) ** 0.5
    worst = sorted(errors)[-max(1, len(errors) // 10):]
    return whole, (sum(worst) / len(worst)) ** 0.5


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("raythrower")
    parser.add_argument("--scene", action="append")
    parser.add_argument("--size", default="320x240")
    parser.add_argument("--threads", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--reference", default="stratified:grid=16")
    parser.add_argument("--sampler", action="append")
    parser.add_argument("--out", default="adaptive_sweep")
    options = parser.parse_args()
    scenes = options.scene or [os.path.join(here, "..", "scenes", "torture_test.ray")]
    samplers = options.sampler or SAMPLERS
    os.makedirs(options.out, exist_ok=True)
    raythrower = os.path.abspath(options.raythrower)
    for scene in scenes:
        scene = os.path.abspath(scene)
        stem = os.path.splitext(os.path.basename(scene))[0]
        reference = os.path.join(os.path.abspath(options.out), "%s-reference.raw" % stem)
        if not os.path.exists(reference):
            print("rendering %s's reference ..." % stem, file=sys.stderr)
            render(raythrower, scene, options.reference, reference, options.size, options.threads)
        _, _, ref_pixels = read_raw(reference)
        print("\n%s at %s, against %s:\n" % (stem, options.size, options.reference))
        print("| Sampler | Rays a pixel | Wall (s) | CPU (s) | RMS, whole | RMS, worst tenth |")
        print("|---|---|---|---|---|---|")
        for index, sampler in enumerate(samplers):
            out = os.path.join(os.path.abspath(options.out), "%s-%d.raw" % (stem, index))
            heat = out[:-4] + "-rays.png" if sampler.startswith("adaptive") else None
            wall, cpu, rays = render(raythrower, scene, sampler, out, options.size, options.threads, heat)
            _, _, pixels = read_raw(out)
            whole, worst = noise(pixels, ref_pixels)
            print("| `%s` | %s | %.1f | %.1f | %.2f | %.2f |" % (sampler, "%.1f" % rays if rays else "?", wall, cpu, whole, worst))
            sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
