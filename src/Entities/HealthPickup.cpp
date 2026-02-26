#include "HealthPickup.h"
#include <glm/gtx/norm.hpp>

HealthPickup::HealthPickup(glm::vec3 position, float amount)
    : position(position), amount(amount), pickedUp(false), pickupRange(2.0f) {
}

bool HealthPickup::canPickup(glm::vec3 playerPosition) const {
    if (pickedUp) return false;
    float distanceSquared = glm::length2(playerPosition - position);
    return distanceSquared < (pickupRange * pickupRange);
}

float HealthPickup::pickup() {
    if (pickedUp) return 0.0f;
    pickedUp = true;
    return amount;
}
