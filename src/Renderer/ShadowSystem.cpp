#include "Renderer/ShadowSystem.h"
#include <iostream>

ShadowSystem::ShadowSystem(unsigned int resolution) : resolution(resolution) {
    setupFramebuffer();
}

ShadowSystem::~ShadowSystem() {
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
}

void ShadowSystem::setupFramebuffer() {
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                 resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ShadowSystem: Framebuffer not complete. Status: 0x"
                  << std::hex << status << std::dec << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowSystem::bindForWriting() {
    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowSystem::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowSystem::updateLightSpaceMatrix(const glm::vec3& lightDir, const glm::vec3& playerPos) {
    // For a directional light, we use orthographic projection
    // We want the shadow box to follow the player to some extent
    float near_plane = 1.0f, far_plane = 100.0f;
    float boxSize = 25.0f;
    glm::mat4 lightProjection = glm::ortho(-boxSize, boxSize, -boxSize, boxSize, near_plane, far_plane);
    
    // Light looks at the player area from some distance away in light direction
    glm::vec3 lightPos = playerPos - glm::normalize(lightDir) * 40.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, playerPos, glm::vec3(0.0, 1.0, 0.0));
    
    lightSpaceMatrix = lightProjection * lightView;
}
