#include "PhysicsSystem.h"
#include "Game.h"
#include "DebugRenderer.h"
#include "Config.h"
#include <iostream>

PhysicsSystem::PhysicsSystem(Game& game) : m_game(game), m_deathTimer(0.0f) {}

PhysicsSystem::~PhysicsSystem() {}

void PhysicsSystem::update(float deltaTime) {
    updatePlayerPhysics(deltaTime);
    updateProjectiles(deltaTime);
    handleCollisions();
}

void PhysicsSystem::updatePlayerPhysics(float deltaTime) {
    glm::vec3 playerPos = m_game.player.getPosition();
    glm::vec3 playerVel = m_game.player.getVelocity();
    bool onPlatform = false;

    // Clamp deltaTime to prevent huge jumps (e.g. after loading or long hitches)
    float activeDt = std::min(deltaTime, 0.05f);
    
    // Sub-stepping to prevent tunneling through walls during high-speed movement (like dashing)
    const int SUB_STEPS = 4;
    float subDeltaTime = activeDt / (float)SUB_STEPS;

    for (int step = 0; step < SUB_STEPS; step++) {
        // 1. Move a small amount
        playerPos += playerVel * subDeltaTime;

        // 2. Resolve collisions
        glm::vec3 preResolutionVel = playerVel;
        bool stepOnPlatform = false;
        
        for (auto& platform : m_game.platforms) {
            if (platform.checkCollision(playerPos, m_game.player.getSize(), playerVel)) {
                stepOnPlatform = true;
                // If we were falling extremely fast (e.g. just spawned and jumped into physics), reset vertical velocity
                if (playerVel.y < -10.0f) playerVel.y = 0.0f;
            }
        }
        
        if (stepOnPlatform) onPlatform = true;

        // 3. Detect if we hit a wall horizontally during a dash
        if (m_game.player.isDashing()) {
            bool hitWallX = (std::abs(preResolutionVel.x) > 0.1f && std::abs(playerVel.x) < 0.001f);
            bool hitWallZ = (std::abs(preResolutionVel.z) > 0.1f && std::abs(playerVel.z) < 0.001f);
            
            if (hitWallX || hitWallZ) {
                m_game.player.stopDash();
                // We stop the dash immediately to prevent further penetration/jitter
                break; 
            }
        }
    }

    m_game.player.setPosition(playerPos);
    m_game.player.setOnGround(onPlatform);
    m_game.player.setVelocity(playerVel);

    static bool wasOnGround = m_game.player.isOnGround();
    if (onPlatform && !wasOnGround && playerVel.y < -3.0f) {
        if (m_game.particleSystem) {
            m_game.particleSystem->emitSmoke(m_game.player.getPosition(), 15);
        }
    }
    wasOnGround = onPlatform;

    if (m_game.particleSystem) m_game.particleSystem->update(deltaTime, m_game.camera.Position);
    if (m_game.debugRenderer) m_game.debugRenderer->update(deltaTime);
}

void PhysicsSystem::updateProjectiles(float deltaTime) {
    for (auto it = m_game.projectiles.begin(); it != m_game.projectiles.end();) {
        if (!it->update(deltaTime)) {
            it = m_game.projectiles.erase(it);
            continue;
        }
        
        if (m_game.debugRenderer) {
            glm::vec3 color = it->isEnemyProjectile() ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 0.0f);
            m_game.debugRenderer->addLine(it->getPreviousPosition(), it->getPosition(), color, 0.2f);
        }
        ++it;
    }
}

void PhysicsSystem::handleCollisions() {
    for (auto it = m_game.projectiles.begin(); it != m_game.projectiles.end();) {
        bool hit = false;
        glm::vec3 pPos = it->getPosition();

        if (it->isEnemyProjectile()) {
            glm::vec3 playerCenter = m_game.player.getPosition() + glm::vec3(0.0f, m_game.player.getSize().y * 0.5f, 0.0f);
            if (glm::distance(pPos, playerCenter) < 1.0f) {
                m_game.player.takeDamage(it->getDamage(), pPos);
                if (m_game.hud) m_game.hud->onDamageTaken(m_game.player.getPosition(), m_game.camera.Front, pPos);
                if (m_game.particleSystem) m_game.particleSystem->emitSmoke(m_game.player.getPosition(), 5);
                hit = true;
            }
        } else {
            for (auto& enemy : m_game.enemies) {
                if (enemy.isAlive() && glm::distance(pPos, enemy.getPosition()) < 1.0f) {
                    enemy.takeDamage(it->getDamage());
                    if (m_game.particleSystem) m_game.particleSystem->emitExplosion(pPos, 2);
                    
                    if (!enemy.isAlive()) {
                        // Drop weapon if not already dropped
                        if (!enemy.isWeaponDropped() && enemy.getWeapon()) {
                            m_game.weaponPickups.emplace_back(enemy.getPosition(), enemy.getWeapon()->getType());
                            enemy.setWeaponDropped(true);
                        }

                        bool anyOtherAlive = false;
                        for (const auto& e : m_game.enemies) {
                            if (&e != &enemy && e.isAlive()) {
                                anyOtherAlive = true;
                                break;
                            }
                        }
                        if (anyOtherAlive) {
                            m_game.triggerBulletTime();
                        }
                    }

                    hit = true;
                    break;
                }
            }
        }

        if (!hit) {
            for (const auto& platform : m_game.platforms) {
                if (platform.checkRayCollision(it->getPreviousPosition(), pPos)) {
                    std::cout << "[Physics] Projectile hit platform! Pos: " << pPos.x << "," << pPos.y << "," << pPos.z << std::endl;
                    if (m_game.particleSystem) m_game.particleSystem->emitExplosion(pPos, 1);
                    hit = true;
                    break;
                }
            }
        }

        if (hit) {
            it = m_game.projectiles.erase(it);
        } else {
            ++it;
        }
    }

    if (!m_game.player.isAlive()) {
        m_deathTimer += 0.016f; 
        if (m_deathTimer > 2.0f) {
            m_game.resetLevel();
            m_deathTimer = 0.0f;
        }
    }
}
