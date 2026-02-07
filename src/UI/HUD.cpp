#include "HUD.h"
#include "imgui.h"
#include "Config.h"
#include <string>

HUD::HUD(unsigned int /*screenWidth*/, unsigned int /*screenHeight*/) {
    m_currentNotification = {"" , 0.0f, 0.0f, false};
}

HUD::~HUD() {
}

void HUD::queueNotification(const std::string& text, float displayTime) {
    Notification notif;
    notif.text = text;
    notif.displayTime = displayTime;
    notif.currentTime = 0.0f;
    notif.active = false;
    m_notificationQueue.push(notif);
}

void HUD::updateNotifications(float deltaTime) {
    // If there's an active notification, update its timer
    if (m_currentNotification.active) {
        m_currentNotification.currentTime += deltaTime;
        if (m_currentNotification.currentTime >= m_currentNotification.displayTime) {
            m_currentNotification.active = false;
        }
    }
    
    // If no active notification and there's one in the queue, show it
    if (!m_currentNotification.active && !m_notificationQueue.empty()) {
        m_currentNotification = m_notificationQueue.front();
        m_currentNotification.active = true;
        m_notificationQueue.pop();
    }
}

void HUD::onDamageTaken(glm::vec3 playerPos, glm::vec3 playerFront, glm::vec3 sourcePos) {
    if (glm::length(sourcePos) < 0.001f) return; // Unknown source

    glm::vec3 toSource = glm::normalize(sourcePos - playerPos);
    
    // Project vectors to flat XZ plane for horizontal direction
    glm::vec2 sourceDir(toSource.x, toSource.z);
    glm::vec2 frontDir(playerFront.x, playerFront.z);
    
    if (glm::length(sourceDir) < 0.001f || glm::length(frontDir) < 0.001f) return;
    
    sourceDir = glm::normalize(sourceDir);
    frontDir = glm::normalize(frontDir);
    
    // Calculate relative angle
    // atan2(y, x) gives angle in [-pi, pi]
    float sourceAngle = atan2(sourceDir.y, sourceDir.x);
    float frontAngle = atan2(frontDir.y, frontDir.x);
    
    float relativeAngle = sourceAngle - frontAngle;
    
    // Normalize to [-pi, pi]
    while (relativeAngle > glm::pi<float>()) relativeAngle -= 2.0f * glm::pi<float>();
    while (relativeAngle < -glm::pi<float>()) relativeAngle += 2.0f * glm::pi<float>();

    // Snap to 4 directions
    float snappedAngle = 0.0f;
    if (std::abs(relativeAngle) < glm::pi<float>() * 0.25f) {
        snappedAngle = 0.0f; // FRONT
    } else if (relativeAngle >= glm::pi<float>() * 0.25f && relativeAngle <= glm::pi<float>() * 0.75f) {
        snappedAngle = glm::pi<float>() * 0.5f; // RIGHT
    } else if (relativeAngle <= -glm::pi<float>() * 0.25f && relativeAngle >= -glm::pi<float>() * 0.75f) {
        snappedAngle = -glm::pi<float>() * 0.5f; // LEFT
    } else {
        snappedAngle = glm::pi<float>(); // BACK
    }
    
    DamageIndicator indicator;
    indicator.angle = snappedAngle;
    indicator.lifetime = 2.0f; // 2 seconds
    indicator.maxLifetime = 2.0f;
    
    m_damageIndicators.push_back(indicator);
}

void HUD::update(float deltaTime) {
    for (auto it = m_damageIndicators.begin(); it != m_damageIndicators.end();) {
        it->lifetime -= deltaTime;
        if (it->lifetime <= 0.0f) {
            it = m_damageIndicators.erase(it);
        } else {
            ++it;
        }
    }
}

void HUD::render(int health, int maxHealth, const std::string& weaponName, 
                int currentAmmo, int reserveAmmo, bool reloading, int enemyCount, const std::string& interactionPrompt,
                float bulletTimeEnergy, float maxBulletTimeEnergy, bool bulletTimeActive) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    float screenWidth = io.DisplaySize.x;
    float screenHeight = io.DisplaySize.y;
    ImVec2 screenPos = ImVec2(0, 0);

    // Fonts are loaded in GuiSystem: 0 is regular (18px), 1 is big (48px)
    if (io.Fonts == nullptr || io.Fonts->Fonts.size() == 0) {
        return; // Safety first
    }
    
    ImFont* bigFont = nullptr;
    ImFont* regularFont = nullptr;
    
    if (io.Fonts->Fonts.size() > 1) {
        bigFont = io.Fonts->Fonts[1];
        regularFont = io.Fonts->Fonts[0];
    } else {
        bigFont = io.Fonts->Fonts[0];
        regularFont = io.Fonts->Fonts[0];
    }

    if (bigFont == nullptr || regularFont == nullptr) return;

    // Resolution scaling
    float referenceHeight = Config::UI_REFERENCE_HEIGHT;
    float scale = screenHeight / referenceHeight;
    
    bigFont->Scale = scale;
    regularFont->Scale = scale;

    float margin = 40.0f * scale;
    float barWidth = 250.0f * scale;
    float barHeight = 20.0f * scale;

    // 1. Health Bar (Bottom Left)
    float healthX = screenPos.x + margin;
    float healthY = screenPos.y + screenHeight - margin - barHeight;
    float healthPercent = (float)health / (float)maxHealth;
    
    /*
    // 1.5 Bullet Time Bar (Above Health Bar)
    float btY = healthY - 45.0f * scale; // Higher to accommodate health text
    float btPercent = bulletTimeEnergy / maxBulletTimeEnergy;
    float btBarHeight = 10.0f * scale;
    ImColor btFillColor = bulletTimeActive ? ImColor(40, 180, 255, 230) : ImColor(60, 100, 200, 180);
    
    // Bar Background
    drawList->AddRectFilled(ImVec2(healthX, btY), ImVec2(healthX + barWidth, btY + btBarHeight), 
                         ImColor(20, 20, 20, 150));
    // Bar Fill
    drawList->AddRectFilled(ImVec2(healthX, btY), ImVec2(healthX + barWidth * btPercent, btY + btBarHeight), 
                         btFillColor);
    // Border
    drawList->AddRect(ImVec2(healthX, btY), ImVec2(healthX + barWidth, btY + btBarHeight), 
                    ImColor(255, 255, 255, 80), 0.0f, 0, 1.0f);

    if (bulletTimeActive) {
        ImGui::PushFont(regularFont);
        drawList->AddText(ImVec2(healthX, btY - 25.0f * scale), ImColor(255, 255, 255, 200), "BULLET TIME");
        ImGui::PopFont();
    }
    */

    // Shadow for text
    std::string healthStr = std::to_string(health);
    ImGui::PushFont(bigFont);
    drawList->AddText(ImVec2(healthX + 2 * scale, healthY - 58.0f * scale), ImColor(0, 0, 0, 200), healthStr.c_str());
    drawList->AddText(ImVec2(healthX, healthY - 60.0f * scale), ImColor(255, 255, 255), healthStr.c_str());
    ImGui::PopFont();

    // Bar Background
    drawList->AddRectFilled(ImVec2(healthX, healthY), ImVec2(healthX + barWidth, healthY + barHeight), 
                         ImColor(20, 20, 20, 150));
    // Bar Fill
    drawList->AddRectFilled(ImVec2(healthX, healthY), ImVec2(healthX + barWidth * healthPercent, healthY + barHeight), 
                         ImColor(220, 40, 40, 230));
    // Border
    drawList->AddRect(ImVec2(healthX, healthY), ImVec2(healthX + barWidth, healthY + barHeight), 
                    ImColor(255, 255, 255, 100), 0.0f, 0, 1.5f);

    // 2. Weapon & Ammo (Bottom Right)
    float ammoY = screenPos.y + screenHeight - margin;
    
    ImGui::PushFont(bigFont);
    std::string ammoStr = std::to_string(currentAmmo) + " / " + std::to_string(reserveAmmo);
    
    ImVec2 ammoSize = ImGui::CalcTextSize(ammoStr.c_str());
    float ammoX = screenPos.x + screenWidth - margin - ammoSize.x;
    
    drawList->AddText(ImVec2(ammoX + 2 * scale, ammoY - 58.0f * scale), ImColor(0, 0, 0, 200), ammoStr.c_str());
    drawList->AddText(ImVec2(ammoX, ammoY - 60.0f * scale), ImColor(255, 230, 50), ammoStr.c_str());
    ImGui::PopFont();

    // Weapon Name (above ammo)
    ImGui::PushFont(regularFont);
    ImVec2 weaponSize = ImGui::CalcTextSize(weaponName.c_str());
    float weaponX = screenPos.x + screenWidth - margin - weaponSize.x;
    drawList->AddText(ImVec2(weaponX + 1 * scale, ammoY - 84.0f * scale), ImColor(0, 0, 0, 200), weaponName.c_str());
    drawList->AddText(ImVec2(weaponX, ammoY - 85.0f * scale), ImColor(255, 255, 255), weaponName.c_str());
    ImGui::PopFont();

    // 3. Enemy Count (Top Right)
    std::string enemyStr = "ENEMIES: " + std::to_string(enemyCount);
    ImGui::PushFont(regularFont);
    ImVec2 enemySize = ImGui::CalcTextSize(enemyStr.c_str());
    drawList->AddText(ImVec2(screenPos.x + screenWidth - margin - enemySize.x + 1 * scale, screenPos.y + margin + 1 * scale), ImColor(0, 0, 0, 200), enemyStr.c_str());
    drawList->AddText(ImVec2(screenPos.x + screenWidth - margin - enemySize.x, screenPos.y + margin), ImColor(255, 80, 80), enemyStr.c_str());
    ImGui::PopFont();

    // 4. CS 1.6 Crosshair (Center)
    float centerX = screenPos.x + screenWidth / 2.0f;
    float centerY = screenPos.y + screenHeight / 2.0f;
    float gap = 4.0f * scale;
    float length = 10.0f * scale;
    float thickness = 2.0f * scale;
    ImColor chColor(50, 255, 50, 200); // Classic Green

    auto drawLine = [&](ImVec2 p1, ImVec2 p2, ImColor col, float thick) {
        drawList->AddLine(p1, p2, col, thick);
    };

    // Vertical Top
    drawLine(ImVec2(centerX, centerY - gap), ImVec2(centerX, centerY - gap - length), chColor, thickness);
    // Vertical Bottom
    drawLine(ImVec2(centerX, centerY + gap), ImVec2(centerX, centerY + gap + length), chColor, thickness);
    // Horizontal Left
    drawLine(ImVec2(centerX - gap, centerY), ImVec2(centerX - gap - length, centerY), chColor, thickness);
    // Horizontal Right
    drawLine(ImVec2(centerX + gap, centerY), ImVec2(centerX + gap + length, centerY), chColor, thickness);

    // 5. Interaction Prompt
    if (!interactionPrompt.empty()) {
        ImGui::PushFont(bigFont);
        ImVec2 promptSize = ImGui::CalcTextSize(interactionPrompt.c_str());
        float promptX = centerX - promptSize.x / 2.0f;
        float promptY = centerY + 40.0f * scale; // Below crosshair
        
        drawList->AddText(ImVec2(promptX + 1 * scale, promptY + 1 * scale), ImColor(0, 0, 0, 200), interactionPrompt.c_str());
        drawList->AddText(ImVec2(promptX, promptY), ImColor(255, 255, 255), interactionPrompt.c_str());
        ImGui::PopFont();
    }
    
    // 6. GTA-style Notification Popup
    renderNotificationPopup();

    // 7. Damage Indicators
    renderDamageIndicators(drawList, screenWidth, screenHeight, scale);
}

void HUD::renderNotificationPopup() {
    if (!m_currentNotification.active) return;
    
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    if (io.Fonts == nullptr || io.Fonts->Fonts.size() == 0) return;
    
    ImFont* regularFont = io.Fonts->Fonts[0];
    
    float screenWidth = io.DisplaySize.x;
    float screenHeight = io.DisplaySize.y;
    float referenceHeight = Config::UI_REFERENCE_HEIGHT;
    float scale = screenHeight / referenceHeight;
    
    regularFont->Scale = scale;
    ImGui::PushFont(regularFont);
    
    // Calculate text size
    ImVec2 textSize = ImGui::CalcTextSize(m_currentNotification.text.c_str());
    
    // Box dimensions (GTA-style: compact black box)
    float padding = 15.0f * scale;
    float boxWidth = textSize.x + padding * 2.0f;
    float boxHeight = textSize.y + padding * 2.0f;
    
    // Position at top left
    float boxX = 20.0f * scale;
    float boxY = 20.0f * scale;
    
    // Fade in/out effect
    float alpha = 1.0f;
    float fadeInTime = 0.3f;
    float fadeOutTime = 0.5f;
    
    if (m_currentNotification.currentTime < fadeInTime) {
        // Fade in
        alpha = m_currentNotification.currentTime / fadeInTime;
    } else if (m_currentNotification.currentTime > m_currentNotification.displayTime - fadeOutTime) {
        // Fade out
        float timeLeft = m_currentNotification.displayTime - m_currentNotification.currentTime;
        alpha = timeLeft / fadeOutTime;
    }
    
    alpha = glm::clamp(alpha, 0.0f, 1.0f);
    int alphaInt = (int)(alpha * 255);
    
    // Draw black semi-transparent background box
    drawList->AddRectFilled(
        ImVec2(boxX, boxY), 
        ImVec2(boxX + boxWidth, boxY + boxHeight),
        ImColor(0, 0, 0, (int)(alphaInt * 0.85f)), // 85% opacity when fully visible
        4.0f * scale // Rounded corners
    );
    
    // Draw border
    drawList->AddRect(
        ImVec2(boxX, boxY), 
        ImVec2(boxX + boxWidth, boxY + boxHeight),
        ImColor(255, 255, 255, (int)(alphaInt * 0.3f)), // Subtle white border
        4.0f * scale,
        0,
        1.5f * scale
    );
    
    // Draw white text
    drawList->AddText(
        ImVec2(boxX + padding, boxY + padding),
        ImColor(255, 255, 255, alphaInt),
        m_currentNotification.text.c_str()
    );
    
    ImGui::PopFont();
}

void HUD::renderDeathScreen() {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Safety check for fonts
    if (io.Fonts == nullptr || io.Fonts->Fonts.size() == 0) return;
    
    ImFont* bigFont = io.Fonts->Fonts.size() > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
    ImFont* regularFont = io.Fonts->Fonts[0];

    float screenWidth = io.DisplaySize.x;
    float screenHeight = io.DisplaySize.y;

    // Resolution scaling
    float referenceHeight = Config::UI_REFERENCE_HEIGHT;
    float scale = screenHeight / referenceHeight;

    // Full screen red overlay with fade
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(screenWidth, screenHeight), ImColor(50, 0, 0, 200));

    std::string text = "GAME OVER";
    bigFont->Scale = scale * 2.5f; // Make it massive
    ImGui::PushFont(bigFont);
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    float textX = (screenWidth - textSize.x) * 0.5f;
    float textY = (screenHeight - textSize.y) * 0.4f;
    
    // Shadow
    drawList->AddText(ImVec2(textX + 4 * scale, textY + 4 * scale), ImColor(0, 0, 0, 255), text.c_str());
    // Red Text
    drawList->AddText(ImVec2(textX, textY), ImColor(180, 0, 0, 255), text.c_str()); // Slightly brighter red
    ImGui::PopFont();

    // "Press 'R' to Restart" text
    std::string subtext = "Press 'R' to Restart";
    regularFont->Scale = scale * 1.5f; // Make it readable
    ImGui::PushFont(regularFont);
    ImVec2 subtextSize = ImGui::CalcTextSize(subtext.c_str());
    float subtextX = (screenWidth - subtextSize.x) * 0.5f;
    float subtextY = (screenHeight - subtextSize.y) * 0.6f;
    
    drawList->AddText(ImVec2(subtextX + 2 * scale, subtextY + 2 * scale), ImColor(0, 0, 0, 255), subtext.c_str());
    drawList->AddText(ImVec2(subtextX, subtextY), ImColor(255, 255, 255, 255), subtext.c_str());
    ImGui::PopFont();
}

void HUD::renderDamageIndicators(ImDrawList* drawList, float screenWidth, float screenHeight, float scale) {
    if (m_damageIndicators.empty()) return;

    ImVec2 center(screenWidth / 2.0f, screenHeight / 2.0f);
    float radius = 120.0f * scale; // Slightly further from center
    float arrowLength = 30.0f * scale;
    float arrowWidth = 120.0f * scale; // Much broader for 4-dir style

    for (const auto& indicator : m_damageIndicators) {
        float alpha = indicator.lifetime / indicator.maxLifetime;
        ImColor color(220, 20, 20, (int)(alpha * 180)); // Brighter red

        // The angle is relative to front. 
        // In screen space, 0 should be "up" (actually the damage source is in front).
        // If source is in front, angle = 0. We want a wedge pointing UP.
        // Screen space Y is positive DOWN.
        // So angle 0 -> vector (0, -1) in screen space.
        
        float cosA = cos(indicator.angle);
        float sinA = sin(indicator.angle);
        
        // Direction vector in screen space (relative to UP)
        // ImGui uses X right, Y down.
        // We want angle 0 to be UP (0, -1).
        // angle pi/2 to be RIGHT (1, 0).
        // angle -pi/2 to be LEFT (-1, 0).
        // angle pi to be DOWN (0, 1).
        
        float dirX = sinA;
        float dirY = -cosA;
        
        ImVec2 pos = ImVec2(center.x + dirX * radius, center.y + dirY * radius);
        
        // Draw a triangle/wedge pointing towards the source
        // Tip of the triangle
        ImVec2 tip = pos;
        
        // Base of the triangle
        ImVec2 baseMid = ImVec2(pos.x - dirX * arrowLength, pos.y - dirY * arrowLength);
        
        // Side vectors (perpendicular to dir)
        float sideX = -dirY;
        float sideY = dirX;
        
        ImVec2 p1 = ImVec2(baseMid.x + sideX * arrowWidth * 0.5f, baseMid.y + sideY * arrowWidth * 0.5f);
        ImVec2 p2 = ImVec2(baseMid.x - sideX * arrowWidth * 0.5f, baseMid.y - sideY * arrowWidth * 0.5f);
        
        drawList->AddTriangleFilled(tip, p1, p2, color);
    }
}
