import csv
import math
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import meshreport  # noqa: E402

SAMPLES = Path(__file__).resolve().parents[2] / "samples"


@pytest.fixture(scope="session")
def meshtool():
    exe = Path(meshreport.default_meshtool())
    if not exe.exists():
        pytest.fail("meshtool not built at %s (build it or set MESHTOOL)" % exe)
    return str(exe)


def test_cube_stl(meshtool):
    r = meshreport.run_meshtool(SAMPLES / "cube.stl", meshtool)
    assert (r["vertices"], r["triangles"], r["edges"]) == (8, 12, 18)
    assert r["surface_area"] == pytest.approx(6.0)
    assert r["volume"] == pytest.approx(1.0)
    assert r["watertight"] is True
    assert r["bbox_min"] == [0, 0, 0] and r["bbox_max"] == [1, 1, 1]


def test_tetrahedron_obj(meshtool):
    r = meshreport.run_meshtool(SAMPLES / "tetrahedron.obj", meshtool)
    assert (r["vertices"], r["triangles"], r["edges"]) == (4, 4, 6)
    assert r["surface_area"] == pytest.approx(1.5 + math.sqrt(3) / 2)
    assert r["volume"] == pytest.approx(1 / 6)


def test_stl_and_obj_agree(meshtool):
    a = meshreport.run_meshtool(SAMPLES / "cube.stl", meshtool)
    b = meshreport.run_meshtool(SAMPLES / "cube.obj", meshtool)
    for key in ("vertices", "triangles", "edges", "surface_area", "volume"):
        assert a[key] == pytest.approx(b[key])


def test_open_mesh_has_null_volume(meshtool):
    r = meshreport.run_meshtool(SAMPLES / "open_square.stl", meshtool)
    assert r["watertight"] is False
    assert r["boundary_edges"] == 4
    assert r["volume"] is None


def test_missing_file_raises(meshtool):
    with pytest.raises(meshreport.MeshToolError, match="cannot open"):
        meshreport.run_meshtool(SAMPLES / "nope.stl", meshtool)


def test_missing_executable_raises(tmp_path):
    with pytest.raises(meshreport.MeshToolError, match="not found"):
        meshreport.run_meshtool(SAMPLES / "cube.stl", str(tmp_path / "no-such-tool"))


def test_find_meshes_filters_and_sorts(tmp_path):
    for name in ("b.stl", "a.OBJ", "notes.txt", "c.ply"):
        (tmp_path / name).write_text("x")
    (tmp_path / "sub.stl").mkdir()
    assert [p.name for p in meshreport.find_meshes(tmp_path)] == ["a.OBJ", "b.stl"]


def test_report_covers_every_sample(meshtool):
    rows = meshreport.build_report(SAMPLES, meshtool)
    assert [r["file"] for r in rows] == [
        "cube.obj", "cube.stl", "open_square.stl", "tetrahedron.obj", "tetrahedron.stl"]
    assert not any(r.get("error") for r in rows)


def test_csv_round_trip(meshtool, tmp_path):
    out = tmp_path / "report.csv"
    meshreport.write_csv(meshreport.build_report(SAMPLES, meshtool), out)
    with open(out, newline="") as f:
        rows = {r["file"]: r for r in csv.DictReader(f)}
    assert list(next(iter(rows.values())).keys()) == meshreport.COLUMNS
    assert int(rows["cube.stl"]["triangles"]) == 12
    assert float(rows["cube.stl"]["volume"]) == pytest.approx(1.0)
    assert rows["open_square.stl"]["volume"] == ""


def test_bad_file_gets_error_row_and_others_still_run(meshtool, tmp_path):
    (tmp_path / "broken.stl").write_text("not an stl")
    (tmp_path / "cube.stl").write_text((SAMPLES / "cube.stl").read_text())
    rows = {r["file"]: r for r in meshreport.build_report(tmp_path, meshtool)}
    assert "no facets" in rows["broken.stl"]["error"]
    assert rows["cube.stl"]["triangles"] == 12 and "error" not in rows["cube.stl"]


def test_empty_folder_writes_header_only(meshtool, tmp_path):
    out = tmp_path / "r.csv"
    assert meshreport.main([str(tmp_path), "-o", str(out), "--meshtool", meshtool]) == 0
    assert out.read_text().strip() == ",".join(meshreport.COLUMNS)


def test_main_success_and_failure_exit_codes(meshtool, tmp_path, capsys):
    out = tmp_path / "r.csv"
    assert meshreport.main([str(SAMPLES), "-o", str(out), "--meshtool", meshtool]) == 0
    assert "5 files, 0 failed" in capsys.readouterr().out

    bad = tmp_path / "bad"
    bad.mkdir()
    (bad / "x.obj").write_text("v 0 0 0\nf 1 2 3\n")
    assert meshreport.main([str(bad), "-o", str(out), "--meshtool", meshtool]) == 1
    assert "1 failed" in capsys.readouterr().out


def test_main_rejects_missing_folder(tmp_path, capsys):
    assert meshreport.main([str(tmp_path / "nope")]) == 1
    assert "not a folder" in capsys.readouterr().err
