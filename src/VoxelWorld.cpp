#include "VoxelWorld.h"
#include <algorithm>
#include <iostream> // Für Debug-Ausgaben

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
	std::lock_guard<std::mutex> lock(chunkMutex); // Thread-Safety!

	glm::ivec3 coord(chunkX, chunkY, chunkZ);
	auto it = chunks.find(coord);
	if (it != chunks.end()) {
		return it->second.get();
	}
	return nullptr;
}

VoxelChunk* VoxelWorld::getOrCreateChunk(int chunkX, int chunkY, int chunkZ) {
	// HINWEIS: Wird nur von setBlock() aufgerufen, welches bereits den Lock hält
	// Daher KEIN Lock hier (würde zu Deadlock führen)

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
	std::lock_guard<std::mutex> lock(chunkMutex); // Thread-Safety!

	glm::ivec3 coord(chunkX, chunkY, chunkZ);
	chunks.erase(coord);
}

void VoxelWorld::markChunkDirty(int chunkX, int chunkY, int chunkZ) {
	// HINWEIS: Wird nur von setBlock() aufgerufen, welches bereits den Lock hält
	// Prüfe ob Chunk existiert
	glm::ivec3 coord(chunkX, chunkY, chunkZ);
	auto it = chunks.find(coord);
	if (it != chunks.end()) {
		dirtyChunks.insert(coord);
	}
}

void VoxelWorld::beginBatchUpdate() {
	batchMode = true;
	dirtyChunks.clear();
}

void VoxelWorld::endBatchUpdate() {
	batchMode = false;

	std::cout << "Batch-Update beendet. Aktualisiere " << dirtyChunks.size() << " dirty chunks..." << std::endl;

	// Kopiere dirty chunks während Lock gehalten wird
	std::set<glm::ivec3, Vec3Compare> dirtyChunksCopy;
	{
		std::lock_guard<std::mutex> lock(chunkMutex);
		dirtyChunksCopy = dirtyChunks;
		dirtyChunks.clear();
	}

	// Update dirty chunks (updateChunkMesh hat seinen eigenen Lock)
	for (const auto& chunkCoord : dirtyChunksCopy) {
		updateChunkMesh(chunkCoord.x, chunkCoord.y, chunkCoord.z);
	}

	// WICHTIG: Stelle sicher, dass ALLE Chunks ein Mesh haben
	// Falls Chunks existieren aber nicht als "dirty" markiert wurden
	int chunksWithoutMesh = 0;
	std::vector<glm::ivec3> chunksToUpdate;

	{
		std::lock_guard<std::mutex> lock(chunkMutex);
		for (auto& pair : chunks) {
			VoxelChunk* chunk = pair.second.get();
			if (!chunk->isEmpty() && chunk->getVertices().empty()) {
				// Chunk hat Blöcke aber kein Mesh - merken für Update!
				chunksToUpdate.push_back(pair.first);
				chunksWithoutMesh++;
			}
		}
	}

	// Update chunks ohne Mesh (außerhalb des Locks)
	for (const auto& coord : chunksToUpdate) {
		updateChunkMesh(coord.x, coord.y, coord.z);
	}

	if (chunksWithoutMesh > 0) {
		std::cout << "WARNUNG: " << chunksWithoutMesh << " Chunks hatten kein Mesh - wurden nachträglich generiert!" << std::endl;
	}

	std::cout << "Batch-Update abgeschlossen. Gesamt-Chunks: ";
	{
		std::lock_guard<std::mutex> lock(chunkMutex);
		std::cout << chunks.size() << std::endl;
	}
}

void VoxelWorld::updateDirtyChunks() {
	for (const auto& chunkCoord : dirtyChunks) {
		updateChunkMesh(chunkCoord.x, chunkCoord.y, chunkCoord.z);
	}
}

void VoxelWorld::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
	std::lock_guard<std::mutex> lock(chunkMutex); // WICHTIG: Thread-Safety!

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
			// Use internal version that doesn't lock (we already hold the lock)
			updateChunkMeshInternal(chunkCoord.x, chunkCoord.y, chunkCoord.z);

			// Update angrenzende Chunks bei Randblöcken
			if (localCoord.x == 0) {
				updateChunkMeshInternal(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z);
			}
			if (localCoord.x == VoxelChunk::CHUNK_SIZE - 1) {
				updateChunkMeshInternal(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z);
			}
			if (localCoord.y == 0) {
				updateChunkMeshInternal(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z);
			}
			if (localCoord.y == VoxelChunk::CHUNK_SIZE - 1) {
				updateChunkMeshInternal(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z);
			}
			if (localCoord.z == 0) {
				updateChunkMeshInternal(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1);
			}
			if (localCoord.z == VoxelChunk::CHUNK_SIZE - 1) {
				updateChunkMeshInternal(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1);
			}
		}
	}
}

BlockType VoxelWorld::getBlock(int worldX, int worldY, int worldZ) const {
	std::lock_guard<std::mutex> lock(chunkMutex); // WICHTIG: Thread-Safety!

	glm::ivec3 chunkCoord = worldToChunkCoord(worldX, worldY, worldZ);
	glm::ivec3 localCoord = worldToLocalCoord(worldX, worldY, worldZ);

	auto it = chunks.find(chunkCoord);
	if (it != chunks.end()) {
		return it->second->getBlock(localCoord.x, localCoord.y, localCoord.z);
	}

	return BlockType::Air;
}

void VoxelWorld::updateChunkMesh(int chunkX, int chunkY, int chunkZ) {
	std::lock_guard<std::mutex> lock(chunkMutex); // Thread-Safety!
	updateChunkMeshInternal(chunkX, chunkY, chunkZ);
}

void VoxelWorld::updateChunkMeshInternal(int chunkX, int chunkY, int chunkZ) {
	// Internal version - assumes caller holds chunkMutex lock!
	
	glm::ivec3 coord(chunkX, chunkY, chunkZ);
	auto it = chunks.find(coord);
	if (it == chunks.end()) {
		return;
	}
	
	VoxelChunk* chunk = it->second.get();
	if (!chunk) {
		return;
	}

	// Hole Nachbar-Chunks
	VoxelChunk* north = nullptr;
	VoxelChunk* south = nullptr;
	VoxelChunk* east = nullptr;
	VoxelChunk* west = nullptr;
	VoxelChunk* top = nullptr;
	VoxelChunk* bottom = nullptr;
	
	{
		glm::ivec3 northCoord(chunkX, chunkY, chunkZ + 1);
		auto northIt = chunks.find(northCoord);
		if (northIt != chunks.end()) north = northIt->second.get();
	}
	{
		glm::ivec3 southCoord(chunkX, chunkY, chunkZ - 1);
		auto southIt = chunks.find(southCoord);
		if (southIt != chunks.end()) south = southIt->second.get();
	}
	{
		glm::ivec3 eastCoord(chunkX + 1, chunkY, chunkZ);
		auto eastIt = chunks.find(eastCoord);
		if (eastIt != chunks.end()) east = eastIt->second.get();
	}
	{
		glm::ivec3 westCoord(chunkX - 1, chunkY, chunkZ);
		auto westIt = chunks.find(westCoord);
		if (westIt != chunks.end()) west = westIt->second.get();
	}
	{
		glm::ivec3 topCoord(chunkX, chunkY + 1, chunkZ);
		auto topIt = chunks.find(topCoord);
		if (topIt != chunks.end()) top = topIt->second.get();
	}
	{
		glm::ivec3 bottomCoord(chunkX, chunkY - 1, chunkZ);
		auto bottomIt = chunks.find(bottomCoord);
		if (bottomIt != chunks.end()) bottom = bottomIt->second.get();
	}

	// Generiere Mesh MIT Lock (CPU-Arbeit)
	chunk->generateMeshWithNeighbors(north, south, east, west, top, bottom);
	
	// setupOpenGL() auch MIT Lock!
	// Dies ist sicher, weil:
	// 1. OpenGL-Calls werden nur vom Haupt-Thread aufgerufen
	// 2. updateChunkMesh wird nur vom Haupt-Thread aufgerufen (nach Terrain-Gen)
	// 3. Andere Threads rufen nur getBlock() auf (read-only)
	chunk->setupOpenGL();
}

void VoxelWorld::updateAllChunks() {
	// Sammle alle Chunk-Koordinaten während Lock gehalten wird
	std::vector<glm::ivec3> chunkCoords;
	{
		std::lock_guard<std::mutex> lock(chunkMutex);
		chunkCoords.reserve(chunks.size());
		for (auto& pair : chunks) {
			chunkCoords.push_back(pair.first);
		}
	}

	// Jetzt ohne Lock die Meshes updaten
	// updateChunkMesh hat seinen eigenen Lock
	for (const auto& coord : chunkCoords) {
		updateChunkMesh(coord.x, coord.y, coord.z);
	}
}

void VoxelWorld::clear() {
	std::lock_guard<std::mutex> lock(chunkMutex); // Thread-Safety!

	chunks.clear();
	dirtyChunks.clear();
}

void VoxelWorld::render() const {
	std::lock_guard<std::mutex> lock(chunkMutex); // Thread-Safety!

	for (const auto& pair : chunks) {
		pair.second->render();
	}
}
