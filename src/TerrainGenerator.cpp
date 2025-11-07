#include "TerrainGenerator.h"
#include <iostream>
#include <chrono>
#include <cmath>

TerrainGenerator::TerrainGenerator() {
}

TerrainGenerator::~TerrainGenerator() {
}

int TerrainGenerator::calculateTotalBlocks(const TerrainConfig& config) const {
int totalColumns = config.sizeX * config.sizeZ;
 int avgHeight = static_cast<int>(config.heightMultiplier / 2.0f);
    return totalColumns * (avgHeight + std::abs(config.minHeight));
}

BlockType TerrainGenerator::getBlockTypeAtHeight(int y, int maxY, float caveValue) const {
    // Höhle (wenn aktiviert)
    if (caveValue > 0.6f) {
        return BlockType::Air;
    }
    
    // Oberste Schicht
    if (y == maxY && maxY > 0) {
     return BlockType::Grass;
    }
  // Erd-Schichten (3 Blöcke unter Oberfläche)
    else if (y > maxY - 4 && y < maxY) {
        return BlockType::Dirt;
    }
    // Stein
    else {
        return BlockType::Stone;
    }
}

float TerrainGenerator::getCaveNoise(int x, int y, int z, float scale) const {
    // Cast away const für Perlin (nicht ideal, aber funktioniert)
    Perlin& p = const_cast<Perlin&>(perlin);
    return p.noise3D(x * scale * 0.5f, y * scale, z * scale * 0.5f);
}

void TerrainGenerator::generateTerrain(VoxelWorld* world, const TerrainConfig& config, ProgressCallback callback) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    int totalColumns = config.sizeX * config.sizeZ;
    int processedColumns = 0;
    
    // Generiere Terrain Spalte für Spalte
  for (int x = -config.sizeX / 2; x < config.sizeX / 2; x++) {
     for (int z = -config.sizeZ / 2; z < config.sizeZ / 2; z++) {
 // Berechne Höhe mit Perlin Noise
            float height = perlin.noise3D(x * config.scale, 0.0f, z * config.scale);
       height *= config.heightMultiplier;
       int maxY = static_cast<int>(height);
    
    // Generiere vertikale Spalte
        for (int y = config.minHeight; y <= maxY; y++) {
             float caveValue = 0.0f;
      if (config.generateCaves && y < maxY - 3) {
         caveValue = getCaveNoise(x, y, z, config.scale);
       }
    
           BlockType blockType = getBlockTypeAtHeight(y, maxY, caveValue);
                
     if (blockType != BlockType::Air) {
        world->setBlock(x, y, z, blockType);
     }
         }
         
            processedColumns++;
            
 // Progress Update (alle 100 Spalten)
        if (callback && processedColumns % 100 == 0) {
      float progress = static_cast<float>(processedColumns) / totalColumns;
            callback(progress, "Generiere Terrain...");
     }
        }
    }
    
    // Finaler Progress-Update
    if (callback) {
        callback(1.0f, "Aktualisiere Chunks...");
    }
    
// Aktualisiere alle Chunks (das kann auch lange dauern)
    world->updateAllChunks();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
std::cout << "Terrain-Generierung abgeschlossen in " << duration.count() << "ms" << std::endl;
    std::cout << "Generierte Spalten: " << totalColumns << std::endl;
    
    if (callback) {
        callback(1.0f, "Fertig!");
    }
}

void TerrainGenerator::generateTerrainBatched(VoxelWorld* world, const TerrainConfig& config, 
    ProgressCallback callback, int batchSize) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    int totalColumns = config.sizeX * config.sizeZ;
    int processedColumns = 0;
    
    std::vector<std::tuple<int, int, int, BlockType>> blockBatch;
blockBatch.reserve(batchSize);
    
    // Generiere Terrain Spalte für Spalte
    for (int x = -config.sizeX / 2; x < config.sizeX / 2; x++) {
   for (int z = -config.sizeZ / 2; z < config.sizeZ / 2; z++) {
            // Berechne Höhe mit Perlin Noise
    float height = perlin.noise3D(x * config.scale, 0.0f, z * config.scale);
  height *= config.heightMultiplier;
            int maxY = static_cast<int>(height);

    // Generiere vertikale Spalte
            for (int y = config.minHeight; y <= maxY; y++) {
          float caveValue = 0.0f;
       if (config.generateCaves && y < maxY - 3) {
          caveValue = getCaveNoise(x, y, z, config.scale);
            }
        
                BlockType blockType = getBlockTypeAtHeight(y, maxY, caveValue);
     
          if (blockType != BlockType::Air) {
           blockBatch.push_back(std::make_tuple(x, y, z, blockType));
         
   // Wenn Batch voll, setze alle Blöcke auf einmal
           if (blockBatch.size() >= static_cast<size_t>(batchSize)) {
  for (const auto& block : blockBatch) {
  world->setBlock(std::get<0>(block), std::get<1>(block), 
        std::get<2>(block), std::get<3>(block));
     }
  blockBatch.clear();
         }
  }
     }
            
            processedColumns++;
       
            // Progress Update (alle 50 Spalten)
        if (callback && processedColumns % 50 == 0) {
        float progress = static_cast<float>(processedColumns) / totalColumns;
    callback(progress, "Generiere Terrain...");
        }
        }
    }
    
    // Setze verbleibende Blöcke
    if (!blockBatch.empty()) {
        for (const auto& block : blockBatch) {
          world->setBlock(std::get<0>(block), std::get<1>(block), 
      std::get<2>(block), std::get<3>(block));
        }
      blockBatch.clear();
    }
    
    // Finaler Progress-Update
    if (callback) {
        callback(0.95f, "Aktualisiere Chunks...");
    }
    
    // Aktualisiere alle Chunks (das kann auch lange dauern)
    world->updateAllChunks();
  
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Terrain-Generierung (Batched) abgeschlossen in " << duration.count() << "ms" << std::endl;
    std::cout << "Generierte Spalten: " << totalColumns << std::endl;
    
 if (callback) {
        callback(1.0f, "Fertig!");
    }
}
