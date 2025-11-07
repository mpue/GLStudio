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
    , radius(0.25f)  // Reduziert von 0.3 auf 0.25 für weniger Hängenbleiben
    , moveSpeed(6.0f)  // Erhöht von 5.0 auf 6.0 für flüssigere Bewegung
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
    // Prüfe Blöcke um die Charakterposition mit kleinerer, präziserer Collision Box
// Reduziere radius leicht für weniger "Hängenbleiben"
    float effectiveRadius = radius * 0.9f;  // 10% kleiner für glattere Bewegung
    
    int minX = static_cast<int>(std::floor(newPos.x - effectiveRadius));
    int maxX = static_cast<int>(std::ceil(newPos.x + effectiveRadius));
    int minY = static_cast<int>(std::floor(newPos.y + 0.1f));  // Kleine Toleranz am Boden
    int maxY = static_cast<int>(std::ceil(newPos.y + height - 0.1f));  // Kleine Toleranz an der Decke
    int minZ = static_cast<int>(std::floor(newPos.z - effectiveRadius));
    int maxZ = static_cast<int>(std::ceil(newPos.z + effectiveRadius));
    
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
    
  // VERBESSERTE HORIZONTALE BEWEGUNG MIT SLIDING
    if (glm::length(movement) > 0.0f) {
  glm::vec3 targetPos = position + movement;
        
  // Versuche volle Bewegung
        glm::vec3 testPos = position;
        testPos.x = targetPos.x;
        testPos.z = targetPos.z;
        
        if (!checkCollision(testPos)) {
    // Volle Bewegung möglich
        position.x = targetPos.x;
            position.z = targetPos.z;
   } else {
    // SLIDING: Versuche Bewegung in X und Z getrennt (ermöglicht entlang Wänden zu gleiten)
            
          // Versuche nur X-Bewegung
        testPos = position;
   testPos.x = targetPos.x;
            if (!checkCollision(testPos)) {
        position.x = targetPos.x;
            }
            
    // Versuche nur Z-Bewegung
       testPos = position;
      testPos.z = targetPos.z;
         if (!checkCollision(testPos)) {
       position.z = targetPos.z;
     }
        
            // BONUS: Versuche kleinere Schritte wenn beides blockiert ist
    if (position.x == testPos.x && position.z == testPos.z) {
     // Versuche 50% der Bewegung
    glm::vec3 halfMovement = movement * 0.5f;
            testPos = position + halfMovement;
 if (!checkCollision(testPos)) {
 position = testPos;
                }
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

    // Prüfe Boden-Kollision mit mehreren Punkten für stabilere Detection
    int footBlockY = static_cast<int>(std::floor(newPos.y));
    int footBlockX = static_cast<int>(std::round(newPos.x));
    int footBlockZ = static_cast<int>(std::round(newPos.z));
    
    // Prüfe ob auf dem Boden
    if (velocity.y <= 0.0f) {
        // Prüfe mehrere Punkte unter dem Spieler für stabilere Kollision
  bool onGround = false;
   float groundY = newPos.y;
        
        // Prüfe Zentrum und 4 Punkte am Rand des Radius
     std::vector<glm::vec2> checkPoints = {
       glm::vec2(0.0f, 0.0f),       // Zentrum
          glm::vec2(radius * 0.7f, 0.0f),     // Rechts
       glm::vec2(-radius * 0.7f, 0.0f),    // Links
            glm::vec2(0.0f, radius * 0.7f),     // Vorne
     glm::vec2(0.0f, -radius * 0.7f)     // Hinten
        };
 
    for (const auto& offset : checkPoints) {
       int checkX = static_cast<int>(std::round(newPos.x + offset.x));
     int checkZ = static_cast<int>(std::round(newPos.z + offset.y));
     
        if (isBlockSolid(checkX, footBlockY, checkZ)) {
            onGround = true;
     groundY = std::max(groundY, static_cast<float>(footBlockY + 1));
  }
        }
  
        if (onGround) {
        // Auf Boden - aber erlaube kleine Stufen (max 0.5 Blöcke hoch)
            float stepHeight = 0.5f;
   if (groundY - position.y <= stepHeight) {
       position.y = groundY;
                velocity.y = 0.0f;
   isOnGround = true;
     isJumping = false;
     } else {
  // Zu hohe Stufe - normale Gravity
   position.y = newPos.y;
 isOnGround = false;
      }
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
