#include <gtest/gtest.h>

#include "Core/Config.h"
#include <cstring>

// Helper: iterate over all "real" weapon types (excludes COUNT)
static constexpr WeaponType ALL_WEAPONS[] = {
    WeaponType::PISTOL,
    WeaponType::RIFLE,
    WeaponType::AUTO_SHOTGUN,
    WeaponType::PUMP_SHOTGUN,
};

// --- Weapon config sanity ---

TEST(Config_Weapons, AllHavePositiveGameplayStats) {
    for (WeaponType wt : ALL_WEAPONS) {
        auto cfg = Config::Weapon::getWeaponConfig(wt);
        EXPECT_GT(cfg.maxAmmo, 0) << "Weapon '" << cfg.name << "' has maxAmmo <= 0";
        EXPECT_GT(cfg.fireRate, 0.0f) << "Weapon '" << cfg.name << "' has fireRate <= 0";
        EXPECT_GT(cfg.damage, 0.0f) << "Weapon '" << cfg.name << "' has damage <= 0";
        EXPECT_GT(cfg.range, 0.0f) << "Weapon '" << cfg.name << "' has range <= 0";
        EXPECT_GT(cfg.projectileSpeed, 0.0f) << "Weapon '" << cfg.name << "' has projectileSpeed <= 0";
        EXPECT_GT(cfg.projectileLifetime, 0.0f) << "Weapon '" << cfg.name << "' has projectileLifetime <= 0";
        EXPECT_GT(cfg.projectileCount, 0) << "Weapon '" << cfg.name << "' has projectileCount <= 0";
        EXPECT_GT(cfg.reloadTime, 0.0f) << "Weapon '" << cfg.name << "' has reloadTime <= 0";
    }
}

TEST(Config_Weapons, AllHaveNames) {
    for (WeaponType wt : ALL_WEAPONS) {
        auto cfg = Config::Weapon::getWeaponConfig(wt);
        EXPECT_NE(cfg.name, nullptr);
        EXPECT_GT(strlen(cfg.name), 0u) << "Weapon type " << static_cast<int>(wt) << " has empty name";
    }
}

TEST(Config_Weapons, AllHaveModelPaths) {
    for (WeaponType wt : ALL_WEAPONS) {
        auto cfg = Config::Weapon::getWeaponConfig(wt);
        EXPECT_NE(cfg.modelPath, nullptr);
        EXPECT_GT(strlen(cfg.modelPath), 0u) << "Weapon '" << cfg.name << "' has empty modelPath";
    }
}

TEST(Config_Weapons, PumpShotgunHasPumpTimeOthersDoNot) {
    auto pump = Config::Weapon::getWeaponConfig(WeaponType::PUMP_SHOTGUN);
    EXPECT_GT(pump.pumpTime, 0.0f) << "Pump shotgun should have pumpTime > 0";

    auto pistol = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    EXPECT_FLOAT_EQ(pistol.pumpTime, 0.0f) << "Pistol should have pumpTime == 0";

    auto rifle = Config::Weapon::getWeaponConfig(WeaponType::RIFLE);
    EXPECT_FLOAT_EQ(rifle.pumpTime, 0.0f) << "Rifle should have pumpTime == 0";

    auto autoShotgun = Config::Weapon::getWeaponConfig(WeaponType::AUTO_SHOTGUN);
    EXPECT_FLOAT_EQ(autoShotgun.pumpTime, 0.0f) << "Auto shotgun should have pumpTime == 0";
}

TEST(Config_Weapons, ShotgunsHaveMultiplePellets) {
    auto autoSg = Config::Weapon::getWeaponConfig(WeaponType::AUTO_SHOTGUN);
    EXPECT_GT(autoSg.projectileCount, 1) << "Auto shotgun should fire multiple pellets";

    auto pumpSg = Config::Weapon::getWeaponConfig(WeaponType::PUMP_SHOTGUN);
    EXPECT_GT(pumpSg.projectileCount, 1) << "Pump shotgun should fire multiple pellets";

    // Non-shotguns should fire single projectiles
    auto pistol = Config::Weapon::getWeaponConfig(WeaponType::PISTOL);
    EXPECT_EQ(pistol.projectileCount, 1);

    auto rifle = Config::Weapon::getWeaponConfig(WeaponType::RIFLE);
    EXPECT_EQ(rifle.projectileCount, 1);
}

// --- Level config sanity ---

TEST(Config_Levels, AllHaveValidConfig) {
    for (size_t i = 0; i < Config::Levels::LEVEL_CONFIGS.size(); ++i) {
        const auto& lc = Config::Levels::LEVEL_CONFIGS[i];
        EXPECT_NE(lc.name, nullptr) << "Level " << i << " has null name";
        EXPECT_GT(strlen(lc.name), 0u) << "Level " << i << " has empty name";
        EXPECT_NE(lc.skyboxPath, nullptr) << "Level " << i << " has null skyboxPath";
        EXPECT_GT(strlen(lc.skyboxPath), 0u) << "Level " << i << " has empty skyboxPath";
    }
}

// --- Global constants sanity ---

TEST(Config_Global, PhysicsConstantsAreSane) {
    EXPECT_GT(Config::GRAVITY, 0.0f);
    EXPECT_GT(Config::JUMP_FORCE, 0.0f);
    EXPECT_GT(Config::MOVE_SPEED, 0.0f);
    EXPECT_GT(Config::PLAYER_ACCELERATION, 0.0f);
    EXPECT_GT(Config::PLAYER_DECELERATION, 0.0f);
}

TEST(Config_Global, PlayerDimensionsAreSane) {
    EXPECT_GT(Config::PLAYER_WIDTH, 0.0f);
    EXPECT_GT(Config::PLAYER_HEIGHT, 0.0f);
    EXPECT_GT(Config::PLAYER_DEPTH, 0.0f);
    // Eye height should be less than half the player height (relative to center)
    EXPECT_LT(Config::EYE_HEIGHT, Config::PLAYER_HEIGHT / 2.0f);
}

TEST(Config_Global, DashSettingsAreSane) {
    EXPECT_GT(Config::DASH_DURATION, 0.0f);
    EXPECT_GT(Config::DASH_SPEED, Config::MOVE_SPEED) << "Dash speed should exceed normal move speed";
    EXPECT_GT(Config::DASH_COOLDOWN, 0.0f);
}

TEST(Config_Global, BulletTimeSettingsAreSane) {
    EXPECT_GT(Config::MAX_BULLET_TIME_ENERGY, 0.0f);
    EXPECT_GT(Config::BULLET_TIME_DRAIN_RATE, 0.0f);
    EXPECT_GT(Config::BULLET_TIME_REGEN_RATE, 0.0f);
    EXPECT_GT(Config::MIN_BULLET_TIME_SCALE, 0.0f);
    EXPECT_LT(Config::MIN_BULLET_TIME_SCALE, 1.0f) << "Bullet time scale must slow things down";
}
