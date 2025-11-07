#pragma once

#include "VoxelWorld.h"
#include "Perlin.h"
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

// Callback-Typen für Fortschritts-Updates
using ProgressCallback = std::function<void(float progress, const std::string& message)>;

struct TerrainConfig {
    int sizeX = 64;         // Terrain-Größe in X-Richtung (in Blöcken, jede Seite)
    int sizeZ = 64;         // Terrain-Größe in Z-Richtung
    float scale = 0.05f;         // Perlin-Noise-Scale (kleiner = größere Features)
    float heightMultiplier = 16.0f; // Maximale Höhe
    int minHeight = -8;     // Minimale Y-Koordinate
    bool generateCaves = false;  // Höhlen generieren
    int seed = 12345;      // Zufalls-Seed für reproduzierbare Welten
    int numThreads = 0;     // Anzahl der Worker-Threads (0 = auto-detect)
};

// Struktur für einen Block, der gesetzt werden soll
struct BlockData {
    int x, y, z;
    BlockType type;
    
    BlockData(int x_, int y_, int z_, BlockType type_) 
        : x(x_), y(y_), z(z_), type(type_) {}
};

class TerrainGenerator {
public:
 TerrainGenerator();
    ~TerrainGenerator();

    // Generiert Terrain mit Fortschritts-Callback
    void generateTerrain(VoxelWorld* world, const TerrainConfig& config, ProgressCallback callback = nullptr);
    
    // Generiert Terrain in Batches (für bessere Performance)
  void generateTerrainBatched(VoxelWorld* world, const TerrainConfig& config, 
   ProgressCallback callback = nullptr, int batchSize = 256);
    
    // NEUE METHODE: Parallelisierte Terrain-Generierung
  void generateTerrainParallel(VoxelWorld* world, const TerrainConfig& config,
        ProgressCallback callback = nullptr);
    
    // Berechnet die Anzahl der zu generierenden Blöcke
    int calculateTotalBlocks(const TerrainConfig& config) const;

private:
    Perlin perlin;
    
    // Thread-sichere Variablen
    std::mutex progressMutex;
    std::atomic<int> processedColumns;
    std::atomic<bool> shouldStop;
    
    // Hilfsfunktionen
    BlockType getBlockTypeAtHeight(int y, int maxY, float caveValue) const;
    float getCaveNoise(int x, int y, int z, float scale) const;
    
    // Worker-Thread-Funktion
    void generateTerrainWorker(
        const TerrainConfig& config,
 int startX, int endX,
    std::vector<BlockData>& blockBuffer,
   std::mutex& bufferMutex
    );
};
