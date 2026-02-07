#include "Systems/RaycastUtility.h"
#include "Entities/Platform.h"
#include <algorithm>
#include <limits>

RaycastUtility::RaycastHit RaycastUtility::raycastPlatforms(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    const std::vector<Platform>& platforms
) {
    RaycastHit result;
    result.hit = false;
    result.distance = maxDistance;
    
    glm::vec3 rayDir = glm::normalize(direction);
    
    for (size_t i = 0; i < platforms.size(); ++i) {
        float t = platforms[i].raycast(origin, rayDir, result.distance);
        if (t >= 0.0f && t < result.distance) {
            result.hit = true;
            result.distance = t;
            result.point = origin + rayDir * t;
            result.platformIndex = static_cast<int>(i);
        }
    }
    
    return result;
}

bool RaycastUtility::hasLineOfSight(
    const glm::vec3& from,
    const glm::vec3& to,
    const std::vector<Platform>& platforms
) {
    glm::vec3 direction = to - from;
    float distance = glm::length(direction);
    
    if (distance < 0.001f) {
        return true;
    }
    
    RaycastHit hit = raycastPlatforms(from, direction, distance, platforms);
    return !hit.hit;
}

bool RaycastUtility::rayAABBIntersection(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,
    const glm::vec3& boxMin,
    const glm::vec3& boxMax,
    float& tMin,
    float& tMax
) {
    tMin = 0.0f;
    tMax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDir[i]) < 0.0001f) {
            // Ray is parallel to slab
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i]) {
                return false;
            }
        } else {
            float ood = 1.0f / rayDir[i];
            float t1 = (boxMin[i] - rayOrigin[i]) * ood;
            float t2 = (boxMax[i] - rayOrigin[i]) * ood;
            
            if (t1 > t2) std::swap(t1, t2);
            
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            
            if (tMin > tMax) {
                return false;
            }
        }
    }
    
    return true;
}

bool RaycastUtility::rayTriangleIntersection(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    float& t
) {
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1, edge2, h, s, q;
    float a, f, u, v;

    edge1 = v1 - v0;
    edge2 = v2 - v0;
    h = glm::cross(rayDir, edge2);
    a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false;

    f = 1.0f / a;
    s = rayOrigin - v0;
    u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    q = glm::cross(s, edge1);
    v = f * glm::dot(rayDir, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * glm::dot(edge2, q);

    if (t > EPSILON) return true;
    else return false;
}
