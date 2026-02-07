#include "Inventory.h"
#include "Config.h"

Inventory::Inventory()
    : primary(nullptr),
      secondary(nullptr),
      currentSlot(0) {
    // Start with a pistol
    auto data = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    primary = std::make_unique<Weapon>(
        WeaponType::PISTOL, 
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

bool Inventory::addWeapon(std::unique_ptr<Weapon> weapon) {
    if (!weapon) {
        return false;
    }
    
    // Try to add to empty slot
    if (!primary) {
        primary = std::move(weapon);
        currentSlot = 0;
        return true;
    } else if (!secondary) {
        secondary = std::move(weapon);
        return true;
    }
    
    // Replace current weapon
    if (currentSlot == 0) {
        primary = std::move(weapon);
    } else {
        secondary = std::move(weapon);
    }
    return true;
}

void Inventory::switchWeapon() {
    if (primary && secondary) {
        currentSlot = (currentSlot == 0) ? 1 : 0;
    }
}

void Inventory::switchToPrimary() {
    if (primary) {
        currentSlot = 0;
    }
}

void Inventory::switchToSecondary() {
    if (secondary) {
        currentSlot = 1;
    }
}

Weapon* Inventory::getCurrentWeapon() const {
    if (currentSlot == 0) {
        return primary.get();
    } else {
        return secondary.get();
    }
}

bool Inventory::canPickupWeapon() const {
    return !primary || !secondary;
}

void Inventory::update(float deltaTime) {
    if (primary) {
        primary->update(deltaTime);
    }
    if (secondary) {
        secondary->update(deltaTime);
    }
}
