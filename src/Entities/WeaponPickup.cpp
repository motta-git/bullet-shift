#include "WeaponPickup.h"
#include <glm/gtx/norm.hpp>
#include "Config.h"

WeaponPickup::WeaponPickup(glm::vec3 position, WeaponType type)
    : position(position),
      weaponType(type),
      pickedUp(false),
      pickupRange(2.0f) {
}

bool WeaponPickup::canPickup(glm::vec3 playerPosition) const {
    if (pickedUp) {
        return false;
    }
    
    float distanceSquared = glm::length2(playerPosition - position);
    return distanceSquared < (pickupRange * pickupRange);
}

std::unique_ptr<Weapon> WeaponPickup::pickup() {
    if (pickedUp) {
        return nullptr;
    }
    
    pickedUp = true;
    
    auto data = Config::Weapon::getWeaponConfig(weaponType);
    return std::make_unique<Weapon>(
        weaponType, 
        data.name, 
        data.maxAmmo, 
        data.initialReserve, 
        data.fireRate, 
        data.damage, 
        data.range, 
        data.projectileSpeed, 
        data.projectileLifetime,
        data.projectileCount,
        data.spread,
        data.reloadTime,
        data.reloadSoundPath,
        data.pumpTime
    );
}
