#pragma once

#include <glm/glm.hpp>
#include "VoxelChunk.h"
#include <map>
#include <memory>
#include <set>
#include <mutex> // NEU: Für Thread-Safety

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

	void removeChunk(int chunkX, int chunkY, int chunkZ);

	// Block-Operationen (Weltkoordinaten)
	void setBlock(int worldX, int worldY, int worldZ, BlockType type);
	BlockType getBlock(int worldX, int worldY, int worldZ) const;

	// NEUE BATCH-MODI
	void beginBatchUpdate();  // Deaktiviert Auto-Mesh-Updates
	void endBatchUpdate(); // Aktualisiert nur betroffene Chunks

	// Mesh-Aktualisierung
	void updateChunkMesh(int chunkX, int chunkY, int chunkZ);
	void updateAllChunks();
	void updateDirtyChunks();  // NEU: Nur dirty chunks updaten

	// Rendering
	void render() const;

	// Utility
	void clear();

	glm::ivec3 worldToChunkCoord(int worldX, int worldY, int worldZ) const;
	glm::ivec3 worldToLocalCoord(int worldX, int worldY, int worldZ) const;

	std::map<glm::ivec3, std::unique_ptr<VoxelChunk>, Vec3Compare> chunks;

private:
	VoxelChunk* getChunk(int chunkX, int chunkY, int chunkZ);
	VoxelChunk* getOrCreateChunk(int chunkX, int chunkY, int chunkZ);

	// Internal unlocked versions (assumes caller holds lock)
	void updateChunkMeshInternal(int chunkX, int chunkY, int chunkZ);

	// NEU: Tracking für Batch-Modus
	bool batchMode = false;
	std::set<glm::ivec3, Vec3Compare> dirtyChunks;

	void markChunkDirty(int chunkX, int chunkY, int chunkZ);
	
	// NEU: Mutex für Thread-Safety beim Zugriff auf Chunks
	mutable std::mutex chunkMutex;
};
