#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

#include "mesh/loader.hpp"
#include "mesh/mesh.hpp"
#include "mesh/report.hpp"

using namespace mesh;

namespace {

const std::string kSamples = SAMPLES_DIR;

// Unit cube [0,1]^3 with outward-facing winding.
Mesh makeCube() {
    Mesh m;
    std::size_t id[8];
    for (int i = 0; i < 8; ++i) id[i] = m.addVertex(Vertex(i & 1, (i >> 1) & 1, (i >> 2) & 1));
    const int quads[6][4] = {{0, 2, 3, 1}, {4, 5, 7, 6}, {0, 1, 5, 4},
                             {2, 6, 7, 3}, {0, 4, 6, 2}, {1, 3, 7, 5}};
    for (const auto& q : quads) {
        m.addTriangle(id[q[0]], id[q[1]], id[q[2]]);
        m.addTriangle(id[q[0]], id[q[2]], id[q[3]]);
    }
    return m;
}

// Right-corner tetrahedron: (0,0,0) (1,0,0) (0,1,0) (0,0,1).
Mesh makeTetrahedron() {
    Mesh m;
    std::size_t o = m.addVertex(Vertex(0, 0, 0));
    std::size_t x = m.addVertex(Vertex(1, 0, 0));
    std::size_t y = m.addVertex(Vertex(0, 1, 0));
    std::size_t z = m.addVertex(Vertex(0, 0, 1));
    m.addTriangle(o, y, x);
    m.addTriangle(o, z, y);
    m.addTriangle(o, x, z);
    m.addTriangle(x, y, z);
    return m;
}

std::string writeTemp(const std::string& name, const std::string& body) {
    std::string path = ::testing::TempDir() + name;
    std::ofstream(path) << body;
    return path;
}

}  // namespace

// ---- cube (area 6, volume 1) -------------------------------------------

TEST(Cube, Counts) {
    Mesh m = makeCube();
    EXPECT_EQ(m.vertexCount(), 8u);
    EXPECT_EQ(m.triangleCount(), 12u);
    EXPECT_EQ(m.edgeAdjacency().size(), 18u);  // 12 cube edges + 6 face diagonals
}

TEST(Cube, BoundingBox) {
    BoundingBox b = makeCube().boundingBox();
    EXPECT_EQ(b.min, Vertex(0, 0, 0));
    EXPECT_EQ(b.max, Vertex(1, 1, 1));
}

TEST(Cube, SurfaceArea) { EXPECT_NEAR(makeCube().surfaceArea(), 6.0, 1e-12); }

TEST(Cube, Volume) { EXPECT_NEAR(makeCube().signedVolume(), 1.0, 1e-12); }

TEST(Cube, EveryEdgeSharedByTwoTriangles) {
    for (const auto& kv : makeCube().edgeAdjacency()) EXPECT_EQ(kv.second.size(), 2u);
}

TEST(Cube, IsWatertight) {
    EdgeStats s = classifyEdges(makeCube().edgeAdjacency());
    EXPECT_EQ(s.boundary, 0u);
    EXPECT_EQ(s.nonManifold, 0u);
    EXPECT_TRUE(s.watertight());
}

TEST(Cube, ScalesWithSideLength) {
    Mesh m;
    std::size_t id[8];
    for (int i = 0; i < 8; ++i) id[i] = m.addVertex(Vertex(2.0 * (i & 1), 2.0 * ((i >> 1) & 1), 2.0 * ((i >> 2) & 1)));
    const int quads[6][4] = {{0, 2, 3, 1}, {4, 5, 7, 6}, {0, 1, 5, 4},
                             {2, 6, 7, 3}, {0, 4, 6, 2}, {1, 3, 7, 5}};
    for (const auto& q : quads) {
        m.addTriangle(id[q[0]], id[q[1]], id[q[2]]);
        m.addTriangle(id[q[0]], id[q[2]], id[q[3]]);
    }
    EXPECT_NEAR(m.surfaceArea(), 24.0, 1e-12);
    EXPECT_NEAR(m.signedVolume(), 8.0, 1e-12);
}

// ---- tetrahedron (area 3/2 + sqrt(3)/2, volume 1/6) ---------------------

TEST(Tetrahedron, Counts) {
    Mesh m = makeTetrahedron();
    EXPECT_EQ(m.vertexCount(), 4u);
    EXPECT_EQ(m.triangleCount(), 4u);
    EXPECT_EQ(m.edgeAdjacency().size(), 6u);
}

TEST(Tetrahedron, SurfaceArea) {
    EXPECT_NEAR(makeTetrahedron().surfaceArea(), 1.5 + std::sqrt(3.0) / 2.0, 1e-12);
}

TEST(Tetrahedron, Volume) { EXPECT_NEAR(makeTetrahedron().signedVolume(), 1.0 / 6.0, 1e-12); }

TEST(Tetrahedron, EveryEdgeSharedByTwoTriangles) {
    EdgeStats s = classifyEdges(makeTetrahedron().edgeAdjacency());
    EXPECT_EQ(s.edges, 6u);
    EXPECT_TRUE(s.watertight());
}

TEST(Tetrahedron, FlippedWindingGivesNegativeVolume) {
    Mesh m;
    std::size_t o = m.addVertex(Vertex(0, 0, 0));
    std::size_t x = m.addVertex(Vertex(1, 0, 0));
    std::size_t y = m.addVertex(Vertex(0, 1, 0));
    std::size_t z = m.addVertex(Vertex(0, 0, 1));
    m.addTriangle(o, x, y);
    m.addTriangle(o, y, z);
    m.addTriangle(o, z, x);
    m.addTriangle(x, z, y);
    EXPECT_NEAR(m.signedVolume(), -1.0 / 6.0, 1e-12);
}

// ---- topology edge cases -----------------------------------------------

TEST(Topology, OpenSquareHasBoundary) {
    Mesh m;
    std::size_t a = m.addVertex(Vertex(0, 0, 0)), b = m.addVertex(Vertex(1, 0, 0));
    std::size_t c = m.addVertex(Vertex(1, 1, 0)), d = m.addVertex(Vertex(0, 1, 0));
    m.addTriangle(a, b, c);
    m.addTriangle(a, c, d);
    EdgeStats s = classifyEdges(m.edgeAdjacency());
    EXPECT_EQ(s.edges, 5u);
    EXPECT_EQ(s.boundary, 4u);
    EXPECT_FALSE(s.watertight());
    EXPECT_NEAR(m.surfaceArea(), 1.0, 1e-12);
}

TEST(Topology, ThreeTrianglesOnOneEdgeIsNonManifold) {
    Mesh m;
    std::size_t a = m.addVertex(Vertex(0, 0, 0)), b = m.addVertex(Vertex(1, 0, 0));
    for (double z : {1.0, 2.0, 3.0}) m.addTriangle(a, b, m.addVertex(Vertex(0, 1, z)));
    EdgeStats s = classifyEdges(m.edgeAdjacency());
    EXPECT_EQ(s.nonManifold, 1u);
    EXPECT_FALSE(s.watertight());
}

TEST(Topology, AdjacencyListsTheSharingTriangles) {
    Mesh m = makeTetrahedron();
    EdgeMap map = m.edgeAdjacency();
    auto it = map.find(Edge(0, 1));  // edge o-x is used by triangles 0 and 2
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, (std::vector<std::size_t>{0, 2}));
}

TEST(Topology, EdgeIsUndirected) { EXPECT_TRUE(Edge(3, 7) == Edge(7, 3)); }

// ---- Mesh API ----------------------------------------------------------

TEST(MeshApi, IdenticalVerticesAreWelded) {
    Mesh m;
    EXPECT_EQ(m.addVertex(Vertex(1, 2, 3)), m.addVertex(Vertex(1, 2, 3)));
    EXPECT_EQ(m.vertexCount(), 1u);
}

TEST(MeshApi, NegativeZeroWeldsWithZero) {
    Mesh m;
    EXPECT_EQ(m.addVertex(Vertex(0, 0, 0)), m.addVertex(Vertex(-0.0, 0, 0)));
}

TEST(MeshApi, TriangleIndexOutOfRangeThrows) {
    Mesh m;
    m.addVertex(Vertex(0, 0, 0));
    EXPECT_THROW(m.addTriangle(0, 1, 2), std::out_of_range);
}

TEST(MeshApi, EmptyBoundingBoxThrows) { EXPECT_THROW(Mesh().boundingBox(), std::runtime_error); }

// ---- loaders -----------------------------------------------------------

TEST(Loader, CubeStl) {
    Mesh m = loadSTL(kSamples + "/cube.stl");
    EXPECT_EQ(m.vertexCount(), 8u);  // 36 STL vertices weld down to 8
    EXPECT_EQ(m.triangleCount(), 12u);
    EXPECT_NEAR(m.surfaceArea(), 6.0, 1e-12);
    EXPECT_NEAR(m.signedVolume(), 1.0, 1e-12);
}

TEST(Loader, CubeObj) {
    Mesh m = loadOBJ(kSamples + "/cube.obj");
    EXPECT_EQ(m.vertexCount(), 8u);
    EXPECT_EQ(m.triangleCount(), 12u);
    EXPECT_NEAR(m.signedVolume(), 1.0, 1e-12);
}

TEST(Loader, TetrahedronStlAndObjAgree) {
    Mesh s = loadMesh(kSamples + "/tetrahedron.stl");
    Mesh o = loadMesh(kSamples + "/tetrahedron.obj");
    EXPECT_EQ(s.vertexCount(), o.vertexCount());
    EXPECT_EQ(s.triangleCount(), o.triangleCount());
    EXPECT_NEAR(s.surfaceArea(), o.surfaceArea(), 1e-12);
    EXPECT_NEAR(s.signedVolume(), 1.0 / 6.0, 1e-12);
}

TEST(Loader, ObjQuadIsTriangulatedAndSlashTokensParse) {
    std::string p = writeTemp("quad.obj",
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nvn 0 0 1\nf 1/1/1 2/1/1 3/1/1 4/1/1\n");
    Mesh m = loadOBJ(p);
    EXPECT_EQ(m.triangleCount(), 2u);
    EXPECT_NEAR(m.surfaceArea(), 1.0, 1e-12);
    std::remove(p.c_str());
}

TEST(Loader, ObjNegativeIndices) {
    std::string p = writeTemp("neg.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
    Mesh m = loadOBJ(p);
    EXPECT_EQ(m.triangleCount(), 1u);
    EXPECT_NEAR(m.surfaceArea(), 0.5, 1e-12);
    std::remove(p.c_str());
}

TEST(Loader, ObjIndexOutOfRangeThrows) {
    std::string p = writeTemp("bad.obj", "v 0 0 0\nf 1 2 3\n");
    EXPECT_THROW(loadOBJ(p), std::runtime_error);
    std::remove(p.c_str());
}

TEST(Loader, MissingFileThrows) { EXPECT_THROW(loadMesh(kSamples + "/nope.stl"), std::runtime_error); }

TEST(Loader, UnsupportedExtensionThrows) { EXPECT_THROW(loadMesh("model.ply"), std::runtime_error); }

TEST(Loader, StlWithoutFacetsThrows) {
    std::string p = writeTemp("empty.stl", "not an stl\n");
    EXPECT_THROW(loadSTL(p), std::runtime_error);
    std::remove(p.c_str());
}

// ---- JSON report -------------------------------------------------------

TEST(Report, ClosedMeshReportsVolume) {
    std::string j = toJson(makeCube(), "cube.stl");
    EXPECT_NE(j.find("\"triangles\": 12"), std::string::npos);
    EXPECT_NE(j.find("\"watertight\": true"), std::string::npos);
    EXPECT_NE(j.find("\"surface_area\": 6"), std::string::npos);
    EXPECT_NE(j.find("\"volume\": 1\n"), std::string::npos);
}

TEST(Report, OpenMeshHasNullVolume) {
    Mesh m = loadSTL(kSamples + "/open_square.stl");
    std::string j = toJson(m, "open_square.stl");
    EXPECT_NE(j.find("\"watertight\": false"), std::string::npos);
    EXPECT_NE(j.find("\"volume\": null"), std::string::npos);
}

TEST(Report, EscapesFileName) { EXPECT_EQ(jsonEscape("a\"b\\c"), "a\\\"b\\\\c"); }
