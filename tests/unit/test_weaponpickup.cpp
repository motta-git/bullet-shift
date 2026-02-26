#include <gtest/gtest.h>

#include "Entities/WeaponPickup.h"
#include "Core/Config.h"

TEST(WeaponPickup_Construction, DefaultState) {
    glm::vec3 pos(5.0f, 0.0f, 3.0f);
    WeaponPickup wp(pos, WeaponType::RIFLE);

    EXPECT_EQ(wp.getPosition(), pos);
    EXPECT_EQ(wp.getType(), WeaponType::RIFLE);
    EXPECT_FALSE(wp.isPickedUp());
}

TEST(WeaponPickup_CanPickup, InRange) {
    glm::vec3 pos(0.0f);
    WeaponPickup wp(pos, WeaponType::PISTOL);

    // Player right next to the pickup (within 2.0 unit range)
    EXPECT_TRUE(wp.canPickup(glm::vec3(1.0f, 0.0f, 0.0f)));
}

TEST(WeaponPickup_CanPickup, OutOfRange) {
    glm::vec3 pos(0.0f);
    WeaponPickup wp(pos, WeaponType::PISTOL);

    // Player far from the pickup
    EXPECT_FALSE(wp.canPickup(glm::vec3(50.0f, 0.0f, 0.0f)));
}

TEST(WeaponPickup_CanPickup, ExactlyAtBoundary) {
    glm::vec3 pos(0.0f);
    WeaponPickup wp(pos, WeaponType::PISTOL);

    // Pickup range is 2.0 — distance squared check means at exactly 2.0 it's NOT in range
    // (distanceSq < rangeSq, strict less than)
    EXPECT_FALSE(wp.canPickup(glm::vec3(2.0f, 0.0f, 0.0f)));
    // Just inside should work
    EXPECT_TRUE(wp.canPickup(glm::vec3(1.9f, 0.0f, 0.0f)));
}

TEST(WeaponPickup_Pickup, ReturnsValidWeapon) {
    WeaponPickup wp(glm::vec3(0.0f), WeaponType::RIFLE);

    auto weapon = wp.pickup();
    ASSERT_NE(weapon, nullptr);
    EXPECT_EQ(weapon->getType(), WeaponType::RIFLE);
    EXPECT_TRUE(wp.isPickedUp());
}

TEST(WeaponPickup_Pickup, DoublePickupReturnsNull) {
    WeaponPickup wp(glm::vec3(0.0f), WeaponType::PISTOL);

    auto first = wp.pickup();
    EXPECT_NE(first, nullptr);

    auto second = wp.pickup();
    EXPECT_EQ(second, nullptr);
}

TEST(WeaponPickup_CanPickup, FalseAfterPickedUp) {
    WeaponPickup wp(glm::vec3(0.0f), WeaponType::PISTOL);

    wp.pickup();
    // Even if player is right on top, canPickup should be false
    EXPECT_FALSE(wp.canPickup(glm::vec3(0.0f)));
}
