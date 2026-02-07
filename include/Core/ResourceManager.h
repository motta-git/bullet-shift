#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include "Shader.h"
#include "Mesh.h"

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    // Shader management
    Shader* loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    Shader* getShader(const std::string& name);

    // Mesh management
    void addMesh(const std::string& name, std::unique_ptr<Mesh> mesh);
    Mesh* getMesh(const std::string& name);

    // Weapon Meshes (special case for multi-mesh objects)
    void addWeaponMeshes(const std::string& name, std::vector<std::unique_ptr<Mesh>> meshes);
    const std::vector<std::unique_ptr<Mesh>>* getWeaponMeshes(const std::string& name);

    void clear();

private:
    std::map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::map<std::string, std::unique_ptr<Mesh>> m_meshes;
    std::map<std::string, std::vector<std::unique_ptr<Mesh>>> m_weaponMeshes;
};
