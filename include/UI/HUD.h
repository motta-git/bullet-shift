#pragma once

#include <memory>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <queue>
#include "Shader.h"
#include "Config.h"

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
    void queueNotification(const std::string& text, float displayTime = Config::UI::NOTIFICATION_DEFAULT);
    void updateNotifications(float deltaTime);

    void onDamageTaken(glm::vec3 playerPos, glm::vec3 playerFront, glm::vec3 sourcePos);

    // Health bar flash when picking up health
    void flashHealthBar(float duration = Config::Pickup::HEALTH_FLASH_DURATION);

    void update(float deltaTime);

    // --- Test helpers / safe accessors ---------------------------------
    // Small read-only accessors used by unit tests to verify HUD state.
    bool isHealthFlashActive() const { return m_healthFlashActive; }
    float getHealthFlashTimer() const { return m_healthFlashTimer; }
    float getHealthFlashDuration() const { return m_healthFlashDuration; }

    size_t queuedNotificationCount() const { return m_notificationQueue.size(); }
    bool currentNotificationActive() const { return m_currentNotification.active; }
    std::string currentNotificationText() const { return m_currentNotification.text; }

private:
    void renderNotificationPopup();
    void renderDamageIndicators(ImDrawList* drawList, float screenWidth, float screenHeight, float scale);
    
    std::queue<Notification> m_notificationQueue;
    Notification m_currentNotification;
    std::vector<DamageIndicator> m_damageIndicators;

    // Health flash state
    bool m_healthFlashActive = false;
    float m_healthFlashTimer = 0.0f;
    float m_healthFlashDuration = Config::Pickup::HEALTH_FLASH_DURATION;
};
