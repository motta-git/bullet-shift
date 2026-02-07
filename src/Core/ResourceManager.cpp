#include "ResourceManager.h"
#include <iostream>

ResourceManager::ResourceManager() {}

ResourceManager::~ResourceManager() {
    clear();
}

Shader* ResourceManager::loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath) {
    auto shader = std::make_unique<Shader>(vertPath.c_str(), fragPath.c_str());
    Shader* ptr = shader.get();
    m_shaders[name] = std::move(shader);
    return ptr;
}

Shader* ResourceManager::getShader(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second.get();
    }
    std::cerr << "ResourceManager: Shader not found: " << name << std::endl;
    return nullptr;
}

void ResourceManager::addMesh(const std::string& name, std::unique_ptr<Mesh> mesh) {
    m_meshes[name] = std::move(mesh);
}

Mesh* ResourceManager::getMesh(const std::string& name) {
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        return it->second.get();
    }
    std::cerr << "ResourceManager: Mesh not found: " << name << std::endl;
    return nullptr;
}

void ResourceManager::addWeaponMeshes(const std::string& name, std::vector<std::unique_ptr<Mesh>> meshes) {
    m_weaponMeshes[name] = std::move(meshes);
}

const std::vector<std::unique_ptr<Mesh>>* ResourceManager::getWeaponMeshes(const std::string& name) {
    auto it = m_weaponMeshes.find(name);
    if (it != m_weaponMeshes.end()) {
        return &it->second;
    }
    return nullptr;
}

void ResourceManager::clear() {
    m_shaders.clear();
    m_meshes.clear();
    m_weaponMeshes.clear();
}
