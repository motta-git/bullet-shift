#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Game;

struct SceneObject {
    std::string name;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec3 size;
    bool hasMesh;
};

class LevelManager {
public:
    LevelManager(Game& game);
    ~LevelManager();

    bool loadLevel(int levelIndex);
    bool levelExists(int levelIndex);

    const std::vector<std::unique_ptr<Mesh>>& getLevelMeshes() const { return m_levelMeshes; }
    const std::vector<glm::mat4>& getLevelMeshTransforms() const { return m_levelMeshTransforms; }

private:
    void loadHardcodedFallback();
    void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    void handleObject(const SceneObject& obj, aiNode* node, const aiScene* scene, const glm::mat4& transform);
    void resolveSpawns();

    Game& m_game;
    std::string m_currentLevelPath;
    std::vector<std::unique_ptr<Mesh>> m_levelMeshes;
    std::vector<glm::mat4> m_levelMeshTransforms;
    std::vector<SceneObject> m_pendingSpawns;
};
