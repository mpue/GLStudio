#pragma once

#include "VoxelWorld.h"
#include "Perlin.h"
#include <functional>
#include <string>

// Callback-Typen für Fortschritts-Updates
using ProgressCallback = std::function<void(float progress, const std::string& message)>;

struct TerrainConfig {
    int sizeX = 64;         // Terrain-Größe in X-Richtung (in Blöcken, jede Seite)
    int sizeZ = 64;            // Terrain-Größe in Z-Richtung
    float scale = 0.05f;         // Perlin-Noise-Scale (kleiner = größere Features)
    float heightMultiplier = 16.0f; // Maximale Höhe
    int minHeight = -8;     // Minimale Y-Koordinate
    bool generateCaves = false;  // Höhlen generieren
    int seed = 12345;      // Zufalls-Seed für reproduzierbare Welten
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
    
    // Berechnet die Anzahl der zu generierenden Blöcke
    int calculateTotalBlocks(const TerrainConfig& config) const;

private:
    Perlin perlin;
    
    // Hilfsfunktionen
    BlockType getBlockTypeAtHeight(int y, int maxY, float caveValue) const;
    float getCaveNoise(int x, int y, int z, float scale) const;
};
