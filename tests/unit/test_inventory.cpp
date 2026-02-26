#include <gtest/gtest.h>

#include "Entities/Inventory.h"
#include "Entities/Weapon.h"
#include "Core/Config.h"

TEST(Inventory_AddSwitch_GetCurrent, AddsWeaponToSecondaryAndSwitches) {
    Inventory inv; // starts with primary pistol
    EXPECT_TRUE(inv.getCurrentWeapon() != nullptr);
    EXPECT_TRUE(inv.canPickupWeapon()); // secondary is empty

    auto cfg = Config::Weapon::getWeaponConfig(WeaponType::RIFLE);
    auto rifle = std::make_unique<Weapon>(WeaponType::RIFLE, cfg.name, cfg.maxAmmo, cfg.initialReserve,
                                         cfg.fireRate, cfg.damage, cfg.range, cfg.projectileSpeed,
                                         cfg.projectileLifetime, cfg.projectileCount, cfg.spread,
                                         cfg.reloadTime, cfg.reloadSoundPath, cfg.pumpTime);

    bool added = inv.addWeapon(std::move(rifle));
    EXPECT_TRUE(added);
    EXPECT_FALSE(inv.canPickupWeapon()); // both slots filled now

    Weapon* before = inv.getCurrentWeapon();
    inv.switchWeapon();
    Weapon* after = inv.getCurrentWeapon();
    EXPECT_NE(before, after);

    // Switch back
    inv.switchToPrimary();
    EXPECT_EQ(inv.getCurrentWeapon(), before);
}
