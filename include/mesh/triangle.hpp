#pragma once

#include <array>
#include <cstddef>

namespace mesh {

// A triangle stores indices into the owning Mesh's vertex list.
struct Triangle {
    std::array<std::size_t, 3> v{};

    Triangle() = default;
    Triangle(std::size_t a, std::size_t b, std::size_t c) : v{a, b, c} {}
};

}  // namespace mesh
