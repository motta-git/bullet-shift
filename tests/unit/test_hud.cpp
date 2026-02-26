#include <gtest/gtest.h>

#include "UI/HUD.h"

TEST(HUD_FlashHealthBar, ActivatesAndExpires) {
    HUD hud(800, 600);

    EXPECT_FALSE(hud.isHealthFlashActive());

    hud.flashHealthBar(0.5f);
    EXPECT_TRUE(hud.isHealthFlashActive());
    EXPECT_NEAR(hud.getHealthFlashTimer(), 0.5f, 1e-6);

    // Advance half the duration
    hud.update(0.25f);
    EXPECT_TRUE(hud.isHealthFlashActive());
    EXPECT_LT(hud.getHealthFlashTimer(), 0.5f);

    // Advance remaining time -> should expire
    hud.update(0.3f);
    EXPECT_FALSE(hud.isHealthFlashActive());
    EXPECT_EQ(hud.getHealthFlashTimer(), 0.0f);
}

TEST(HUD_NotificationsQueue, PromotionAndExpiry) {
    HUD hud(1024, 768);

    hud.queueNotification("one", 0.1f);
    hud.queueNotification("two", 0.1f);

    EXPECT_EQ(hud.queuedNotificationCount(), 2u);

    // Promote first notification
    hud.updateNotifications(0.0f);
    EXPECT_TRUE(hud.currentNotificationActive());
    EXPECT_EQ(hud.currentNotificationText(), "one");
    EXPECT_EQ(hud.queuedNotificationCount(), 1u);

    // Advance enough time to expire first and immediately promote second
    hud.updateNotifications(0.2f);
    EXPECT_TRUE(hud.currentNotificationActive());
    EXPECT_EQ(hud.currentNotificationText(), "two");
    EXPECT_EQ(hud.queuedNotificationCount(), 0u);

    // Expire the second notification
    hud.updateNotifications(0.2f);
    EXPECT_FALSE(hud.currentNotificationActive());
}
