# Unit tests (GoogleTest)

enable_testing()

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# Keep it quiet / fast on CI
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Gather test sources
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/tests/unit/*.cpp")

add_executable(unit_tests
    ${TEST_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/Entities/Player.cpp
    ${CMAKE_SOURCE_DIR}/src/Entities/Weapon.cpp
    ${CMAKE_SOURCE_DIR}/src/Entities/Inventory.cpp
    ${CMAKE_SOURCE_DIR}/src/Entities/HealthPickup.cpp
    ${CMAKE_SOURCE_DIR}/src/Entities/Projectile.cpp
    ${CMAKE_SOURCE_DIR}/src/Entities/Enemy.cpp
    ${CMAKE_SOURCE_DIR}/src/Entities/WeaponPickup.cpp
    ${CMAKE_SOURCE_DIR}/src/Entities/Platform.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/RaycastUtility.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/NavigationGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/AudioSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Core/Settings.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/HUD.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(unit_tests PRIVATE
    gtest_main
    glm::glm
    imgui
    miniaudio
    glfw
)

add_test(NAME unit_tests COMMAND unit_tests)
