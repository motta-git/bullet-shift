#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>

class Shader;

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float life;
    float initialLife;   // store initial life for smooth fading
    float size;
    float initialAlpha;  // store initial alpha for fading
};

enum ParticleType {
    EXPLOSION,
    FIRE,
    SMOKE
};

class ParticleSystem {
public:
    ParticleSystem(int maxParticles = 1000);
    ~ParticleSystem();
    
    void update(float deltaTime);
    // Overload that accepts a central position (e.g., the camera) for atmospheric spawning
    void update(float deltaTime, const glm::vec3& center);
    void draw(const glm::mat4& projection, const glm::mat4& view, Shader& shader);
    
    // Emit particles based on type
    void emitExplosion(glm::vec3 position, int count = 50);
    void emitFire(glm::vec3 position, int count = 10);
    void emitSmoke(glm::vec3 position, int count = 5);
    void emitMuzzleFlash(glm::vec3 position, glm::vec3 forward, int count = 10);

    // Atmospheric particles: enable/disable and tune density/radius
    void enableAtmospheric(bool enabled);
    void setAtmosphereRate(int particlesPerSecond);
    void setAtmosphereRadius(float radius);
    
    // Get particles for rendering
    const std::vector<Particle>& getParticles() const; 
    
private:
    std::vector<Particle> particles;
    int maxParticles;
    unsigned int VAO, VBO;
    std::mt19937 rng;
    
    // Atmospheric particle settings
    bool atmosphereEnabled = false;
    int atmosphereRate = 8; // particles per second
    float atmosphereRadius = 25.0f;
    float atmosphereAccumulator = 0.0f;

    void setupBuffers();
    void emitParticle(glm::vec3 position, glm::vec3 velocity, glm::vec4 color, float life, float size);
    float randomFloat(float min, float max);
};
