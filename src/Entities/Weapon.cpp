#include "Weapon.h"
#include <algorithm>

Weapon::Weapon(WeaponType type, const std::string& name, int maxAmmo, int reserveAmmo,
               float fireRate, float damage, float range, float projectileSpeed, float projectileLifetime,
               int projectileCount, float spread, float reloadTime, const std::string& reloadSoundPath,
               float pumpTime)
    : type(type),
      name(name),
      currentAmmo(maxAmmo),
      maxAmmo(maxAmmo),
      reserveAmmo(reserveAmmo),
      fireRate(fireRate),
      damage(damage),
      range(range),
      projectileSpeed(projectileSpeed),
      projectileLifetime(projectileLifetime),
      projectileCount(projectileCount),
      spread(spread),
      lastFireTime(0.0f),
      reloading(false),
      reloadTime(reloadTime),
      reloadTimer(0.0f),
      reloadSoundPath(reloadSoundPath),
      pumpTime(pumpTime),
      pumping(false),
      pumpTimer(0.0f) {
}

bool Weapon::fire(float currentTime) {
    // If shotgun and reloading, cancel reload and fire if we have ammo
    if ((type == WeaponType::AUTO_SHOTGUN || type == WeaponType::PUMP_SHOTGUN) && reloading && currentAmmo > 0) {
        reloading = false;
        reloadTimer = 0.0f;
        // Proceed to fire logic below
    } else if (reloading || currentAmmo <= 0) {
        return false;
    }

    // Disallow firing while pump action is in progress
    if (pumping) return false;
    
    float fireInterval = 1.0f / fireRate;
    if (currentTime - lastFireTime < fireInterval) {
        return false;
    }
    
    currentAmmo--;
    lastFireTime = currentTime;

    // If this is a pump-action weapon, start pump timer which will prevent immediate follow-up
    if (pumpTime > 0.0f) {
        pumping = true;
        pumpTimer = 0.0f;
    }

    return true;
}

void Weapon::reload() {
    if (reloading || reserveAmmo <= 0 || currentAmmo == maxAmmo) {
        return;
    }
    
    reloading = true;
    reloadTimer = 0.0f;
}

void Weapon::addAmmo(int amount) {
    reserveAmmo += amount;
}

void Weapon::update(float deltaTime) {
    // Pump action progress (if any)
    if (pumping) {
        pumpTimer += deltaTime;
        if (pumpTimer >= pumpTime) {
            pumping = false;
            pumpTimer = 0.0f;
        }
    }

    if (!reloading) return;

    reloadTimer += deltaTime;
    
    if (type == WeaponType::AUTO_SHOTGUN || type == WeaponType::PUMP_SHOTGUN) {
        if (reloadTimer >= reloadTime) {
            // Add one shell
            if (reserveAmmo > 0 && currentAmmo < maxAmmo) {
                currentAmmo++;
                reserveAmmo--;
                reloadTimer = 0.0f; // Reset for next shell
            }
            
            // Allow "topping off" - only stop if full or out of reserve
            if (currentAmmo >= maxAmmo || reserveAmmo <= 0) {
                reloading = false;
                reloadTimer = 0.0f;
            }
        }
    } else {
        // Standard magazine reload
        if (reloadTimer >= reloadTime) {
            // Transfer ammo from reserve to current
            int ammoNeeded = maxAmmo - currentAmmo;
            int ammoToAdd = std::min(ammoNeeded, reserveAmmo);
            currentAmmo += ammoToAdd;
            reserveAmmo -= ammoToAdd;
            
            reloading = false;
            reloadTimer = 0.0f;
        }
    }
}
