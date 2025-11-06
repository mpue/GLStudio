#include "VoxelCharacterController.h"
#include "VoxelWorld.h"
#include "VoxelChunk.h"
#include <cmath>
#include <algorithm>

VoxelCharacterController::VoxelCharacterController(VoxelWorld* world, GLFWwindow* window)
    : voxelWorld(world)
    , window(window)
  , position(0.0f, 10.0f, 0.0f)
    , velocity(0.0f)
    , front(0.0f, 0.0f, -1.0f)
    , up(0.0f, 1.0f, 0.0f)
    , right(1.0f, 0.0f, 0.0f)
  , yaw(-90.0f)
    , pitch(0.0f)
    , height(1.8f)
    , radius(0.3f)
    , moveSpeed(5.0f)
    , mouseSensitivity(0.1f)
    , jumpForce(8.0f)
    , gravity(-20.0f)
    , isOnGround(false)
    , isJumping(false)
{
    updateVectors();
}

VoxelCharacterController::~VoxelCharacterController() {
}

void VoxelCharacterController::updateVectors() {
    // Berechne Front-Vektor
    glm::vec3 newFront;
newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    // Berechne Right und Up Vektoren
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}

void VoxelCharacterController::onMouseMove(double dx, double dy) {
    yaw += static_cast<float>(dx) * mouseSensitivity;
    pitch -= static_cast<float>(dy) * mouseSensitivity;
    
    // Begrenze Pitch
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    
  updateVectors();
}

bool VoxelCharacterController::isBlockSolid(int x, int y, int z) {
  BlockType type = voxelWorld->getBlock(x, y, z);
    return type != BlockType::Air;
}

bool VoxelCharacterController::checkCollision(const glm::vec3& newPos) {
    // Prüfe Blöcke um die Charakterposition
    int minX = static_cast<int>(std::floor(newPos.x - radius));
    int maxX = static_cast<int>(std::ceil(newPos.x + radius));
    int minY = static_cast<int>(std::floor(newPos.y));
    int maxY = static_cast<int>(std::ceil(newPos.y + height));
    int minZ = static_cast<int>(std::floor(newPos.z - radius));
    int maxZ = static_cast<int>(std::ceil(newPos.z + radius));
    
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                if (isBlockSolid(x, y, z)) {
                return true;
          }
         }
      }
    }
    
    return false;
}

void VoxelCharacterController::resolveCollision(glm::vec3& newPos) {
    // Einfache Kollisionsauflösung: Schiebe Position zurück
    const float step = 0.01f;
    glm::vec3 direction = glm::normalize(newPos - position);
    
  while (checkCollision(newPos) && glm::length(newPos - position) > 0.01f) {
    newPos -= direction * step;
    }
}

glm::vec3 VoxelCharacterController::getGroundPosition() {
    // Finde den Boden unter dem Charakter
    glm::vec3 checkPos = position;
    
    for (int dy = 0; dy < 10; ++dy) {
        checkPos.y = position.y - dy;
        
        int blockY = static_cast<int>(std::floor(checkPos.y));
     int blockX = static_cast<int>(std::round(checkPos.x));
        int blockZ = static_cast<int>(std::round(checkPos.z));
        
        if (isBlockSolid(blockX, blockY, blockZ)) {
            return glm::vec3(checkPos.x, blockY + 1.0f, checkPos.z);
 }
    }
    
    return position;
}

void VoxelCharacterController::processKeyboard(float deltaTime) {
    glm::vec3 movement(0.0f);
    
    // Bewegung nur in horizontaler Ebene
    glm::vec3 frontHorizontal = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 rightHorizontal = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        movement += frontHorizontal;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
 movement -= frontHorizontal;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        movement -= rightHorizontal;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        movement += rightHorizontal;
    }
    
 // Normalisiere Bewegung wenn diagonal
    if (glm::length(movement) > 0.0f) {
 movement = glm::normalize(movement) * moveSpeed * deltaTime;
    }
    
 // Springen
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isOnGround && !isJumping) {
        jump();
    }
    
    // Aktualisiere Position horizontal
    if (glm::length(movement) > 0.0f) {
        glm::vec3 newPos = position + movement;
        
     // Prüfe Kollision nur horizontal
        glm::vec3 testPos = newPos;
        testPos.y = position.y; // Behalte Y-Position für Test

        if (!checkCollision(testPos)) {
         position.x = newPos.x;
         position.z = newPos.z;
 } else {
     // Versuche Bewegung in X und Z einzeln
 testPos = position;
         testPos.x = newPos.x;
            if (!checkCollision(testPos)) {
          position.x = newPos.x;
  }
            
         testPos = position;
            testPos.z = newPos.z;
 if (!checkCollision(testPos)) {
       position.z = newPos.z;
   }
        }
    }
}

void VoxelCharacterController::jump() {
    if (isOnGround) {
        velocity.y = jumpForce;
        isOnGround = false;
        isJumping = true;
    }
}

void VoxelCharacterController::update(float deltaTime) {
    processKeyboard(deltaTime);
    
    // Gravitation
  velocity.y += gravity * deltaTime;
    
    // Aktualisiere vertikale Position
    glm::vec3 newPos = position;
    newPos.y += velocity.y * deltaTime;

    // Prüfe Boden-Kollision
 int footBlockY = static_cast<int>(std::floor(newPos.y));
    int footBlockX = static_cast<int>(std::round(newPos.x));
    int footBlockZ = static_cast<int>(std::round(newPos.z));
    
    // Prüfe ob auf dem Boden
    if (velocity.y <= 0.0f) {
        if (isBlockSolid(footBlockX, footBlockY, footBlockZ)) {
            // Auf Boden
            position.y = footBlockY + 1.0f;
      velocity.y = 0.0f;
     isOnGround = true;
  isJumping = false;
        } else {
            // In der Luft
     position.y = newPos.y;
       isOnGround = false;
        }
    } else {
        // Steigt nach oben
        // Prüfe Decken-Kollision
        int headBlockY = static_cast<int>(std::ceil(newPos.y + height));
        
        if (isBlockSolid(footBlockX, headBlockY, footBlockZ)) {
            // Kopf stößt an Decke
            velocity.y = 0.0f;
      position.y = headBlockY - height - 0.01f;
   } else {
          position.y = newPos.y;
            isOnGround = false;
        }
    }
    
    // Verhindere Fallen ins Void
    if (position.y < -50.0f) {
      position.y = 20.0f;
        velocity.y = 0.0f;
    }
}
