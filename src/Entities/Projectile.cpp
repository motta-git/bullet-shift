#include "Projectile.h"

Projectile::Projectile(glm::vec3 position, glm::vec3 direction, float speed, float damage, float lifetime, bool isEnemy)
    : position(position), previousPosition(position), damage(damage), lifetime(lifetime), timeElapsed(0.0f), isEnemy(isEnemy) {
    velocity = glm::normalize(direction) * speed;
}

bool Projectile::update(float deltaTime) {
    lifetime -= deltaTime;
    timeElapsed += deltaTime;
    if (lifetime <= 0.0f) {
        return false;
    }

    previousPosition = position;
    position += velocity * deltaTime;
    
    return true;
}
