#pragma once

#include <memory>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <queue>
#include "Shader.h"

struct ImDrawList;

struct Notification {
    std::string text;
    float displayTime;
    float currentTime;
    bool active;
};

struct DamageIndicator {
    float angle; // Direction in radians relative to player front
    float lifetime;
    float maxLifetime;
};

class HUD {
public:
    HUD(unsigned int screenWidth, unsigned int screenHeight);
    ~HUD();
    
    void render(int health, int maxHealth, const std::string& weaponName, 
                int currentAmmo, int reserveAmmo, bool reloading, int enemyCount, const std::string& interactionPrompt = "",
                float bulletTimeEnergy = 0.0f, float maxBulletTimeEnergy = 100.0f, bool bulletTimeActive = false);
    
    void renderDeathScreen();
    
    // Popup notification system
    void queueNotification(const std::string& text, float displayTime = 5.0f);
    void updateNotifications(float deltaTime);

    void onDamageTaken(glm::vec3 playerPos, glm::vec3 playerFront, glm::vec3 sourcePos);
    void update(float deltaTime);

private:
    void renderNotificationPopup();
    void renderDamageIndicators(ImDrawList* drawList, float screenWidth, float screenHeight, float scale);
    
    std::queue<Notification> m_notificationQueue;
    Notification m_currentNotification;
    std::vector<DamageIndicator> m_damageIndicators;
};
