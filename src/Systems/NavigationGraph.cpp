#include "Systems/NavigationGraph.h"
#include "Entities/Platform.h"
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <limits>

NavigationGraph::NavigationGraph() {}

void NavigationGraph::buildFromPlatforms(const std::vector<Platform>& platforms) {
    nodes.clear();
    edges.clear();
    
    if (platforms.empty()) {
        return;
    }
    
    // Create a node for each platform at its center
    for (size_t i = 0; i < platforms.size(); ++i) {
        nodes.emplace_back(platforms[i].getPosition(), static_cast<int>(i));
    }
    
    // Create edges between nearby platforms
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            float distance = glm::distance(nodes[i].position, nodes[j].position);
            
            // Only connect platforms within walking distance
            if (distance <= MAX_WALK_DISTANCE) {
                edges.emplace_back(i, j, distance);
                edges.emplace_back(j, i, distance); // Bidirectional
            }
        }
    }
}

std::vector<glm::vec3> NavigationGraph::findPath(const glm::vec3& start, const glm::vec3& goal) const {
    if (nodes.empty()) {
        return {};
    }
    
    int startNode = getClosestNode(start);
    int goalNode = getClosestNode(goal);
    
    if (startNode == -1 || goalNode == -1) {
        return {};
    }
    
    // If already at goal, return empty path
    if (startNode == goalNode) {
        return {goal};
    }
    
    // A* algorithm
    std::priority_queue<
        std::pair<float, int>,
        std::vector<std::pair<float, int>>,
        std::greater<std::pair<float, int>>
    > openSet;
    
    std::unordered_map<int, float> gScore;
    std::unordered_map<int, float> fScore;
    std::unordered_map<int, int> cameFrom;
    
    // Initialize scores
    for (size_t i = 0; i < nodes.size(); ++i) {
        gScore[i] = std::numeric_limits<float>::infinity();
        fScore[i] = std::numeric_limits<float>::infinity();
    }
    
    gScore[startNode] = 0.0f;
    fScore[startNode] = heuristic(nodes[startNode].position, nodes[goalNode].position);
    openSet.push({fScore[startNode], startNode});
    
    while (!openSet.empty()) {
        int current = openSet.top().second;
        openSet.pop();
        
        if (current == goalNode) {
            // Reconstruct path
            std::vector<int> pathIndices;
            pathIndices.push_back(current);
            while (cameFrom.find(current) != cameFrom.end()) {
                current = cameFrom[current];
                pathIndices.push_back(current);
            }
            std::reverse(pathIndices.begin(), pathIndices.end());
            
            // Convert to positions
            std::vector<glm::vec3> path;
            for (int idx : pathIndices) {
                path.push_back(nodes[idx].position);
            }
            // Add final goal position
            path.push_back(goal);
            return path;
        }
        
        // Check all neighbors
        for (const NavEdge& edge : edges) {
            if (edge.fromNode != current) continue;
            
            int neighbor = edge.toNode;
            float tentativeGScore = gScore[current] + edge.cost;
            
            if (tentativeGScore < gScore[neighbor]) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeGScore;
                fScore[neighbor] = gScore[neighbor] + heuristic(nodes[neighbor].position, nodes[goalNode].position);
                openSet.push({fScore[neighbor], neighbor});
            }
        }
    }
    
    // No path found
    return {};
}

int NavigationGraph::getClosestNode(const glm::vec3& position) const {
    if (nodes.empty()) {
        return -1;
    }
    
    int closestIndex = 0;
    float closestDistSq = glm::distance2(position, nodes[0].position);
    
    for (size_t i = 1; i < nodes.size(); ++i) {
        float distSq = glm::distance2(position, nodes[i].position);
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestIndex = i;
        }
    }
    
    return closestIndex;
}

float NavigationGraph::heuristic(const glm::vec3& a, const glm::vec3& b) const {
    return glm::distance(a, b);
}

std::vector<int> NavigationGraph::getNeighbors(int nodeIndex) const {
    std::vector<int> neighbors;
    for (const NavEdge& edge : edges) {
        if (edge.fromNode == nodeIndex) {
            neighbors.push_back(edge.toNode);
        }
    }
    return neighbors;
}

std::vector<glm::vec3> NavigationGraph::reconstructPath(const std::vector<int>& cameFrom, int current) const {
    std::vector<glm::vec3> path;
    // This helper is not currently used but kept for potential future use
    return path;
}
