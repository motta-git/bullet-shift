#pragma once

struct InputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jumpHeld = false;
    bool jumpTriggered = false;

    bool fireHeld = false;
    bool reloadHeld = false;
    bool reloadTriggered = false;

    bool switchHeld = false;
    bool switchTriggered = false;

    bool pickupHeld = false;
    bool pickupTriggered = false;

    bool escHeld = false;
    bool escTriggered = false;

    bool bulletTimeHeld = false;
    bool bulletTimeTriggered = false;

    bool dashHeld = false;
    bool dashTriggered = false;

    bool firstMouse = true;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;

    bool isMoving() const {
        return moveForward || moveBackward || moveLeft || moveRight;
    }
};
