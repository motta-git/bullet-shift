#include <gtest/gtest.h>

#include "Entities/Weapon.h"
#include "Core/Config.h"


TEST(Weapon_FireAndRateLimit, ConsumesAmmoAndRespectsFireRate) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    int before = w.getCurrentAmmo();
    EXPECT_GT(before, 0);

    // Fire at an explicit non-zero time (weapon tracks lastFireTime)
    EXPECT_TRUE(w.fire(1.0f));
    EXPECT_EQ(w.getCurrentAmmo(), before - 1);

    // Fire too soon -> should be blocked by fireRate
    EXPECT_FALSE(w.fire(1.1f)); // pistol fireRate is ~3.0 -> interval ~= 0.333s

    // Fire after enough time -> allowed
    EXPECT_TRUE(w.fire(2.0f));
}

TEST(Weapon_ReloadCompletesAfterTime, ReloadRefillsToMax) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Empty current ammo by firing until zero (advance time each shot)
    float currentTime = 1000.0f;
    while (w.getCurrentAmmo() > 0) {
        EXPECT_TRUE(w.fire(currentTime));
        currentTime += 1.0f; // advance time so rate-limit isn't a factor
    }
    EXPECT_EQ(w.getCurrentAmmo(), 0);

    // Request reload and advance time
    w.reload();
    EXPECT_TRUE(w.isReloading());

    w.update(cfg.reloadTime + 0.01f);
    EXPECT_FALSE(w.isReloading());
    EXPECT_EQ(w.getCurrentAmmo(), w.getMaxAmmo());
}

// Intentionally failing test to verify build is blocked when unit tests fail
/*TEST(ForceFailure_Demo, IntentionalFail) {
    // This assertion is expected to fail on purpose
    EXPECT_EQ(1, 2);
}
*/

// --- Empty magazine ---

TEST(Weapon_EmptyMag, CannotFire) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Drain the magazine
    float t = 1000.0f;
    while (w.getCurrentAmmo() > 0) {
        w.fire(t);
        t += 1.0f;
    }
    EXPECT_EQ(w.getCurrentAmmo(), 0);

    // Firing with empty mag should fail
    EXPECT_FALSE(w.fire(t + 1.0f));
}

// --- Reload blocking ---

TEST(Weapon_ReloadBlocking, CannotFireDuringReload) {
    // Use a PISTOL (non-shotgun) — reload blocks fire
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Fire once to ensure we're not at full ammo
    w.fire(1.0f);
    EXPECT_LT(w.getCurrentAmmo(), w.getMaxAmmo());

    w.reload();
    EXPECT_TRUE(w.isReloading());

    // Attempt to fire while reloading — should be blocked for non-shotgun
    EXPECT_FALSE(w.fire(2.0f));
}

TEST(Weapon_ShotgunReloadCancel, CanFireDuringReload) {
    // Shotguns can cancel reload and fire if they have ammo
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::AUTO_SHOTGUN);
    ::Weapon w(WeaponType::AUTO_SHOTGUN, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Fire once to make room for reload
    w.fire(1.0f);
    int ammoAfterFire = w.getCurrentAmmo();
    EXPECT_LT(ammoAfterFire, w.getMaxAmmo());

    // Start reload
    w.reload();
    EXPECT_TRUE(w.isReloading());

    // Fire during reload — shotgun should allow it
    EXPECT_TRUE(w.fire(2.0f));
    EXPECT_FALSE(w.isReloading()); // Reload should be cancelled
}

// --- Reserve & addAmmo ---

TEST(Weapon_ReloadFromReserve, ReserveDecreasesCorrectly) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Fire all ammo
    float t = 1000.0f;
    while (w.getCurrentAmmo() > 0) {
        w.fire(t);
        t += 1.0f;
    }
    int reserveBefore = w.getReserveAmmo();

    // Reload
    w.reload();
    w.update(cfg.reloadTime + 0.01f);

    EXPECT_EQ(w.getCurrentAmmo(), w.getMaxAmmo());
    EXPECT_EQ(w.getReserveAmmo(), reserveBefore - w.getMaxAmmo());
}

TEST(Weapon_AddAmmo, IncreasesReserve) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    int before = w.getReserveAmmo();
    w.addAmmo(50);
    EXPECT_EQ(w.getReserveAmmo(), before + 50);
}

// --- Pump action ---

TEST(Weapon_PumpAction, BlocksImmediateFollowUpFire) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PUMP_SHOTGUN);
    ::Weapon w(WeaponType::PUMP_SHOTGUN, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // First shot should succeed
    EXPECT_TRUE(w.fire(1.0f));
    EXPECT_TRUE(w.isPumping());

    // Immediate follow-up should be blocked by pump
    EXPECT_FALSE(w.fire(1.5f));
}

TEST(Weapon_PumpAction, FiresAfterPumpCompletes) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PUMP_SHOTGUN);
    ::Weapon w(WeaponType::PUMP_SHOTGUN, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    EXPECT_TRUE(w.fire(1.0f));
    EXPECT_TRUE(w.isPumping());

    // Advance time past pump duration
    w.update(cfg.pumpTime + 0.01f);
    EXPECT_FALSE(w.isPumping());

    // Now fire again (also need to respect fire rate interval)
    float fireInterval = 1.0f / cfg.fireRate;
    EXPECT_TRUE(w.fire(1.0f + fireInterval + 0.01f));
}

TEST(Weapon_PumpAction, PumpProgressAdvances) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PUMP_SHOTGUN);
    ::Weapon w(WeaponType::PUMP_SHOTGUN, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    EXPECT_FLOAT_EQ(w.getPumpProgress(), 0.0f);

    w.fire(1.0f);
    EXPECT_TRUE(w.isPumping());

    // Advance halfway through pump
    w.update(cfg.pumpTime / 2.0f);
    EXPECT_NEAR(w.getPumpProgress(), 0.5f, 0.05f);
}

// --- Shotgun shell-by-shell reload ---

TEST(Weapon_ShotgunReload, AddsShellsOneAtATime) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::AUTO_SHOTGUN);
    ::Weapon w(WeaponType::AUTO_SHOTGUN, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Fire 3 shots to create room
    float t = 1000.0f;
    for (int i = 0; i < 3; ++i) {
        w.fire(t);
        t += 1.0f;
    }
    int ammoAfterFire = w.getCurrentAmmo();
    EXPECT_EQ(ammoAfterFire, cfg.maxAmmo - 3);

    // Start reload
    w.reload();
    EXPECT_TRUE(w.isReloading());

    // After one reload cycle, one shell should be added
    w.update(cfg.reloadTime + 0.01f);
    EXPECT_EQ(w.getCurrentAmmo(), ammoAfterFire + 1);
    // Should still be reloading (not full yet)
    EXPECT_TRUE(w.isReloading());
}

// --- No-op reload scenarios ---

TEST(Weapon_Reload, NoEffectWhenFull) {
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, cfg.initialReserve,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Magazine is already full
    EXPECT_EQ(w.getCurrentAmmo(), w.getMaxAmmo());
    w.reload();
    EXPECT_FALSE(w.isReloading());
}

TEST(Weapon_Reload, NoEffectWithoutReserve) {
    // Create weapon with zero reserve
    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    ::Weapon w(WeaponType::PISTOL, cfg.name, cfg.maxAmmo, 0 /* no reserve */,
             cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
             cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
             cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    // Fire once to not be full
    w.fire(1.0f);
    EXPECT_LT(w.getCurrentAmmo(), w.getMaxAmmo());

    // Attempt reload with 0 reserve
    w.reload();
    EXPECT_FALSE(w.isReloading());
}