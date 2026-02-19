#include <gtest/gtest.h>

#include "Entities/Player.h"
#include "Core/Config.h"

TEST(Player_Heal, IncreasesAndClamps) {
    Player p;            // default ctor -> maxHealth = 100

    // Damage player then heal
    p.takeDamage(50.0f);
    EXPECT_FLOAT_EQ(p.getHealth(), 50.0f);

    p.heal(25.0f);
    EXPECT_FLOAT_EQ(p.getHealth(), 75.0f);

    // Over-heal clamps to max
    p.heal(1000.0f);
    EXPECT_FLOAT_EQ(p.getHealth(), p.getMaxHealth());
}

// --- Damage & Death ---

TEST(Player_TakeDamage, ReducesHealth) {
    Player p;

    p.takeDamage(30.0f);
    EXPECT_FLOAT_EQ(p.getHealth(), 70.0f);
    EXPECT_TRUE(p.isAlive());
}

TEST(Player_TakeDamage, ClampsToZero) {
    Player p;

    p.takeDamage(150.0f);
    EXPECT_FLOAT_EQ(p.getHealth(), 0.0f);
    EXPECT_FALSE(p.isAlive());
}

TEST(Player_IsAlive, FalseWhenDead) {
    Player p;

    p.takeDamage(100.0f);
    EXPECT_FALSE(p.isAlive());
}

// --- Reset ---

TEST(Player_Reset, RestoresHealth) {
    Player p;

    p.takeDamage(80.0f);
    EXPECT_FLOAT_EQ(p.getHealth(), 20.0f);

    p.reset();
    EXPECT_FLOAT_EQ(p.getHealth(), p.getMaxHealth());
    EXPECT_TRUE(p.isAlive());
}

TEST(Player_Reset, ClearsVelocity) {
    Player p;

    // Give player some velocity via movement
    p.setOnGround(true);
    p.processMovement(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                      true, false, false, false, false, false, 0.1f);
    glm::vec3 velBefore = p.getVelocity();
    EXPECT_GT(glm::length(velBefore), 0.0f);

    p.reset();
    EXPECT_FLOAT_EQ(glm::length(p.getVelocity()), 0.0f);
}

// --- No movement when dead ---

TEST(Player_Movement, NoEffectWhenDead) {
    Player p;

    p.takeDamage(100.0f);
    EXPECT_FALSE(p.isAlive());

    glm::vec3 posBefore = p.getPosition();
    p.processMovement(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                      true, false, false, false, false, false, 0.5f);
    // Velocity should remain zero (processMovement returns early if dead)
    EXPECT_FLOAT_EQ(p.getVelocity().x, 0.0f);
    EXPECT_FLOAT_EQ(p.getVelocity().z, 0.0f);
}

// --- Dash ---

TEST(Player_Dash, RequiresAirborne) {
    Player p;

    // Set player on ground — dash should NOT activate (dash requires !onGround)
    p.setOnGround(true);
    p.processMovement(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                      true, false, false, false, false, true /* dash */, 0.016f);
    EXPECT_FALSE(p.isDashing());
}

TEST(Player_Dash, ActivatesInAir) {
    Player p;

    // Player is airborne — dash should activate
    p.setOnGround(false);
    p.processMovement(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                      true, false, false, false, false, true /* dash */, 0.016f);
    EXPECT_TRUE(p.isDashing());
    EXPECT_GT(p.getDashCooldown(), 0.0f);

    // Velocity should be at dash speed
    float horizontalSpeed = glm::length(glm::vec3(p.getVelocity().x, 0.0f, p.getVelocity().z));
    EXPECT_NEAR(horizontalSpeed, Config::DASH_SPEED, 0.5f);
}

// --- Fall death ---

TEST(Player_Update, FallDeathBelowThreshold) {
    Player p;

    // Place player far below the death threshold
    p.setPosition(glm::vec3(0.0f, Config::FALL_DEATH_THRESHOLD - 10.0f, 0.0f));
    p.update(0.016f);

    EXPECT_FALSE(p.isAlive());
    EXPECT_FLOAT_EQ(p.getHealth(), 0.0f);
}
