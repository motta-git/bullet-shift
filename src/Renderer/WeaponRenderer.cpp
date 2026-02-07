#include "WeaponRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#include "GLDebug.h"
#include "Config.h" // Assuming Config.h is needed for Config::Weapon constants
#include <GLFW/glfw3.h> // Assuming glfwGetTime() requires this

WeaponRenderer::WeaponRenderer()
    : animationTime(0.0f),
      lastX(0.0f), lastY(0.0f),
      horizontalSway(0.0f),
      verticalBob(0.0f),
      recoilOffset(0.0f),
      recoilRotation(0.0f) {
}

void WeaponRenderer::triggerRecoil(float rotation) {
    recoilRotation = rotation;
    recoilOffset = 0.2f; // Derive kickback from rotation
}

void WeaponRenderer::update(float deltaTime, const InputState& input, Weapon* weapon) {
    if (!weapon) return;

    recoilOffset = glm::mix(recoilOffset, 0.0f, deltaTime * Config::Weapon::RECOIL_RECOVERY_SPEED);
    recoilRotation = glm::mix(recoilRotation, 0.0f, deltaTime * Config::Weapon::RECOIL_RECOVERY_SPEED);

    bool isMoving = input.moveForward || input.moveBackward || input.moveLeft || input.moveRight;
    float targetSpeed = isMoving ? Config::Weapon::MOVE_BOB_SPEED : Config::Weapon::IDLE_BOB_SPEED;
    animationTime += deltaTime * targetSpeed;

    float targetBob = isMoving ? std::sin(animationTime) * 0.0125f : std::sin(animationTime) * 0.005f;
    verticalBob = glm::mix(verticalBob, targetBob, deltaTime * 8.0f);

    float targetSway = isMoving ? std::cos(animationTime * 0.5f) * 0.015f : 0.0f;
    horizontalSway = glm::mix(horizontalSway, targetSway, deltaTime * 4.0f);
}

void WeaponRenderer::render(const Camera& camera, Shader& lightingShader, Weapon* weapon, ResourceManager& resourceManager, float gameTime) {
    if (!weapon) return;
    
    auto data = Config::Weapon::getWeaponConfig(weapon->getType());
    const auto* meshes = resourceManager.getWeaponMeshes(data.name);
    if (!meshes || meshes->empty()) return;

    float scale = data.scale;
    glm::vec3 baseOffset = data.offset;
    glm::vec3 modelRot = data.rotation;

    float breathing = std::sin(gameTime * 1.5f) * 0.003f;
    glm::vec3 currentOffset = baseOffset;
    currentOffset.x += horizontalSway;
    currentOffset.y += verticalBob + breathing;
    currentOffset.z += std::sin(animationTime * 0.3f) * 0.015f;
    currentOffset.z += recoilOffset; 

    if (weapon->isReloading()) {
        currentOffset.y -= 0.08f;
        currentOffset.z -= 0.05f;
    }

    glm::vec3 weaponPosition = camera.Position
        + camera.Right * currentOffset.x
        + camera.Up * currentOffset.y
        + (-camera.Front) * currentOffset.z;

    glm::mat4 orientation(1.0f);
    orientation[0] = glm::vec4(camera.Right, 0.0f);
    orientation[1] = glm::vec4(camera.Up, 0.0f);
    orientation[2] = glm::vec4(-camera.Front, 0.0f);

    // 1. Procedural Animations (applied in a standard FPS coordinate system)
    glm::mat4 animMatrix = glm::mat4(1.0f);
    // Recoil Pitch (Kick UP) - Always rotate around X axis here
    animMatrix = glm::rotate(animMatrix, glm::radians(recoilRotation), glm::vec3(1.0f, 0.0f, 0.0f));
    // Sway and Bob
    animMatrix = glm::rotate(animMatrix, horizontalSway * 1.3f, glm::vec3(0.0f, 0.0f, 1.0f));
    animMatrix = glm::rotate(animMatrix, verticalBob * 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    
    if (weapon->isReloading()) {
        animMatrix = glm::rotate(animMatrix, glm::radians(18.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    // Pump action animation: small slide back and forth during pump progress
    if (weapon->getType() == WeaponType::PUMP_SHOTGUN && weapon->isPumping()) {
        float p = weapon->getPumpProgress(); // 0..1
        // Smooth in-out curve
        float t = sinf(p * 3.14159265f);
        float pumpOffset = -t * 0.12f; // move slightly towards camera when pumping
        animMatrix = glm::translate(animMatrix, glm::vec3(0.0f, 0.0f, pumpOffset));
    }

    // 2. Model-specific Corrections (to match Blender orientation to Engine Forward)
    glm::mat4 modelCorrect = glm::mat4(1.0f);
    modelCorrect = glm::rotate(modelCorrect, glm::radians(modelRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelCorrect = glm::rotate(modelCorrect, glm::radians(modelRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelCorrect = glm::rotate(modelCorrect, glm::radians(modelRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 weaponModel = glm::translate(glm::mat4(1.0f), weaponPosition)
                           * orientation
                           * animMatrix
                           * modelCorrect
                           * glm::scale(glm::mat4(1.0f), glm::vec3(scale));

    lightingShader.setVec3("material.ambient", 0.25f, 0.25f, 0.28f);
    lightingShader.setVec3("material.diffuse", 0.45f, 0.45f, 0.5f);
    lightingShader.setVec3("material.specular", 0.9f, 0.9f, 0.95f);
    lightingShader.setFloat("material.shininess", 96.0f);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    lightingShader.setMat4("model", weaponModel);
    
    // Draw each mesh of the weapon model
    for (const auto& mesh : *meshes) {
        mesh->draw();
    }
    
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_ERROR();
}
