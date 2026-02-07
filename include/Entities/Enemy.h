#pragma once

#include <memory>
#include <vector>
#include "Entities/Weapon.h"

class NavigationGraph;
class Platform;
class AudioSystem; // Forward declaration for optional audio callback


class Enemy {
public:
    Enemy(glm::vec3 position, WeaponType weaponType = WeaponType::PISTOL);
    
    // Update enemy AI and behavior
    // ... rest of header ...
    void update(float deltaTime, glm::vec3 playerPosition, 
                const NavigationGraph* navGraph,
                const std::vector<Platform>& platforms,
                AudioSystem* audio = nullptr);

    // Alert state accessors
    bool isAlerted() const;
    float getAlertProgress() const; // 1.0 -> just alerted, 0.0 -> faded

    
    // Enemy shooting
    bool shouldShoot(float currentTime) const;
    void shoot(float currentTime);
    
    // Take damage
    void takeDamage(float damage);
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getSize() const { return size; }
    bool isAlive() const { return health > 0.0f; }
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    glm::vec3 getLookDirection() const { return lookDirection; }
    glm::vec3 getVelocity() const { return velocity; }
    bool isOnGround() const { return onGround; }
    Weapon* getWeapon() const { return weapon.get(); }
    
    // Weapon dropping
    bool isWeaponDropped() const { return m_weaponDropped; }
    void setWeaponDropped(bool dropped) { m_weaponDropped = dropped; }
    
    // Check if can see player
    bool canSeePlayer(glm::vec3 playerPosition) const;
    
private:
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 lookDirection;
    
    float health;
    float maxHealth;
    
    float detectionRange;
    std::unique_ptr<Weapon> weapon;
    bool m_weaponDropped;
    
    // Movement physics
    glm::vec3 velocity;
    glm::vec3 acceleration;
    bool onGround;
    float moveSpeed;
    
    // AI state
    bool hasSeenPlayer;
    float timeSinceLastSaw;
    glm::vec3 lastSeenPosition;

    // Alerted visual/audio state
    bool alerted;
    float alertedTimer;
    float alertedDuration; // seconds

    
    // Pathfinding
    std::vector<glm::vec3> currentPath;
    int currentWaypointIndex;
    float pathRecalculateTimer;
    float pathRecalculateInterval;
    
    // Helper methods
    void updateMovement(float deltaTime, glm::vec3 playerPosition,
                       const NavigationGraph* navGraph,
                       const std::vector<Platform>& platforms);
    void followPath(float deltaTime);
    void applyPhysics(float deltaTime, const std::vector<Platform>& platforms);
    bool checkLineOfSight(glm::vec3 playerPosition, const std::vector<Platform>& platforms) const;
};
