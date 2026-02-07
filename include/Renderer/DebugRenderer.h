#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Shader.h"

struct DebugLine {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
    float lifetime;
    float timeLeft;
};

class DebugRenderer {
public:
    DebugRenderer();
    ~DebugRenderer();
    
    void addLine(glm::vec3 start, glm::vec3 end, glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f), 
                 float lifetime = 0.1f);
    
    void update(float deltaTime);
    void render(const glm::mat4& projection, const glm::mat4& view);
    
private:
    void initializeRenderData();
    
    std::vector<DebugLine> lines;
    unsigned int VAO, VBO;
    Shader* lineShader;
};
