#pragma once

#include "Weapon.h"
#include <memory>
#include <array>

class Inventory {
public:
    Inventory();
    
    // Weapon management
    bool addWeapon(std::unique_ptr<Weapon> weapon);
    void switchWeapon();  // Switch between primary and secondary
    void switchToPrimary();
    void switchToSecondary();
    
    // Get current weapon
    Weapon* getCurrentWeapon() const;
    Weapon* getPrimaryWeapon() const { return primary.get(); }
    Weapon* getSecondaryWeapon() const { return secondary.get(); }
    
    // Check if can pick up weapon
    bool canPickupWeapon() const;
    
    // Update inventory
    void update(float deltaTime);
    
    // Get current slot index (0 = primary, 1 = secondary)
    int getCurrentSlot() const { return currentSlot; }
    
private:
    std::unique_ptr<Weapon> primary;
    std::unique_ptr<Weapon> secondary;
    int currentSlot;  // 0 = primary, 1 = secondary
};
