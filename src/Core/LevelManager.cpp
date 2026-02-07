#include "LevelManager.h"
#include "Game.h"
#include "ModelLoader.h"
#include "Settings.h"
#include <iostream>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>

// Helper to get readable key name from GLFW key code
static std::string getKeyNameStr(int keyCode) {
    // Try GLFW's native key name first (works for printable keys)
    const char* name = glfwGetKeyName(keyCode, 0);
    if (name) {
        std::string keyStr(name);
        // Uppercase first letter
        if (!keyStr.empty() && keyStr[0] >= 'a' && keyStr[0] <= 'z') {
            keyStr[0] -= 32;
        }
        return keyStr;
    }
    
    // Handle special keys manually
    switch (keyCode) {
        case GLFW_KEY_SPACE: return "Space";
        case GLFW_KEY_LEFT_SHIFT: return "Shift";
        case GLFW_KEY_RIGHT_SHIFT: return "Shift";
        case GLFW_KEY_LEFT_CONTROL: return "Ctrl";
        case GLFW_KEY_RIGHT_CONTROL: return "Ctrl";
        case GLFW_KEY_LEFT_ALT: return "Alt";
        case GLFW_KEY_RIGHT_ALT: return "Alt";
        case GLFW_KEY_TAB: return "Tab";
        case GLFW_KEY_ENTER: return "Enter";
        case GLFW_KEY_ESCAPE: return "Esc";
        default: return "Key";
    }
}

// Helper to convert aiMatrix4x4 to glm::mat4
glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
    glm::mat4 to;
    // Map Assimp row-major to GLM column-major correctly
    to[0][0] = from.a1; to[0][1] = from.b1; to[0][2] = from.c1; to[0][3] = from.d1;
    to[1][0] = from.a2; to[1][1] = from.b2; to[1][2] = from.c2; to[1][3] = from.d2;
    to[2][0] = from.a3; to[2][1] = from.b3; to[2][2] = from.c3; to[2][3] = from.d3;
    to[3][0] = from.a4; to[3][1] = from.b4; to[3][2] = from.c4; to[3][3] = from.d4;
    return to;
}

LevelManager::LevelManager(Game& game) : m_game(game) {}

LevelManager::~LevelManager() {}

bool LevelManager::loadLevel(int levelIndex) {
    m_currentLevelPath = "assets/levels/level_" + std::to_string(levelIndex) + ".glb";
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(m_currentLevelPath, 
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "LevelManager: Level file missing or invalid, using fallback: " << m_currentLevelPath << std::endl;
        loadHardcodedFallback();
        return true; 
    }

    // Clear existing world state
    m_game.platforms.clear();
    m_game.enemies.clear();
    m_game.player.reset();
    m_levelMeshes.clear();
    m_levelMeshTransforms.clear();

    if (m_game.projectiles.size() > 0) m_game.projectiles.clear();
    if (m_game.weaponPickups.size() > 0) m_game.weaponPickups.clear();
    m_pendingSpawns.clear();

    std::cout << "LevelManager: Processing level " << m_currentLevelPath << "..." << std::endl;
    processNode(scene->mRootNode, scene, glm::mat4(1.0f));
    
    // Resolve ground heights for all spawns
    resolveSpawns();

    // DEBUG: Log all platforms and draw their AABBs (short lived lines) to help diagnose unexpected platforms
    std::cout << "LevelManager: Loaded " << m_game.platforms.size() << " platforms:" << std::endl;
    for (size_t i = 0; i < m_game.platforms.size(); ++i) {
        const Platform& p = m_game.platforms[i];
        std::cout << "  [" << i << "] '" << p.getName() << "' pos(" << p.getPosition().x << "," << p.getPosition().y << "," << p.getPosition().z << ") size(" << p.getSize().x << "," << p.getSize().y << "," << p.getSize().z << ") meshes=" << p.getMeshes().size() << std::endl;

        if (m_game.debugRenderer) {
            glm::vec3 min = p.getPosition() - p.getSize() * 0.5f;
            glm::vec3 max = p.getPosition() + p.getSize() * 0.5f;
            glm::vec3 c(1.0f, 1.0f, 0.0f);

            glm::vec3 c000(min.x, min.y, min.z);
            glm::vec3 c100(max.x, min.y, min.z);
            glm::vec3 c010(min.x, max.y, min.z);
            glm::vec3 c110(max.x, max.y, min.z);
            glm::vec3 c001(min.x, min.y, max.z);
            glm::vec3 c101(max.x, min.y, max.z);
            glm::vec3 c011(min.x, max.y, max.z);
            glm::vec3 c111(max.x, max.y, max.z);
            float life = 10.0f; // seconds

            // Bottom
            m_game.debugRenderer->addLine(c000, c100, c, life);
            m_game.debugRenderer->addLine(c100, c101, c, life);
            m_game.debugRenderer->addLine(c101, c001, c, life);
            m_game.debugRenderer->addLine(c001, c000, c, life);
            // Top
            m_game.debugRenderer->addLine(c010, c110, c, life);
            m_game.debugRenderer->addLine(c110, c111, c, life);
            m_game.debugRenderer->addLine(c111, c011, c, life);
            m_game.debugRenderer->addLine(c011, c010, c, life);
            // Sides
            m_game.debugRenderer->addLine(c000, c010, c, life);
            m_game.debugRenderer->addLine(c100, c110, c, life);
            m_game.debugRenderer->addLine(c101, c111, c, life);
            m_game.debugRenderer->addLine(c001, c011, c, life);
        }
    }

    // Trigger level-specific tutorial notifications
    if (levelIndex == 1) {
        // Tutorial for level 1 - use actual key bindings
        const auto& keys = Settings::getInstance().keybinds;
        std::string tutorial = getKeyNameStr(keys.moveForward) + "/" + 
                               getKeyNameStr(keys.moveLeft) + "/" + 
                               getKeyNameStr(keys.moveBackward) + "/" + 
                               getKeyNameStr(keys.moveRight) + ": Move | Mouse: Look | Left Click: Shoot | " +
                               getKeyNameStr(keys.reload) + ": Reload | " +
                               getKeyNameStr(keys.jump) + ": Jump";
        m_game.showNotification(tutorial, 8.0f);
    } else if (levelIndex == 2) {
        // Tutorial for level 2
        m_game.showNotification("Level 2: The Battle Begins", 4.0f);
        m_game.showNotification("Enemies will chase and attack you", 4.0f);
        m_game.showNotification("Look for weapon pickups around the map", 5.0f);
        m_game.showNotification("Stay alert and keep moving!", 4.0f);
    } else if (levelIndex == 3) {
        const auto& keys = Settings::getInstance().keybinds;
        m_game.showNotification("Level 3", 4.0f);
        m_game.showNotification("You can dash by pressing " + getKeyNameStr(keys.dash), 4.0f);
    } else {
        // Generic notification for other levels
        m_game.showNotification("Level " + std::to_string(levelIndex) + " - Good luck!", 4.0f);
    }

    return true;
}

bool LevelManager::levelExists(int levelIndex) {
    if (levelIndex == 0) return false; // 0 is invalid or test level, usually we start at 1
    std::string path = "assets/levels/level_" + std::to_string(levelIndex) + ".glb";
    
    // Check if file exists
    std::ifstream f(path.c_str());
    return f.good();
}

void LevelManager::loadHardcodedFallback() {
    m_game.platforms.clear();
    m_game.platforms.emplace_back(glm::vec3(0.0f, -0.25f, 0.0f), glm::vec3(50.0f, 0.5f, 50.0f));
    m_game.platforms.emplace_back(glm::vec3(5.0f, 1.0f, -5.0f), glm::vec3(4.0f, 0.5f, 4.0f));
    m_game.platforms.emplace_back(glm::vec3(-6.0f, 1.5f, 3.0f), glm::vec3(3.0f, 0.5f, 3.0f));
    m_game.platforms.emplace_back(glm::vec3(8.0f, 2.0f, 5.0f), glm::vec3(3.5f, 0.5f, 3.5f));
    m_game.platforms.emplace_back(glm::vec3(-4.0f, 2.5f, -8.0f), glm::vec3(4.0f, 0.5f, 4.0f));
    m_game.platforms.emplace_back(glm::vec3(10.0f, 3.5f, -3.0f), glm::vec3(3.0f, 0.5f, 3.0f));
    m_game.platforms.emplace_back(glm::vec3(0.0f, 2.0f, -15.0f), glm::vec3(30.0f, 4.0f, 1.0f));
    m_game.platforms.emplace_back(glm::vec3(0.0f, 2.0f, 15.0f), glm::vec3(30.0f, 4.0f, 1.0f));
    m_game.platforms.emplace_back(glm::vec3(-15.0f, 2.0f, 0.0f), glm::vec3(1.0f, 4.0f, 30.0f));
    m_game.platforms.emplace_back(glm::vec3(15.0f, 2.0f, 0.0f), glm::vec3(1.0f, 4.0f, 30.0f));

    m_game.enemies.clear();
    m_game.enemies.emplace_back(glm::vec3(10.0f, 1.0f, 5.0f));
    m_game.enemies.emplace_back(glm::vec3(-8.0f, 1.0f, -6.0f));
    m_game.enemies.emplace_back(glm::vec3(5.0f, 1.0f, -10.0f));

    m_game.projectiles.clear();

    m_game.weaponPickups.clear();
    m_game.weaponPickups.emplace_back(glm::vec3(3.0f, 0.5f, 3.0f), WeaponType::RIFLE); 
    m_game.weaponPickups.emplace_back(glm::vec3(-5.0f, 0.5f, -5.0f), WeaponType::PISTOL);
    m_game.weaponPickups.emplace_back(glm::vec3(0.0f, 0.5f, -2.0f), WeaponType::AUTO_SHOTGUN);

    m_game.player.reset();
    m_game.camera.Position = m_game.player.getEyePosition();
}

void LevelManager::processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
    glm::mat4 nodeTransform = parentTransform * aiMatrix4x4ToGlm(node->mTransformation);

    // Extract position, rotation, scale from the global transform
    glm::vec3 position(nodeTransform[3]);
    
    // Simple extraction (might not handle complex rotation/skew perfectly but good for spawn points)
    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(nodeTransform[0]));
    scale.y = glm::length(glm::vec3(nodeTransform[1]));
    scale.z = glm::length(glm::vec3(nodeTransform[2]));

    SceneObject obj;
    obj.name = node->mName.C_Str();
    obj.position = position;
    obj.scale = scale;
    obj.size = scale; // default to transform scale
    obj.hasMesh = (node->mNumMeshes > 0);

    // When the node has geometry, compute its world-space AABB to match actual mesh dimensions
    if (obj.hasMesh) {
        glm::vec3 minBounds(std::numeric_limits<float>::max());
        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

        for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIndex]];
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                glm::vec4 localPos(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z, 1.0f);
                glm::vec4 worldPos = nodeTransform * localPos;
                minBounds = glm::min(minBounds, glm::vec3(worldPos));
                maxBounds = glm::max(maxBounds, glm::vec3(worldPos));
            }
        }

        obj.position = (minBounds + maxBounds) * 0.5f;
        obj.size = maxBounds - minBounds;
    }
    
    handleObject(obj, node, scene, nodeTransform);

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, nodeTransform);
    }
}

void LevelManager::handleObject(const SceneObject& obj, aiNode* node, const aiScene* scene, const glm::mat4& transform) {
    // Check for spawn points and pickups by name (these are usually empties without meshes)
    std::string name = obj.name;
    
    if (name.find("SPAWN_PLAYER") != std::string::npos || 
        name.find("SPAWN_ENEMY") != std::string::npos ||
        name.find("PICKUP") != std::string::npos) {
        m_pendingSpawns.push_back(obj);
    }
    else if (obj.hasMesh) {
        // Any object with geometry automatically becomes a platform for collision
        m_game.platforms.emplace_back(obj.position, obj.size, nullptr, transform, name);
        Platform& platform = m_game.platforms.back();
        
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            auto newMesh = ModelLoader::processMesh(mesh, scene);
            
            // Add mesh to platform for rendering and narrow collision
            platform.getMeshes().push_back(newMesh.get());

            m_levelMeshes.push_back(std::move(newMesh));
            m_levelMeshTransforms.push_back(transform);
        }
        
        std::cout << "  - Created Platform '" << name << "' at (" << obj.position.x << "," << obj.position.y << "," << obj.position.z 
                  << ") size (" << obj.size.x << "," << obj.size.y << "," << obj.size.z << ") with " << platform.getMeshes().size() << " meshes" << std::endl;
    }
}

void LevelManager::resolveSpawns() {
    std::cout << "LevelManager: Resolving ground heights for " << m_pendingSpawns.size() << " spawns..." << std::endl;
    
    for (const auto& spawn : m_pendingSpawns) {
        // Raycast from high up to find the actual floor at this spot
        glm::vec3 rayOrigin(spawn.position.x, 100.0f, spawn.position.z);
        glm::vec3 rayDir(0.0f, -1.0f, 0.0f);
        float bestY = -100.0f;
        bool foundFloor = false;

        for (const auto& platform : m_game.platforms) {
            float t = platform.raycast(rayOrigin, rayDir, 200.0f);
            if (t >= 0.0f) {
                float hitY = rayOrigin.y - t;
                // We prefer the highest surface that is roughly near or above our marker
                if (!foundFloor || hitY > bestY) {
                    bestY = hitY;
                    foundFloor = true;
                }
            }
        }

        // Use the found floor height, or try to align to a nearby platform top if none found
        float resolvedY;
        std::string name = spawn.name;

        if (foundFloor) {
            resolvedY = bestY;
        } else {
            // No mesh hit; try to find a platform whose XZ AABB contains the spawn marker and use its top
            float bestPlatformTop = std::numeric_limits<float>::lowest();
            bool foundPlatform = false;
            for (const auto& platform : m_game.platforms) {
                glm::vec3 pmin = platform.getPosition() - platform.getSize() * 0.5f;
                glm::vec3 pmax = platform.getPosition() + platform.getSize() * 0.5f;
                if (spawn.position.x >= pmin.x - 0.01f && spawn.position.x <= pmax.x + 0.01f &&
                    spawn.position.z >= pmin.z - 0.01f && spawn.position.z <= pmax.z + 0.01f) {
                    float top = platform.getPosition().y + platform.getSize().y * 0.5f;
                    if (top > bestPlatformTop) {
                        bestPlatformTop = top;
                        foundPlatform = true;
                    }
                }
            }

            if (foundPlatform) {
                resolvedY = bestPlatformTop;
                std::cout << "  - Spawn '" << spawn.name << "' aligned to platform top at " << resolvedY << std::endl;
            } else {
                // As a last resort, clamp extremely high spawn markers down to the highest platform top
                float highestTop = std::numeric_limits<float>::lowest();
                for (const auto& platform : m_game.platforms) {
                    float top = platform.getPosition().y + platform.getSize().y * 0.5f;
                    highestTop = std::max(highestTop, top);
                }
                if (highestTop > std::numeric_limits<float>::lowest() && spawn.position.y > highestTop + 2.0f) {
                    resolvedY = highestTop;
                    std::cout << "  - Spawn '" << spawn.name << "' dropped to highest platform top " << resolvedY << " (was " << spawn.position.y << ")" << std::endl;
                } else {
                    resolvedY = spawn.position.y; // keep original marker Y if reasonable
                }
            }
        }

        if (name.find("SPAWN_PLAYER") != std::string::npos) {
            // Add extra buffer to prevent first-frame jitter/tunneling
            glm::vec3 spawnPos = glm::vec3(spawn.position.x, resolvedY + Config::SPAWN_HALF_HEIGHT + Config::SPAWN_BUFFER, spawn.position.z);
            m_game.player.setPosition(spawnPos);
            m_game.camera.Position = m_game.player.getEyePosition();
            m_game.player.setVelocity(glm::vec3(0.0f)); // Reset velocity
            std::cout << "  - Resolved Player Spawn at (" << spawnPos.x << "," << spawnPos.y << "," << spawnPos.z << ")" << (foundFloor ? "" : " [FALLBACK]") << std::endl;
        }
        else if (name.find("SPAWN_ENEMY") != std::string::npos) {
            // Determine weapon type from name suffix
            WeaponType weaponType = WeaponType::PISTOL;
            if (name.find("RIFLE") != std::string::npos) weaponType = WeaponType::RIFLE;
            else if (name.find("PUMP_SHOTGUN") != std::string::npos) weaponType = WeaponType::PUMP_SHOTGUN;
            else if (name.find("AUTO_SHOTGUN") != std::string::npos) weaponType = WeaponType::AUTO_SHOTGUN;
            else if (name.find("SHOTGUN") != std::string::npos) weaponType = WeaponType::AUTO_SHOTGUN; // Shorthand
            else if (name.find("PISTOL") != std::string::npos) weaponType = WeaponType::PISTOL;
            else {
                // If generic SPAWN_ENEMY, randomize to demonstrate variety
                int r = rand() % 4;
                if (r == 0) weaponType = WeaponType::PISTOL;
                else if (r == 1) weaponType = WeaponType::RIFLE;
                else if (r == 2) weaponType = WeaponType::AUTO_SHOTGUN;
                else weaponType = WeaponType::PUMP_SHOTGUN;
            }

            // Add extra buffer for enemies too
            glm::vec3 spawnPos = glm::vec3(spawn.position.x, resolvedY + Config::SPAWN_HALF_HEIGHT + Config::SPAWN_BUFFER, spawn.position.z);
            m_game.enemies.emplace_back(spawnPos, weaponType);
            std::cout << "  - Resolved Enemy '" << name << "' (Weapon: " << (int)weaponType << ") at (" << spawnPos.x << "," << spawnPos.y << "," << spawnPos.z << ")" << (foundFloor ? "" : " [FALLBACK]") << std::endl;
        }
        else if (name.find("PICKUP_RIFLE") != std::string::npos) {
            m_game.weaponPickups.emplace_back(glm::vec3(spawn.position.x, resolvedY + 0.2f, spawn.position.z), WeaponType::RIFLE);
        }
        else if (name.find("PICKUP_PISTOL") != std::string::npos) {
            m_game.weaponPickups.emplace_back(glm::vec3(spawn.position.x, resolvedY + 0.2f, spawn.position.z), WeaponType::PISTOL);
        }
        else if (name.find("PICKUP_SHOTGUN") != std::string::npos) {
            m_game.weaponPickups.emplace_back(glm::vec3(spawn.position.x, resolvedY + 0.2f, spawn.position.z), WeaponType::AUTO_SHOTGUN);
        }
        else if (name.find("PICKUP_PUMP_SHOTGUN") != std::string::npos) {
            m_game.weaponPickups.emplace_back(glm::vec3(spawn.position.x, resolvedY + 0.2f, spawn.position.z), WeaponType::PUMP_SHOTGUN);
        }
    }
}
