#include "mesh/mesh.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mesh {

std::size_t Mesh::addVertex(const Vertex& v) {
    // x + 0.0 turns -0.0 into +0.0 so both weld to the same vertex.
    Vertex key(v.x + 0.0, v.y + 0.0, v.z + 0.0);
    auto it = vertexIndex_.find(key);
    if (it != vertexIndex_.end()) return it->second;
    std::size_t idx = vertices_.size();
    vertices_.push_back(key);
    vertexIndex_.emplace(key, idx);
    return idx;
}

void Mesh::addTriangle(std::size_t a, std::size_t b, std::size_t c) {
    const std::size_t n = vertices_.size();
    if (a >= n || b >= n || c >= n) throw std::out_of_range("triangle vertex index out of range");
    triangles_.emplace_back(a, b, c);
}

BoundingBox Mesh::boundingBox() const {
    if (vertices_.empty()) throw std::runtime_error("bounding box of an empty mesh");
    BoundingBox box{vertices_.front(), vertices_.front()};
    for (const Vertex& p : vertices_) {
        box.min.x = std::min(box.min.x, p.x);
        box.min.y = std::min(box.min.y, p.y);
        box.min.z = std::min(box.min.z, p.z);
        box.max.x = std::max(box.max.x, p.x);
        box.max.y = std::max(box.max.y, p.y);
        box.max.z = std::max(box.max.z, p.z);
    }
    return box;
}

double Mesh::surfaceArea() const {
    double area = 0;
    for (const Triangle& t : triangles_) {
        const Vertex& a = vertices_[t.v[0]];
        const Vertex& b = vertices_[t.v[1]];
        const Vertex& c = vertices_[t.v[2]];
        area += 0.5 * (b - a).cross(c - a).length();
    }
    return area;
}

double Mesh::signedVolume() const {
    double vol = 0;
    for (const Triangle& t : triangles_) {
        const Vertex& a = vertices_[t.v[0]];
        const Vertex& b = vertices_[t.v[1]];
        const Vertex& c = vertices_[t.v[2]];
        vol += a.dot(b.cross(c)) / 6.0;
    }
    return vol;
}

EdgeMap Mesh::edgeAdjacency() const {
    EdgeMap map;
    map.reserve(triangles_.size() * 3 / 2 + 1);
    for (std::size_t i = 0; i < triangles_.size(); ++i) {
        const auto& v = triangles_[i].v;
        for (int k = 0; k < 3; ++k) {
            std::size_t a = v[k], b = v[(k + 1) % 3];
            if (a == b) continue;
            map[Edge(a, b)].push_back(i);
        }
    }
    return map;
}

EdgeStats classifyEdges(const EdgeMap& map) {
    EdgeStats s;
    s.edges = map.size();
    for (const auto& kv : map) {
        std::size_t uses = kv.second.size();
        if (uses == 1) ++s.boundary;
        else if (uses > 2) ++s.nonManifold;
    }
    return s;
}

}  // namespace mesh
