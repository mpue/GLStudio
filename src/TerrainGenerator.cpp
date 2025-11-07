#include "TerrainGenerator.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>

TerrainGenerator::TerrainGenerator() : processedColumns(0), shouldStop(false) {
}

TerrainGenerator::~TerrainGenerator() {
    shouldStop = true;
}

int TerrainGenerator::calculateTotalBlocks(const TerrainConfig& config) const {
    int totalColumns = config.sizeX * config.sizeZ;
    int avgHeight = static_cast<int>(config.heightMultiplier / 2.0f);
    return totalColumns * (avgHeight + std::abs(config.minHeight));
}

BlockType TerrainGenerator::getBlockTypeAtHeight(int y, int maxY, float caveValue) const {
    if (caveValue > 0.6f) {
   return BlockType::Air;
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

float TerrainGenerator::getCaveNoise(int x, int y, int z, float scale) const {
    Perlin& p = const_cast<Perlin&>(perlin);
    return p.noise3D(x * scale * 0.5f, y * scale, z * scale * 0.5f);
}

void TerrainGenerator::generateTerrainWorker(
    const TerrainConfig& config,
    int startX, int endX,
    std::vector<BlockData>& blockBuffer,
    std::mutex& bufferMutex
) {
    Perlin localPerlin;
    
 int halfSizeZ = config.sizeZ / 2;
    int estimatedBlocks = (endX - startX) * config.sizeZ * 
        (static_cast<int>(config.heightMultiplier / 2.0f) + std::abs(config.minHeight));
    
    // OPTIMIERUNG 1: Richtige Reserve-Größe
    std::vector<BlockData> localBuffer;
    localBuffer.reserve(estimatedBlocks);
    
  for (int x = startX; x < endX; x++) {
     if (shouldStop) break;
    
        for (int z = -halfSizeZ; z < halfSizeZ; z++) {
        float height = localPerlin.noise3D(x * config.scale, 0.0f, z * config.scale);
            height *= config.heightMultiplier;
int maxY = static_cast<int>(height);
       
          for (int y = config.minHeight; y <= maxY; y++) {
                float caveValue = 0.0f;
     if (config.generateCaves && y < maxY - 3) {
       caveValue = localPerlin.noise3D(
         x * config.scale * 0.5f, 
        y * config.scale, 
z * config.scale * 0.5f
     );
     }
  
           BlockType blockType = getBlockTypeAtHeight(y, maxY, caveValue);
  
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
    
    std::cout << "Starte Terrain-Generierung mit " << numThreads << " Threads..." << std::endl;
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
    
    auto launchStart = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; i++) {
        int startX = -halfSizeX + (i * columnsPerThread);
      int endX = (i == numThreads - 1) ? halfSizeX : startX + columnsPerThread;
      
        threads.emplace_back(&TerrainGenerator::generateTerrainWorker, this,
       std::ref(config), startX, endX, std::ref(blockBuffer), std::ref(bufferMutex));
    }
 
    auto launchEnd = std::chrono::high_resolution_clock::now();
    auto launchDuration = std::chrono::duration_cast<std::chrono::microseconds>(launchEnd - launchStart);
    std::cout << "Thread-Launch: " << launchDuration.count() / 1000.0f << "ms" << std::endl;
    
    // Progress-Monitoring (reduzierte Frequenz für weniger Overhead)
    std::thread progressThread([this, callback, totalColumns]() {
        int lastReported = 0;
        while (processedColumns < totalColumns && !shouldStop) {
            int current = processedColumns.load();
          if (callback && (current - lastReported) >= 100) {  // Nur alle 100 Spalten updaten
   float progress = static_cast<float>(current) / totalColumns;
      callback(progress * 0.9f, "Generiere Terrain (Parallel)...");
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
 
    std::cout << "\n=== Chunk-Update (Nur Dirty-Chunks) ===" << std::endl;
  
    // endBatchUpdate() updatet nur die tatsächlich betroffenen Chunks
    world->endBatchUpdate();
    
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
    
    float theoreticalSpeedup = static_cast<float>(numThreads);
    float actualSpeedup = 1.0f;  // Wir haben keinen Single-Thread-Vergleich hier
    std::cout << "Threads verwendet: " << numThreads << std::endl;
    std::cout << "Effizienz: " << (actualSpeedup / theoreticalSpeedup * 100.0f) << "%" << std::endl;
 
    if (callback) {
  callback(1.0f, "Fertig!");
    }
}
