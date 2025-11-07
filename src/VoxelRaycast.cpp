#include "VoxelRaycast.h"
#include <cmath>
#include <algorithm>

RaycastHit VoxelRaycast::raycast(const glm::vec3& origin, const glm::vec3& direction, 
    float maxDistance, VoxelWorld* world) {
    return dda(origin, direction, maxDistance, world);
}

RaycastHit VoxelRaycast::dda(const glm::vec3& origin, const glm::vec3& direction,
    float maxDistance, VoxelWorld* world) {
    
    RaycastHit hit;
    hit.hit = false;
    hit.distance = maxDistance;

    // Normalisiere die Richtung
    glm::vec3 dir = glm::normalize(direction);

  // Startposition (aktueller Voxel)
    glm::ivec3 voxel(
        static_cast<int>(std::floor(origin.x)),
 static_cast<int>(std::floor(origin.y)),
        static_cast<int>(std::floor(origin.z))
    );

    // Schritt-Richtung (+1 oder -1 für jede Achse)
    glm::ivec3 step(
        dir.x >= 0 ? 1 : -1,
    dir.y >= 0 ? 1 : -1,
        dir.z >= 0 ? 1 : -1
    );

    // tMax: Distanz zum nächsten Voxel-Übergang auf jeder Achse
    glm::vec3 tMax;
    
    // tDelta: Distanz zwischen Voxel-Übergängen auf jeder Achse
    glm::vec3 tDelta;

    // Berechne tMax und tDelta für jede Achse
    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < 0.0001f) {
            // Ray ist parallel zu dieser Achse
     tMax[i] = std::numeric_limits<float>::max();
  tDelta[i] = std::numeric_limits<float>::max();
        } else {
            // Berechne die Distanz zum nächsten Voxel-Übergang
     float nextBoundary;
        if (step[i] > 0) {
    nextBoundary = std::floor(origin[i]) + 1.0f;
     } else {
           nextBoundary = std::floor(origin[i]);
            }
          
   tMax[i] = (nextBoundary - origin[i]) / dir[i];
      tDelta[i] = static_cast<float>(step[i]) / dir[i];
        }
    }

    // Speichere die vorherige Position für "place position"
    glm::ivec3 prevVoxel = voxel;
    glm::vec3 normal(0.0f);

    // DDA-Schleife
    float currentDistance = 0.0f;
    int maxSteps = static_cast<int>(maxDistance * 2.0f); // Sicherheits-Limit

    for (int i = 0; i < maxSteps && currentDistance < maxDistance; ++i) {
   // Prüfe ob aktueller Voxel solid ist
  BlockType blockType = world->getBlock(voxel.x, voxel.y, voxel.z);
  
        if (blockType != BlockType::Air) {
            // Treffer!
hit.hit = true;
            hit.blockPos = voxel;
         hit.placePos = prevVoxel;
   hit.distance = currentDistance;
     hit.normal = normal;
 
            // Berechne genauen Trefferpunkt
          hit.hitPoint = origin + dir * currentDistance;
            
     return hit;
        }

 // Speichere aktuelle Position als "prevVoxel"
 prevVoxel = voxel;

 // Bewege zum nächsten Voxel
        // Finde die Achse mit der kleinsten tMax
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
        // X-Achse
              voxel.x += step.x;
        currentDistance = tMax.x;
          tMax.x += tDelta.x;
      normal = glm::vec3(-step.x, 0, 0);
            } else {
            // Z-Achse
     voxel.z += step.z;
       currentDistance = tMax.z;
            tMax.z += tDelta.z;
        normal = glm::vec3(0, 0, -step.z);
        }
        } else {
  if (tMax.y < tMax.z) {
       // Y-Achse
        voxel.y += step.y;
      currentDistance = tMax.y;
      tMax.y += tDelta.y;
       normal = glm::vec3(0, -step.y, 0);
  } else {
          // Z-Achse
          voxel.z += step.z;
       currentDistance = tMax.z;
        tMax.z += tDelta.z;
    normal = glm::vec3(0, 0, -step.z);
    }
        }
    }

    // Kein Treffer
  return hit;
}
