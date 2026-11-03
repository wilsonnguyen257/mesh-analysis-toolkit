#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

#include "mesh/triangle.hpp"
#include "mesh/vertex.hpp"

namespace mesh {

struct BoundingBox {
    Vertex min;
    Vertex max;
};

// Undirected edge, stored with the smaller vertex index first.
struct Edge {
    std::size_t a, b;
    Edge(std::size_t i, std::size_t j) : a(i < j ? i : j), b(i < j ? j : i) {}
    bool operator==(const Edge& o) const { return a == o.a && b == o.b; }
};

struct EdgeHash {
    std::size_t operator()(const Edge& e) const {
        return std::hash<std::size_t>()(e.a) * 1000003u ^ std::hash<std::size_t>()(e.b);
    }
};

// Edge -> indices of the triangles that use it.
using EdgeMap = std::unordered_map<Edge, std::vector<std::size_t>, EdgeHash>;

class Mesh {
public:
    // Adds a vertex, reusing an existing one with identical coordinates.
    std::size_t addVertex(const Vertex& v);
    // Throws std::out_of_range if an index does not refer to an existing vertex.
    void addTriangle(std::size_t a, std::size_t b, std::size_t c);

    const std::vector<Vertex>& vertices() const { return vertices_; }
    const std::vector<Triangle>& triangles() const { return triangles_; }

    std::size_t triangleCount() const { return triangles_.size(); }
    std::size_t vertexCount() const { return vertices_.size(); }

    // Throws std::runtime_error on an empty mesh.
    BoundingBox boundingBox() const;
    double surfaceArea() const;
    // Signed volume (divergence theorem); positive for outward-facing winding.
    // Only meaningful for closed meshes.
    double signedVolume() const;

    // Builds the edge -> triangles lookup. Degenerate edges (a == b) are skipped.
    EdgeMap edgeAdjacency() const;

private:
    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;
    std::unordered_map<Vertex, std::size_t, VertexHash> vertexIndex_;
};

struct EdgeStats {
    std::size_t edges = 0;
    std::size_t boundary = 0;     // used by exactly one triangle
    std::size_t nonManifold = 0;  // used by three or more triangles
    bool watertight() const { return edges > 0 && boundary == 0 && nonManifold == 0; }
};

EdgeStats classifyEdges(const EdgeMap& map);

}  // namespace mesh
