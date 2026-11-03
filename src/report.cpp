#include "mesh/report.hpp"

#include <cstdio>
#include <sstream>

namespace mesh {

namespace {

std::string num(double d) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.9g", d);
    return buf;
}

std::string vec(const Vertex& v) { return "[" + num(v.x) + ", " + num(v.y) + ", " + num(v.z) + "]"; }

}  // namespace

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

std::string toJson(const Mesh& mesh, const std::string& file) {
    EdgeStats es = classifyEdges(mesh.edgeAdjacency());
    BoundingBox box = mesh.boundingBox();
    bool closed = es.watertight();

    std::ostringstream o;
    o << "{\n";
    o << "  \"file\": \"" << jsonEscape(file) << "\",\n";
    o << "  \"vertices\": " << mesh.vertexCount() << ",\n";
    o << "  \"triangles\": " << mesh.triangleCount() << ",\n";
    o << "  \"edges\": " << es.edges << ",\n";
    o << "  \"boundary_edges\": " << es.boundary << ",\n";
    o << "  \"non_manifold_edges\": " << es.nonManifold << ",\n";
    o << "  \"watertight\": " << (closed ? "true" : "false") << ",\n";
    o << "  \"bbox_min\": " << vec(box.min) << ",\n";
    o << "  \"bbox_max\": " << vec(box.max) << ",\n";
    o << "  \"surface_area\": " << num(mesh.surfaceArea()) << ",\n";
    o << "  \"volume\": ";
    if (closed) {
        double v = mesh.signedVolume();
        o << num(v < 0 ? -v : v);
    } else {
        o << "null";
    }
    o << "\n}\n";
    return o.str();
}

}  // namespace mesh
