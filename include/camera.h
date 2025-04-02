#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <btBulletDynamicsCommon.h>
#include "FPSController.h"

// Default-Werte
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
    // Kamera-Attribute
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Euler-Winkel
    float Yaw;
    float Pitch;

    // Einstellungen
    float MouseSensitivity;
    float Zoom;

    // Konstruktor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Verknüpfung mit dem FPSController
    void AttachToController(const FPSController& controller)
    {
        // Rotation vom Controller übernehmen
        Yaw = controller.yaw;
        Pitch = controller.pitch;

        // Position vom Physik-Body holen
        btTransform trans;
        controller.body->getMotionState()->getWorldTransform(trans);
        btVector3 pos = trans.getOrigin();

        // Kamera etwas nach oben versetzen (Augenhöhe)
        Position = glm::vec3(pos.getX(), pos.getY() + 1.0f, pos.getZ());

        updateCameraVectors();
    }

    // Verarbeite Mausbewegung
    void ProcessMouseMovement(float xoffset, float yoffset)
    {
        Yaw += xoffset * MouseSensitivity;
        Pitch -= yoffset * MouseSensitivity;

        // Vertikalen Winkel begrenzen
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;

        updateCameraVectors();
    }

    // View-Matrix generieren
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Maus-Sensitivität ändern
    void SetMouseSensitivity(float sensitivity)
    {
        MouseSensitivity = sensitivity;
    }

private:
    // Vektoren neu berechnen
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
