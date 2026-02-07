#pragma once

#include <memory>
#include <map>
#include <vector>
#include <glm/glm.hpp>

#include "Mesh.h"
#include "Camera.h"
#include "Shader.h"
#include "InputState.h"
#include "Weapon.h"

#include "ResourceManager.h"

class WeaponRenderer {
public:
    WeaponRenderer();

    void update(float deltaTime, const InputState& input, Weapon* weapon);
    void render(const Camera& camera, Shader& lightingShader, Weapon* weapon, ResourceManager& resourceManager, float gameTime);
    void triggerRecoil(float rotation);

private:
    float animationTime;
    float lastX, lastY;
    float horizontalSway;
    float verticalBob;
    
    // Recoil animation state
    float recoilOffset;    // Z-axis kickback
    float recoilRotation;  // Pitch climb
};
