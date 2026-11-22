#!/usr/bin/env python3
"""Time raythrower on a fixed set of scenes, for performance work.

    bench.py [--raythrower build/raythrower] [--runs N] [--only name,...] [--json out.json]

Each case renders one scene with fixed settings and reports the wall time,
the CPU time (user + system), the peak memory and an MD5 of the image, so a
change can be checked for speed and for producing exactly the same bytes.
Run it from the repository root. It needs /usr/bin/time (GNU time).

The cases are the scenes 2026-alpha-0.21 was tuned on: the torture test at
240x180 with the full-size adaptive settings, Benchy, the depth-of-field
chessboard, the fractals, the AS11-40-5903 recreation, and a networked
render with three engines.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile

TORTURE_AA = "adaptive:min=16,max=100,threshold=0.004,confidence=0.95,channels=rgb"

CASES = [
    ("torture", ["scenes/torture_test.ray", "--size", "240x180", "--aa", TORTURE_AA]),
    ("benchy", ["scenes/benchy_test.ray"]),
    ("chess", ["scenes/chess_dof_test.ray", "--size", "480x360"]),
    ("fractal", ["scenes/fractal_test.ray"]),
    ("as11", ["scenes/as11_40_5903_test.ray", "--size", "256x256"]),
    ("network", ["scenes/benchy_test.ray", "--size", "160x120", "-N3", "--listen", "0", "--spawn"]),
]


def run(raythrower, args, tasks):
    with tempfile.TemporaryDirectory() as tmp:
        out = os.path.join(tmp, "out.raw")
        cmd = ["/usr/bin/time", "-v", raythrower] + args + ["-q", "-o", out]
        if tasks and not any(a.startswith("-N") for a in args):
            cmd.insert(3, "-N%d" % tasks)
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            sys.exit("failed: %s\n%s" % (" ".join(cmd), proc.stderr[-2000:]))
        err = proc.stderr

        def field(name):
            m = re.search(re.escape(name) + r":\s*([0-9.:]+)", err)
            return m.group(1) if m else "0"

        wall = field("Elapsed (wall clock) time (h:mm:ss or m:ss)")
        parts = [float(p) for p in wall.split(":")]
        seconds = 0.0
        for p in parts:
            seconds = seconds * 60 + p
        with open(out, "rb") as f:
            digest = hashlib.md5(f.read()).hexdigest()
        return {
            "wall": seconds,
            "cpu": float(field("User time (seconds)")) + float(field("System time (seconds)")),
            "rss_mb": int(field("Maximum resident set size (kbytes)")) / 1024.0,
            "md5": digest,
        }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raythrower", default="build/raythrower")
    ap.add_argument("--runs", type=int, default=1)
    ap.add_argument("--tasks", type=int, default=16)
    ap.add_argument("--only", default="")
    ap.add_argument("--json", default="")
    a = ap.parse_args()
    only = set(a.only.split(",")) if a.only else None
    results = {}
    print("%-9s %9s %9s %9s  %s" % ("case", "wall s", "cpu s", "peak MB", "md5"))
    for name, args in CASES:
        if only and name not in only:
            continue
        best = None
        for _ in range(a.runs):
            r = run(a.raythrower, args, a.tasks)
            if best and r["md5"] != best["md5"]:
                sys.exit("%s: the image changed between runs" % name)
            if best is None or r["cpu"] < best["cpu"]:
                best = r
        results[name] = best
        print("%-9s %9.2f %9.2f %9.0f  %s" % (name, best["wall"], best["cpu"], best["rss_mb"], best["md5"]))
    if a.json:
        with open(a.json, "w") as f:
            json.dump(results, f, indent=1)


if __name__ == "__main__":
    main()
