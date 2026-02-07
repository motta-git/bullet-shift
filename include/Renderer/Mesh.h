#pragma once

#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;
    
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    ~Mesh();
    
    void draw() const;
    
private:
    unsigned int VBO, EBO;
    
    void setupMesh();
};
