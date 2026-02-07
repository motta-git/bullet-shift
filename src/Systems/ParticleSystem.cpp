#include "ParticleSystem.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <cmath>

ParticleSystem::ParticleSystem(int maxParticles) 
    : maxParticles(maxParticles), rng(std::random_device{}()) {
    particles.reserve(maxParticles);
    setupBuffers();
}

ParticleSystem::~ParticleSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void ParticleSystem::setupBuffers() {
    // Simple quad for billboard particles
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ParticleSystem::update(float deltaTime) {
    // Backwards-compatible update (no center provided)
    update(deltaTime, glm::vec3(0.0f));
}

void ParticleSystem::update(float deltaTime, const glm::vec3& center) {
    // Atmospheric spawning (spawn near provided center, typically camera)
    if (atmosphereEnabled && atmosphereRate > 0) {
        atmosphereAccumulator += deltaTime * static_cast<float>(atmosphereRate);
        int toSpawn = static_cast<int>(floor(atmosphereAccumulator));
        atmosphereAccumulator -= static_cast<float>(toSpawn);

        for (int i = 0; i < toSpawn; ++i) {
            const float PI = 3.14159265f;
            float theta = randomFloat(0.0f, 2.0f * PI);
            // Use sqrt to give a uniform distribution over area
            float r = atmosphereRadius * sqrt(randomFloat(0.0f, 1.0f));
            float x = r * cos(theta);
            float z = r * sin(theta);
            float y = randomFloat(-atmosphereRadius * 0.25f, atmosphereRadius * 0.5f);

            glm::vec3 pos = center + glm::vec3(x, y, z);
            glm::vec3 vel = glm::vec3(randomFloat(-0.05f, 0.05f), randomFloat(0.01f, 0.12f), randomFloat(-0.05f, 0.05f));
            glm::vec4 color = glm::vec4(randomFloat(0.85f, 1.0f), randomFloat(0.85f, 1.0f), randomFloat(0.9f, 1.0f), randomFloat(0.04f, 0.18f));
            emitParticle(pos, vel, color, randomFloat(3.0f, 7.0f), randomFloat(0.05f, 0.25f));
        }
    }

    // Update all particles
    for (auto& particle : particles) {
        particle.life -= deltaTime;
        if (particle.life > 0.0f) {
            particle.position += particle.velocity * deltaTime;
            // Reduced gravity effect for small particles
            particle.velocity.y -= 9.8f * 0.1f * deltaTime;

            // Smooth fade based on initial life and initial alpha
            if (particle.initialLife > 0.0001f) {
                float lifeRatio = particle.life / particle.initialLife;
                lifeRatio = glm::clamp(lifeRatio, 0.0f, 1.0f);
                particle.color.a = glm::clamp(particle.initialAlpha * lifeRatio, 0.0f, 1.0f);
            } else {
                particle.color.a = glm::clamp(particle.color.a - deltaTime, 0.0f, 1.0f);
            }
        }
    }
    
    // Remove dead particles
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }),
        particles.end()
    );
}
void ParticleSystem::draw(const glm::mat4& projection, const glm::mat4& view, Shader& shader) {
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glBindVertexArray(VAO);
    for (const auto& particle : particles) {
        if (particle.life <= 0.0f) continue;
        shader.setVec3("particlePos", particle.position);
        shader.setFloat("particleSize", particle.size);
        shader.setVec4("particleColor", particle.color);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glBindVertexArray(0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void ParticleSystem::emitExplosion(glm::vec3 position, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = randomFloat(0.0f, 6.28318f);
        float speed = randomFloat(2.0f, 8.0f);
        glm::vec3 velocity(
            cos(angle) * speed,
            randomFloat(2.0f, 6.0f),
            sin(angle) * speed
        );
        
        glm::vec4 color = glm::vec4(
            randomFloat(0.8f, 1.0f),
            randomFloat(0.3f, 0.6f),
            randomFloat(0.0f, 0.3f),
            1.0f
        );
        
        emitParticle(position, velocity, color, randomFloat(0.5f, 1.5f), randomFloat(0.1f, 0.3f));
    }
}

void ParticleSystem::emitFire(glm::vec3 position, int count) {
    for (int i = 0; i < count; ++i) {
        glm::vec3 velocity(
            randomFloat(-0.5f, 0.5f),
            randomFloat(1.0f, 3.0f),
            randomFloat(-0.5f, 0.5f)
        );
        
        glm::vec4 color = glm::vec4(
            randomFloat(0.8f, 1.0f),
            randomFloat(0.2f, 0.5f),
            0.0f,
            1.0f
        );
        
        emitParticle(position, velocity, color, randomFloat(0.3f, 1.0f), randomFloat(0.1f, 0.2f));
    }
}

void ParticleSystem::emitSmoke(glm::vec3 position, int count) {
    for (int i = 0; i < count; ++i) {
        glm::vec3 velocity(
            randomFloat(-0.3f, 0.3f),
            randomFloat(0.5f, 1.5f),
            randomFloat(-0.3f, 0.3f)
        );
        
        float gray = randomFloat(0.3f, 0.6f);
        glm::vec4 color = glm::vec4(gray, gray, gray, 0.8f);
        
        emitParticle(position, velocity, color, randomFloat(1.0f, 2.0f), randomFloat(0.2f, 0.4f));
    }
}

void ParticleSystem::emitMuzzleFlash(glm::vec3 position, glm::vec3 forward, int count) {
    glm::vec3 dir = (glm::length2(forward) > 0.0001f) ? glm::normalize(forward) : glm::vec3(0.0f, 0.0f, 1.0f);

    for (int i = 0; i < count; ++i) {
        glm::vec3 jitter(
            randomFloat(-0.25f, 0.25f),
            randomFloat(-0.05f, 0.25f),
            randomFloat(-0.25f, 0.25f)
        );

        glm::vec3 velocity = glm::normalize(dir + jitter) * randomFloat(8.0f, 14.0f);
        glm::vec4 color = glm::vec4(
            randomFloat(0.9f, 1.0f),
            randomFloat(0.65f, 0.85f),
            randomFloat(0.1f, 0.25f),
            1.0f
        );

        emitParticle(position + dir * 0.05f, velocity, color, randomFloat(0.06f, 0.14f), randomFloat(0.08f, 0.14f));
    }

    // Small trailing smoke puff for added depth (kept minimal for performance)
    int smokeCount = std::max(1, count / 5);
    for (int i = 0; i < smokeCount; ++i) {
        glm::vec3 velocity = dir * randomFloat(1.0f, 3.0f) + glm::vec3(randomFloat(-0.2f, 0.2f), randomFloat(0.2f, 0.6f), randomFloat(-0.2f, 0.2f));
        float gray = randomFloat(0.35f, 0.55f);
        glm::vec4 color = glm::vec4(gray, gray, gray, 0.5f);
        emitParticle(position, velocity, color, randomFloat(0.25f, 0.45f), randomFloat(0.1f, 0.2f));
    }
}

void ParticleSystem::emitParticle(glm::vec3 position, glm::vec3 velocity, 
                                   glm::vec4 color, float life, float size) {
    if (particles.size() < static_cast<size_t>(maxParticles)) {
        Particle particle;
        particle.position = position;
        particle.velocity = velocity;
        particle.color = color;
        particle.life = life;
        particle.initialLife = life;
        particle.size = size;
        particle.initialAlpha = color.a;
        particles.push_back(particle);
    }
}

float ParticleSystem::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

// Atmospheric controls
void ParticleSystem::enableAtmospheric(bool enabled) {
    atmosphereEnabled = enabled;
}

void ParticleSystem::setAtmosphereRate(int particlesPerSecond) {
    atmosphereRate = std::max(0, particlesPerSecond);
}

void ParticleSystem::setAtmosphereRadius(float radius) {
    atmosphereRadius = std::max(0.0f, radius);
}

const std::vector<Particle>& ParticleSystem::getParticles() const {
    return particles;
}
