"""Run meshtool on every mesh file in a folder and write a CSV report."""
from __future__ import annotations

import argparse
import csv
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional

MESH_EXTENSIONS = {".stl", ".obj"}

COLUMNS = [
    "file", "vertices", "triangles", "edges", "boundary_edges", "non_manifold_edges",
    "watertight", "bbox_min_x", "bbox_min_y", "bbox_min_z",
    "bbox_max_x", "bbox_max_y", "bbox_max_z", "surface_area", "volume", "error",
]


class MeshToolError(RuntimeError):
    """meshtool could not be run or rejected the file."""


def default_meshtool() -> str:
    """$MESHTOOL if set, otherwise build/meshtool next to this repo."""
    env = os.environ.get("MESHTOOL")
    if env:
        return env
    return str(Path(__file__).resolve().parent.parent / "build" / "meshtool")


def run_meshtool(path: os.PathLike, meshtool: Optional[str] = None) -> dict:
    """Run meshtool on one file and return its parsed JSON output."""
    exe = meshtool or default_meshtool()
    try:
        proc = subprocess.run([exe, str(path)], capture_output=True, text=True, timeout=60)
    except FileNotFoundError:
        raise MeshToolError("meshtool executable not found: %s" % exe)
    except subprocess.TimeoutExpired:
        raise MeshToolError("meshtool timed out on %s" % path)
    if proc.returncode != 0:
        raise MeshToolError(proc.stderr.strip() or "meshtool exited with %d" % proc.returncode)
    try:
        return json.loads(proc.stdout)
    except json.JSONDecodeError as e:
        raise MeshToolError("meshtool printed invalid JSON: %s" % e)


def find_meshes(folder: os.PathLike) -> List[Path]:
    """All .stl/.obj files directly inside folder, sorted by name."""
    return sorted(
        p for p in Path(folder).iterdir()
        if p.is_file() and p.suffix.lower() in MESH_EXTENSIONS
    )


def to_row(result: dict) -> Dict[str, object]:
    row = {k: result.get(k) for k in COLUMNS if k in result}
    for prefix, key in (("bbox_min", "bbox_min"), ("bbox_max", "bbox_max")):
        for axis, value in zip("xyz", result[key]):
            row["%s_%s" % (prefix, axis)] = value
    return row


def build_report(folder: os.PathLike, meshtool: Optional[str] = None) -> List[Dict[str, object]]:
    """One row per mesh file. A file meshtool rejects gets a row with `error` set."""
    rows = []
    for path in find_meshes(folder):
        try:
            row = to_row(run_meshtool(path, meshtool))
            row["file"] = path.name
        except MeshToolError as e:
            row = {"file": path.name, "error": str(e)}
        rows.append(row)
    return rows


def write_csv(rows: List[Dict[str, object]], out_path: os.PathLike) -> None:
    with open(out_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=COLUMNS, restval="")
        writer.writeheader()
        writer.writerows(rows)


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("folder", help="folder containing .stl / .obj files")
    ap.add_argument("-o", "--output", default="report.csv", help="CSV path (default: report.csv)")
    ap.add_argument("--meshtool", help="path to the meshtool executable")
    args = ap.parse_args(argv)

    if not Path(args.folder).is_dir():
        print("meshreport: not a folder: %s" % args.folder, file=sys.stderr)
        return 1
    rows = build_report(args.folder, args.meshtool)
    write_csv(rows, args.output)
    failed = sum(1 for r in rows if r.get("error"))
    print("wrote %s: %d files, %d failed" % (args.output, len(rows), failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
