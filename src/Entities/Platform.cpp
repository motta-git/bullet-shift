#include "Platform.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <string>
#include <cctype>

#include "Renderer/Mesh.h"
#include "Systems/RaycastUtility.h"

Platform::Platform(glm::vec3 position, glm::vec3 size, const Mesh* mesh, const glm::mat4& transform, const std::string& name)
    : position(position), size(size), m_transform(transform), m_name(name) {
    
    if (mesh) {
        m_meshes.push_back(mesh);
    }
    
    // Detect if this platform is intended to be ground/floor
    // This helps decide whether to skip AABB side-collisions (invisible walls)
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    m_isFloor = (upperName.find("FLOOR") != std::string::npos || 
                 upperName.find("GROUND") != std::string::npos || 
                 upperName.find("RAMP") != std::string::npos ||
                 size.x > 10.0f || size.z > 10.0f); // Robust fallback for huge surfaces
}

float Platform::getSurfaceHeight(glm::vec3 xzPos, float currentY) const {
    if (m_meshes.empty()) {
        return position.y + size.y / 2.0f;
    }

    // Raycast from slightly above the player to find the exact floor height
    glm::vec3 rayOrigin(xzPos.x, currentY + 2.0f, xzPos.z);
    glm::vec3 rayDir(0.0f, -1.0f, 0.0f);

    // Transform ray to model space for intersection
    glm::mat4 invTransform = glm::inverse(m_transform);
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(rayOrigin, 1.0f));
    glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(rayDir, 0.0f));

    float closestT = std::numeric_limits<float>::max();
    bool hit = false;

    for (const Mesh* m_mesh : m_meshes) {
        if (!m_mesh) continue;
        const std::vector<Vertex>& verts = m_mesh->vertices;
        const std::vector<unsigned int>& indices = m_mesh->indices;
        
        if (indices.empty() || verts.empty()) continue;

        for (size_t i = 0; i < indices.size(); i += 3) {
            if (i + 2 >= indices.size()) break;
            
            if (indices[i] >= verts.size() || indices[i+1] >= verts.size() || indices[i+2] >= verts.size()) {
                 // std::cerr << "Mesh collision index out of bounds!" << std::endl;
                 continue;
            }

            glm::vec3 v0 = verts[indices[i]].Position;
            glm::vec3 v1 = verts[indices[i+1]].Position;
            glm::vec3 v2 = verts[indices[i+2]].Position;

            float t;
            if (RaycastUtility::rayTriangleIntersection(localOrigin, localDir, v0, v1, v2, t)) {
                if (t < closestT) {
                    closestT = t;
                    hit = true;
                }
            }
        }
    }

    if (hit) {
        glm::vec3 localHit = localOrigin + localDir * closestT;
        glm::vec3 worldHit = glm::vec3(m_transform * glm::vec4(localHit, 1.0f));
        return worldHit.y;
    }

    // If no mesh hit (e.g. off the edge of the geometry but inside AABB), fall back to AABB bottom to avoid sticking? 
    // Or return very low to ignore.
    return -1000.0f; 
}

bool Platform::checkCollision(glm::vec3& playerPos, glm::vec3 playerSize, glm::vec3& playerVelocity) {
    // 1. Broad Phase (AABB check)
    bool collisionX = playerPos.x + playerSize.x / 2.0f >= position.x - size.x / 2.0f &&
                      position.x + size.x / 2.0f >= playerPos.x - playerSize.x / 2.0f;
    
    bool collisionY = playerPos.y + playerSize.y / 2.0f >= position.y - size.y / 2.0f &&
                      position.y + size.y / 2.0f >= playerPos.y - playerSize.y / 2.0f;
    
    bool collisionZ = playerPos.z + playerSize.z / 2.0f >= position.z - size.z / 2.0f &&
                      position.z + size.z / 2.0f >= playerPos.z - playerSize.z / 2.0f;
    
    if (!collisionX || !collisionY || !collisionZ) {
        return false;
    }

    // 2. Narrow Phase
    if (!m_meshes.empty()) {
        // Find the precise mesh surface at the player's XZ location
        float exactHeight = getSurfaceHeight(playerPos, playerPos.y);
        
        if (exactHeight > -900.0f) {
            float playerBottom = playerPos.y - playerSize.y / 2.0f;
            const float STEP_HEIGHT = 0.5f;
            const float PENETRATION_THRESHOLD = 1.0f;
            
            // Ground Snapping / Resolution
            if (playerBottom <= exactHeight + STEP_HEIGHT && playerBottom >= exactHeight - PENETRATION_THRESHOLD) {
                // Return true to indicate we are grounded on this mesh surface
                if (playerVelocity.y <= 0.1f) {
                    playerPos.y = exactHeight + playerSize.y / 2.0f;
                    playerVelocity.y = std::max(0.0f, playerVelocity.y);
                }
                return true; 
            }

            // If we are significantly BELOW the mesh surface, treat it as a solid obstacle (e.g. Columns)
            // unless we've explicitly marked it as a floor-only model.
            if (!m_isFloor && playerBottom < exactHeight - STEP_HEIGHT) {
                // Fall through to AABB resolution below
            } else {
                return false; 
            }
        } else {
            // Ray missed the mesh.
            // For floor models, prefer not to create invisible walls, but if the player's
            // bottom is very close to the platform AABB top, conservatively snap to it
            // to avoid falling through thin floor geometry.
            if (m_isFloor) {
                float playerBottom = playerPos.y - playerSize.y / 2.0f;
                float aabbTop = position.y + size.y / 2.0f;
                const float STEP_HEIGHT = 0.5f;
                const float PENETRATION_THRESHOLD = 1.0f;
                if (playerBottom <= aabbTop + STEP_HEIGHT && playerBottom >= aabbTop - PENETRATION_THRESHOLD) {
                    if (playerVelocity.y <= 0.1f) {
                        playerPos.y = aabbTop + playerSize.y / 2.0f;
                        playerVelocity.y = std::max(0.0f, playerVelocity.y);
                    }
                    return true;
                }
                return false;
            }
        }
    }

    // 3. Fallback: Standard AABB resolution (for simple platforms without meshes)
    float overlapX = std::min(playerPos.x + playerSize.x / 2.0f - (position.x - size.x / 2.0f),
                                position.x + size.x / 2.0f - (playerPos.x - playerSize.x / 2.0f));
    float overlapY = std::min(playerPos.y + playerSize.y / 2.0f - (position.y - size.y / 2.0f),
                                position.y + size.y / 2.0f - (playerPos.y - playerSize.y / 2.0f));
    float overlapZ = std::min(playerPos.z + playerSize.z / 2.0f - (position.z - size.z / 2.0f),
                                position.z + size.z / 2.0f - (playerPos.z - playerSize.z / 2.0f));
    
    if (overlapY < overlapX && overlapY < overlapZ) {
        if (playerPos.y > position.y) { 
            playerPos.y = position.y + size.y / 2.0f + playerSize.y / 2.0f;
            playerVelocity.y = std::max(0.0f, playerVelocity.y);
            return true; // GROUNDED
        } else {
            playerPos.y = position.y - size.y / 2.0f - playerSize.y / 2.0f;
            playerVelocity.y = std::min(0.0f, playerVelocity.y);
            return false;
        }
    } else if (overlapX < overlapZ) {
        float sign = (playerPos.x > position.x) ? 1.0f : -1.0f;
        playerPos.x = position.x + (size.x * 0.5f + playerSize.x * 0.5f) * sign;
        playerVelocity.x = 0.0f;
    } else {
        float sign = (playerPos.z > position.z) ? 1.0f : -1.0f;
        playerPos.z = position.z + (size.z * 0.5f + playerSize.z * 0.5f) * sign;
        playerVelocity.z = 0.0f;
    }
    
    return false;
}

bool Platform::checkRayCollision(const glm::vec3& start, const glm::vec3& end) const {
    glm::vec3 dir = end - start;
    float dist = glm::length(dir);
    if (dist < 0.0001f) return false;
    dir /= dist;

    // 1. Broad Phase (Ray-AABB)
    float tMin, tMax;
    glm::vec3 boxMin = position - size * 0.5f;
    glm::vec3 boxMax = position + size * 0.5f;
    
    if (!RaycastUtility::rayAABBIntersection(start, dir, boxMin, boxMax, tMin, tMax)) {
        return false;
    }

    // Intersection must be within the segment distance
    if (tMin > dist) return false;

    // 2. Narrow Phase (Ray-Mesh)
    if (m_meshes.empty()) {
        // For standard platforms, AABB hit is enough
        return true;
    }

    // Transform ray to model space
    glm::mat4 invTransform = glm::inverse(m_transform);
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(start, 1.0f));
    glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(dir, 0.0f));
    float localDist = glm::length(glm::vec3(invTransform * glm::vec4(dir * dist, 0.0f)));
    localDir = glm::normalize(localDir);

    for (const Mesh* m_mesh : m_meshes) {
        if (!m_mesh) continue;
        const std::vector<Vertex>& verts = m_mesh->vertices;
        const std::vector<unsigned int>& indices = m_mesh->indices;

        for (size_t i = 0; i < indices.size(); i += 3) {
            if (i + 2 >= indices.size()) break;
            
            if (indices[i] >= verts.size() || indices[i+1] >= verts.size() || indices[i+2] >= verts.size()) continue;

            glm::vec3 v0 = verts[indices[i]].Position;
            glm::vec3 v1 = verts[indices[i+1]].Position;
            glm::vec3 v2 = verts[indices[i+2]].Position;

            float hitT;
            if (RaycastUtility::rayTriangleIntersection(localOrigin, localDir, v0, v1, v2, hitT)) {
                if (hitT <= localDist) return true;
            }
        }
    }

    return false;
}

float Platform::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    // 1. Broad Phase (Ray-AABB)
    float tMin, tMax;
    glm::vec3 boxMin = position - size * 0.5f;
    glm::vec3 boxMax = position + size * 0.5f;
    
    if (!RaycastUtility::rayAABBIntersection(origin, direction, boxMin, boxMax, tMin, tMax)) {
        return -1.0f;
    }

    if (tMin > maxDistance) return -1.0f;

    // 2. Narrow Phase (Ray-Mesh)
    if (m_meshes.empty()) {
        return (tMin >= 0.0f) ? tMin : 0.0f;
    }

    // Transform ray to model space
    glm::mat4 invTransform = glm::inverse(m_transform);
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(origin, 1.0f));
    glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(direction, 0.0f));
    float scale = glm::length(localDir);
    if (scale > 0.0001f) localDir /= scale;

    float closestT = std::numeric_limits<float>::max();
    bool hit = false;

    for (const Mesh* m_mesh : m_meshes) {
        if (!m_mesh) continue;
        const std::vector<Vertex>& verts = m_mesh->vertices;
        const std::vector<unsigned int>& indices = m_mesh->indices;

        for (size_t i = 0; i < indices.size(); i += 3) {
            if (i + 2 >= indices.size()) break;
            if (indices[i] >= verts.size() || indices[i+1] >= verts.size() || indices[i+2] >= verts.size()) continue;

            float t;
            if (RaycastUtility::rayTriangleIntersection(localOrigin, localDir, verts[indices[i]].Position, 
                                                       verts[indices[i+1]].Position, verts[indices[i+2]].Position, t)) {
                if (t < closestT) {
                    closestT = t;
                    hit = true;
                }
            }
        }
    }

    if (hit) {
        float worldT = closestT / scale;
        if (worldT <= maxDistance) return worldT;
    }

    return -1.0f;
}
