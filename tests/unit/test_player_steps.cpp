#include <gtest/gtest.h>

#include "Entities/Player.h"
#include "Core/Config.h"

TEST(Player_Footstep, ProcessMovementAccumulatesStepCounterAndTriggersFootstep) {
    Player p;
    p.setOnGround(true);

    // Simulate moving forward enough time to trigger a footstep
    float moveDelta = (Config::Audio::STEP_DISTANCE / Config::MOVE_SPEED) + 0.1f; // ensure crossing threshold
    p.processMovement(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), true, false, false, false, false, false, moveDelta);

    // checkFootstep should now return true (and reset the step counter internally)
    EXPECT_TRUE(p.checkFootstep());
    // Next immediate check should be false until more movement accumulates
    EXPECT_FALSE(p.checkFootstep());
}
