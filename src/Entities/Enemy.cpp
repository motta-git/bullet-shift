#include "Enemy.h"
#include "Entities/Platform.h"
#include "Systems/NavigationGraph.h"
#include "Systems/RaycastUtility.h"
#include "Systems/AudioSystem.h"
#include "Config.h"
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Enemy::Enemy(glm::vec3 position, WeaponType weaponType)
    : position(position),
      size(0.6f, 1.8f, 0.6f),
      lookDirection(0.0f, 0.0f, -1.0f),
      health(100.0f),
      maxHealth(100.0f),
      detectionRange(30.0f),
      m_weaponDropped(false),
      velocity(0.0f),
      acceleration(0.0f),
      onGround(false),
      moveSpeed(6.0f),
      hasSeenPlayer(false),
      timeSinceLastSaw(0.0f),
      lastSeenPosition(position),
      alerted(false),
      alertedTimer(0.0f),
      alertedDuration(3.0f),
      currentWaypointIndex(0),
      pathRecalculateTimer(0.0f),
      pathRecalculateInterval(0.5f) {
    
    // Initialize enemy weapon
    auto config = Config::Weapon::getWeaponConfig(weaponType);
    weapon = std::make_unique<Weapon>(
        weaponType, config.name, config.maxAmmo, 9999, // Infinite reserve for enemies
        config.fireRate, config.damage, config.range, config.projectileSpeed, 
        config.projectileLifetime, config.projectileCount, config.spread, 
        config.reloadTime, config.reloadSoundPath, config.pumpTime
    );
}  

void Enemy::update(float deltaTime, glm::vec3 playerPosition,
                   const NavigationGraph* navGraph,
                   const std::vector<Platform>& platforms,
                   AudioSystem* audio) {
    if (!isAlive()) {
        return;
    }
    
    // Calculate direction and distance to player
    glm::vec3 toPlayer = playerPosition - position;
    float distanceToPlayer = glm::length(toPlayer);
    
    // Check if can see player
    bool canSee = distanceToPlayer < detectionRange && checkLineOfSight(playerPosition, platforms);
    
    if (canSee) {
        hasSeenPlayer = true;
        timeSinceLastSaw = 0.0f;
        lastSeenPosition = playerPosition;

        // Clear alerted state when regaining sight
        alerted = false;
        alertedTimer = 0.0f;
        
        // Update look direction to face player
        if (distanceToPlayer > 0.1f) {
            lookDirection = glm::normalize(toPlayer);
        }
    } else {
        // Player not visible this frame
        // Start alerted state the first frame we lose sight (if we had seen the player)
        if (!alerted && hasSeenPlayer) {
            alerted = true;
            alertedTimer = 0.0f;
            // Play an alert sound if audio system available
            if (audio) {
                audio->playSound("enemy_alert");
            }
        }

        timeSinceLastSaw += deltaTime;
        if (timeSinceLastSaw > 5.0f) {
            hasSeenPlayer = false;
        }
    }

    // Advance alerted timer (visual fades over alertedDuration)
    if (alerted) {
        alertedTimer += deltaTime;
        if (alertedTimer > alertedDuration) {
            // Let progress fade to zero but keep alerted true for logic until we reach last seen
            alertedTimer = alertedDuration;
        }
    }
    
    // Update weapon state and handle auto-reload if empty
    if (weapon) {
        weapon->update(deltaTime);
        if (weapon->getCurrentAmmo() == 0 && !weapon->isReloading()) {
            weapon->reload();
        }
    }
    
    // Update movement and pathfinding (uses lastSeenPosition if we recently saw the player)
    updateMovement(deltaTime, playerPosition, navGraph, platforms);
    
    // Apply physics (gravity, collision)
    applyPhysics(deltaTime, platforms);
} 

bool Enemy::shouldShoot(float currentTime) const {
    if (!isAlive() || !hasSeenPlayer || !weapon) {
        return false;
    }
    
    // Use weapon's fire logic without consuming ammo yet
    // Since Weapon doesn't have a canFire, we'll check it manually here
    // based on weapon's stats and last fire time is internal.
    // Actually, let's just use weapon->fire() in a way that doesn't consume ammo?
    // No, let's just let Game call shoot() which calls weapon->fire().
    // If weapon->fire() returns true, Game spawns projectile.
    
    // For now, we'll just return true if it's been enough time since last shot.
    // We'll need to know weapon's fireRate.
    // But since Game calls shouldShoot then shoot, we'll just check distance here.
    
    return true; 
}

void Enemy::shoot(float /*currentTime*/) {
    // Logic handled by weapon->fire() in Game.cpp or here.
}

void Enemy::takeDamage(float damage) {
    health -= damage;
    if (health < 0.0f) {
        health = 0.0f;
    }
}

bool Enemy::canSeePlayer(glm::vec3 playerPosition) const {
    float distance = glm::length(playerPosition - position);
    return isAlive() && distance < detectionRange;
}

void Enemy::updateMovement(float deltaTime, glm::vec3 playerPosition,
                           const NavigationGraph* navGraph,
                           const std::vector<Platform>& platforms) {
    if (!navGraph || !navGraph->isValid()) {
        static bool printed = false;
        if (!printed) {
            std::cout << "[Enemy] Navigation graph is invalid or null!" << std::endl;
            printed = true;
        }
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        return;
    }
    
    // If enemy hasn't detected player yet, don't move
    float distToPlayer = glm::length(playerPosition - position);
    if (distToPlayer > detectionRange && !hasSeenPlayer) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        return;
    }
    
    static int debugCounter = 0;
    if (debugCounter++ % 60 == 0) {
        std::cout << "[Enemy] Dist to player: " << distToPlayer << " units" << std::endl;
    }
    
    float distanceToPlayer = glm::length(playerPosition - position);
    bool hasLOS = checkLineOfSight(playerPosition, platforms);
    
    float range = weapon ? weapon->getRange() : 10.0f;
    static int debugCounter2 = 0;
    if (debugCounter2++ % 60 == 0) {
        std::cout << "[Enemy] HasLOS: " << hasLOS << ", DistToPlayer: " << distanceToPlayer 
                  << ", ShootRange: " << range << ", Velocity: (" 
                  << velocity.x << "," << velocity.y << "," << velocity.z << ")" << std::endl;
    }
    
    // If player is visible and in shoot range, stop and shoot
    if (hasLOS && distanceToPlayer <= range) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        return;
    }
    
    // If player is visible but far, chase directly
    if (hasLOS && distanceToPlayer > range) {
        glm::vec3 direction = playerPosition - position;
        direction.y = 0.0f; // Keep movement horizontal
        
        float horizDist = glm::length(direction);
        if (horizDist > 0.1f) {
            direction = direction / horizDist; // Normalize
            velocity.x = direction.x * moveSpeed;
            velocity.z = direction.z * moveSpeed;
        }
        
        // Clear path since we're doing direct chase
        currentPath.clear();
        return;
    }
    
    // Player not visible, use pathfinding
    pathRecalculateTimer += deltaTime;
    if (pathRecalculateTimer >= pathRecalculateInterval) {
        pathRecalculateTimer = 0.0f;
        glm::vec3 target = hasSeenPlayer ? lastSeenPosition : playerPosition;
        currentPath = navGraph->findPath(position, target);
        currentWaypointIndex = 0;
    }
    
    // Follow the path
    followPath(deltaTime);

    // If we were chasing towards the last seen position and we've reached it, stop chasing
    if (hasSeenPlayer) {
        float distToLastSeen = glm::length(lastSeenPosition - position);
        if (distToLastSeen < 1.0f && (currentPath.empty() || currentWaypointIndex >= static_cast<int>(currentPath.size()))) {
            // Reached last known spot; stop chasing
            hasSeenPlayer = false;
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            currentPath.clear();
        }
    }
}

void Enemy::followPath(float /*deltaTime*/) {
    if (currentPath.empty() || currentWaypointIndex >= static_cast<int>(currentPath.size())) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        return;
    }
    
    glm::vec3 targetWaypoint = currentPath[currentWaypointIndex];
    glm::vec3 toWaypoint = targetWaypoint - position;
    toWaypoint.y = 0.0f; // Only move horizontally
    
    float distanceToWaypoint = glm::length(toWaypoint);
    
    // If close to waypoint, move to next one
    if (distanceToWaypoint < 1.0f) {
        currentWaypointIndex++;
        if (currentWaypointIndex >= static_cast<int>(currentPath.size())) {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            return;
        }
        targetWaypoint = currentPath[currentWaypointIndex];
        toWaypoint = targetWaypoint - position;
        toWaypoint.y = 0.0f;
        distanceToWaypoint = glm::length(toWaypoint);
    }
    
    // Move toward waypoint
    if (distanceToWaypoint > 0.1f) {
        glm::vec3 direction = glm::normalize(toWaypoint);
        velocity.x = direction.x * moveSpeed;
        velocity.z = direction.z * moveSpeed;
    } else {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
    }
}

void Enemy::applyPhysics(float deltaTime, const std::vector<Platform>& platforms) {
    // Apply gravity to velocity
    velocity.y -= Config::GRAVITY * deltaTime;

    // Sub-stepping to prevent tunneling through floors during lag or high speed
    const int SUB_STEPS = 4;
    float subDeltaTime = deltaTime / static_cast<float>(SUB_STEPS);

    for (int step = 0; step < SUB_STEPS; step++) {
        // Update position
        position += velocity * subDeltaTime;
        
        // Check collisions with platforms using the unified CheckCollision logic
        onGround = false;
        for (auto& platform : const_cast<std::vector<Platform>&>(platforms)) {
            if (platform.checkCollision(position, size, velocity)) {
                onGround = true;
            }
        }

        // Death threshold check - if enemy falls out of map
        if (position.y < Config::FALL_DEATH_THRESHOLD) {
            std::cout << "[Enemy] Died from falling below threshold! Y: " << position.y << std::endl;
            health = 0.0f;
            break;
        }
    }
}

bool Enemy::checkLineOfSight(glm::vec3 playerPosition, const std::vector<Platform>& platforms) const {
    glm::vec3 eyePos = position + glm::vec3(0.0f, Config::EYE_HEIGHT, 0.0f);
    glm::vec3 playerEyePos = playerPosition + glm::vec3(0.0f, Config::EYE_HEIGHT, 0.0f);
    
    return RaycastUtility::hasLineOfSight(eyePos, playerEyePos, platforms);
}

bool Enemy::isAlerted() const {
    return alerted;
}

float Enemy::getAlertProgress() const {
    if (!alerted || alertedDuration <= 0.0f) return 0.0f;
    // 1.0 when just alerted, linearly decays to 0.0 over alertedDuration
    return std::max(0.0f, 1.0f - (alertedTimer / alertedDuration));
}
