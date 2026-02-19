#include <gtest/gtest.h>

#include "Entities/HealthPickup.h"
#include "Core/Config.h"

TEST(HealthPickup_Behavior, PickupConsumesOnceAndCanPickup) {
    glm::vec3 pos(0.0f);
    HealthPickup hp(pos, Config::Pickup::HEALTH_AMOUNT);

    // Player within pickup range
    glm::vec3 playerNear(0.0f, 0.0f, 1.0f);
    EXPECT_TRUE(hp.canPickup(playerNear));

    // Pickup first time
    float amt = hp.pickup();
    EXPECT_FLOAT_EQ(amt, Config::Pickup::HEALTH_AMOUNT);
    EXPECT_TRUE(hp.isPickedUp());

    // Further pickups return 0 and canPickup becomes false
    EXPECT_FALSE(hp.canPickup(playerNear));
    EXPECT_FLOAT_EQ(hp.pickup(), 0.0f);
}
