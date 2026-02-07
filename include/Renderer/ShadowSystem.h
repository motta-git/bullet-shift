#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

class ShadowSystem {
public:
    ShadowSystem(unsigned int resolution = 2048);
    ~ShadowSystem();

    void bindForWriting();
    void unbind();

    unsigned int getDepthMap() const { return depthMap; }
    glm::mat4 getLightSpaceMatrix() const { return lightSpaceMatrix; }

    void updateLightSpaceMatrix(const glm::vec3& lightDir, const glm::vec3& playerPos);

    unsigned int getResolution() const { return resolution; }

private:
    unsigned int depthMapFBO;
    unsigned int depthMap;
    unsigned int resolution;
    glm::mat4 lightSpaceMatrix;

    void setupFramebuffer();
};
