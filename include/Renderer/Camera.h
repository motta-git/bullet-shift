#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Config.h"

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = Config::Camera::YAW, 
           float pitch = Config::Camera::PITCH);
    
    glm::mat4 getViewMatrix() const;
    
    void processKeyboard(Camera_Movement direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    
    void addRecoil(float pitch, float yaw);
    void update(float deltaTime);
    
private:
    float RecoilPitch;
    float RecoilYaw;

    void updateCameraVectors();
};
