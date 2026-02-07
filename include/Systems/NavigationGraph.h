#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Platform;

struct NavNode {
    glm::vec3 position;
    int platformIndex;
    
    NavNode(const glm::vec3& pos, int platformIdx)
        : position(pos), platformIndex(platformIdx) {}
};

struct NavEdge {
    int fromNode;
    int toNode;
    float cost;
    
    NavEdge(int from, int to, float c)
        : fromNode(from), toNode(to), cost(c) {}
};

class NavigationGraph {
public:
    NavigationGraph();
    
    // Build the navigation graph from platforms
    void buildFromPlatforms(const std::vector<Platform>& platforms);
    
    // Find path using A* algorithm
    std::vector<glm::vec3> findPath(const glm::vec3& start, const glm::vec3& goal) const;
    
    // Get the closest node to a position
    int getClosestNode(const glm::vec3& position) const;
    
    // Check if graph is valid
    bool isValid() const { return !nodes.empty(); }
    
    // Get all nodes (for debug visualization)
    const std::vector<NavNode>& getNodes() const { return nodes; }
    
    // Get all edges (for debug visualization)
    const std::vector<NavEdge>& getEdges() const { return edges; }
    
private:
    std::vector<NavNode> nodes;
    std::vector<NavEdge> edges;
    
    // Maximum distance for walkable connections
    static constexpr float MAX_WALK_DISTANCE = 8.0f;
    
    // A* helper functions
    float heuristic(const glm::vec3& a, const glm::vec3& b) const;
    std::vector<int> getNeighbors(int nodeIndex) const;
    std::vector<glm::vec3> reconstructPath(const std::vector<int>& cameFrom, int current) const;
};
