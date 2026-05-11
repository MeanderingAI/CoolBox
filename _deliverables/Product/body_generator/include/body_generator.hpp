#pragma once
#include <vector>
#include <array>

struct Vertex {
    float x, y, z;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::array<int, 3>> faces; // triangle indices
};

class BodyGenerator {
public:
    BodyGenerator();
    Mesh generateBody(float height, float weight, float armLength, float legLength);
};
