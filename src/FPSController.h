#pragma once
#include <btBulletDynamicsCommon.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class FPSController {
public:
    FPSController(btDynamicsWorld* world, GLFWwindow* window);
    FPSController() {};
    void update(float deltaTime);
    void onMouseMove(double dx, double dy);
    void jump();

    float yaw, pitch;
    btRigidBody* body;
private:
    btDynamicsWorld* world;
    GLFWwindow* window;

    float moveSpeed = 5.0f;
    float jumpStrength = 10.0f;

    glm::vec3 forward, right, up;
    bool isGrounded();

    void updateVectors();
};
