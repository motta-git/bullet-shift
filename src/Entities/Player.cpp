#include "Player.h"
#include "Config.h"

Player::Player(glm::vec3 position)
    : position(position),
      velocity(0.0f),
      size(Config::PLAYER_WIDTH, Config::PLAYER_HEIGHT, Config::PLAYER_DEPTH),
      onGround(false),
      eyeHeight(Config::EYE_HEIGHT),
      moveSpeed(Config::MOVE_SPEED),
      jumpForce(Config::JUMP_FORCE),
      gravity(Config::GRAVITY),
      health(100.0f),
      maxHealth(100.0f),
      inventory() {
}

void Player::update(float deltaTime) {
    // Apply gravity
    if (!onGround) {
        velocity.y -= gravity * deltaTime;
    } else {
        if (velocity.y < 0.0f) {
            velocity.y = 0.0f;
        }
    }
    
    // Update position - removed (now handled in PhysicsSystem with sub-stepping)
    // position += velocity * deltaTime;
    
    // Check for falling out of the map
    if (position.y < Config::FALL_DEATH_THRESHOLD) {
        health = 0.0f; // Instant death from falling out of map
    }
    
    // Update dash state
    if (m_isDashing) {
        m_dashTimer -= deltaTime;
        if (m_dashTimer <= 0.0f) {
            m_isDashing = false;
        }
    }
    if (m_dashCooldown > 0.0f) {
        m_dashCooldown -= deltaTime;
    }

    // Update inventory
    inventory.update(deltaTime);
}

void Player::takeDamage(float damage, glm::vec3 /*sourcePosition*/) {
    health -= damage;
    if (health < 0.0f) {
        health = 0.0f;
    }
}

void Player::reset() {
    health = maxHealth;
    velocity = glm::vec3(0.0f);
    position = glm::vec3(0.0f, 2.0f, 0.0f); // Safe default height (center)
    onGround = false; 
    inventory = Inventory(); // Reset weapons
}

void Player::processMovement(glm::vec3 front, glm::vec3 right, bool moveForward, bool moveBackward,
                             bool moveLeft, bool moveRight, bool jump, bool dash, float deltaTime) {
    if (health <= 0.0f) return; // No movement if dead

    if (dash && m_dashCooldown <= 0.0f && !m_isDashing && !onGround) {
        m_isDashing = true;
        m_dashTimer = Config::DASH_DURATION;
        m_dashCooldown = Config::DASH_COOLDOWN;
        
        // Dash direction is either movement direction or camera front
        glm::vec3 moveDir = glm::vec3(0.0f);
        glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
        glm::vec3 flatRight = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
        
        if (moveForward) moveDir += flatFront;
        if (moveBackward) moveDir -= flatFront;
        if (moveRight) moveDir += flatRight;
        if (moveLeft) moveDir -= flatRight;
        
        if (glm::length(moveDir) > 0.1f) {
            m_dashDirection = glm::normalize(moveDir);
        } else {
            m_dashDirection = flatFront;
        }
    }

    if (m_isDashing) {
        velocity.x = m_dashDirection.x * Config::DASH_SPEED;
        velocity.z = m_dashDirection.z * Config::DASH_SPEED;
        return; // Skip normal movement while dashing
    }

    // Horizontal movement based on camera direction
    glm::vec3 targetDir = glm::vec3(0.0f);
    glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
    
    if (moveForward) targetDir += flatFront;
    if (moveBackward) targetDir -= flatFront;
    if (moveRight) targetDir += flatRight;
    if (moveLeft) targetDir -= flatRight;
    
    // Normalize target direction
    if (glm::length(targetDir) > 0.0f) {
        targetDir = glm::normalize(targetDir);
    }

    glm::vec3 targetVelocity = targetDir * moveSpeed;
    
    // Smoothly interpolate current horizontal velocity towards target velocity
    glm::vec3 currentHorizontalVel(velocity.x, 0.0f, velocity.z);
    
    // Determine if we are accelerating or decelerating (friction)
    float accel = (glm::length(targetDir) > 0.0f) ? Config::PLAYER_ACCELERATION : Config::PLAYER_DECELERATION;
    
    // Manual move towards target
    glm::vec3 diff = targetVelocity - currentHorizontalVel;
    float diffLen = glm::length(diff);
    
    if (diffLen > 0.001f) {
        glm::vec3 dir = diff / diffLen;
        float change = accel * deltaTime;
        if (change > diffLen) change = diffLen; // Don't overshoot
        currentHorizontalVel += dir * change;
    } else {
        currentHorizontalVel = targetVelocity;
    }
    
    velocity.x = currentHorizontalVel.x;
    velocity.z = currentHorizontalVel.z;
    
    // Jump
    if (jump && onGround) {
        velocity.y = jumpForce;
        onGround = false;
    }

    // Update footstep counter if moving on ground
    if (onGround) {
        float horizontalSpeed = glm::length(glm::vec3(velocity.x, 0.0f, velocity.z));
        if (horizontalSpeed > 0.1f) {
            stepCounter += horizontalSpeed * deltaTime;
        }
    }
}

bool Player::checkFootstep() {
    if (stepCounter >= Config::Audio::STEP_DISTANCE) {
        stepCounter -= Config::Audio::STEP_DISTANCE;
        return true;
    }
    return false;
}
