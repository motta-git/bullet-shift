#pragma once

#include <string>
#include <glm/glm.hpp>

enum class WeaponType {
    PISTOL,
    RIFLE,
    AUTO_SHOTGUN,
    PUMP_SHOTGUN,
    COUNT
};

class Weapon {
public:
    Weapon(WeaponType type, const std::string& name, int maxAmmo, int reserveAmmo, 
           float fireRate, float damage, float range, float projectileSpeed, float projectileLifetime,
           int projectileCount, float spread, float reloadTime, const std::string& reloadSoundPath,
           float pumpTime = 0.0f);
    
    // Fire the weapon
    bool fire(float currentTime);
    
    // Reload the weapon
    void reload();
    
    // Add ammo to reserve
    void addAmmo(int amount);
    
    // Getters
    WeaponType getType() const { return type; }
    std::string getName() const { return name; }
    int getCurrentAmmo() const { return currentAmmo; }
    int getReserveAmmo() const { return reserveAmmo; }
    int getMaxAmmo() const { return maxAmmo; }
    float getDamage() const { return damage; }
    float getRange() const { return range; }
    float getProjectileSpeed() const { return projectileSpeed; }
    float getProjectileLifetime() const { return projectileLifetime; }
    int getProjectileCount() const { return projectileCount; }
    float getSpread() const { return spread; }
    bool isReloading() const { return reloading; }
    std::string getReloadSoundPath() const { return reloadSoundPath; }

    // Pump action accessors
    bool isPumping() const { return pumping; }
    // 0.0 -> 1.0 progress of pump animation
    float getPumpProgress() const { return (pumpTime > 0.0f) ? (pumpTimer / pumpTime) : 0.0f; }
    
    // Update weapon state
    void update(float deltaTime);
    
private:
    WeaponType type;
    std::string name;
    int currentAmmo;
    int maxAmmo;
    int reserveAmmo;
    float fireRate;  // Shots per second
    float damage;
    float range;  // Range for hitscan or max distance
    float projectileSpeed;
    float projectileLifetime;
    int projectileCount;
    float spread;
    float lastFireTime;
    bool reloading;
    float reloadTime;
    float reloadTimer;
    std::string reloadSoundPath;

    // Pump-action state
    float pumpTime; // seconds required to pump after a shot
    bool pumping;
    float pumpTimer;
};
