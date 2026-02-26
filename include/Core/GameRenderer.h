#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

class Game;
class Camera;
class Player;
class ParticleSystem;
class HUD;
class DebugRenderer;
class Enemy;
class WeaponPickup;
class HealthPickup;
class Projectile;
class ResourceManager;
class PostProcessingSystem;
class Skybox;
class ShadowSystem;
class WeaponRenderer;
class Platform;
class NavigationGraph;
class GuiSystem;
class MenuSystem;
class DevConsole;
enum class GameState;

class GameRenderer {
public:
    GameRenderer(Game& game);
    ~GameRenderer();

    // Main render function called every frame
    void render(
        float accumulatedTime, 
        GameState state, 
        int currentLevel,
        DevConsole& console,
        Camera& camera,
        Player& player,
        WeaponRenderer& weaponRenderer,
        std::unique_ptr<ParticleSystem>& particleSystem,
        std::unique_ptr<PostProcessingSystem>& postProcessing,
        std::unique_ptr<ResourceManager>& resourceManager,
        std::unique_ptr<HUD>& hud,
        std::unique_ptr<DebugRenderer>& debugRenderer,
        std::unique_ptr<Skybox>& skybox,
        std::unique_ptr<ShadowSystem>& shadowSystem,
        std::unique_ptr<GuiSystem>& guiSystem,
        std::unique_ptr<MenuSystem>& menuSystem,
        std::unique_ptr<NavigationGraph>& navigationGraph,
        const std::vector<Platform>& platforms,
        const std::vector<Enemy>& enemies,
        const std::vector<WeaponPickup>& weaponPickups,
        const std::vector<HealthPickup>& healthPickups,
        const std::vector<Projectile>& projectiles,
        float techStyleIntensity,
        float timeScale,
        float playerMuzzleFlashTimer,
        const glm::vec3& playerMuzzleFlashPos,
        const glm::vec3& playerMuzzleFlashColor,
        const std::string& interactionPrompt,
        float bulletTimeEnergy,
        bool bulletTimeActive
    );

    void renderHUD(
        GameState state,
        Player& player,
        std::unique_ptr<HUD>& hud,
        const std::vector<Enemy>& enemies,
        const std::string& interactionPrompt,
        float bulletTimeEnergy,
        bool bulletTimeActive
    );
    
    void renderGUI(
        GameState state,
        int currentLevel,
        DevConsole& console,
        std::unique_ptr<GuiSystem>& guiSystem,
        std::unique_ptr<MenuSystem>& menuSystem,
        Game& game
    );

private:
    void renderScene(
        const glm::mat4& projection, 
        const glm::mat4& view,
        float accumulatedTime,
        Camera& camera,
        Player& player,
        std::unique_ptr<ResourceManager>& resourceManager,
        std::unique_ptr<ShadowSystem>& shadowSystem,
        const std::vector<Platform>& platforms,
        const std::vector<Enemy>& enemies,
        const std::vector<WeaponPickup>& weaponPickups,
        const std::vector<HealthPickup>& healthPickups,
        float techStyleIntensity,
        float playerMuzzleFlashTimer,
        const glm::vec3& playerMuzzleFlashPos,
        const glm::vec3& playerMuzzleFlashColor
    );
    
    void renderDepthScene(
        class Shader& depthShader,
        std::unique_ptr<ResourceManager>& resourceManager,
        const std::vector<Platform>& platforms,
        const std::vector<Enemy>& enemies,
        const std::vector<WeaponPickup>& weaponPickups,
        const std::vector<HealthPickup>& healthPickups
    );
    
    void renderLights(
        const glm::mat4& projection, 
        const glm::mat4& view,
        std::unique_ptr<ResourceManager>& resourceManager
    );
    
    void renderProjectiles(
        const glm::mat4& projection, 
        const glm::mat4& view,
        std::unique_ptr<ResourceManager>& resourceManager,
        const std::vector<Projectile>& projectiles
    );

    Game& m_game;
};
