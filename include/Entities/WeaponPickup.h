#pragma once

#include "Weapon.h"
#include <glm/glm.hpp>
#include <memory>

class WeaponPickup {
public:
    WeaponPickup(glm::vec3 position, WeaponType type);
    
    // Check if player is close enough to pick up
    bool canPickup(glm::vec3 playerPosition) const;
    
    // Pick up the weapon (transfers ownership)
    std::unique_ptr<Weapon> pickup();
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    WeaponType getType() const { return weaponType; }
    bool isPickedUp() const { return pickedUp; }
    
private:
    glm::vec3 position;
    WeaponType weaponType;
    bool pickedUp;
    float pickupRange;
};
