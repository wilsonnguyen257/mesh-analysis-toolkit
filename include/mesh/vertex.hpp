#pragma once

#include <cmath>
#include <cstddef>
#include <functional>

namespace mesh {

struct Vertex {
    double x = 0, y = 0, z = 0;

    Vertex() = default;
    Vertex(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vertex operator-(const Vertex& o) const { return {x - o.x, y - o.y, z - o.z}; }
    bool operator==(const Vertex& o) const { return x == o.x && y == o.y && z == o.z; }

    double dot(const Vertex& o) const { return x * o.x + y * o.y + z * o.z; }
    Vertex cross(const Vertex& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    double length() const { return std::sqrt(dot(*this)); }
};

struct VertexHash {
    std::size_t operator()(const Vertex& v) const {
        std::hash<double> h;
        std::size_t seed = h(v.x);
        seed ^= h(v.y) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        seed ^= h(v.z) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }
};

}  // namespace mesh
