#include "body_generator.hpp"
#include <cmath>

BodyGenerator::BodyGenerator() {}

Mesh BodyGenerator::generateBody(float height, float weight, float armLength, float legLength) {
    Mesh mesh;
    // Placeholder: create a simple stick figure (torso, arms, legs, head)
    // In a real app, replace with parametric or mesh-based modeling
    float torsoHeight = height * 0.4f;
    float legHeight = legLength;
    float armSpan = armLength * 2.0f;
    float headRadius = height * 0.1f;

    // Torso (vertical line)
    mesh.vertices.push_back({0, 0, 0}); // pelvis
    mesh.vertices.push_back({0, torsoHeight, 0}); // neck base

    // Head (single vertex for now)
    mesh.vertices.push_back({0, torsoHeight + headRadius, 0});

    // Arms (horizontal line at neck base)
    mesh.vertices.push_back({-armSpan/2, torsoHeight, 0}); // left hand
    mesh.vertices.push_back({armSpan/2, torsoHeight, 0}); // right hand

    // Legs (vertical lines from pelvis)
    mesh.vertices.push_back({-0.1f, -legHeight, 0}); // left foot
    mesh.vertices.push_back({0.1f, -legHeight, 0}); // right foot

    // Faces: just connect as lines for now (not real triangles)
    // In a real mesh, faces would be triangles/quads
    // This is a placeholder for demonstration
    return mesh;
}
