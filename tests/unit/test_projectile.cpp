#include <gtest/gtest.h>

#include "Entities/Projectile.h"
#include <glm/glm.hpp>

TEST(Projectile_Lifetime, UpdateExpiresAfterLifetime) {
    Projectile p(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 10.0f, 5.0f, 0.5f);

    // First update: should be alive
    EXPECT_TRUE(p.update(0.2f));
    EXPECT_NEAR(p.getTimeElapsed(), 0.2f, 1e-4);

    // Second update: exceed lifetime -> should return false (dead)
    EXPECT_FALSE(p.update(0.4f));
    EXPECT_GE(p.getTimeElapsed(), 0.5f);
}
