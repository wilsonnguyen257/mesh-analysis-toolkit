#include <cstdio>
#include <exception>
#include <iostream>

#include "mesh/loader.hpp"
#include "mesh/report.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: meshtool <file.stl|file.obj>\n");
        return 2;
    }
    try {
        mesh::Mesh m = mesh::loadMesh(argv[1]);
        std::cout << mesh::toJson(m, argv[1]);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "meshtool: %s\n", e.what());
        return 1;
    }
    return 0;
}
