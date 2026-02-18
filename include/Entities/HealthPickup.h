#pragma once

#include <glm/glm.hpp>
#include "Core/Config.h"

class HealthPickup {
public:
    HealthPickup(glm::vec3 position, float amount = Config::Pickup::HEALTH_AMOUNT);

    // Check if player is close enough to auto-collect
    bool canPickup(glm::vec3 playerPosition) const;

    // Pick up the health; returns the heal amount (0.0 if already picked)
    float pickup();

    // Getters
    glm::vec3 getPosition() const { return position; }
    bool isPickedUp() const { return pickedUp; }

private:
    glm::vec3 position;
    float amount;
    bool pickedUp;
    float pickupRange;
};
