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
#include <imgui.h>

namespace {
const char* title = "Dodger";
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
      state(GameState::MAIN_MENU),
      currentLevel(0) {
    auto& settings = Settings::getInstance();
    // Load settings first (try to load file, if not, it uses defaults)
    settings.load();
    
    techStyleIntensity = settings.graphics.techStyleIntensity;
    
    input.lastMouseX = settings.window.width / 2.0f;
    input.lastMouseY = settings.window.height / 2.0f;
}

Game::~Game() {
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

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
    instance = nullptr;
}

bool Game::initialize() {
    if (instance) {
        std::cerr << "Game instance already exists" << std::endl;
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
        
        this->renderHUD();

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
        const std::string pickupSoundId = "assets/sounds/sfx/pickup.ogg";
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

    
    particleSystem = std::make_unique<ParticleSystem>(Config::MAX_PARTICLES);
    // Enable atmospheric particles (ambient dust/motes) around the camera
    particleSystem->enableAtmospheric(true);
    particleSystem->setAtmosphereRate(8);   // particles per second
    particleSystem->setAtmosphereRadius(25.0f); // spawn radius around camera

    debugRenderer = std::make_unique<DebugRenderer>();

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
                             audioSystem->playSound("assets/sounds/sfx/pickup.ogg");
                        }
                        break;
                    }
                }
            }
        }

        // Update Enemies
        bool anyEnemyAlive = false;
        for (auto& enemy : enemies) {
            if (!enemy.isAlive()) {
                continue;
            }
            anyEnemyAlive = true;

            // Pass audioSystem to allow enemy to play alert SFX when it loses sight
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
                particleSystem->emitExplosion(platforms[2].getPosition() + glm::vec3(0.0f, 1.5f, 0.0f), 60);
            }
            explosionTimer = 0.0f;
        }

        if (fireTimer > 0.1f && platforms.size() > 4) {
            if (particleSystem) {
                particleSystem->emitFire(platforms[4].getPosition() + glm::vec3(0.0f, 1.0f, 0.0f), 8);
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
        glfwSetWindowTitle(window, "Dodger");
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
             audioSystem->playSound("assets/sounds/sfx/pickup.ogg"); 
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
    // Use manual gamma correction in post-processing if possible, 
    // but for now we follow the existing toggle logic.
    // When rendering to HDR FBO, we should work in linear space.
    if (Settings::getInstance().graphics.gammaCorrection && !postProcessing) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    // --- Shadow Pass ---
    if (shadowSystem) {
        Shader* depthShader = resourceManager->getShader("shadowDepth");
        if (depthShader) {
            glm::vec3 lightDir(-0.3f, -1.0f, -0.2f); // Same as dirLight in renderScene
            shadowSystem->updateLightSpaceMatrix(lightDir, player.getPosition());
            
            depthShader->use();
            depthShader->setMat4("lightSpaceMatrix", shadowSystem->getLightSpaceMatrix());
            
            shadowSystem->bindForWriting();
            renderDepthScene(*depthShader);
            shadowSystem->unbind();
            
            // Restore viewport after shadow pass
            glViewport(0, 0, Settings::getInstance().window.width, Settings::getInstance().window.height);
        }
    }

    if (postProcessing) {
        postProcessing->begin();
    } else {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            static_cast<float>(Settings::getInstance().window.width) / Settings::getInstance().window.height,
                                            Config::NEAR_PLANE,
                                            Config::FAR_PLANE);
    glm::mat4 view = camera.getViewMatrix();

    renderScene(projection, view);
    
    // Render Skybox (using GL_LEQUAL depth test)
    if (skybox) {
        Shader* skyShader = resourceManager->getShader("skybox");
        if (skyShader) {
            skybox->render(projection, view, *skyShader);
        }
    }

    // Render Weapon Hand Model (after skybox so it's always on top)
    Shader* lightingShader = resourceManager->getShader("lighting");
    if (lightingShader) {
        lightingShader->use();
        weaponRenderer.render(camera, *lightingShader, player.getInventory().getCurrentWeapon(), *resourceManager, m_accumulatedTime);
    }

    renderLights(projection, view);
    renderProjectiles(projection, view);

    Shader* particleShader = resourceManager->getShader("particle");
    if (particleShader && particleSystem) {
        particleSystem->draw(projection, view, *particleShader);
    }

    if (debugRenderer) {
        debugRenderer->render(projection, view);
        
        // Debug visualization for navigation graph
        if (navigationGraph && navigationGraph->isValid() && state == GameState::PLAYING) {
            const auto& nodes = navigationGraph->getNodes();
            const auto& edges = navigationGraph->getEdges();
            
            // Draw navigation graph edges
            for (const auto& edge : edges) {
                if (edge.fromNode < static_cast<int>(nodes.size()) && 
                    edge.toNode < static_cast<int>(nodes.size())) {
                    glm::vec3 from = nodes[edge.fromNode].position;
                    glm::vec3 to = nodes[edge.toNode].position;
                    debugRenderer->addLine(from, to, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);
                }
            }
            
            // Draw enemy paths
            for (const auto& enemy : enemies) {
                if (!enemy.isAlive()) continue;
                
                // Draw line of sight check
                glm::vec3 enemyEye = enemy.getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
                glm::vec3 playerEye = player.getEyePosition();
                bool hasLOS = enemy.canSeePlayer(player.getPosition());
                glm::vec3 losColor = hasLOS ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.5f, 0.5f, 0.5f);
                debugRenderer->addLine(enemyEye, playerEye, losColor, 0.0f);
            }
        }
    }

    if (postProcessing) {
        // Calculate intensity based on current time scale
        float btIntensity = (1.0f - m_timeScale) / (1.0f - Config::MIN_BULLET_TIME_SCALE);
        postProcessing->setBulletTimeIntensity(btIntensity);
        
        postProcessing->end();
        
        // Enable gamma correction for the final resolve if enabled
        if (Settings::getInstance().graphics.gammaCorrection) {
            glEnable(GL_FRAMEBUFFER_SRGB);
        }
        
        postProcessing->render(Settings::getInstance().window.width, Settings::getInstance().window.height, 
                               Config::NEAR_PLANE, Config::FAR_PLANE, resourceManager.get());
    }

    // Disable Gamma Correction for UI to avoid double correction (Linear -> sRGB -> sRGB)
    // UI is usually already sRGB
    if (Settings::getInstance().graphics.gammaCorrection) {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    renderGUI();
}

void Game::renderScene(const glm::mat4& projection, const glm::mat4& view) {
    Shader* lightingShader = resourceManager->getShader("lighting");
    if (!lightingShader) return;

    lightingShader->use();
    lightingShader->setVec3("viewPos", camera.Position);
    lightingShader->setMat4("projection", projection);
    lightingShader->setMat4("view", view);
    lightingShader->setBool("u_useHardwareGamma", Settings::getInstance().graphics.gammaCorrection);
    
    if (shadowSystem) {
        lightingShader->setMat4("u_lightSpaceMatrix", shadowSystem->getLightSpaceMatrix());
        glActiveTexture(GL_TEXTURE4); // Texture unit 4 for shadow map
        glBindTexture(GL_TEXTURE_2D, shadowSystem->getDepthMap());
        lightingShader->setInt("shadowMap", 4);
    }
    
    // Tech-style effects
    lightingShader->setFloat("u_time", m_accumulatedTime);
    lightingShader->setFloat("u_techStyleIntensity", techStyleIntensity);

    // Directional Light
    lightingShader->setVec3("dirLight.direction", -0.3f, -1.0f, -0.2f);
    lightingShader->setVec3("dirLight.ambient", 0.35f, 0.35f, 0.4f);
    lightingShader->setVec3("dirLight.diffuse", 0.7f, 0.7f, 0.8f);
    lightingShader->setVec3("dirLight.specular", 0.3f, 0.3f, 0.3f);

    // Point Lights
    glm::vec3 pointPositions[] = {
        glm::vec3(-8.0f, 3.0f, -8.0f), glm::vec3(8.0f, 3.0f, -8.0f),
        glm::vec3(-8.0f, 3.0f, 8.0f), glm::vec3(8.0f, 3.0f, 8.0f)
    };
    glm::vec3 pointColors[] = {
        glm::vec3(1.0f, 0.8f, 0.6f), glm::vec3(0.8f, 0.9f, 1.0f),
        glm::vec3(1.0f, 0.7f, 0.5f), glm::vec3(0.6f, 0.8f, 1.0f)
    };

    for (int i = 0; i < 4; ++i) {
        std::string prefix = "pointLights[" + std::to_string(i) + "].";
        lightingShader->setVec3(prefix + "position", pointPositions[i]);
        lightingShader->setVec3(prefix + "ambient", pointColors[i] * 0.1f);
        lightingShader->setVec3(prefix + "diffuse", pointColors[i]);
        lightingShader->setVec3(prefix + "specular", pointColors[i]);
        lightingShader->setFloat(prefix + "constant", 1.0f);
        lightingShader->setFloat(prefix + "linear", 0.09f);
        lightingShader->setFloat(prefix + "quadratic", 0.032f);
    }

    // Spot Light (Flashlight)
    lightingShader->setVec3("spotLight.position", camera.Position);
    lightingShader->setVec3("spotLight.direction", camera.Front);
    lightingShader->setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    lightingShader->setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    lightingShader->setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    lightingShader->setFloat("spotLight.constant", 1.0f);
    lightingShader->setFloat("spotLight.linear", 0.09f);
    lightingShader->setFloat("spotLight.quadratic", 0.032f);
    lightingShader->setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    lightingShader->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));

    // Platforms / Level Geometry
    lightingShader->setVec3("material.ambient", 0.3f, 0.3f, 0.4f);
    lightingShader->setVec3("material.diffuse", 0.5f, 0.5f, 0.7f);
    lightingShader->setVec3("material.specular", 0.3f, 0.3f, 0.3f);
    lightingShader->setFloat("material.shininess", 32.0f);
    
    Mesh* cubeMesh = resourceManager->getMesh("cube");

    // Unified platform rendering (supports both GLB meshes and procedural cubes)
    for (const auto& platform : platforms) {
        if (platform.hasMesh()) {
            // Render the GLB mesh using its stored transform
            lightingShader->setMat4("model", platform.getTransform());
            for (const Mesh* mesh : platform.getMeshes()) {
                mesh->draw();
            }
        } else if (cubeMesh) {
            // Fallback to generic cubes for hardcoded levels or manual platforms
            glm::mat4 model = glm::translate(glm::mat4(1.0f), platform.getPosition());
            model = glm::scale(model, platform.getSize());
            lightingShader->setMat4("model", model);
            cubeMesh->draw();
        }
    }

    if (cubeMesh) {
        for (const auto& enemy : enemies) {
            if (!enemy.isAlive()) continue;

            // Base material colors for enemy
            glm::vec3 ambient(0.7f, 0.2f, 0.2f);
            glm::vec3 diffuse(0.9f, 0.3f, 0.3f);
            glm::vec3 spec(0.5f, 0.5f, 0.5f);
            float shininess = 64.0f;

            // Apply alert tint based on enemy alert progress (1.0 -> bright red)
            float alert = enemy.getAlertProgress();
            if (alert > 0.001f) {
                glm::vec3 alertColor(1.0f, 0.2f, 0.2f);
                ambient = glm::mix(ambient, alertColor, alert);
                diffuse = glm::mix(diffuse, alertColor, alert);
            }

            lightingShader->setVec3("material.ambient", ambient);
            lightingShader->setVec3("material.diffuse", diffuse);
            lightingShader->setVec3("material.specular", spec);
            lightingShader->setFloat("material.shininess", shininess);

            // Entities are now correctly center-aligned in physics, so we translate directly to their position.
            glm::mat4 model = glm::translate(glm::mat4(1.0f), enemy.getPosition());
            model = glm::scale(model, enemy.getSize());
            lightingShader->setMat4("model", model);
            cubeMesh->draw();
        }
    }

    // Weapon Pickups
    for (const auto& pickup : weaponPickups) {
        if (pickup.isPickedUp()) continue;
        
        auto data = Config::Weapon::getWeaponConfig(pickup.getType());
        const auto* meshes = resourceManager->getWeaponMeshes(data.name);

        if (meshes && !meshes->empty()) {
            lightingShader->setVec3("material.ambient", 0.5f, 0.5f, 0.5f);
            lightingShader->setVec3("material.diffuse", 0.8f, 0.8f, 0.8f);
            lightingShader->setVec3("material.specular", 1.0f, 1.0f, 1.0f);
            lightingShader->setFloat("material.shininess", 128.0f);

            glm::vec3 pickupPos = pickup.getPosition();
            pickupPos.y += 0.2f + 0.1f * std::sin(m_accumulatedTime * 2.0f);
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pickupPos);
            // Global rotation
            model = glm::rotate(model, m_accumulatedTime, glm::vec3(0.0f, 1.0f, 0.0f));
            
            // Apply model-specific corrections from Config to ensure they are oriented correctly
            model = glm::rotate(model, glm::radians(data.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(data.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(data.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

            // Use the base scale from Config (1.0 relative to the intended size)
            // Scale down pickups slightly as they might look too large on the floor compared to FP view
            float scale = data.scale * 0.6f; 
            model = glm::scale(model, glm::vec3(scale));

            lightingShader->setMat4("model", model);
            for (const auto& mesh : *meshes) {
                mesh->draw();
            }
        } else if (cubeMesh) {
            // Fallback to cube if model not found
            lightingShader->setVec3("material.ambient", 0.7f, 0.6f, 0.2f);
            lightingShader->setVec3("material.diffuse", 0.9f, 0.8f, 0.3f);
            lightingShader->setVec3("material.specular", 0.8f, 0.8f, 0.8f);
            lightingShader->setFloat("material.shininess", 96.0f);

            glm::vec3 pickupPos = pickup.getPosition();
            pickupPos.y += 0.2f * std::sin(m_accumulatedTime * 2.0f);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pickupPos);
            model = glm::rotate(model, m_accumulatedTime, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.3f, 0.5f, 0.2f));
            lightingShader->setMat4("model", model);
            cubeMesh->draw();
        }
    }

    // Player (Self) is not rendered in first-person view to avoid clipping with the camera.
    /*
    lightingShader->setVec3("material.ambient", 0.25f, 0.35f, 0.75f);
    lightingShader->setVec3("material.diffuse", 0.4f, 0.6f, 1.0f);
    lightingShader->setVec3("material.specular", 0.9f, 0.9f, 1.0f);
    lightingShader->setFloat("material.shininess", 128.0f);

    glm::mat4 playerModel = glm::translate(glm::mat4(1.0f), player.getPosition());
    playerModel = glm::scale(playerModel, player.getSize());
    lightingShader->setMat4("model", playerModel);
    if (cubeMesh) cubeMesh->draw();
    */
}

void Game::renderLights(const glm::mat4& projection, const glm::mat4& view) {
    Shader* lightSourceShader = resourceManager->getShader("lightSource");
    if (!lightSourceShader) return;

    lightSourceShader->use();
    lightSourceShader->setMat4("projection", projection);
    lightSourceShader->setMat4("view", view);

    glm::vec3 pointPositions[] = {
        glm::vec3(-8.0f, 3.0f, -8.0f), glm::vec3(8.0f, 3.0f, -8.0f),
        glm::vec3(-8.0f, 3.0f, 8.0f), glm::vec3(8.0f, 3.0f, 8.0f)
    };
    glm::vec3 pointColors[] = {
        glm::vec3(1.0f, 0.8f, 0.6f), glm::vec3(0.8f, 0.9f, 1.0f),
        glm::vec3(1.0f, 0.7f, 0.5f), glm::vec3(0.6f, 0.8f, 1.0f)
    };

    // Floating light spheres removed per user request.
    // Lights are still applied in the lighting shader, we only remove the decorative geometry.
    (void)pointPositions; (void)pointColors; // silence unused-variable warnings if any
}

void Game::renderProjectiles(const glm::mat4& projection, const glm::mat4& view) {
    Shader* lightSourceShader = resourceManager->getShader("lightSource");
    if (!lightSourceShader) return;

    lightSourceShader->use();
    lightSourceShader->setMat4("projection", projection);
    lightSourceShader->setMat4("view", view);

    Mesh* cubeMesh = resourceManager->getMesh("cube");
    if (cubeMesh) {
        for (const auto& proj : projectiles) {
            if (proj.getTimeElapsed() < 0.05f) continue;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), proj.getPosition());
            glm::vec3 dir = glm::normalize(proj.getVelocity());
            glm::vec3 up = std::abs(dir.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
            glm::mat4 rot = glm::lookAt(glm::vec3(0.0f), dir, up);
            model = model * glm::inverse(rot);
            model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.4f));

            lightSourceShader->setMat4("model", model);
            lightSourceShader->setVec3("lightColor", proj.isEnemyProjectile() ? glm::vec3(1.0f, 0.2f, 0.2f) : glm::vec3(1.0f, 1.0f, 0.4f));
            cubeMesh->draw();
        }
    }
}

void Game::renderHUD() {
    if (state == GameState::GAME_OVER && hud) {
        hud->renderDeathScreen();
        return;
    }

    if (player.isAlive() && hud) {
        Weapon* currentWeapon = player.getInventory().getCurrentWeapon();
        std::string name = currentWeapon ? currentWeapon->getName() : "None";
        int ammo = currentWeapon ? currentWeapon->getCurrentAmmo() : 0;
        int reserve = currentWeapon ? currentWeapon->getReserveAmmo() : 0;
        bool reloading = currentWeapon ? currentWeapon->isReloading() : false;

        int enemyCount = 0;
        for (const auto& enemy : enemies) if (enemy.isAlive()) enemyCount++;

        hud->render(player.getHealth(), player.getMaxHealth(),
                    name, ammo, reserve, reloading,
                    enemyCount, interactionPrompt, m_bulletTimeEnergy, Config::MAX_BULLET_TIME_ENERGY, m_bulletTimeActive);
    }
}

void Game::renderGUI() {
    if (menuSystem) {
        menuSystem->render(state, currentLevel);
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

void Game::renderDepthScene(Shader& depthShader) {
    Mesh* cubeMesh = resourceManager->getMesh("cube");


    // Platforms
    for (const auto& platform : platforms) {
        if (platform.hasMesh()) {
            depthShader.setMat4("model", platform.getTransform());
            for (const Mesh* mesh : platform.getMeshes()) {
                mesh->draw();
            }
        } else if (cubeMesh) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), platform.getPosition());
            model = glm::scale(model, platform.getSize());
            depthShader.setMat4("model", model);
            cubeMesh->draw();
        }
    }

    // Enemies
    if (cubeMesh) {
        for (const auto& enemy : enemies) {
            if (!enemy.isAlive()) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), enemy.getPosition());
            model = glm::scale(model, enemy.getSize());
            depthShader.setMat4("model", model);
            cubeMesh->draw();
        }
    }
}
