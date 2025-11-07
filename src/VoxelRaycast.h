#pragma once

#include <glm/glm.hpp>
#include "VoxelWorld.h"

struct RaycastHit {
    bool hit;
    glm::ivec3 blockPos;      // Position des getroffenen Blocks
    glm::ivec3 placePos;    // Position wo ein neuer Block platziert werden sollte
    glm::vec3 hitPoint;       // Genauer Trefferpunkt
    glm::vec3 normal;         // Normale der getroffenen Fläche
    float distance;    // Distanz zum Trefferpunkt
};

class VoxelRaycast {
public:
    // Führt einen Raycast durch die Voxel-Welt durch
    // origin: Startpunkt des Rays (z.B. Kamera-Position)
    // direction: Richtung des Rays (normalisiert)
    // maxDistance: Maximale Distanz des Raycasts
    // world: Die Voxel-Welt
    static RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, 
   float maxDistance, VoxelWorld* world);

private:
    // DDA (Digital Differential Analyzer) Algorithmus für Voxel-Raycasting
    static RaycastHit dda(const glm::vec3& origin, const glm::vec3& direction,
        float maxDistance, VoxelWorld* world);
};
