#pragma once

#include <glm/glm.hpp>
#include <vector>

class Platform;
class Mesh;

class RaycastUtility {
public:
    // Raycast result structure
    struct RaycastHit {
        bool hit;
        glm::vec3 point;
        float distance;
        int platformIndex;
        
        RaycastHit() : hit(false), point(0.0f), distance(0.0f), platformIndex(-1) {}
    };
    
    // Cast a ray and check for platform intersection
    static RaycastHit raycastPlatforms(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        const std::vector<Platform>& platforms
    );
    
    // Check if line of sight is clear between two points
    static bool hasLineOfSight(
        const glm::vec3& from,
        const glm::vec3& to,
        const std::vector<Platform>& platforms
    );
    
public:
    // Ray-AABB intersection test
    static bool rayAABBIntersection(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const glm::vec3& boxMin,
        const glm::vec3& boxMax,
        float& tMin,
        float& tMax
    );

    // Ray-Triangle intersection
    static bool rayTriangleIntersection(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& t
    );
};
