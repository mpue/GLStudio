#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include "VoxelChunk.h"
#include <map>
#include <memory>
#include <glm/glm.hpp>

// Vergleichsfunktor fÅr glm::ivec3
struct Vec3Compare {
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
        if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

class VoxelWorld {
public:
    VoxelWorld();
    ~VoxelWorld();

    // Chunk-Verwaltung
    VoxelChunk* getChunk(int chunkX, int chunkY, int chunkZ);
    VoxelChunk* getOrCreateChunk(int chunkX, int chunkY, int chunkZ);
    void removeChunk(int chunkX, int chunkY, int chunkZ);
    
    // Block-Operationen (Weltkoordinaten)
    void setBlock(int worldX, int worldY, int worldZ, BlockType type);
    BlockType getBlock(int worldX, int worldY, int worldZ) const;
    
    // Mesh-Aktualisierung
    void updateChunkMesh(int chunkX, int chunkY, int chunkZ);
    void updateAllChunks();

    // Rendering
    void render() const;
    
    // Utility
  void clear();
    glm::ivec3 worldToChunkCoord(int worldX, int worldY, int worldZ) const;
    glm::ivec3 worldToLocalCoord(int worldX, int worldY, int worldZ) const;

private:
    std::map<glm::ivec3, std::unique_ptr<VoxelChunk>, Vec3Compare> chunks;
};

#endif // VOXEL_WORLD_H
