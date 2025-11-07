#include "TerrainGenerator.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>

TerrainGenerator::TerrainGenerator() : processedColumns(0), shouldStop(false), currentSeed(12345) {
}

TerrainGenerator::~TerrainGenerator() {
    shouldStop = true;
}

int TerrainGenerator::calculateTotalBlocks(const TerrainConfig& config) const {
    int totalColumns = config.sizeX * config.sizeZ;
    int avgHeight = static_cast<int>(config.heightMultiplier / 2.0f);
    return totalColumns * (avgHeight + std::abs(config.minHeight));
}

float TerrainGenerator::getTerrainHeight(int x, int z, const TerrainConfig& config, Perlin& localPerlin) const {
    // Basis-Höhe mit Multi-Oktaven für Details
    float baseHeight = localPerlin.octaveNoise3D(
        x * config.scale, 
        0.0f, 
        z * config.scale, 
        config.octaves, 
 config.persistence, 
        config.lacunarity
    );
    
  // Kontinentalität (große Landmassen)
    float continentalness = localPerlin.octaveNoise3D(
x * config.continentalnessScale, 
        1000.0f,  // Unterschiedlicher Offset für Varianz
        z * config.continentalnessScale, 
        2, 
   0.5f, 
        2.0f
    );
    
    // Erosion (Rauheit des Terrains)
    float erosion = localPerlin.octaveNoise3D(
        x * config.erosionScale, 
        2000.0f, 
   z * config.erosionScale, 
        3, 
        0.6f, 
    2.0f
    );
    
    // Berg-Faktor
    float mountainFactor = localPerlin.octaveNoise3D(
        x * config.mountainScale, 
3000.0f, 
      z * config.mountainScale, 
        2, 
        0.5f, 
        2.0f
    );
    
    // Kombiniere alles für finale Höhe
    float height = baseHeight;
    
    // Kontinentalität beeinflusst Basis-Höhe
    height = height * 0.6f + continentalness * 0.4f;
  
    // Erosion macht das Terrain rauer
    height += (erosion - 0.5f) * 0.3f;
    
    // Berge: Wenn mountainFactor über Schwellwert, erhöhe Höhe stark
    if (mountainFactor > config.mountainThreshold) {
        float mountainIntensity = (mountainFactor - config.mountainThreshold) / (1.0f - config.mountainThreshold);
 // Quadratische Kurve für dramatischere Berge
        mountainIntensity = mountainIntensity * mountainIntensity;
  height += mountainIntensity * config.mountainHeightMultiplier;
    }
    
    return height * config.heightMultiplier;
}

BlockType TerrainGenerator::getBlockTypeAtHeight(int y, int maxY, float caveValue, bool isMountain, bool isBeach) const {
    if (caveValue > 0.6f) {
   return BlockType::Air;
    }
    
 // Strand-Logik
 if (isBeach && y >= maxY - 2 && y <= maxY) {
      return BlockType::Dirt; // Könnte Sand sein, wenn du einen Sand-Block-Typ hast
    }
    
 // Berg-Logik (mehr Stein an der Oberfläche)
    if (isMountain && y >= maxY - 2 && y <= maxY) {
   return BlockType::Stone;
    }
    
    if (y == maxY && maxY > 0) {
    return BlockType::Grass;
    }
    else if (y > maxY - 4 && y < maxY) {
        return BlockType::Dirt;
    }
 else {
        return BlockType::Stone;
    }
}

float TerrainGenerator::getCaveNoise(int x, int y, int z, const TerrainConfig& config, Perlin& localPerlin) const {
    // Verwende 3D Noise für organische Höhlen
    return localPerlin.octaveNoise3D(
        x * config.caveScale, 
        y * config.caveScale * 1.5f,  // Höhlen in Y-Richtung etwas strecken
        z * config.caveScale, 
        2,  // Weniger Oktaven für Höhlen
    0.5f, 
        2.0f
    );
}

void TerrainGenerator::generateTerrainWorker(
    const TerrainConfig& config,
    int startX, int endX,
    std::vector<BlockData>& blockBuffer,
    std::mutex& bufferMutex
) {
    Perlin localPerlin(config.seed);
    
 int halfSizeZ = config.sizeZ / 2;
    int estimatedBlocks = (endX - startX) * config.sizeZ * 
        (static_cast<int>(config.heightMultiplier / 2.0f) + std::abs(config.minHeight));
    
    // OPTIMIERUNG 1: Richtige Reserve-Größe
    std::vector<BlockData> localBuffer;
    localBuffer.reserve(estimatedBlocks);
    
  for (int x = startX; x < endX; x++) {
     if (shouldStop) break;
    
        for (int z = -halfSizeZ; z < halfSizeZ; z++) {
            // Berechne Höhe mit erweitertem System
    float height = getTerrainHeight(x, z, config, localPerlin);
int maxY = static_cast<int>(height);
       
            // Berg-Detektion
            float mountainFactor = localPerlin.octaveNoise3D(
            x * config.mountainScale, 
              3000.0f, 
     z * config.mountainScale, 
       2, 
    0.5f, 
         2.0f
            );
            bool isMountain = mountainFactor > config.mountainThreshold;
            
      // Strand-Detektion
   bool isBeach = config.generateBeaches && maxY >= config.waterLevel - 2 && maxY <= config.waterLevel + 2;
         
          for (int y = config.minHeight; y <= maxY; y++) {
         float caveValue = 0.0f;
     
       // Höhlen nur unterhalb einer bestimmten Tiefe
 if (config.generateCaves && y < maxY - config.caveMinDepth) {
  caveValue = getCaveNoise(x, y, z, config, localPerlin);
  
           // Höhlen-Schwellwert anwenden
       if (caveValue < config.caveThreshold) {
    caveValue = 0.0f; // Kein Loch
           }
     }
  
 BlockType blockType = getBlockTypeAtHeight(y, maxY, caveValue, isMountain, isBeach);
  
           if (blockType != BlockType::Air) {
          localBuffer.emplace_back(x, y, z, blockType);
     }
            }
    }
        
    processedColumns++;
    }
  
    // OPTIMIERUNG 2: Nur ein Mutex-Lock pro Thread (am Ende)
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
      blockBuffer.insert(blockBuffer.end(), 
    std::make_move_iterator(localBuffer.begin()),
        std::make_move_iterator(localBuffer.end()));
    }
}

void TerrainGenerator::generateTerrainParallel(VoxelWorld* world, const TerrainConfig& config,
    ProgressCallback callback) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    processedColumns = 0;
    shouldStop = false;
currentSeed = config.seed;
    
    // Initialisiere Perlin mit neuem Seed
    perlin.initializePermutation(config.seed);
    
    int totalColumns = config.sizeX * config.sizeZ;
    int halfSizeX = config.sizeX / 2;
    
    // OPTIMIERUNG 3: Begrenze Thread-Anzahl auf sinnvolle Werte
    int numThreads = config.numThreads;
    if (numThreads <= 0) {
   numThreads = std::thread::hardware_concurrency();
      if (numThreads == 0) numThreads = 4;
    }
    
    // Begrenze auf maximal Anzahl der Spalten oder Hardware-Threads * 2
    int maxUsefulThreads = std::min(config.sizeX, static_cast<int>(std::thread::hardware_concurrency() * 2));
    if (numThreads > maxUsefulThreads) {
   std::cout << "WARNUNG: " << numThreads << " Threads sind zu viel. Reduziere auf " << maxUsefulThreads << std::endl;
        numThreads = maxUsefulThreads;
    }
    
    std::cout << "Starte erweiterte Terrain-Generierung mit " << numThreads << " Threads..." << std::endl;
    std::cout << "Seed: " << config.seed << std::endl;
    std::cout << "Oktaven: " << config.octaves << ", Persistence: " << config.persistence 
        << ", Lacunarity: " << config.lacunarity << std::endl;
    std::cout << "Terrain-Größe: " << config.sizeX << "×" << config.sizeZ 
        << " (" << totalColumns << " Spalten)" << std::endl;
    
    // OPTIMIERUNG 4: Pre-allocate mit korrekter Größe
    std::vector<BlockData> blockBuffer;
    int estimatedBlocks = calculateTotalBlocks(config);
    blockBuffer.reserve(estimatedBlocks);
    std::cout << "Reserviere Speicher für ~" << estimatedBlocks << " Blöcke..." << std::endl;
    
    std::mutex bufferMutex;
    
    // OPTIMIERUNG 5: Bessere Work-Distribution
    std::vector<std::thread> threads;
  int columnsPerThread = config.sizeX / numThreads;
    
    // Stelle sicher, dass jeder Thread mindestens 4 Spalten bekommt
    if (columnsPerThread < 4) {
    std::cout << "WARNUNG: Zu viele Threads für diese Terrain-Größe!" << std::endl;
        numThreads = config.sizeX / 4;
        if (numThreads < 1) numThreads = 1;
    columnsPerThread = config.sizeX / numThreads;
        std::cout << "Reduziere auf " << numThreads << " Threads" << std::endl;
    }
    
    std::cout << "Spalten pro Thread: " << columnsPerThread << std::endl;
    
    for (int i = 0; i < numThreads; i++) {
        int startX = -halfSizeX + (i * columnsPerThread);
      int endX = (i == numThreads - 1) ? halfSizeX : startX + columnsPerThread;
      
        threads.emplace_back(&TerrainGenerator::generateTerrainWorker, this,
       std::ref(config), startX, endX, std::ref(blockBuffer), std::ref(bufferMutex));
 }
    
    // Progress-Monitoring (reduzierte Frequenz für weniger Overhead)
    std::thread progressThread([this, callback, totalColumns]() {
        int lastReported = 0;
        while (processedColumns < totalColumns && !shouldStop) {
  int current = processedColumns.load();
      if (callback && (current - lastReported) >= 100) {  // Nur alle 100 Spalten updaten
   float progress = static_cast<float>(current) / totalColumns;
      callback(progress * 0.9f, "Generiere erweitertes Terrain...");
   lastReported = current;
 }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Warte auf alle Worker-Threads
    for (auto& thread : threads) {
if (thread.joinable()) {
    thread.join();
        }
    }
  
    shouldStop = true;
    if (progressThread.joinable()) {
        progressThread.join();
    }
    
  auto genTime = std::chrono::high_resolution_clock::now();
    auto genDuration = std::chrono::duration_cast<std::chrono::milliseconds>(genTime - startTime);
  
    std::cout << "\n=== Block-Generierung Abgeschlossen ===" << std::endl;
    std::cout << "Zeit: " << genDuration.count() << "ms" << std::endl;
    std::cout << "Generierte Blöcke: " << blockBuffer.size() << " / " << estimatedBlocks << " geschätzt" << std::endl;
    std::cout << "Blöcke/Sekunde: " << (blockBuffer.size() * 1000.0f / genDuration.count()) << std::endl;
 
    // OPTIMIERUNG 7: Aktiviere Batch-Modus für VoxelWorld
    if (callback) {
callback(0.9f, "Setze Blöcke (Batch)...");
    }
  
    std::cout << "\n=== Block-Platzierung (Batch-Modus) ===" << std::endl;
    
  // Aktiviere Batch-Modus: Keine sofortigen Mesh-Updates!
    world->beginBatchUpdate();
    
    size_t blockCount = blockBuffer.size();
    size_t blocksPerUpdate = std::max<size_t>(blockCount / 100, 1000);
  
    for (size_t i = 0; i < blockCount; i++) {
   const auto& block = blockBuffer[i];
        world->setBlock(block.x, block.y, block.z, block.type);
  
      if (callback && (i % blocksPerUpdate == 0)) {
          float progress = 0.9f + (0.05f * static_cast<float>(i) / blockCount);
   callback(progress, "Setze Blöcke...");
    }
    }
    
    auto setTime = std::chrono::high_resolution_clock::now();
    auto setDuration = std::chrono::duration_cast<std::chrono::milliseconds>(setTime - genTime);
  
    std::cout << "Zeit: " << setDuration.count() << "ms" << std::endl;
    std::cout << "Blöcke/Sekunde: " << (blockCount * 1000.0f / setDuration.count()) << std::endl;
    
  // OPTIMIERUNG 8: Beende Batch-Modus - aktualisiert nur betroffene Chunks!
    if (callback) {
		callback(0.95f, "Aktualisiere Chunks...");
	}
	
	std::cout << "\n=== Chunk-Update (Alle Chunks) ===" << std::endl;
	
	// WICHTIG: Verwende updateAllChunks() statt endBatchUpdate()
	// um sicherzustellen, dass ALLE Chunks ein Mesh bekommen
	world->endBatchUpdate();
	
	// Prüfe ob alle Chunks ein Mesh haben und generiere fehlende
	std::cout << "Überprüfe alle Chunks auf fehlende Meshes..." << std::endl;
	int chunksWithoutMesh = 0;
	int totalChunks = 0;
	
	for (auto& pair : world->chunks) {
		totalChunks++;
		VoxelChunk* chunk = pair.second.get();
		if (!chunk->isEmpty() && chunk->getVertices().empty()) {
			// Chunk hat Blöcke aber kein Mesh - generiere es!
			glm::ivec3 coord = pair.first;
			world->updateChunkMesh(coord.x, coord.y, coord.z);
			chunksWithoutMesh++;
		}
	}
	
	std::cout << "Gesamt-Chunks: " << totalChunks << std::endl;
	std::cout << "Chunks ohne Mesh (nachträglich generiert): " << chunksWithoutMesh << std::endl;
	
	auto endTime = std::chrono::high_resolution_clock::now();
	auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
	auto chunkDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - setTime);
 
    std::cout << "Zeit: " << chunkDuration.count() << "ms" << std::endl;

    std::cout << "\n=== GESAMT-STATISTIK ===" << std::endl;
    std::cout << "Block-Generierung: " << genDuration.count() << "ms (" 
              << (100.0f * genDuration.count() / totalDuration.count()) << "%)" << std::endl;
    std::cout << "Block-Platzierung: " << setDuration.count() << "ms (" 
     << (100.0f * setDuration.count() / totalDuration.count()) << "%)" << std::endl;
    std::cout << "Chunk-Update: " << chunkDuration.count() << "ms (" 
   << (100.0f * chunkDuration.count() / totalDuration.count()) << "%)" << std::endl;
    std::cout << "GESAMT: " << totalDuration.count() << "ms" << std::endl;
 
    if (callback) {
  callback(1.0f, "Fertig!");
    }
}
