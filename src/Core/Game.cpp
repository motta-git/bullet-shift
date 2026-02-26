#include "Game.h"
#include "MenuSystem.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <cstdlib>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "GeometryFactory.h"
#include "Projectile.h"
#include "Mesh.h"
#include "ModelLoader.h"
#include "Config.h"
#include "GLDebug.h"
#include "LevelManager.h"
#include "ResourceManager.h"
#include "PhysicsSystem.h"
#include "Settings.h"
#include "GameRenderer.h"
#include "DevConsole.h"
#include <imgui.h>

namespace {
const char* title = "Bullet Shift";
}

Game* Game::instance = nullptr;

Game::Game()
    : window(nullptr),
      camera(glm::vec3(0.0f, 1.7f, 5.0f)),
      player(glm::vec3(0.0f, 1.0f, 0.0f)),
      particleSystem(nullptr),
      audioSystem(nullptr),
      guiSystem(nullptr),
      hud(nullptr),
    debugRenderer(nullptr),
      weaponRenderer(),
      pickupKey(GLFW_KEY_E),
      lastGlfwTime(0.0f),
      explosionTimer(0.0f),
      fireTimer(0.0f),
      deathTimer(0.0f),
      techStyleIntensity(0.6f), // Will be overwritten by settings
      m_timeScale(1.0f),
      m_bulletTimeActive(false),
      m_bulletTimeEnergy(100.0f),
      m_accumulatedTime(0.0f),
      m_playerMuzzleFlashTimer(0.0f),
      m_playerMuzzleFlashColor(1.0f, 0.8f, 0.3f),
      state(GameState::MAIN_MENU),
      currentLevel(0) {
    auto& settings = Settings::getInstance();
    // Load settings first (try to load file, if not, it uses defaults)
    settings.load();
    
    techStyleIntensity = settings.graphics.techStyleIntensity;
    
    input.lastMouseX = settings.window.width / 2.0f;
    input.lastMouseY = settings.window.height / 2.0f;

    if (!instance) {
        instance = this;
    }

    m_gameRenderer = std::make_unique<GameRenderer>(*this);
    m_console = std::make_unique<DevConsole>();
}

Game::~Game() {
    // Destroy Renderer first, it might depend on other systems
    m_gameRenderer.reset();
    m_console.reset();
    
    // Explicitly reset all unique_ptrs before glfwTerminate
    debugRenderer.reset();
    hud.reset();
    levelManager.reset();
    menuSystem.reset();
    guiSystem.reset();
    resourceManager.reset();
    
    if (audioSystem) {
        audioSystem->shutdown();
    }
    audioSystem.reset();
    particleSystem.reset();
    physicsSystem.reset();
    shadowSystem.reset();
    skybox.reset();
    navigationGraph.reset();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
    instance = nullptr;
}

bool Game::initialize() {
    if (instance && instance != this) {
        std::cerr << "Another Game instance already exists" << std::endl;
        return false;
    }

    glfwSetErrorCallback(Game::glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    auto& settings = Settings::getInstance();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS, good for others
    glfwWindowHint(GLFW_SAMPLES, settings.window.msaaSamples);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, Config::DEPTH_BITS);
    glfwWindowHint(GLFW_STENCIL_BITS, Config::STENCIL_BITS);

    // Smart Resolution Detection: Cap resolution to monitor's native mode
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    if (primary) {
        const GLFWvidmode* mode = glfwGetVideoMode(primary);
        if (mode) {
            if (settings.window.width > mode->width || settings.window.height > mode->height) {
                std::cerr << "Warning: Requested resolution " << settings.window.width << "x" << settings.window.height 
                          << " exceeds monitor native " << mode->width << "x" << mode->height 
                          << ". Capping to native." << std::endl;
                settings.window.width = mode->width;
                settings.window.height = mode->height;
            }
        }
    }

    window = glfwCreateWindow(
        settings.window.width, 
        settings.window.height, 
        title, 
        settings.window.fullscreen ? glfwGetPrimaryMonitor() : nullptr, 
        nullptr
    );
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    // Check for errors (moved up)
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Print renderer info for debugging
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    // Write renderer info to file
    std::ofstream debugFile("renderer.txt");
    if (debugFile.is_open()) {
        debugFile << "OpenGL Renderer: " << (renderer ? (const char*)renderer : "Unknown") << std::endl;
        debugFile << "OpenGL Version: " << (version ? (const char*)version : "Unknown") << std::endl;
        debugFile.close();
    }

    // Apply settings (VSync, Sensitivity, etc.)
    applySettings();

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, Game::framebufferSizeCallback);
    glfwSetCursorPosCallback(window, Game::mouseCallback);
    glfwSetScrollCallback(window, Game::scrollCallback);
    
    // Initial cursor state depends on game state
    if (state == GameState::MAIN_MENU) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    // Seed random number generator
    srand(static_cast<unsigned int>(time(NULL)));

    instance = this;

    // Initialize Audio System
    audioSystem = std::make_unique<AudioSystem>();
    if (!audioSystem->init()) {
        std::cerr << "Failed to initialize Audio System" << std::endl;
        // deciding not to crash entire game for audio fail, but logging it
    }

    // Initialize Gui System (AFTER Audio and GL)
    guiSystem = std::make_unique<GuiSystem>(window);

    // Initialize Menu System
    MenuSystem::Callbacks callbacks;
    callbacks.onLoadLevel = [this](int level) { this->loadLevel(level); };
    callbacks.onResume = [this]() { 
        this->state = GameState::PLAYING; 
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    };
    callbacks.onExitToMenu = [this]() {
        this->state = GameState::MAIN_MENU;
        this->currentLevel = 0;
        this->syncMusicWithState(true);
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    };
    callbacks.onQuitApp = [this]() {
        if (this->currentLevel > 0 || this->state == GameState::PAUSED) {
            this->state = GameState::QUIT_CONFIRMATION;
        } else {
            std::cout << "User exited game" << std::endl;
            glfwSetWindowShouldClose(this->window, true);
        }
    };
    callbacks.onRenderHUD = [this]() {
        // Debug text to confirm callback is running
        // ImGui::SetNextWindowPos(ImVec2(10, 50));
        // ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
        // ImGui::Text("HUD CALLBACK ACTIVE");
        // ImGui::End();

        // Create a full-screen transparent window for the HUD
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("HUDOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        if (this->m_gameRenderer) {
            this->m_gameRenderer->renderHUD(this->state, this->player, this->hud, this->enemies, this->interactionPrompt, this->m_bulletTimeEnergy, this->m_bulletTimeActive);
        }

        ImGui::End();

        // Performance window (visible if enabled)
        if (Settings::getInstance().graphics.showFPS) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::End();
        }
    };
    callbacks.onSettingsChanged = [this]() {
        this->applySettings();
    };
    menuSystem = std::make_unique<MenuSystem>(*guiSystem, *audioSystem, callbacks);
    levelManager = std::make_unique<LevelManager>(*this);
    postProcessing = std::make_unique<PostProcessingSystem>(settings.window.width, settings.window.height);
    resourceManager = std::make_unique<ResourceManager>();
    physicsSystem = std::make_unique<PhysicsSystem>(*this);
    shadowSystem = std::make_unique<ShadowSystem>(2048);

    initializeOpenGLState();
    loadResources();
    // Don't initialize world here, wait for "Start Game" in menu
    
    hud = std::make_unique<HUD>(settings.window.width, settings.window.height);

    // Initialize wall-clock frame timer. We use raw GLFW time here because
    // game world time (`m_accumulatedTime`) is scaled by bullet-time and should not be
    // used to compute frame-to-frame delta for system timing.
    lastGlfwTime = glfwGetTime();

    return true;
}

void Game::run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastGlfwTime;
        lastGlfwTime = currentFrame;

        processInput();
        update(deltaTime);
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Game::initializeOpenGLState() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    GL_CHECK_ERROR();
}

void Game::loadResources() {
    resourceManager->loadShader("lighting", "shaders/lighting.vert", "shaders/lighting.frag");
    resourceManager->loadShader("lightSource", "shaders/light_source.vert", "shaders/light_source.frag");
    resourceManager->loadShader("particle", "shaders/particle.vert", "shaders/particle.frag");
    
    // Post-processing shaders
    resourceManager->loadShader("post_processing", "shaders/post_processing.vert", "shaders/post_processing.frag");
    resourceManager->loadShader("bloom_blur", "shaders/post_processing.vert", "shaders/bloom_blur.frag");
    resourceManager->loadShader("bright_filter", "shaders/post_processing.vert", "shaders/bright_filter.frag");
    resourceManager->loadShader("skybox", "shaders/skybox.vert", "shaders/skybox.frag");
    resourceManager->loadShader("equirect_to_cubemap", "shaders/equirect_to_cubemap.vert", "shaders/equirect_to_cubemap.frag");
    resourceManager->loadShader("shadowDepth", "shaders/shadow_depth.vert", "shaders/shadow_depth.frag");

    resourceManager->addMesh("cube", GeometryFactory::createCube());
    resourceManager->addMesh("sphere", GeometryFactory::createSphere(48, 24));
    resourceManager->addMesh("torus", GeometryFactory::createTorus(1.5f, 0.5f, 48, 24));
    resourceManager->addMesh("plane", GeometryFactory::createPlane(50.0f));

    // Load common sounds and music
    if (audioSystem) {
        const std::string pickupSoundId = Config::Audio::PICKUP_SOUND;
        std::string pickupSource = pickupSoundId;
        if (!std::filesystem::exists(pickupSource)) {
            const std::string fallbackPickup = "assets/sounds/sfx/pistol-fire.ogg";
            if (std::filesystem::exists(fallbackPickup)) {
                std::cerr << "Warning: missing pickup sound '" << pickupSoundId
                          << "', using fallback '" << fallbackPickup << "'" << std::endl;
                pickupSource = fallbackPickup;
            } else {
                std::cerr << "Warning: missing pickup sound '" << pickupSoundId
                          << "' and fallback asset '" << fallbackPickup << "'" << std::endl;
                pickupSource.clear();
            }
        }
        if (!pickupSource.empty()) {
            audioSystem->loadSound(pickupSoundId, pickupSource);
        }

        const auto& menuTrack = Config::Audio::MAIN_MENU_TRACK;
        audioSystem->loadMusic(menuTrack.id, menuTrack.filePath, menuTrack.loop, menuTrack.gain);

        for (const auto& levelTrack : Config::Audio::LEVEL_MUSIC) {
            audioSystem->loadMusic(levelTrack.id, levelTrack.filePath, levelTrack.loop, levelTrack.gain);
        }

        // Load footstep sounds
        std::cout << "[Audio] Loading footstep sounds..." << std::endl;
        audioSystem->loadSound("footstep_1", Config::Audio::FOOTSTEP_SOUND_1);
        if (std::filesystem::exists(Config::Audio::FOOTSTEP_SOUND_2)) {
            audioSystem->loadSound("footstep_2", Config::Audio::FOOTSTEP_SOUND_2);
        } else {
            std::cout << "[Audio] footstep_2.ogg not found, using footstep_1.ogg as fallback" << std::endl;
        }

        // Load UI sounds
        std::cout << "[Audio] Loading UI sounds..." << std::endl;
        audioSystem->loadSound("ui_click", Config::Audio::UI_CLICK_SOUND);
        audioSystem->loadSound("ui_cancel", Config::Audio::UI_CANCEL_SOUND);

        // Enemy alert sound (short SFX used when enemy loses sight of player)
        // Fallback to pistol-fire if a dedicated alert sound isn't available
        /*const std::string enemyAlert = "assets/sounds/sfx/enemy-alert.ogg";
        if (std::filesystem::exists(enemyAlert)) {
            audioSystem->loadSound("enemy_alert", enemyAlert);
        } else {
            audioSystem->loadSound("enemy_alert", "assets/sounds/sfx/pistol-fire.ogg");
        }*/
    }
    
    // Load all weapon models defined in Config
    std::cout << "Loading weapon models..." << std::endl;
    for (int i = 0; i < (int)WeaponType::COUNT; ++i) {
        WeaponType type = static_cast<WeaponType>(i);
        auto weaponData = Config::Weapon::getWeaponConfig(type);
        
        if (strlen(weaponData.modelPath) == 0) continue;

        std::cout << "  - Loading " << weaponData.name << " from " << weaponData.modelPath << "..." << std::endl;
        auto meshes = ModelLoader::loadModel(weaponData.modelPath);
        if (meshes.empty()) {
            std::cerr << "Warning: Failed to load " << weaponData.name << " model, falling back to procedural" << std::endl;
            meshes.push_back(GeometryFactory::createWeaponMesh());
        }
        resourceManager->addWeaponMeshes(weaponData.name, std::move(meshes));

        // Load weapon fire sound
        if (strlen(weaponData.fireSoundPath) > 0) {
            std::cout << "  - Loading sound " << weaponData.fireSoundPath << "..." << std::endl;
            audioSystem->loadSound(weaponData.fireSoundPath, weaponData.fireSoundPath);
        }
        
        // Load weapon reload sound
        if (strlen(weaponData.reloadSoundPath) > 0) {
            std::cout << "  - Loading reload sound " << weaponData.reloadSoundPath << "..." << std::endl;
            audioSystem->loadSound(weaponData.reloadSoundPath, weaponData.reloadSoundPath);
        }
        
        // If the weapon has a pump sound or extra Fx we might load them here in future (placeholder)
    }

    // Load health pickup model if present (user-provided model in assets/models/pickups)
    const std::string healthModelPath = Config::Pickup::HEALTH_MODEL_PATH;
    if (!healthModelPath.empty() && std::filesystem::exists(healthModelPath)) {
        std::cout << "Loading pickup model: " << healthModelPath << std::endl;
        auto hpMeshes = ModelLoader::loadModel(healthModelPath);
        if (!hpMeshes.empty()) {
            // Use first mesh as the visual for health pickups
            resourceManager->addMesh("health_pickup", std::move(hpMeshes[0]));
        } else {
            std::cerr << "Warning: failed to load health pickup model: " << healthModelPath << std::endl;
        }
    }

    particleSystem = std::make_unique<ParticleSystem>(Config::MAX_PARTICLES);
    // Enable atmospheric particles (ambient dust/motes) around the camera
    particleSystem->enableAtmospheric(true);
    particleSystem->setAtmosphereRate(Config::Particle::ATMOSPHERE_RATE);   // particles per second
    particleSystem->setAtmosphereRadius(Config::Particle::ATMOSPHERE_RADIUS); // spawn radius around camera

    debugRenderer = std::make_unique<DebugRenderer>();

    // Reserve likely projectile capacity to avoid runtime reallocations during combat
    projectiles.reserve(Config::Performance::PROJECTILE_RESERVE);

    syncMusicWithState(true);

    // Initialize Skybox for first level
    loadSkybox(Settings::getInstance().progress.lastLevelPlayed > 0 ? Settings::getInstance().progress.lastLevelPlayed : 1);

    GL_CHECK_ERROR();
}


void Game::resetLevel() {
    if (currentLevel <= 0) {
        currentLevel = 1; // Default to Level 1 if invalid
    }
    loadLevel(currentLevel);
}

void Game::loadLevel(int level) {
    // Save progress
    Settings::getInstance().progress.lastLevelPlayed = level;
    Settings::getInstance().save();
    
    currentLevel = level;
    if (levelManager) {
        levelManager->loadLevel(level);
    }
    
    // Switch Skybox for the new level
    loadSkybox(level);
    
    // Reset bullet time on level load
    m_bulletTimeActive = false;
    m_bulletTimeEnergy = Config::MAX_BULLET_TIME_ENERGY;
    m_timeScale = 1.0f;
    
    // Build navigation graph after level is loaded
    if (!navigationGraph) {
        navigationGraph = std::make_unique<NavigationGraph>();
    }
    navigationGraph->buildFromPlatforms(platforms);
    
    std::cout << "[NavigationGraph] Built with " << navigationGraph->getNodes().size() 
              << " nodes and " << navigationGraph->getEdges().size() << " edges" << std::endl;
    
    state = GameState::PLAYING;
    syncMusicWithState(true);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Reset wall-clock frame timer to avoid a large deltaTime on the first update after heavy level load.
    // Note: We do NOT reset `m_accumulatedTime` here because it represents game-world time (scaled) and
    // is intentionally decoupled from raw GLFW time used to compute frame-to-frame deltas.
    lastGlfwTime = glfwGetTime();
}

void Game::loadSkybox(int levelIndex) {
    const auto& levelConfig = Config::Levels::getLevelConfig(levelIndex);
    std::string hdrPath = levelConfig.skyboxPath;
    Shader* convShader = resourceManager->getShader("equirect_to_cubemap");

    std::cout << "Skybox: Loading configuration for Level " << levelIndex << " (" << levelConfig.name << ")" << std::endl;

    // 1. Try HDR from Config first
    if (std::filesystem::exists(hdrPath) && convShader) {
        std::cout << "Skybox: Loading HDR from " << hdrPath << "..." << std::endl;
        skybox = std::make_unique<Skybox>(hdrPath, *convShader);
        
        // Restore viewport after baking
        glViewport(0, 0, Settings::getInstance().window.width, Settings::getInstance().window.height);
    } 
    // 2. Fallback to standard cubemap faces if HDR missing
    else {
        std::vector<std::string> skyboxFaces = {
            "assets/textures/skyboxes/right.jpg",
            "assets/textures/skyboxes/left.jpg",
            "assets/textures/skyboxes/top.jpg",
            "assets/textures/skyboxes/bottom.jpg",
            "assets/textures/skyboxes/front.jpg",
            "assets/textures/skyboxes/back.jpg"
        };
        
        if (std::filesystem::exists(skyboxFaces[0])) {
            std::cout << "Skybox: Falling back to 6-face cubemap..." << std::endl;
            skybox = std::make_unique<Skybox>(skyboxFaces);
        } else {
            std::cout << "Skybox: Texture assets not found for Level " << levelIndex << ", skipping skybox update." << std::endl;
        }
    }
}

void Game::syncMusicWithState(bool forceRestart) {
    if (!audioSystem) {
        return;
    }

    const bool usingLevelContext = currentLevel > 0 && (
        state == GameState::PLAYING ||
        state == GameState::PAUSED ||
        state == GameState::GAME_OVER ||
        state == GameState::LEVEL_WIN
    );

    const auto& track = usingLevelContext
        ? Config::Audio::getLevelMusic(currentLevel)
        : Config::Audio::MAIN_MENU_TRACK;

    const std::string targetTrackId = track.id ? track.id : "";
    if (targetTrackId.empty()) {
        std::cerr << "Audio: Missing track id for current state" << std::endl;
        return;
    }

    const bool trackChanged = targetTrackId != activeMusicTrackId;
    const bool targetPlaying = audioSystem->isMusicPlaying(targetTrackId);

    if (!forceRestart && !trackChanged && targetPlaying) {
        return;
    }

    const bool shouldRestart = forceRestart || trackChanged || !targetPlaying;
    if (shouldRestart) {
        std::cout << "[Audio] Sync request -> track='" << targetTrackId
                  << "' state=" << static_cast<int>(state)
                  << " level=" << currentLevel
                  << " force=" << (forceRestart ? "yes" : "no")
                  << " changed=" << (trackChanged ? "yes" : "no")
                  << " playing=" << (targetPlaying ? "yes" : "no")
                  << std::endl;
    }

    if (!audioSystem->playMusic(track.id, shouldRestart)) {
        std::cerr << "Audio: Failed to start track '" << track.id
                  << "' (state=" << static_cast<int>(state)
                  << ", level=" << currentLevel << ")" << std::endl;
        return;
    }

    activeMusicTrackId = targetTrackId;
    if (shouldRestart) {
        std::cout << "[Audio] Active track set -> '" << activeMusicTrackId << "'" << std::endl;
    }
}

void Game::processInput() {
    // Always update ESC state
    const bool escHeldNow = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    input.escTriggered = escHeldNow && !input.escHeld;
    input.escHeld = escHeldNow;

    if (state == GameState::PLAYING) {
        const auto& keys = Settings::getInstance().keybinds;
        
        input.moveForward = glfwGetKey(window, keys.moveForward) == GLFW_PRESS;
        input.moveBackward = glfwGetKey(window, keys.moveBackward) == GLFW_PRESS;
        input.moveLeft = glfwGetKey(window, keys.moveLeft) == GLFW_PRESS;
        input.moveRight = glfwGetKey(window, keys.moveRight) == GLFW_PRESS;

        const bool jumpHeldNow = glfwGetKey(window, keys.jump) == GLFW_PRESS;
        input.jumpTriggered = jumpHeldNow && !input.jumpHeld;
        input.jumpHeld = jumpHeldNow;

        input.fireHeld = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        const bool reloadHeldNow = glfwGetKey(window, keys.reload) == GLFW_PRESS;
        input.reloadTriggered = reloadHeldNow && !input.reloadHeld;
        input.reloadHeld = reloadHeldNow;

        const bool switchHeldNow = glfwGetKey(window, keys.switchWeapon) == GLFW_PRESS;
        input.switchTriggered = switchHeldNow && !input.switchHeld;
        input.switchHeld = switchHeldNow;

        const bool pickupHeldNow = glfwGetKey(window, keys.interact) == GLFW_PRESS;
        input.pickupTriggered = pickupHeldNow && !input.pickupHeld;
        input.pickupHeld = pickupHeldNow;

        const bool bulletTimeHeldNow = glfwGetKey(window, keys.bulletTime) == GLFW_PRESS;
        input.bulletTimeHeld = bulletTimeHeldNow;

        const bool dashHeldNow = glfwGetKey(window, keys.dash) == GLFW_PRESS;
        input.dashTriggered = dashHeldNow && !input.dashHeld;
        input.dashHeld = dashHeldNow;
    } else if (state == GameState::GAME_OVER) {
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            resetLevel();
        }
    } else {
        // Reset all inputs when not playing
        input.moveForward = input.moveBackward = input.moveLeft = input.moveRight = false;
        input.jumpTriggered = input.fireHeld = input.reloadTriggered = input.switchTriggered = input.pickupTriggered = input.bulletTimeTriggered = false;
    }

    // Console Toggle
    const bool debugHeldNow = (glfwGetKey(window, Settings::getInstance().keybinds.consoleToggle) == GLFW_PRESS) || 
                              (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);
    input.debugTriggered = debugHeldNow && !input.debugHeld;
    input.debugHeld = debugHeldNow;

    if (input.debugTriggered && m_console) {
        bool wasOpen = m_console->isOpen();
        m_console->toggle();
        if (m_console->isOpen() && state == GameState::PLAYING) {
            state = GameState::PAUSED;
        }
        // Sync cursor
        if (m_console->isOpen()) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else if (wasOpen && state == GameState::PLAYING) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            input.firstMouse = true;
        }
    }

    if (input.escTriggered) {
        if (menuSystem && menuSystem->isSettingsOpen()) {
            menuSystem->closeSettings();
        } else if (state == GameState::PLAYING) {
            state = GameState::PAUSED;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else if (state == GameState::PAUSED) {
            state = GameState::PLAYING;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            input.firstMouse = true;
        } else if (state == GameState::MAIN_MENU) {
            // In main menu, escape can prompt quit confirmation or just exit
            state = GameState::QUIT_CONFIRMATION;
        } else if (state == GameState::QUIT_CONFIRMATION) {
             // Go back to where we came from? 
             // Usually QUIT_CONFIRMATION comes from Main Menu or Pause (via Quit button)
             // If we logic it simplistically:
             if (currentLevel == 0) state = GameState::MAIN_MENU;
             else state = GameState::PAUSED;
        }
    }
}

void Game::update(float deltaTime) {
    syncMusicWithState();

    if (m_playerMuzzleFlashTimer > 0.0f) {
        m_playerMuzzleFlashTimer -= deltaTime;
    }

    if (state == GameState::PLAYING) {
        if (!player.isAlive()) {
            state = GameState::GAME_OVER;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            m_bulletTimeActive = false;
            m_timeScale = 1.0f;
            return;
        }

        // Bullet Time Toggle removed (now triggered on enemy death)

        // Bullet Time Energy and Time Scale Update
        float targetScale = m_bulletTimeActive ? Config::MIN_BULLET_TIME_SCALE : 1.0f;
        m_timeScale = glm::mix(m_timeScale, targetScale, deltaTime * 10.0f);

        if (m_bulletTimeActive) {
            m_bulletTimeEnergy -= Config::BULLET_TIME_DRAIN_RATE * deltaTime;
            if (m_bulletTimeEnergy <= 0.0f) {
                m_bulletTimeEnergy = 0.0f;
                m_bulletTimeActive = false;
            }
        } else {
            m_bulletTimeEnergy += Config::BULLET_TIME_REGEN_RATE * deltaTime;
            if (m_bulletTimeEnergy > Config::MAX_BULLET_TIME_ENERGY) {
                m_bulletTimeEnergy = Config::MAX_BULLET_TIME_ENERGY;
            }
        }

        float worldDeltaTime = deltaTime * m_timeScale;
        m_accumulatedTime += worldDeltaTime;

        player.processMovement(camera.Front, camera.Right,
                               input.moveForward, input.moveBackward,
                               input.moveLeft, input.moveRight,
                               input.jumpTriggered, input.dashTriggered, worldDeltaTime);
        player.update(worldDeltaTime);
        camera.update(deltaTime);

        // Check for footsteps
        if (player.checkFootstep()) {
            int stepNum = (rand() % 2) + 1;
            std::string stepId = "footstep_" + std::to_string(stepNum);
            
            // If the selected variant isn't loaded (like footstep_2), fallback to footstep_1
            if (stepNum == 2 && !std::filesystem::exists(Config::Audio::FOOTSTEP_SOUND_2)) {
                stepId = "footstep_1";
            }
            
            std::cout << "[Game] Playing footstep: " << stepId << std::endl;
            audioSystem->playSound(stepId);
        }
        
        // Update HUD notifications
        if (hud) {
            hud->updateNotifications(deltaTime);
            hud->update(deltaTime);
        }

        Weapon* currentWeapon = player.getInventory().getCurrentWeapon();

        if (currentWeapon && input.fireHeld) {
            if (currentWeapon->fire(m_accumulatedTime)) {
                glm::vec3 muzzlePos = camera.Position + camera.Front * 0.5f;
                glm::vec3 fireDir = camera.Front;
                
                // Spawn projectile
                float speed = currentWeapon->getProjectileSpeed();
                float damage = currentWeapon->getDamage();
                float lifetime = currentWeapon->getProjectileLifetime();
                int projectileCount = currentWeapon->getProjectileCount();
                float spread = currentWeapon->getSpread();

                for (int i = 0; i < projectileCount; ++i) {
                    glm::vec3 spreadDir = fireDir;
                    if (spread > 0.0f) {
                        float r1 = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * spread;
                        float r2 = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * spread;
                        float r3 = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * spread;
                        spreadDir += glm::vec3(r1, r2, r3);
                        spreadDir = glm::normalize(spreadDir);
                    }
                    projectiles.emplace_back(muzzlePos, spreadDir, speed, damage, lifetime);
                }

                if (particleSystem) {
                    particleSystem->emitMuzzleFlash(muzzlePos, camera.Front, 12);
                }
                
                // Muzzle Flash Light
                m_playerMuzzleFlashTimer = 0.05f;
                m_playerMuzzleFlashPos = muzzlePos;
                // You could vary color by weapon type here if desired
                
                // Recoil
                auto data = Config::Weapon::getWeaponConfig(currentWeapon->getType());
                weaponRenderer.triggerRecoil(data.recoilRotation);
                camera.addRecoil(data.recoilAmount, ((float)(rand() % 100) / 50.0f - 1.0f) * 0.5f);

                // Audio
                if (audioSystem && strlen(data.fireSoundPath) > 0) {
                    audioSystem->playSound(data.fireSoundPath);
                }
            }
        }

        if (currentWeapon && input.reloadTriggered) {
            // Only play sound if we actually start reloading
            if (!currentWeapon->isReloading() && 
                currentWeapon->getReserveAmmo() > 0 && 
                currentWeapon->getCurrentAmmo() < currentWeapon->getMaxAmmo()) {
                
                currentWeapon->reload();
                
                // Play reload sound
                if (audioSystem) {
                    std::string reloadSound = currentWeapon->getReloadSoundPath();
                    if (!reloadSound.empty()) {
                        audioSystem->playSound(reloadSound);
                    }
                }
            }
        }

        if (input.switchTriggered) {
            player.getInventory().switchWeapon();
        }

        interactionPrompt = "";
        for (const auto& pickup : weaponPickups) {
            if (!pickup.isPickedUp() && pickup.canPickup(player.getPosition())) {
                int interactKey = Settings::getInstance().keybinds.interact;
                const char* keyName = glfwGetKeyName(interactKey, 0);
                std::string keyStr = (keyName) ? std::string(keyName) : "Key";
                // Simple uppercase
                if (!keyStr.empty() && keyStr[0] >= 'a' && keyStr[0] <= 'z') {
                    keyStr[0] -= 32;
                }
                interactionPrompt = "Press " + keyStr + " to pickup " + Config::Weapon::getWeaponConfig(pickup.getType()).name;
                break;
            }
        }

        if (input.pickupTriggered) {
            for (auto& pickup : weaponPickups) {
                if (!pickup.isPickedUp() && pickup.canPickup(player.getPosition())) {
                    auto weapon = pickup.pickup();
                    if (weapon) {
                        // Check if we are filling the secondary slot (which assumes primary is full)
                        bool shouldSwitch = player.getInventory().getSecondaryWeapon() == nullptr;
                        
                        player.getInventory().addWeapon(std::move(weapon));
                        
                        if (shouldSwitch) {
                            player.getInventory().switchToSecondary();
                        }
                        
                        // Robust sound playback
                        if (audioSystem) {
                             audioSystem->playSound(Config::Audio::PICKUP_SOUND);
                        }
                        break;
                    }
                }
            }
        }

        // Auto-collect health pickups on touch (no interaction key required)
        // Micro-optimizations: cache subsystem pointers and do a cheap broad-phase DSQ test
        AudioSystem* audio = audioSystem.get();
        ParticleSystem* particles = particleSystem.get();
        HUD* localHud = hud.get();

        const float pickupBroadphaseRadius = Config::Pickup::BROADPHASE_RADIUS; // tweakable early-reject radius
        const float pickupBroadphaseRadiusSq = pickupBroadphaseRadius * pickupBroadphaseRadius;
        const glm::vec3 playerPos = player.getPosition();

        for (auto& hp : healthPickups) {
            if (hp.isPickedUp()) continue;

            // If player already at max health, don't consume the pickup (only allow when < max)
            if (player.getHealth() >= player.getMaxHealth()) continue;

            // Broad-phase: squared-distance check before invoking pickup() / canPickup
            glm::vec3 d = hp.getPosition() - playerPos;
            if (glm::dot(d, d) > pickupBroadphaseRadiusSq) continue;

            // Narrow-phase: perform pickup (HealthPickup still guards against repeat pickups)
            float healed = hp.pickup();
            if (healed <= 0.0f) continue;

            player.heal(healed);

            if (localHud) localHud->flashHealthBar(Config::Pickup::HEALTH_FLASH_DURATION);
            if (audio) audio->playSound(Config::Audio::PICKUP_SOUND);
            if (particles) particles->emitExplosion(playerPos, Config::Effects::HEALTH_PICKUP_PARTICLES);
        }

        // Update Enemies
        bool anyEnemyAlive = false;
        for (auto& enemy : enemies) {
            if (enemy.justDied()) {
                if (!enemy.isWeaponDropped() && enemy.getWeapon()) {
                    weaponPickups.emplace_back(enemy.getPosition(), enemy.getWeapon()->getType());
                    enemy.setWeaponDropped(true);
                    if (particleSystem) {
                        particleSystem->emitExplosion(enemy.getPosition(), 10);
                    }
                }
            }

            if (!enemy.isAlive()) {
                continue;
            }
            anyEnemyAlive = true;

            enemy.update(worldDeltaTime, player.getPosition(), navigationGraph.get(), platforms, audioSystem.get());

            if (enemy.shouldShoot(m_accumulatedTime)) {
                Weapon* enemyWeapon = enemy.getWeapon();
                if (enemyWeapon && enemyWeapon->fire(m_accumulatedTime)) {
                    glm::vec3 enemyPos = enemy.getPosition();
                    glm::vec3 muzzlePos = enemyPos + glm::vec3(0.0f, 0.5f, 0.0f);
                    glm::vec3 targetPos = player.getPosition();
                    glm::vec3 shootDir = glm::normalize(targetPos - muzzlePos);

                    int projectileCount = enemyWeapon->getProjectileCount();
                    float spread = enemyWeapon->getSpread();
                    float speed = enemyWeapon->getProjectileSpeed();
                    float damage = enemyWeapon->getDamage();
                    float lifetime = enemyWeapon->getProjectileLifetime();

                    for (int i = 0; i < projectileCount; ++i) {
                        glm::vec3 currentSpreadDir = shootDir;
                        if (spread > 0.0f) {
                            float rx = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * spread;
                            float ry = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * spread;
                            float rz = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * spread;
                            currentSpreadDir = glm::normalize(shootDir + glm::vec3(rx, ry, rz));
                        }
                        
                        // Spawn Enemy Projectile (isEnemy=true)
                        projectiles.emplace_back(muzzlePos, currentSpreadDir, speed, damage, lifetime, true);
                    }

                    enemy.triggerMuzzleFlash(muzzlePos); // Trigger muzzle flash light effect

                    // Play enemy fire sound from weapon config
                    auto config = Config::Weapon::getWeaponConfig(enemyWeapon->getType());
                    if (audioSystem && strlen(config.fireSoundPath) > 0) {
                        audioSystem->play3DSound(config.fireSoundPath, enemy.getPosition());
                    }

                    if (particleSystem) {
                        particleSystem->emitMuzzleFlash(muzzlePos, shootDir, 8);
                    }
                }
            }
        }

        if (physicsSystem) {
            physicsSystem->update(worldDeltaTime);
        }

        explosionTimer += worldDeltaTime;
        fireTimer += worldDeltaTime;

        if (explosionTimer > 4.0f && platforms.size() > 2) {
            if (particleSystem) {
                particleSystem->emitExplosion(platforms[2].getPosition() + glm::vec3(0.0f, 1.5f, 0.0f), Config::Effects::EXPLOSION_PARTICLE_COUNT);
            }
            explosionTimer = 0.0f;
        }

        if (fireTimer > 0.1f && platforms.size() > 4) {
            if (particleSystem) {
                particleSystem->emitFire(platforms[4].getPosition() + glm::vec3(0.0f, 1.0f, 0.0f), Config::Effects::FIRE_PARTICLE_COUNT);
            }
            fireTimer = 0.0f;
        }

        camera.Position = player.getEyePosition();
        if (audioSystem) {
            audioSystem->updateListener(camera.Position, camera.Front, camera.Up);
        }

        // Refresh current weapon pointer in case it changed (pickup/switch)
        currentWeapon = player.getInventory().getCurrentWeapon();
        weaponRenderer.update(worldDeltaTime, input, currentWeapon);

        // Check for level completion (all enemies defeated)
        if (!anyEnemyAlive && currentLevel > 0) {
            // Check if next level exists
            if (levelManager && levelManager->levelExists(currentLevel + 1)) {
                state = GameState::LEVEL_WIN;
                // Save progress to next level
                Settings::getInstance().progress.lastLevelPlayed = currentLevel + 1;
                Settings::getInstance().save();
            } else {
                // No more levels -> Game Win!
                state = GameState::GAME_WIN;
                // Completed the game. Maybe reset progress or keep at last level?
                // Let's keep it at this level so they can replay it, or maybe set to 0.
                // Usually for 'completed' game we might just leave it be.
            }
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (static_cast<int>(glfwGetTime() * 2) % 2 == 0) {
        glfwSetWindowTitle(window, "Bullet Shift");
    }
}

void Game::handleCollisions() {
    // This now just handles basic game flow collisions if needed, 
    // or we can remove it entirely from Game class if PhysicsSystem handles all.
    // For now keeping it for future game-specific high level events.
}

void Game::showNotification(const std::string& text, float duration) {
    if (hud) {
        hud->queueNotification(text, duration);
    }
}

void Game::triggerBulletTime() {
    if (!m_bulletTimeActive && m_bulletTimeEnergy > 10.0f) {
        m_bulletTimeActive = true;
        // Play sound if available
        if (audioSystem) {
             audioSystem->playSound(Config::Audio::PICKUP_SOUND); 
        }
    }
}

void Game::applySettings() {
    auto& settings = Settings::getInstance();

    // Input
    camera.MouseSensitivity = settings.input.mouseSensitivity;
    if (settings.input.invertY) {
        // Inversion handled in mouse callback
    }

    // Graphics
    techStyleIntensity = settings.graphics.techStyleIntensity;
    
    if (settings.graphics.gammaCorrection) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    // Window
    if (window) {
        glfwSwapInterval(settings.window.vsync ? 1 : 0);

        // Fullscreen toggle
        
        GLFWmonitor* currentMonitor = glfwGetWindowMonitor(window);
        bool currentlyFullscreen = (currentMonitor != nullptr);

        if (settings.window.fullscreen != currentlyFullscreen) {
            if (settings.window.fullscreen) {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            } else {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                // Center window
                int xpos = (mode->width - settings.window.width) / 2;
                int ypos = (mode->height - settings.window.height) / 2;
                glfwSetWindowMonitor(window, nullptr, xpos, ypos, settings.window.width, settings.window.height, 0);
            }
        }
    }
}

void Game::render() {
    if (m_gameRenderer) {
        m_gameRenderer->render(
            m_accumulatedTime, 
            state, 
            currentLevel,
            *m_console,
            camera,
            player,
            weaponRenderer,
            particleSystem,
            postProcessing,
            resourceManager,
            hud,
            debugRenderer,
            skybox,
            shadowSystem,
            guiSystem,
            menuSystem,
            navigationGraph,
            platforms,
            enemies,
            weaponPickups,
            healthPickups,
            projectiles,
            techStyleIntensity,
            m_timeScale,
            m_playerMuzzleFlashTimer,
            m_playerMuzzleFlashPos,
            m_playerMuzzleFlashColor,
            interactionPrompt,
            m_bulletTimeEnergy,
            m_bulletTimeActive
        );
    }
}

void Game::framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
    if (instance) {
        Settings::getInstance().window.width = width;
        Settings::getInstance().window.height = height;
        if (instance->postProcessing) {
            instance->postProcessing->resize(width, height);
        }
    }
}

void Game::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!self || self->state != GameState::PLAYING) {
        return;
    }

    if (self->input.firstMouse) {
        self->input.lastMouseX = static_cast<float>(xpos);
        self->input.lastMouseY = static_cast<float>(ypos);
        self->input.firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - self->input.lastMouseX;
    float yoffset = self->input.lastMouseY - static_cast<float>(ypos);

    if (Settings::getInstance().input.invertY) {
        yoffset = -yoffset;
    }

    self->input.lastMouseX = static_cast<float>(xpos);
    self->input.lastMouseY = static_cast<float>(ypos);

    // If ImGui wants mouse, don't rotate camera
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) {
        return; 
    }

    self->camera.processMouseMovement(xoffset, yoffset);
}

void Game::scrollCallback(GLFWwindow* /*window*/, double, double /*yoffset*/) {
    // Zoom disabled
}

void Game::glfwErrorCallback(int errorCode, const char* description) {
    std::cerr << "GLFW Error [" << errorCode << "]: " << (description ? description : "<no description>") << std::endl;
}
