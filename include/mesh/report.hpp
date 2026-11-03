#pragma once

#include <string>

#include "mesh/mesh.hpp"

namespace mesh {

// Serialises the analysis of a mesh as a single JSON object.
// Volume is null when the mesh is not watertight.
std::string toJson(const Mesh& mesh, const std::string& file);

std::string jsonEscape(const std::string& s);

}  // namespace mesh
