#pragma once

#include <string>
#include <vector>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.h"

class ModelLoader {
public:
    static std::vector<std::unique_ptr<Mesh>> loadModel(const std::string& path);
    static std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
private:
    static void processNode(aiNode* node, const aiScene* scene, std::vector<std::unique_ptr<Mesh>>& meshes);
};
