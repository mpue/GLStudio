#include "VoxelWorld.h"
#include <algorithm>

VoxelWorld::VoxelWorld()
{
}

VoxelWorld::~VoxelWorld() {
	clear();
}

glm::ivec3 VoxelWorld::worldToChunkCoord(int worldX, int worldY, int worldZ) const {
	auto floorDiv = [](int a, int b) {
		return (a < 0) ? ((a - b + 1) / b) : (a / b);
		};

	return glm::ivec3(
		floorDiv(worldX, VoxelChunk::CHUNK_SIZE),
		floorDiv(worldY, VoxelChunk::CHUNK_SIZE),
		floorDiv(worldZ, VoxelChunk::CHUNK_SIZE)
	);
}

glm::ivec3 VoxelWorld::worldToLocalCoord(int worldX, int worldY, int worldZ) const {
	auto mod = [](int a, int b) {
		int r = a % b;
		return (r < 0) ? r + b : r;
		};

	return glm::ivec3(
		mod(worldX, VoxelChunk::CHUNK_SIZE),
		mod(worldY, VoxelChunk::CHUNK_SIZE),
		mod(worldZ, VoxelChunk::CHUNK_SIZE)
	);
}

VoxelChunk* VoxelWorld::getChunk(int chunkX, int chunkY, int chunkZ) {
	glm::ivec3 coord(chunkX, chunkY, chunkZ);
	auto it = chunks.find(coord);
	if (it != chunks.end()) {
		return it->second.get();
	}
	return nullptr;
}

VoxelChunk* VoxelWorld::getOrCreateChunk(int chunkX, int chunkY, int chunkZ) {
	glm::ivec3 coord(chunkX, chunkY, chunkZ);
	auto it = chunks.find(coord);
	if (it != chunks.end()) {
		return it->second.get();
	}

	// Erstelle neuen Chunk
	auto chunk = std::make_unique<VoxelChunk>(chunkX, chunkY, chunkZ);
	VoxelChunk* ptr = chunk.get();
	chunks[coord] = std::move(chunk);
	return ptr;
}

void VoxelWorld::removeChunk(int chunkX, int chunkY, int chunkZ) {
	glm::ivec3 coord(chunkX, chunkY, chunkZ);
	chunks.erase(coord);
}

void VoxelWorld::markChunkDirty(int chunkX, int chunkY, int chunkZ) {
	// Prüfe ob Chunk existiert
	if (getChunk(chunkX, chunkY, chunkZ)) {
		dirtyChunks.insert(glm::ivec3(chunkX, chunkY, chunkZ));
	}
}

void VoxelWorld::beginBatchUpdate() {
	batchMode = true;
	dirtyChunks.clear();
}

void VoxelWorld::endBatchUpdate() {
	batchMode = false;
	updateDirtyChunks();
	dirtyChunks.clear();
}

void VoxelWorld::updateDirtyChunks() {
	for (const auto& chunkCoord : dirtyChunks) {
		updateChunkMesh(chunkCoord.x, chunkCoord.y, chunkCoord.z);
	}
}

void VoxelWorld::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
	glm::ivec3 chunkCoord = worldToChunkCoord(worldX, worldY, worldZ);
	glm::ivec3 localCoord = worldToLocalCoord(worldX, worldY, worldZ);

	VoxelChunk* chunk = getOrCreateChunk(chunkCoord.x, chunkCoord.y, chunkCoord.z);
	if (chunk) {
		chunk->setBlock(localCoord.x, localCoord.y, localCoord.z, type);

		if (batchMode) {
			// Im Batch-Modus: Nur markieren, nicht sofort updaten
			markChunkDirty(chunkCoord.x, chunkCoord.y, chunkCoord.z);

			// Markiere auch angrenzende Chunks bei Randblöcken
			if (localCoord.x == 0) {
				markChunkDirty(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z);
			}
			if (localCoord.x == VoxelChunk::CHUNK_SIZE - 1) {
				markChunkDirty(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z);
			}
			if (localCoord.y == 0) {
				markChunkDirty(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z);
			}
			if (localCoord.y == VoxelChunk::CHUNK_SIZE - 1) {
				markChunkDirty(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z);
			}
			if (localCoord.z == 0) {
				markChunkDirty(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1);
			}
			if (localCoord.z == VoxelChunk::CHUNK_SIZE - 1) {
				markChunkDirty(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1);
			}
		}
		else {
			// Normaler Modus: Sofort updaten (wie vorher)
			updateChunkMesh(chunkCoord.x, chunkCoord.y, chunkCoord.z);

			// Update angrenzende Chunks bei Randblöcken
			if (localCoord.x == 0) {
				updateChunkMesh(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z);
			}
			if (localCoord.x == VoxelChunk::CHUNK_SIZE - 1) {
				updateChunkMesh(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z);
			}
			if (localCoord.y == 0) {
				updateChunkMesh(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z);
			}
			if (localCoord.y == VoxelChunk::CHUNK_SIZE - 1) {
				updateChunkMesh(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z);
			}
			if (localCoord.z == 0) {
				updateChunkMesh(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1);
			}
			if (localCoord.z == VoxelChunk::CHUNK_SIZE - 1) {
				updateChunkMesh(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1);
			}
		}
	}
}

BlockType VoxelWorld::getBlock(int worldX, int worldY, int worldZ) const {
	glm::ivec3 chunkCoord = worldToChunkCoord(worldX, worldY, worldZ);
	glm::ivec3 localCoord = worldToLocalCoord(worldX, worldY, worldZ);

	auto it = chunks.find(chunkCoord);
	if (it != chunks.end()) {
		return it->second->getBlock(localCoord.x, localCoord.y, localCoord.z);
	}

	return BlockType::Air;
}

void VoxelWorld::updateChunkMesh(int chunkX, int chunkY, int chunkZ) {
	VoxelChunk* chunk = getChunk(chunkX, chunkY, chunkZ);
	if (!chunk) {
		return;
	}

	// Hole Nachbar-Chunks
	VoxelChunk* north = getChunk(chunkX, chunkY, chunkZ + 1);
	VoxelChunk* south = getChunk(chunkX, chunkY, chunkZ - 1);
	VoxelChunk* east = getChunk(chunkX + 1, chunkY, chunkZ);
	VoxelChunk* west = getChunk(chunkX - 1, chunkY, chunkZ);
	VoxelChunk* top = getChunk(chunkX, chunkY + 1, chunkZ);
	VoxelChunk* bottom = getChunk(chunkX, chunkY - 1, chunkZ);

	// Generiere Mesh mit Nachbarn
	chunk->generateMeshWithNeighbors(north, south, east, west, top, bottom);
	chunk->setupOpenGL();
}

void VoxelWorld::updateAllChunks() {
	for (auto& pair : chunks) {
		glm::ivec3 coord = pair.first;
		updateChunkMesh(coord.x, coord.y, coord.z);
	}
}

void VoxelWorld::render() const {
	for (const auto& pair : chunks) {
		pair.second->render();
	}
}

void VoxelWorld::clear() {
	chunks.clear();
	dirtyChunks.clear();
}
