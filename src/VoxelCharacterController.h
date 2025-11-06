#ifndef VOXEL_CHARACTER_CONTROLLER_H
#define VOXEL_CHARACTER_CONTROLLER_H

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class VoxelWorld;

class VoxelCharacterController {
public:
    VoxelCharacterController(VoxelWorld* world, GLFWwindow* window);
    ~VoxelCharacterController();

    void update(float deltaTime);
    void onMouseMove(double dx, double dy);
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
  
    // Movement
    void jump();

private:
    VoxelWorld* voxelWorld;
    GLFWwindow* window;
    
    // Position und Rotation
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    
    // Kamera-Rotation
    float yaw;
    float pitch;
    
    // Character-Eigenschaften
    float height;          // Höhe des Charakters
    float radius;   // Radius für Kollision
    float moveSpeed;
    float mouseSensitivity;
    float jumpForce;
    float gravity;
    
    // Zustand
    bool isOnGround;
    bool isJumping;
    
    // Hilfsfunktionen
    void processKeyboard(float deltaTime);
    void updateVectors();
    bool checkCollision(const glm::vec3& newPos);
    bool isBlockSolid(int x, int y, int z);
    void resolveCollision(glm::vec3& newPos);
    glm::vec3 getGroundPosition();
};

#endif // VOXEL_CHARACTER_CONTROLLER_H
