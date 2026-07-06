#include "body_generator.hpp"
#include <iostream>

int main() {
    BodyGenerator generator;
    // Example parameters: height, weight, armLength, legLength
    Mesh body = generator.generateBody(1.75f, 70.0f, 0.7f, 0.9f);
    std::cout << "Generated body with " << body.vertices.size() << " vertices and " << body.faces.size() << " faces.\n";
    return 0;
}
