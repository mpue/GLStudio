#include "FPSController.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

FPSController::FPSController(btDynamicsWorld* world, GLFWwindow* window)
    : world(world), window(window), yaw(0.0f), pitch(0.0f) {

    btCollisionShape* shape = new btCapsuleShape(0.5f, 1.0f);
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0, 2, 0));

    btScalar mass = 75.0f;
    btVector3 inertia(0, 0, 0);
    shape->calculateLocalInertia(mass, inertia);

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, inertia);
    body = new btRigidBody(rbInfo);
    body->setAngularFactor(btVector3(0, 1, 0));

    world->addRigidBody(body);
    updateVectors();
}

void FPSController::update(float dt) {
    glm::vec3 move(0);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += right;

    move.y = 0;
    if (glm::length(move) > 0) move = glm::normalize(move);

    btVector3 vel = body->getLinearVelocity();
    vel.setX(move.x * moveSpeed);
    vel.setZ(move.z * moveSpeed);
    body->setLinearVelocity(vel);

    // Jump
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded()) {
        jump();
    }
}

void FPSController::onMouseMove(double dx, double dy) {
    yaw += dx * 0.1f;
    pitch -= dy * 0.1f;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);
    updateVectors();
}

void FPSController::updateVectors() {
    forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    forward.y = sin(glm::radians(pitch));
    forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    forward = glm::normalize(forward);

    right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    up = glm::normalize(glm::cross(right, forward));
}

void FPSController::jump() {
    btVector3 vel = body->getLinearVelocity();
    vel.setY(jumpStrength);
    body->setLinearVelocity(vel);
}

bool FPSController::isGrounded() {
    btTransform trans;
    body->getMotionState()->getWorldTransform(trans);
    btVector3 start = trans.getOrigin();
    btVector3 end = start - btVector3(0, 1.1f, 0);

    btCollisionWorld::ClosestRayResultCallback ray(start, end);
    world->rayTest(start, end, ray);
    return ray.hasHit();
}
