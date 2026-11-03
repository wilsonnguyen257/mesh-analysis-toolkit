# Mesh Analysis Toolkit

A small C++17 core with a Python automation layer, structured the way a
simulation tool is: fast native analysis underneath, scripting on top.

- **C++ core** (`include/mesh`, `src`): `Vertex`, `Triangle` and `Mesh` classes. Loads
  ASCII STL and OBJ, welds duplicate vertices, and computes triangle count, bounding box,
  surface area, volume and edge adjacency (`std::vector` for geometry,
  `std::unordered_map` for the edge lookup).
- **`meshtool`**: command-line tool, `meshtool file.stl` prints the results as JSON.
- **Python layer** (`python/meshreport.py`): runs `meshtool` on every mesh in a folder
  (`subprocess` + `json`) and writes a CSV report.
- **Tests**: GoogleTest (cube and tetrahedron with hand-checked area and volume) and pytest.
  GitHub Actions builds and runs both suites on every push.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure   # C++ tests
python -m pip install pytest
python -m pytest python                       # Python tests
```

GoogleTest is fetched by CMake on first configure (needs network). Pass
`-DMESH_BUILD_TESTS=OFF` to skip it.

## Usage

```sh
./build/meshtool samples/tetrahedron.obj
```

```json
{
  "file": "samples/tetrahedron.obj",
  "vertices": 4,
  "triangles": 4,
  "edges": 6,
  "boundary_edges": 0,
  "non_manifold_edges": 0,
  "watertight": true,
  "bbox_min": [0, 0, 0],
  "bbox_max": [1, 1, 1],
  "surface_area": 2.3660254,
  "volume": 0.166666667
}
```

```sh
python python/meshreport.py samples -o report.csv
```

One row per `.stl`/`.obj` file. A file `meshtool` rejects gets a row with the `error`
column filled in, and the script exits non-zero.

## Notes

- Volume is the absolute value of the signed volume and is `null` unless the mesh is
  watertight (every edge shared by exactly two triangles).
- Binary STL is not supported; `meshtool` reports an error for it.
- Set `MESHTOOL=/path/to/meshtool` if the executable is not at `build/meshtool`.
