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
    
    // NEUE Parameter für mehr Varianz
    int octaves = 4;      // Anzahl der Noise-Oktaven (mehr = detaillierter)
    float persistence = 0.5f; // Amplitude-Abnahme pro Oktave (0-1)
    float lacunarity = 2.0f;           // Frequenz-Zunahme pro Oktave (>1)
    
  // Biom-Parameter
    float continentalnessScale = 0.01f;  // Scale für große Landmassen
    float erosionScale = 0.03f;          // Scale für Erosion/Rauheit
    float mountainScale = 0.02f;   // Scale für Berge
    float mountainThreshold = 0.6f;      // Ab welchem Wert werden Berge generiert
    float mountainHeightMultiplier = 2.5f; // Zusätzliche Höhe für Berge
    
    // Höhlen-Parameter (erweitert)
    float caveScale = 0.05f;        // Scale für Höhlen-Noise
    float caveThreshold = 0.55f;  // Schwellwert für Höhlen (höher = weniger Höhlen)
    int caveMinDepth = 5;// Minimale Tiefe für Höhlen
    
    // Terrain-Features
    bool generateBeaches = true;        // Strände an Wasserlinie
    int waterLevel = 0;    // Wasser-Höhe
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
    
    // Aktueller Seed (für UI-Anzeige)
    int getCurrentSeed() const { return currentSeed; }

private:
    Perlin perlin;
    int currentSeed;
    
    // Thread-sichere Variablen
    std::mutex progressMutex;
    std::atomic<int> processedColumns;
    std::atomic<bool> shouldStop;
    
    // Hilfsfunktionen
    BlockType getBlockTypeAtHeight(int y, int maxY, float caveValue, bool isMountain, bool isBeach) const;
    float getCaveNoise(int x, int y, int z, const TerrainConfig& config, Perlin& localPerlin) const;
    float getTerrainHeight(int x, int z, const TerrainConfig& config, Perlin& localPerlin) const;
    
    // Worker-Thread-Funktion
    void generateTerrainWorker(
        const TerrainConfig& config,
 int startX, int endX,
    std::vector<BlockData>& blockBuffer,
   std::mutex& bufferMutex
    );
};
