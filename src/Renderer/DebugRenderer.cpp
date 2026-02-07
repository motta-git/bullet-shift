#include "DebugRenderer.h"

DebugRenderer::DebugRenderer() {
    // Load line shader
    lineShader = new Shader("shaders/debug_line.vert", "shaders/debug_line.frag");
    initializeRenderData();
}

DebugRenderer::~DebugRenderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    delete lineShader;
}

void DebugRenderer::initializeRenderData() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Allocate space for line vertices (2 vertices per line, 3 floats per vertex)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 1000, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void DebugRenderer::addLine(glm::vec3 start, glm::vec3 end, glm::vec3 color, float lifetime) {
    DebugLine line;
    line.start = start;
    line.end = end;
    line.color = color;
    line.lifetime = lifetime;
    line.timeLeft = lifetime;
    lines.push_back(line);
}

void DebugRenderer::update(float deltaTime) {
    // Update line lifetimes and remove expired lines
    for (auto it = lines.begin(); it != lines.end();) {
        it->timeLeft -= deltaTime;
        if (it->timeLeft <= 0.0f) {
            it = lines.erase(it);
        } else {
            ++it;
        }
    }
}

void DebugRenderer::render(const glm::mat4& projection, const glm::mat4& view) {
    if (lines.empty()) return;
    
    // Prepare line vertices
    std::vector<float> vertices;
    for (const auto& line : lines) {
        vertices.push_back(line.start.x);
        vertices.push_back(line.start.y);
        vertices.push_back(line.start.z);
        
        vertices.push_back(line.end.x);
        vertices.push_back(line.end.y);
        vertices.push_back(line.end.z);
    }
    
    // Update VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Render lines
    lineShader->use();
    lineShader->setMat4("projection", projection);
    lineShader->setMat4("view", view);
    lineShader->setMat4("model", glm::mat4(1.0f));
    
    glBindVertexArray(VAO);
    glLineWidth(8.0f); // Thickened for better projectile visibility
    
    // Render each line with its color
    for (size_t i = 0; i < lines.size(); i++) {
        lineShader->setVec3("color", lines[i].color);
        glDrawArrays(GL_LINES, i * 2, 2);
    }
    
    glBindVertexArray(0);
}
