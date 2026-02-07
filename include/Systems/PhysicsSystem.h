#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Player.h"
#include "Platform.h"
#include "Enemy.h"
#include "Projectile.h"
#include "WeaponPickup.h"
#include "ParticleSystem.h"

class Game;

class PhysicsSystem {
public:
    PhysicsSystem(Game& game);
    ~PhysicsSystem();

    void update(float deltaTime);

private:
    void updatePlayerPhysics(float deltaTime);
    void updateProjectiles(float deltaTime);
    void handleCollisions();

    Game& m_game;
    float m_deathTimer;
};
