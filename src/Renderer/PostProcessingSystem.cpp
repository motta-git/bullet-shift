#include "Renderer/PostProcessingSystem.h"
#include "Renderer/GeometryFactory.h"
#include "Core/ResourceManager.h"
#include "Core/Settings.h"
#include <iostream>
#include <iostream>

PostProcessingSystem::PostProcessingSystem(int width, int height) 
    : width(width), height(height), msaaFBO(0), msaaColorBuffer(0), msaaDepthBuffer(0) {
    setupFramebuffers();
    screenQuad = GeometryFactory::createQuad();
}

PostProcessingSystem::~PostProcessingSystem() {
    cleanupFramebuffers();
}

void PostProcessingSystem::setupFramebuffers() {
    // 1. HDR FBO
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // Color buffer
    glGenTextures(1, &hdrColorBuffer);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0);

    // Depth buffer (as texture for Screen-space Fog)
    glGenTextures(1, &depthBuffer);
    glBindTexture(GL_TEXTURE_2D, depthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "PostProcessingSystem: HDR Framebuffer not complete!" << std::endl;

    // 2. Bright Extraction FBO
    glGenFramebuffers(1, &brightFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);
    glGenTextures(1, &brightColorBuffer);
    glBindTexture(GL_TEXTURE_2D, brightColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightColorBuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "PostProcessingSystem: Bright Framebuffer not complete!" << std::endl;

    // 3. Ping-pong FBOs for blurring
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorBuffers[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "PostProcessingSystem: Pingpong Framebuffer " << i << " not complete!" << std::endl;
    }

    // 4. MSAA FBO
    int samples = Settings::getInstance().window.msaaSamples;
    if (samples > 0) {
        glGenFramebuffers(1, &msaaFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
        
        glGenTextures(1, &msaaColorBuffer);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaColorBuffer);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA16F, width, height, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaaColorBuffer, 0);
        
        glGenRenderbuffers(1, &msaaDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaaDepthBuffer);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "PostProcessingSystem: MSAA Framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessingSystem::cleanupFramebuffers() {
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(1, &hdrColorBuffer);
    glDeleteTextures(1, &depthBuffer);
    
    glDeleteFramebuffers(1, &brightFBO);
    glDeleteTextures(1, &brightColorBuffer);
    
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongColorBuffers);
    
    if (msaaFBO != 0) {
        glDeleteFramebuffers(1, &msaaFBO);
        glDeleteTextures(1, &msaaColorBuffer);
        glDeleteRenderbuffers(1, &msaaDepthBuffer);
        msaaFBO = 0;
    }
}

void PostProcessingSystem::resize(int w, int h) {
    if (width == w && height == h) return;
    width = w;
    height = h;
    cleanupFramebuffers();
    setupFramebuffers();
}

void PostProcessingSystem::begin() {
    if (Settings::getInstance().window.msaaSamples > 0 && msaaFBO != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessingSystem::end() {
    // If using MSAA, resolve (blit) to the standard HDR FBO for post-processing
    if (Settings::getInstance().window.msaaSamples > 0 && msaaFBO != 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFBO); // Resolve to HDR FBO
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
        // Also blit depth if needed? 
        // We technically need the depth buffer in 'depthBuffer' texture for Fog.
        // But MSAA depth resolve is tricky.
        // However, we only used depthBuffer for fog as a sampler. 
        // We can't easily resolve depth to a texture automatically with Blit in all drivers/GL versions correctly for sampling.
        // A common trick without complex shaders:
        // Just accept that Fog might be slightly aliased or blit it.
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessingSystem::render(unsigned int screenWidth, unsigned int screenHeight, float nearPlane, float farPlane, ResourceManager* rm) {
    auto& settings = Settings::getInstance().graphics;
    
    if (!rm) return;

    // 1. Extract bright areas for Bloom
    if (settings.bloomEnabled) {
        Shader* brightShader = rm->getShader("bright_filter");
        if (brightShader) {
            glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);
            glViewport(0, 0, width / 2, height / 2);
            glClear(GL_COLOR_BUFFER_BIT);
            brightShader->use();
            brightShader->setFloat("threshold", settings.bloomThreshold);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
            screenQuad->draw();
        }

        // 2. Blur bright areas
        Shader* blurShader = rm->getShader("bloom_blur");
        if (blurShader) {
            bool horizontal = true, first_iteration = true;
            unsigned int amount = 10;
            blurShader->use();
            for (unsigned int i = 0; i < amount; i++) {
                glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
                blurShader->setBool("horizontal", horizontal);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, first_iteration ? brightColorBuffer : pingpongColorBuffers[!horizontal]);
                screenQuad->draw();
                horizontal = !horizontal;
                if (first_iteration) first_iteration = false;
            }
        }
    }

    // 3. Final Composite
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shader* postShader = rm->getShader("post_processing");
    if (postShader) {
        postShader->use();
        
        // Texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        postShader->setInt("sceneTexture", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[0]); // Result of blurring
        postShader->setInt("bloomBlurTexture", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthBuffer);
        postShader->setInt("depthTexture", 2);

        // Uniforms
        postShader->setBool("bloomEnabled", settings.bloomEnabled);
        postShader->setFloat("bloomIntensity", settings.bloomIntensity);
        postShader->setFloat("exposure", settings.exposure);
        
        postShader->setBool("fogEnabled", settings.fogEnabled);
        postShader->setFloat("fogDensity", settings.fogDensity);
        postShader->setVec3("fogColor", settings.fogColor);
        postShader->setFloat("nearPlane", nearPlane);
        postShader->setFloat("farPlane", farPlane);
        postShader->setFloat("bulletTimeIntensity", m_bulletTimeIntensity);

        screenQuad->draw();
    }
}
