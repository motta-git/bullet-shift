#pragma once

#include <memory>
#include <vector>
#include <string>

struct GLFWwindow;

#include "Camera.h"
#include "Player.h"
#include "Platform.h"
#include "ParticleSystem.h"
#include "HUD.h"
#include "DebugRenderer.h"
#include "WeaponRenderer.h"
#include "InputState.h"
#include "Enemy.h"
#include "WeaponPickup.h"
#include "Shader.h"
#include "AudioSystem.h"
#include "GuiSystem.h"
#include "Projectile.h"
#include "Mesh.h"
#include "NavigationGraph.h"
#include "PostProcessingSystem.h"
#include "Skybox.h"
#include "ShadowSystem.h"

class MenuSystem;
class LevelManager;
class ResourceManager;
class PhysicsSystem;

enum class GameState {
    MAIN_MENU,
    PLAYING,
    PAUSED,
    QUIT_CONFIRMATION,
    GAME_OVER,
    LEVEL_WIN,
    GAME_WIN
};

class Game {
    friend class LevelManager;
    friend class PhysicsSystem;
public:
    Game();
    ~Game();

    bool initialize();
    void run();
    
    // Notification system
    void showNotification(const std::string& text, float duration = 5.0f);
    
    // Bullet Time trigger
    void triggerBulletTime();

private:
    void processInput();
    void update(float deltaTime);
    void render();
    void initializeOpenGLState();
    void loadResources();
    void resetLevel();
    void loadLevel(int level);
    void loadSkybox(int level);
    void applySettings();
    void syncMusicWithState(bool forceRestart = false);

    // Helper rendering methods to keep render() clean
    void renderScene(const glm::mat4& projection, const glm::mat4& view);
    void renderDepthScene(Shader& depthShader);
    void renderLights(const glm::mat4& projection, const glm::mat4& view);
    void renderHUD();
    void renderGUI();
    void renderProjectiles(const glm::mat4& projection, const glm::mat4& view);

    void handleCollisions();

    // Callback functions
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfwErrorCallback(int errorCode, const char* description);

    static Game* instance;

    GLFWwindow* window;


    Camera camera;
    Player player;

    std::unique_ptr<ParticleSystem> particleSystem;
    std::unique_ptr<AudioSystem> audioSystem;
    std::unique_ptr<GuiSystem> guiSystem;
    std::unique_ptr<PostProcessingSystem> postProcessing;
    std::unique_ptr<MenuSystem> menuSystem;
    std::unique_ptr<LevelManager> levelManager;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<PhysicsSystem> physicsSystem;
    std::unique_ptr<HUD> hud;
    std::unique_ptr<DebugRenderer> debugRenderer;
    std::unique_ptr<NavigationGraph> navigationGraph;
    std::unique_ptr<Skybox> skybox;
    std::unique_ptr<ShadowSystem> shadowSystem;
    WeaponRenderer weaponRenderer;

    std::vector<Platform> platforms;
    std::vector<Enemy> enemies;
    std::vector<WeaponPickup> weaponPickups;
    std::vector<Projectile> projectiles;

    InputState input;

    int pickupKey;
    std::string interactionPrompt;

    // Wall-clock timestamp used to compute raw frame delta: currentGLFWTime - lastGlfwTime.
    // This is intentionally separate from `m_accumulatedTime` which tracks game world time (scaled by m_timeScale).
    float lastGlfwTime;
    float explosionTimer;
    float fireTimer;
    float deathTimer;
    
    float techStyleIntensity; // 0.0 to 1.0 for tech-style graphics effect

    // Bullet Time
    float m_timeScale;
    bool m_bulletTimeActive;
    float m_bulletTimeEnergy;
    float m_accumulatedTime;

    GameState state;
    int currentLevel;
    std::string activeMusicTrackId;
};
