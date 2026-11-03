#include "mesh/loader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mesh {

namespace {

std::ifstream openOrThrow(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open file: " + path);
    return in;
}

}  // namespace

Mesh loadSTL(const std::string& path) {
    std::ifstream in = openOrThrow(path);
    Mesh mesh;
    std::vector<std::size_t> pending;
    std::string line;
    bool sawFacet = false;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string word;
        ls >> word;
        if (word == "facet") {
            sawFacet = true;
            pending.clear();
        } else if (word == "vertex") {
            double x, y, z;
            if (!(ls >> x >> y >> z)) throw std::runtime_error("malformed vertex line in " + path);
            pending.push_back(mesh.addVertex(Vertex(x, y, z)));
        } else if (word == "endfacet") {
            if (pending.size() != 3) throw std::runtime_error("facet without 3 vertices in " + path);
            mesh.addTriangle(pending[0], pending[1], pending[2]);
            pending.clear();
        }
    }
    if (!sawFacet) throw std::runtime_error("no facets found (binary STL is not supported): " + path);
    return mesh;
}

Mesh loadOBJ(const std::string& path) {
    std::ifstream in = openOrThrow(path);
    Mesh mesh;
    // OBJ vertices are shared by index, but welding by coordinate keeps the
    // adjacency consistent if the file repeats a position.
    std::vector<std::size_t> remap;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string word;
        ls >> word;
        if (word == "v") {
            double x, y, z;
            if (!(ls >> x >> y >> z)) throw std::runtime_error("malformed v line in " + path);
            remap.push_back(mesh.addVertex(Vertex(x, y, z)));
        } else if (word == "f") {
            std::vector<std::size_t> idx;
            std::string tok;
            while (ls >> tok) {
                // Token forms: i, i/t, i//n, i/t/n; negative i counts from the end.
                long i = std::stol(tok.substr(0, tok.find('/')));
                long n = static_cast<long>(remap.size());
                if (i < 0) i = n + i + 1;
                if (i < 1 || i > n) throw std::runtime_error("face index out of range in " + path);
                idx.push_back(remap[static_cast<std::size_t>(i - 1)]);
            }
            if (idx.size() < 3) throw std::runtime_error("face with fewer than 3 vertices in " + path);
            for (std::size_t k = 1; k + 1 < idx.size(); ++k)  // triangle fan
                mesh.addTriangle(idx[0], idx[k], idx[k + 1]);
        }
    }
    return mesh;
}

Mesh loadMesh(const std::string& path) {
    auto dot = path.find_last_of('.');
    std::string ext = dot == std::string::npos ? "" : path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (ext == ".stl") return loadSTL(path);
    if (ext == ".obj") return loadOBJ(path);
    throw std::runtime_error("unsupported file type (expected .stl or .obj): " + path);
}

}  // namespace mesh
