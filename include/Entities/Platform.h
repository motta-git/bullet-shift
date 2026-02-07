#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

class Mesh;

class Platform {
public:
    Platform(glm::vec3 position, glm::vec3 size, const Mesh* mesh = nullptr, const glm::mat4& transform = glm::mat4(1.0f), const std::string& name = "Platform");
    
    // Check collision with player - modifies player position to resolve penetration
    // Returns true if the player is grounded on this platform
    bool checkCollision(glm::vec3& playerPos, glm::vec3 playerSize, glm::vec3& playerVelocity);
    
    // Get precise surface height at given XZ position
    // Returns -1000.0f if no ground found vertically
    float getSurfaceHeight(glm::vec3 xzPos, float currentY) const;

    // Check collision with a ray segment (for projectiles)
    bool checkRayCollision(const glm::vec3& start, const glm::vec3& end) const;

    // Perform a raycast against this platform, returns distance to hit or -1.0f if no hit
    float raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const;
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getSize() const { return size; }
    const std::string& getName() const { return m_name; }
    bool isFloor() const { return m_isFloor; }
    bool hasMesh() const { return !m_meshes.empty(); }
    const std::vector<const Mesh*>& getMeshes() const { return m_meshes; }
    std::vector<const Mesh*>& getMeshes() { return m_meshes; }
    const glm::mat4& getTransform() const { return m_transform; }

private:
    glm::vec3 position;
    glm::vec3 size;
    std::vector<const Mesh*> m_meshes;
    glm::mat4 m_transform;
    std::string m_name;
    bool m_isFloor;
};
