#pragma once

#include <glm/glm.hpp>
#include "Inventory.h"
#include <memory>

class Player {
public:
    Player(glm::vec3 position = glm::vec3(0.0f, 1.0f, 0.0f));
    
    void update(float deltaTime);
    void processMovement(glm::vec3 front, glm::vec3 right, bool moveForward, bool moveBackward, 
                        bool moveLeft, bool moveRight, bool jump, bool dash, float deltaTime);
    
    // Combat
    void takeDamage(float damage, glm::vec3 sourcePosition = glm::vec3(0.0f));
    bool isAlive() const { return health > 0.0f; }
    void reset();
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    
    // Inventory management
    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getSize() const { return size; }
    glm::vec3 getEyePosition() const { return position + glm::vec3(0.0f, eyeHeight, 0.0f); }
    bool isOnGround() const { return onGround; }
    
    // Setters
    void setPosition(glm::vec3 pos) { position = pos; }
    void setOnGround(bool ground) { onGround = ground; }
    void setVelocity(glm::vec3 vel) { velocity = vel; }
    
    glm::vec3 getVelocity() const { return velocity; }

    bool checkFootstep();

    // Dash
    bool isDashing() const { return m_isDashing; }
    float getDashCooldown() const { return m_dashCooldown; }
    void stopDash() { m_isDashing = false; }
    
private:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 size;
    bool onGround;
    float eyeHeight;
    
    float moveSpeed;
    float jumpForce;
    float gravity;
    
    // Health
    float health;
    float maxHealth;
    
    // Inventory
    Inventory inventory;

    float stepCounter = 0.0f;

    // Dash state
    bool m_isDashing = false;
    float m_dashTimer = 0.0f;
    float m_dashCooldown = 0.0f;
    glm::vec3 m_dashDirection = glm::vec3(0.0f);


};
