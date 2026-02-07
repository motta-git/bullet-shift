#pragma once

#include <glm/glm.hpp>

class Projectile {
public:
    Projectile(glm::vec3 position, glm::vec3 direction, float speed, float damage, float lifetime, bool isEnemy = false);

    bool update(float deltaTime); // returns false if dead
    
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getPreviousPosition() const { return previousPosition; }
    glm::vec3 getVelocity() const { return velocity; }
    float getDamage() const { return damage; }
    bool isEnemyProjectile() const { return isEnemy; }
    float getTimeElapsed() const { return timeElapsed; }

private:
    glm::vec3 position;
    glm::vec3 previousPosition;
    glm::vec3 velocity;
    float damage;
    float lifetime;
    float timeElapsed;
    bool isEnemy;
};
