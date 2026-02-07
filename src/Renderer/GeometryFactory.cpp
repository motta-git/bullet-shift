#include "GeometryFactory.h"

#include <vector>
#include <cmath>
#include <glm/glm.hpp>

namespace GeometryFactory {

std::unique_ptr<Mesh> createCube() {
    std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
    };

    std::vector<unsigned int> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> createSphere(int segments, int rings) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    for (int ring = 0; ring <= rings; ring++) {
    float theta = ring * PI / rings;
    float sinTheta = std::sin(theta);
    float cosTheta = std::cos(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi = seg * 2.0f * PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            Vertex vertex;
            vertex.Position = glm::vec3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            vertex.Normal = glm::normalize(vertex.Position);
            vertex.TexCoords = glm::vec2(static_cast<float>(seg) / segments,
                                         static_cast<float>(ring) / rings);

            vertices.push_back(vertex);
        }
    }

    for (int ring = 0; ring < rings; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int first = ring * (segments + 1) + seg;
            int second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> createTorus(float majorRadius, float minorRadius, int segments, int rings) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    for (int ring = 0; ring <= rings; ring++) {
    float theta = ring * 2.0f * PI / rings;
    float cosTheta = std::cos(theta);
    float sinTheta = std::sin(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi = seg * 2.0f * PI / segments;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            Vertex vertex;
            vertex.Position = glm::vec3(
                (majorRadius + minorRadius * cosPhi) * cosTheta,
                minorRadius * sinPhi,
                (majorRadius + minorRadius * cosPhi) * sinTheta
            );

            glm::vec3 center = glm::vec3(majorRadius * cosTheta, 0.0f, majorRadius * sinTheta);
            vertex.Normal = glm::normalize(vertex.Position - center);
            vertex.TexCoords = glm::vec2(static_cast<float>(seg) / segments,
                                         static_cast<float>(ring) / rings);

            vertices.push_back(vertex);
        }
    }

    for (int ring = 0; ring < rings; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int first = ring * (segments + 1) + seg;
            int second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> createPlane(float size) {
    float halfSize = size / 2.0f;

    std::vector<Vertex> vertices = {
        {{-halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {size, 0.0f}},
        {{ halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {size, size}},
        {{-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, size}}
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> createQuad() {
    std::vector<Vertex> vertices = {
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}
    };
    std::vector<unsigned int> indices = {
        0, 1, 2,
        0, 2, 3
    };
    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> createWeaponMesh() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addBox = [&](const glm::vec3& center, const glm::vec3& size) {
        glm::vec3 half = size * 0.5f;

        glm::vec3 corners[8] = {
            {center.x - half.x, center.y - half.y, center.z - half.z},
            {center.x + half.x, center.y - half.y, center.z - half.z},
            {center.x + half.x, center.y + half.y, center.z - half.z},
            {center.x - half.x, center.y + half.y, center.z - half.z},
            {center.x - half.x, center.y - half.y, center.z + half.z},
            {center.x + half.x, center.y - half.y, center.z + half.z},
            {center.x + half.x, center.y + half.y, center.z + half.z},
            {center.x - half.x, center.y + half.y, center.z + half.z}
        };

        auto pushFace = [&](int i0, int i1, int i2, int i3, const glm::vec3& normal) {
            unsigned int start = static_cast<unsigned int>(vertices.size());
            const glm::vec2 texCoords[4] = {
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f}
            };

            glm::vec3 positions[4] = {
                corners[i0],
                corners[i1],
                corners[i2],
                corners[i3]
            };

            for (int i = 0; i < 4; ++i) {
                Vertex vertex;
                vertex.Position = positions[i];
                vertex.Normal = normal;
                vertex.TexCoords = texCoords[i];
                vertices.push_back(vertex);
            }

            indices.push_back(start);
            indices.push_back(start + 1);
            indices.push_back(start + 2);
            indices.push_back(start + 2);
            indices.push_back(start + 3);
            indices.push_back(start);
        };

        pushFace(0, 1, 2, 3, glm::vec3(0.0f, 0.0f, -1.0f));
        pushFace(4, 5, 6, 7, glm::vec3(0.0f, 0.0f, 1.0f));
        pushFace(7, 3, 0, 4, glm::vec3(-1.0f, 0.0f, 0.0f));
        pushFace(1, 2, 6, 5, glm::vec3(1.0f, 0.0f, 0.0f));
        pushFace(0, 1, 5, 4, glm::vec3(0.0f, -1.0f, 0.0f));
        pushFace(3, 2, 6, 7, glm::vec3(0.0f, 1.0f, 0.0f));
    };

    // Pistol oriented along Z-axis (forward)
    // Slide (top part)
    addBox(glm::vec3(0.0f, 0.15f, 0.0f), glm::vec3(0.15f, 0.12f, 0.8f));
    
    // Barrel (extends forward)
    addBox(glm::vec3(0.0f, 0.15f, 0.5f), glm::vec3(0.08f, 0.08f, 0.3f));
    
    // Frame (lower receiver)
    addBox(glm::vec3(0.0f, 0.0f, -0.1f), glm::vec3(0.14f, 0.18f, 0.5f));
    
    // Grip (handle)
    addBox(glm::vec3(0.0f, -0.2f, -0.3f), glm::vec3(0.12f, 0.35f, 0.2f));
    
    // Trigger guard
    addBox(glm::vec3(0.0f, -0.05f, -0.05f), glm::vec3(0.16f, 0.02f, 0.15f));
    
    // Magazine (mag well)
    addBox(glm::vec3(0.0f, -0.3f, -0.25f), glm::vec3(0.09f, 0.25f, 0.15f));

    return std::make_unique<Mesh>(vertices, indices);
}

} // namespace GeometryFactory
