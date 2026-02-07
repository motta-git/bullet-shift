#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "Shader.h"
#include "Mesh.h"
#include "../Core/Settings.h"

class ResourceManager;

class PostProcessingSystem {
public:
    PostProcessingSystem(int width, int height);
    ~PostProcessingSystem();

    void resize(int width, int height);
    
    // Start rendering to the HDR FBO
    void begin();
    
    // End rendering to HDR FBO and apply post-processing to screen
    void end();
    
    // Perform the post-processing passes and render to the default framebuffer
    void render(unsigned int screenWidth, unsigned int screenHeight, float nearPlane, float farPlane, ResourceManager* rm);

    unsigned int getHDRFBO() const { return hdrFBO; }
    unsigned int getHDRTexture() const { return hdrColorBuffer; }
    unsigned int getDepthTexture() const { return depthBuffer; }

    void setBulletTimeIntensity(float intensity) { m_bulletTimeIntensity = intensity; }

private:
    int width, height;
    float m_bulletTimeIntensity = 0.0f;
    
    unsigned int hdrFBO;
    unsigned int hdrColorBuffer;
    unsigned int depthBuffer; // Standard depth buffer (texture)
    
    // MSAA Buffers (Intermediate)
    unsigned int msaaFBO;
    unsigned int msaaColorBuffer;
    unsigned int msaaDepthBuffer;
    
    // Bloom FBOs
    unsigned int brightFBO;
    unsigned int brightColorBuffer;
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorBuffers[2];
    
    std::unique_ptr<Mesh> screenQuad;
    
    void setupFramebuffers();
    void cleanupFramebuffers();
};
