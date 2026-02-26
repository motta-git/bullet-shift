#include <gtest/gtest.h>

#include "Entities/Enemy.h"
#include "Core/Config.h"

// --- Construction ---

TEST(Enemy_Construction, DefaultsAreCorrect) {
    glm::vec3 spawnPos(5.0f, 1.0f, 3.0f);
    Enemy e(spawnPos, WeaponType::PISTOL);

    EXPECT_FLOAT_EQ(e.getHealth(), 100.0f);
    EXPECT_FLOAT_EQ(e.getMaxHealth(), 100.0f);
    EXPECT_TRUE(e.isAlive());
    EXPECT_EQ(e.getPosition(), spawnPos);
    EXPECT_NE(e.getWeapon(), nullptr);
    EXPECT_EQ(e.getWeapon()->getType(), WeaponType::PISTOL);
}

TEST(Enemy_Construction, DifferentWeaponType) {
    Enemy e(glm::vec3(0.0f), WeaponType::RIFLE);

    EXPECT_NE(e.getWeapon(), nullptr);
    EXPECT_EQ(e.getWeapon()->getType(), WeaponType::RIFLE);
}

// --- Damage & Death ---

TEST(Enemy_TakeDamage, ReducesHealth) {
    Enemy e(glm::vec3(0.0f));

    e.takeDamage(30.0f);
    EXPECT_FLOAT_EQ(e.getHealth(), 70.0f);
    EXPECT_TRUE(e.isAlive());
}

TEST(Enemy_TakeDamage, ClampsToZero) {
    Enemy e(glm::vec3(0.0f));

    e.takeDamage(100.0f);
    EXPECT_FLOAT_EQ(e.getHealth(), 0.0f);
    EXPECT_FALSE(e.isAlive());
}

TEST(Enemy_TakeDamage, OverkillStaysAtZero) {
    Enemy e(glm::vec3(0.0f));

    e.takeDamage(9999.0f);
    EXPECT_FLOAT_EQ(e.getHealth(), 0.0f);
    EXPECT_FALSE(e.isAlive());
}

TEST(Enemy_TakeDamage, MultipleDamageEvents) {
    Enemy e(glm::vec3(0.0f));

    e.takeDamage(10.0f);
    e.takeDamage(20.0f);
    e.takeDamage(30.0f);
    EXPECT_FLOAT_EQ(e.getHealth(), 40.0f);
    EXPECT_TRUE(e.isAlive());

    e.takeDamage(40.0f);
    EXPECT_FALSE(e.isAlive());
}

// --- Vision / Detection ---

TEST(Enemy_CanSeePlayer, InRange) {
    Enemy e(glm::vec3(0.0f));

    // Detection range is 30 units; place player at 10 units away
    EXPECT_TRUE(e.canSeePlayer(glm::vec3(10.0f, 0.0f, 0.0f)));
}

TEST(Enemy_CanSeePlayer, OutOfRange) {
    Enemy e(glm::vec3(0.0f));

    // Place player far beyond detection range
    EXPECT_FALSE(e.canSeePlayer(glm::vec3(100.0f, 0.0f, 0.0f)));
}

TEST(Enemy_CanSeePlayer, WhenDead) {
    Enemy e(glm::vec3(0.0f));

    e.takeDamage(100.0f);
    // Dead enemy cannot "see" player even if in range
    EXPECT_FALSE(e.canSeePlayer(glm::vec3(1.0f, 0.0f, 0.0f)));
}

TEST(Enemy_CanSeePlayer, ExactlyAtBoundary) {
    Enemy e(glm::vec3(0.0f));

    // At exactly 30 units, distance < 30 is false
    EXPECT_FALSE(e.canSeePlayer(glm::vec3(30.0f, 0.0f, 0.0f)));
    // Just under 30 units should be visible
    EXPECT_TRUE(e.canSeePlayer(glm::vec3(29.9f, 0.0f, 0.0f)));
}

// --- shouldShoot ---

TEST(Enemy_ShouldShoot, FalseForFreshEnemy) {
    Enemy e(glm::vec3(0.0f));

    // Enemy has not seen player yet -> should not shoot
    EXPECT_FALSE(e.shouldShoot(1.0f));
}

TEST(Enemy_ShouldShoot, FalseWhenDead) {
    Enemy e(glm::vec3(0.0f));

    e.takeDamage(100.0f);
    EXPECT_FALSE(e.shouldShoot(1.0f));
}

// --- Weapon drop flag ---

TEST(Enemy_WeaponDrop, StartsNotDropped) {
    Enemy e(glm::vec3(0.0f));

    EXPECT_FALSE(e.isWeaponDropped());
}

TEST(Enemy_WeaponDrop, CanBeSet) {
    Enemy e(glm::vec3(0.0f));

    e.setWeaponDropped(true);
    EXPECT_TRUE(e.isWeaponDropped());

    e.setWeaponDropped(false);
    EXPECT_FALSE(e.isWeaponDropped());
}

// --- Alert state ---

TEST(Enemy_AlertState, StartsInactive) {
    Enemy e(glm::vec3(0.0f));

    EXPECT_FALSE(e.isAlerted());
    EXPECT_FLOAT_EQ(e.getAlertProgress(), 0.0f);
}
