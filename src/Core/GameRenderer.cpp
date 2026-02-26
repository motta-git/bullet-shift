#include "GameRenderer.h"
#include "Game.h"
#include "Camera.h"
#include "Player.h"
#include "ParticleSystem.h"
#include "HUD.h"
#include "DebugRenderer.h"
#include "Enemy.h"
#include "WeaponPickup.h"
#include "HealthPickup.h"
#include "Projectile.h"
#include "ResourceManager.h"
#include "PostProcessingSystem.h"
#include "Skybox.h"
#include "ShadowSystem.h"
#include "WeaponRenderer.h"
#include "Platform.h"
#include "NavigationGraph.h"
#include "GuiSystem.h"
#include "MenuSystem.h"
#include "Settings.h"
#include "Config.h"
#include "DevConsole.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <filesystem>
#include <algorithm>

GameRenderer::GameRenderer(Game& game) : m_game(game) {
}

GameRenderer::~GameRenderer() {
}

void GameRenderer::render(
    float accumulatedTime, 
    GameState state, 
    int currentLevel,
    DevConsole& console,
    Camera& camera,
    Player& player,
    WeaponRenderer& weaponRenderer,
    std::unique_ptr<ParticleSystem>& particleSystem,
    std::unique_ptr<PostProcessingSystem>& postProcessing,
    std::unique_ptr<ResourceManager>& resourceManager,
    std::unique_ptr<HUD>& hud,
    std::unique_ptr<DebugRenderer>& debugRenderer,
    std::unique_ptr<Skybox>& skybox,
    std::unique_ptr<ShadowSystem>& shadowSystem,
    std::unique_ptr<GuiSystem>& guiSystem,
    std::unique_ptr<MenuSystem>& menuSystem,
    std::unique_ptr<NavigationGraph>& navigationGraph,
    const std::vector<Platform>& platforms,
    const std::vector<Enemy>& enemies,
    const std::vector<WeaponPickup>& weaponPickups,
    const std::vector<HealthPickup>& healthPickups,
    const std::vector<Projectile>& projectiles,
    float techStyleIntensity,
    float timeScale,
    float playerMuzzleFlashTimer,
    const glm::vec3& playerMuzzleFlashPos,
    const glm::vec3& playerMuzzleFlashColor,
    const std::string& interactionPrompt,
    float bulletTimeEnergy,
    bool bulletTimeActive) 
{
    (void)hud;
    (void)interactionPrompt;
    (void)bulletTimeEnergy;
    (void)bulletTimeActive;
    
    if (Settings::getInstance().graphics.gammaCorrection && !postProcessing) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    // --- Shadow Pass ---
    if (shadowSystem) {
        Shader* depthShader = resourceManager->getShader("shadowDepth");
        if (depthShader) {
            glm::vec3 lightDir(-0.3f, -1.0f, -0.2f); // Same as dirLight in renderScene
            shadowSystem->updateLightSpaceMatrix(lightDir, player.getPosition());
            
            depthShader->use();
            depthShader->setMat4("lightSpaceMatrix", shadowSystem->getLightSpaceMatrix());
            
            shadowSystem->bindForWriting();
            renderDepthScene(*depthShader, resourceManager, platforms, enemies, weaponPickups, healthPickups);
            shadowSystem->unbind();
            
            glViewport(0, 0, Settings::getInstance().window.width, Settings::getInstance().window.height);
        }
    }

    if (postProcessing) {
        postProcessing->begin();
    } else {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            static_cast<float>(Settings::getInstance().window.width) / Settings::getInstance().window.height,
                                            Config::NEAR_PLANE,
                                            Config::FAR_PLANE);
    glm::mat4 view = camera.getViewMatrix();

    renderScene(projection, view, accumulatedTime, camera, player, resourceManager, shadowSystem, platforms, enemies, weaponPickups, healthPickups, techStyleIntensity, playerMuzzleFlashTimer, playerMuzzleFlashPos, playerMuzzleFlashColor);
    
    if (skybox) {
        Shader* skyShader = resourceManager->getShader("skybox");
        if (skyShader) {
            skybox->render(projection, view, *skyShader);
        }
    }

    Shader* lightingShader = resourceManager->getShader("lighting");
    if (lightingShader) {
        lightingShader->use();
        weaponRenderer.render(camera, *lightingShader, player.getInventory().getCurrentWeapon(), *resourceManager, accumulatedTime);
    }

    renderLights(projection, view, resourceManager);
    renderProjectiles(projection, view, resourceManager, projectiles);

    Shader* particleShader = resourceManager->getShader("particle");
    if (particleShader && particleSystem) {
        particleSystem->draw(projection, view, *particleShader);
    }

    if (debugRenderer) {
        debugRenderer->render(projection, view);
        
        if (navigationGraph && navigationGraph->isValid() && state == GameState::PLAYING) {
            const auto& nodes = navigationGraph->getNodes();
            const auto& edges = navigationGraph->getEdges();
            
            for (const auto& edge : edges) {
                if (edge.fromNode < static_cast<int>(nodes.size()) && 
                    edge.toNode < static_cast<int>(nodes.size())) {
                    glm::vec3 from = nodes[edge.fromNode].position;
                    glm::vec3 to = nodes[edge.toNode].position;
                    debugRenderer->addLine(from, to, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);
                }
            }
            
            for (const auto& enemy : enemies) {
                if (!enemy.isAlive()) continue;
                glm::vec3 enemyEye = enemy.getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
                glm::vec3 playerEye = player.getEyePosition();
                bool hasLOS = enemy.canSeePlayer(player.getPosition());
                glm::vec3 losColor = hasLOS ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.5f, 0.5f, 0.5f);
                debugRenderer->addLine(enemyEye, playerEye, losColor, 0.0f);
            }
        }
    }

    if (postProcessing) {
        float btIntensity = (1.0f - timeScale) / (1.0f - Config::MIN_BULLET_TIME_SCALE);
        postProcessing->setBulletTimeIntensity(btIntensity);
        postProcessing->end();
        
        if (Settings::getInstance().graphics.gammaCorrection) {
            glEnable(GL_FRAMEBUFFER_SRGB);
        }
        postProcessing->render(Settings::getInstance().window.width, Settings::getInstance().window.height, 
                               Config::NEAR_PLANE, Config::FAR_PLANE, resourceManager.get());
    }

    if (Settings::getInstance().graphics.gammaCorrection) {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    renderGUI(state, currentLevel, console, guiSystem, menuSystem, m_game);
}

void GameRenderer::renderScene(const glm::mat4& projection, const glm::mat4& view, float accumulatedTime, Camera& camera, Player& player, std::unique_ptr<ResourceManager>& resourceManager, std::unique_ptr<ShadowSystem>& shadowSystem, const std::vector<Platform>& platforms, const std::vector<Enemy>& enemies, const std::vector<WeaponPickup>& weaponPickups, const std::vector<HealthPickup>& healthPickups, float techStyleIntensity, float playerMuzzleFlashTimer, const glm::vec3& playerMuzzleFlashPos, const glm::vec3& playerMuzzleFlashColor) {
    (void)player;
    Shader* lightingShader = resourceManager->getShader("lighting");
    if (!lightingShader) return;

    lightingShader->use();
    lightingShader->setVec3("viewPos", camera.Position);
    lightingShader->setMat4("projection", projection);
    lightingShader->setMat4("view", view);
    lightingShader->setBool("u_useHardwareGamma", Settings::getInstance().graphics.gammaCorrection);
    
    if (shadowSystem) {
        lightingShader->setMat4("u_lightSpaceMatrix", shadowSystem->getLightSpaceMatrix());
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, shadowSystem->getDepthMap());
        lightingShader->setInt("shadowMap", 4);
    }
    
    lightingShader->setFloat("u_time", accumulatedTime);
    lightingShader->setFloat("u_techStyleIntensity", techStyleIntensity);

    lightingShader->setVec3("dirLight.direction", -0.3f, -1.0f, -0.2f);
    lightingShader->setVec3("dirLight.ambient", 0.35f, 0.35f, 0.4f);
    lightingShader->setVec3("dirLight.diffuse", 0.7f, 0.7f, 0.8f);
    lightingShader->setVec3("dirLight.specular", 0.3f, 0.3f, 0.3f);

    glm::vec3 pointPositions[] = {
        glm::vec3(-8.0f, 3.0f, -8.0f), glm::vec3(8.0f, 3.0f, -8.0f),
        glm::vec3(-8.0f, 3.0f, 8.0f), glm::vec3(8.0f, 3.0f, 8.0f)
    };
    glm::vec3 pointColors[] = {
        glm::vec3(1.0f, 0.8f, 0.6f), glm::vec3(0.8f, 0.9f, 1.0f),
        glm::vec3(1.0f, 0.7f, 0.5f), glm::vec3(0.6f, 0.8f, 1.0f)
    };

    for (int i = 0; i < 4; ++i) {
        std::string prefix = "pointLights[" + std::to_string(i) + "].";
        lightingShader->setVec3(prefix + "position", pointPositions[i]);
        lightingShader->setVec3(prefix + "ambient", pointColors[i] * 0.1f);
        lightingShader->setVec3(prefix + "diffuse", pointColors[i]);
        lightingShader->setVec3(prefix + "specular", pointColors[i]);
        lightingShader->setFloat(prefix + "constant", 1.0f);
        lightingShader->setFloat(prefix + "linear", 0.09f);
        lightingShader->setFloat(prefix + "quadratic", 0.032f);
    }

    if (playerMuzzleFlashTimer > 0.0f) {
        std::string prefix = "pointLights[4].";
        lightingShader->setVec3(prefix + "position", playerMuzzleFlashPos);
        lightingShader->setVec3(prefix + "ambient", playerMuzzleFlashColor * 0.2f);
        lightingShader->setVec3(prefix + "diffuse", playerMuzzleFlashColor * 2.0f);
        lightingShader->setVec3(prefix + "specular", playerMuzzleFlashColor);
        lightingShader->setFloat(prefix + "constant", 1.0f);
        lightingShader->setFloat(prefix + "linear", 0.14f);
        lightingShader->setFloat(prefix + "quadratic", 0.07f);
    } else {
        std::string prefix = "pointLights[4].";
        lightingShader->setVec3(prefix + "diffuse", 0.0f, 0.0f, 0.0f);
        lightingShader->setVec3(prefix + "ambient", 0.0f, 0.0f, 0.0f);
        lightingShader->setVec3(prefix + "specular", 0.0f, 0.0f, 0.0f);
        lightingShader->setFloat(prefix + "constant", 1.0f);
        lightingShader->setFloat(prefix + "linear", 0.14f);
        lightingShader->setFloat(prefix + "quadratic", 0.07f);
    }

    int lightIndex = 5;
    for (const auto& enemy : enemies) {
        if (enemy.isAlive() && enemy.getMuzzleFlashTimer() > 0.0f) {
            std::string prefix = "pointLights[" + std::to_string(lightIndex) + "].";
            glm::vec3 flashPos = enemy.getMuzzleFlashPos();
            glm::vec3 flashColor = glm::vec3(1.0f, 0.7f, 0.3f);
            
            lightingShader->setVec3(prefix + "position", flashPos);
            lightingShader->setVec3(prefix + "ambient", flashColor * 0.2f);
            lightingShader->setVec3(prefix + "diffuse", flashColor * 2.0f);
            lightingShader->setVec3(prefix + "specular", flashColor);
            lightingShader->setFloat(prefix + "constant", 1.0f);
            lightingShader->setFloat(prefix + "linear", 0.14f);
            lightingShader->setFloat(prefix + "quadratic", 0.07f);
            
            lightIndex++;
            if (lightIndex > 7) break;
        }
    }

    for (int i = lightIndex; i < 8; i++) {
        std::string prefix = "pointLights[" + std::to_string(i) + "].";
        lightingShader->setVec3(prefix + "diffuse", 0.0f, 0.0f, 0.0f);
        lightingShader->setVec3(prefix + "ambient", 0.0f, 0.0f, 0.0f);
        lightingShader->setVec3(prefix + "specular", 0.0f, 0.0f, 0.0f);
        lightingShader->setFloat(prefix + "constant", 1.0f);
        lightingShader->setFloat(prefix + "linear", 0.09f);
        lightingShader->setFloat(prefix + "quadratic", 0.032f);
    }

    lightingShader->setVec3("spotLight.position", camera.Position);
    lightingShader->setVec3("spotLight.direction", camera.Front);
    lightingShader->setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    lightingShader->setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    lightingShader->setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    lightingShader->setFloat("spotLight.constant", 1.0f);
    lightingShader->setFloat("spotLight.linear", 0.09f);
    lightingShader->setFloat("spotLight.quadratic", 0.032f);
    lightingShader->setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    lightingShader->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));

    lightingShader->setVec3("material.ambient", 0.3f, 0.3f, 0.4f);
    lightingShader->setVec3("material.diffuse", 0.5f, 0.5f, 0.7f);
    lightingShader->setVec3("material.specular", 0.3f, 0.3f, 0.3f);
    lightingShader->setFloat("material.shininess", 32.0f);
    
    Mesh* cubeMesh = resourceManager->getMesh("cube");

    for (const auto& platform : platforms) {
        if (platform.hasMesh()) {
            lightingShader->setMat4("model", platform.getTransform());
            for (const Mesh* mesh : platform.getMeshes()) {
                mesh->draw();
            }
        } else if (cubeMesh) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), platform.getPosition());
            model = glm::scale(model, platform.getSize());
            lightingShader->setMat4("model", model);
            cubeMesh->draw();
        }
    }

    if (cubeMesh) {
        for (const auto& enemy : enemies) {
            if (!enemy.isAlive()) continue;

            glm::vec3 ambient(0.7f, 0.2f, 0.2f);
            glm::vec3 diffuse(0.9f, 0.3f, 0.3f);
            glm::vec3 spec(0.5f, 0.5f, 0.5f);
            float shininess = 64.0f;

            float alert = enemy.getAlertProgress();
            if (alert > 0.001f) {
                glm::vec3 alertColor(1.0f, 0.2f, 0.2f);
                ambient = glm::mix(ambient, alertColor, alert);
                diffuse = glm::mix(diffuse, alertColor, alert);
            }

            lightingShader->setVec3("material.ambient", ambient);
            lightingShader->setVec3("material.diffuse", diffuse);
            lightingShader->setVec3("material.specular", spec);
            lightingShader->setFloat("material.shininess", shininess);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), enemy.getPosition());
            model = glm::scale(model, enemy.getSize());
            lightingShader->setMat4("model", model);
            cubeMesh->draw();

            // Render Enemy Weapon
            Weapon* enemyWeapon = enemy.getWeapon();
            if (enemyWeapon) {
                auto data = Config::Weapon::getWeaponConfig(enemyWeapon->getType());
                const auto* meshes = resourceManager->getWeaponMeshes(data.name);
                if (meshes && !meshes->empty()) {
                    // Start with enemy position (chest height)
                    glm::vec3 weaponPos = enemy.getPosition() + glm::vec3(0.0f, 0.4f, 0.0f);
                    
                    // Add forward and right offsets based on look direction
                    glm::vec3 forward = enemy.getLookDirection();
                    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
                    if (glm::length(right) < 0.001f) right = glm::vec3(1.0f, 0.0f, 0.0f);
                    
                    weaponPos += forward * 0.4f + right * 0.25f;
                    
                    glm::mat4 wModel = glm::translate(glm::mat4(1.0f), weaponPos);
                    
                    // Rotate to face look direction (yaw only for horizontal alignment)
                    float yaw = std::atan2(forward.x, forward.z);
                    wModel = glm::rotate(wModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
                    
                    // The weapon model faces the opposite direction of the enemy's forward, 
                    // so we need an extra 180 degree rotation around the Y axis
                    wModel = glm::rotate(wModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

                    // Apply base model rotation from config
                    wModel = glm::rotate(wModel, glm::radians(data.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    wModel = glm::rotate(wModel, glm::radians(data.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    wModel = glm::rotate(wModel, glm::radians(data.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                    
                    wModel = glm::scale(wModel, glm::vec3(data.scale * 0.8f));
                    
                    lightingShader->setMat4("model", wModel);
                    lightingShader->setVec3("material.ambient", 0.5f, 0.5f, 0.5f);
                    lightingShader->setVec3("material.diffuse", 0.8f, 0.8f, 0.8f);
                    
                    for (const auto& mesh : *meshes) {
                        mesh->draw();
                    }
                    
                    // Restore material for next enemy
                    lightingShader->setVec3("material.ambient", ambient);
                    lightingShader->setVec3("material.diffuse", diffuse);
                }
            }
        }
    }

    for (const auto& pickup : weaponPickups) {
        if (pickup.isPickedUp()) continue;
        
        auto data = Config::Weapon::getWeaponConfig(pickup.getType());
        const auto* meshes = resourceManager->getWeaponMeshes(data.name);

        if (meshes && !meshes->empty()) {
            lightingShader->setVec3("material.ambient", 0.5f, 0.5f, 0.5f);
            lightingShader->setVec3("material.diffuse", 0.8f, 0.8f, 0.8f);
            lightingShader->setVec3("material.specular", 1.0f, 1.0f, 1.0f);
            lightingShader->setFloat("material.shininess", 128.0f);

            glm::vec3 pickupPos = pickup.getPosition();
            pickupPos.y += 0.2f + 0.1f * std::sin(accumulatedTime * 2.0f);
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pickupPos);
            model = glm::rotate(model, accumulatedTime, glm::vec3(0.0f, 1.0f, 0.0f));
            
            model = glm::rotate(model, glm::radians(data.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(data.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(data.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

            float scale = data.scale * 0.6f; 
            model = glm::scale(model, glm::vec3(scale));

            lightingShader->setMat4("model", model);
            for (const auto& mesh : *meshes) {
                mesh->draw();
            }
        } else if (cubeMesh) {
            lightingShader->setVec3("material.ambient", 0.7f, 0.6f, 0.2f);
            lightingShader->setVec3("material.diffuse", 0.9f, 0.8f, 0.3f);
            lightingShader->setVec3("material.specular", 0.8f, 0.8f, 0.8f);
            lightingShader->setFloat("material.shininess", 96.0f);

            glm::vec3 pickupPos = pickup.getPosition();
            pickupPos.y += 0.2f * std::sin(accumulatedTime * 2.0f);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pickupPos);
            model = glm::rotate(model, accumulatedTime, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.3f, 0.5f, 0.2f));
            lightingShader->setMat4("model", model);
            cubeMesh->draw();
        }
    }

    Mesh* healthMesh = resourceManager->getMesh("health_pickup");
    Mesh* sphereMesh = resourceManager->getMesh("sphere");

    for (const auto& hp : healthPickups) {
        if (hp.isPickedUp()) continue;

        glm::vec3 pickupPos = hp.getPosition();
        pickupPos.y += 0.15f + 0.08f * std::sin(accumulatedTime * 2.0f);

        if (healthMesh) {
            lightingShader->setVec3("material.ambient", 0.5f, 0.6f, 0.5f);
            lightingShader->setVec3("material.diffuse", 0.8f, 0.9f, 0.8f);
            lightingShader->setVec3("material.specular", 1.0f, 1.0f, 1.0f);
            lightingShader->setFloat("material.shininess", 96.0f);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pickupPos);
            model = glm::rotate(model, accumulatedTime, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(Config::Pickup::HEALTH_SCALE));

            lightingShader->setMat4("model", model);
            healthMesh->draw();
        } else if (sphereMesh) {
            lightingShader->setVec3("material.ambient", 0.2f, 0.6f, 0.2f);
            lightingShader->setVec3("material.diffuse", 0.3f, 0.9f, 0.3f);
            lightingShader->setVec3("material.specular", 0.6f, 0.8f, 0.6f);
            lightingShader->setFloat("material.shininess", 64.0f);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pickupPos);
            model = glm::rotate(model, accumulatedTime, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.28f));

            lightingShader->setMat4("model", model);
            sphereMesh->draw();
        } else if (cubeMesh) {
            lightingShader->setVec3("material.ambient", 0.2f, 0.6f, 0.2f);
            lightingShader->setVec3("material.diffuse", 0.3f, 0.9f, 0.3f);
            lightingShader->setVec3("material.specular", 0.6f, 0.8f, 0.6f);
            lightingShader->setFloat("material.shininess", 64.0f);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pickupPos);
            model = glm::rotate(model, accumulatedTime, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
            lightingShader->setMat4("model", model);
            cubeMesh->draw();
        }
    }
}

void GameRenderer::renderDepthScene(class Shader& depthShader, std::unique_ptr<ResourceManager>& resourceManager, const std::vector<Platform>& platforms, const std::vector<Enemy>& enemies, const std::vector<WeaponPickup>& weaponPickups, const std::vector<HealthPickup>& healthPickups) {
    (void)weaponPickups;
    (void)healthPickups;
    Mesh* cubeMesh = resourceManager->getMesh("cube");

    for (const auto& platform : platforms) {
        if (platform.hasMesh()) {
            depthShader.setMat4("model", platform.getTransform());
            for (const Mesh* mesh : platform.getMeshes()) {
                mesh->draw();
            }
        } else if (cubeMesh) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), platform.getPosition());
            model = glm::scale(model, platform.getSize());
            depthShader.setMat4("model", model);
            cubeMesh->draw();
        }
    }

    if (cubeMesh) {
        for (const auto& enemy : enemies) {
            if (!enemy.isAlive()) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), enemy.getPosition());
            model = glm::scale(model, enemy.getSize());
            depthShader.setMat4("model", model);
            cubeMesh->draw();
        }
    }
}

void GameRenderer::renderLights(const glm::mat4& projection, const glm::mat4& view, std::unique_ptr<ResourceManager>& resourceManager) {
    Shader* lightSourceShader = resourceManager->getShader("lightSource");
    if (!lightSourceShader) return;

    lightSourceShader->use();
    lightSourceShader->setMat4("projection", projection);
    lightSourceShader->setMat4("view", view);

    glm::vec3 pointPositions[] = {
        glm::vec3(-8.0f, 3.0f, -8.0f), glm::vec3(8.0f, 3.0f, -8.0f),
        glm::vec3(-8.0f, 3.0f, 8.0f), glm::vec3(8.0f, 3.0f, 8.0f)
    };
    glm::vec3 pointColors[] = {
        glm::vec3(1.0f, 0.8f, 0.6f), glm::vec3(0.8f, 0.9f, 1.0f),
        glm::vec3(1.0f, 0.7f, 0.5f), glm::vec3(0.6f, 0.8f, 1.0f)
    };
    (void)pointPositions; (void)pointColors;
}

void GameRenderer::renderProjectiles(const glm::mat4& projection, const glm::mat4& view, std::unique_ptr<ResourceManager>& resourceManager, const std::vector<Projectile>& projectiles) {
    Shader* lightSourceShader = resourceManager->getShader("lightSource");
    if (!lightSourceShader) return;

    lightSourceShader->use();
    lightSourceShader->setMat4("projection", projection);
    lightSourceShader->setMat4("view", view);

    Mesh* cubeMesh = resourceManager->getMesh("cube");
    if (cubeMesh) {
        for (const auto& proj : projectiles) {
            if (proj.getTimeElapsed() < 0.05f) continue;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), proj.getPosition());
            glm::vec3 dir = glm::normalize(proj.getVelocity());
            glm::vec3 up = std::abs(dir.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
            glm::mat4 rot = glm::lookAt(glm::vec3(0.0f), dir, up);
            model = model * glm::inverse(rot);
            model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.4f));

            lightSourceShader->setMat4("model", model);
            lightSourceShader->setVec3("lightColor", proj.isEnemyProjectile() ? glm::vec3(1.0f, 0.2f, 0.2f) : glm::vec3(1.0f, 1.0f, 0.4f));
            cubeMesh->draw();
        }
    }
}

void GameRenderer::renderHUD(GameState state, Player& player, std::unique_ptr<HUD>& hud, const std::vector<Enemy>& enemies, const std::string& interactionPrompt, float bulletTimeEnergy, bool bulletTimeActive) {
    if (state == GameState::GAME_OVER && hud) {
        hud->renderDeathScreen();
        return;
    }

    if (player.isAlive() && hud) {
        Weapon* currentWeapon = player.getInventory().getCurrentWeapon();
        std::string name = currentWeapon ? currentWeapon->getName() : "None";
        int ammo = currentWeapon ? currentWeapon->getCurrentAmmo() : 0;
        int reserve = currentWeapon ? currentWeapon->getReserveAmmo() : 0;
        bool reloading = currentWeapon ? currentWeapon->isReloading() : false;

        int enemyCount = 0;
        for (const auto& enemy : enemies) if (enemy.isAlive()) enemyCount++;

        hud->render(player.getHealth(), player.getMaxHealth(),
                    name, ammo, reserve, reloading,
                    enemyCount, interactionPrompt, bulletTimeEnergy, Config::MAX_BULLET_TIME_ENERGY, bulletTimeActive);
    }
}

void GameRenderer::renderGUI(GameState state, int currentLevel, DevConsole& console, std::unique_ptr<GuiSystem>& guiSystem, std::unique_ptr<MenuSystem>& menuSystem, Game& game) {
    (void)game;
    if (guiSystem) {
        guiSystem->beginFrame();
    }

    if (menuSystem) {
        menuSystem->render(state, currentLevel);
    }

    // Render the developer console only when NOT in playing state
    if (state != GameState::PLAYING) {
        console.render(game);
    }

    if (guiSystem) {
        guiSystem->endFrame();
        guiSystem->render();
    }
}
