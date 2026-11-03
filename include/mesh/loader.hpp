#pragma once

#include <string>

#include "mesh/mesh.hpp"

namespace mesh {

// Loaders throw std::runtime_error on unreadable or malformed input.
Mesh loadSTL(const std::string& path);  // ASCII STL only
Mesh loadOBJ(const std::string& path);
// Picks the loader from the file extension (.stl / .obj, case-insensitive).
Mesh loadMesh(const std::string& path);

}  // namespace mesh
