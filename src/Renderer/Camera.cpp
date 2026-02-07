#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
      MovementSpeed(Config::Camera::SPEED), 
      MouseSensitivity(Config::Camera::SENSITIVITY), 
      Zoom(Config::Camera::ZOOM), 
      RecoilPitch(0.0f), RecoilYaw(0.0f) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position += WorldUp * velocity;
    if (direction == DOWN)
        Position -= WorldUp * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    
    Yaw   += xoffset;
    Pitch += yoffset;
    
    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }
    
    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::updateCameraVectors() {
    float effectivePitch = Pitch + RecoilPitch;
    float effectiveYaw = Yaw + RecoilYaw;
    
    // Constrain pitch with recoil
    if (effectivePitch > 89.0f) effectivePitch = 89.0f;
    if (effectivePitch < -89.0f) effectivePitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(effectiveYaw)) * cos(glm::radians(effectivePitch));
    front.y = sin(glm::radians(effectivePitch));
    front.z = sin(glm::radians(effectiveYaw)) * cos(glm::radians(effectivePitch));
    Front = glm::normalize(front);
    
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
void Camera::addRecoil(float pitch, float yaw) {
    RecoilPitch += pitch;
    RecoilYaw += yaw;
}

void Camera::update(float deltaTime) {
    // Decay recoil
    RecoilPitch = glm::mix(RecoilPitch, 0.0f, deltaTime * 10.0f);
    RecoilYaw = glm::mix(RecoilYaw, 0.0f, deltaTime * 10.0f);
    updateCameraVectors();
}
