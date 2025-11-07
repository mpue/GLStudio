# Terrain Generator - Dokumentation

## Übersicht

Das Terrain-Generator-System ermöglicht die Erstellung großer, prozedural generierter Voxel-Landschaften mit Fortschrittsanzeige und optimierter Performance.

## Features

- ? **Große Terrains** - Bis zu 256×256 Blöcke und mehr
- ? **Fortschrittsanzeige** - Echtzeit-Updates während der Generierung
- ? **Batched Generation** - Optimierte Performance durch Block-Batching
- ? **Perlin Noise** - Realistische, organische Landschaften
- ? **Konfigurierbar** - Höhe, Größe, Höhlen, Seed, etc.
- ? **Reproduzierbar** - Gleicher Seed = Gleiche Welt

## Klassen

### TerrainGenerator

Hauptklasse für Terrain-Generierung.

```cpp
class TerrainGenerator {
public:
    TerrainGenerator();
    ~TerrainGenerator();

 // Generiert Terrain mit Fortschritts-Callback
    void generateTerrain(VoxelWorld* world, const TerrainConfig& config, 
     ProgressCallback callback = nullptr);
    
    // Generiert Terrain in Batches (schneller)
    void generateTerrainBatched(VoxelWorld* world, const TerrainConfig& config, 
        ProgressCallback callback = nullptr, int batchSize = 512);
};
```

### TerrainConfig

Konfigurationsstruktur für Terrain-Parameter.

```cpp
struct TerrainConfig {
    int sizeX = 64;          // Breite in Blöcken
    int sizeZ = 64;        // Tiefe in Blöcken
    float scale = 0.05f;   // Perlin-Noise-Scale
    float heightMultiplier = 16.0f; // Maximale Höhe
    int minHeight = -8;      // Minimale Y-Koordinate
    bool generateCaves = false;   // Höhlen aktivieren
    int seed = 12345;  // Zufalls-Seed
};
```

## Verwendung

### Basis-Setup

```cpp
// In GLStudio.cpp
#include "TerrainGenerator.h"

// Globale Variablen
TerrainGenerator* terrainGenerator = nullptr;
float terrainGenerationProgress = 0.0f;
std::string terrainGenerationMessage = "";

// In main()
terrainGenerator = new TerrainGenerator();

TerrainConfig config;
config.sizeX = 128;  // 128×128 Blöcke
config.sizeZ = 128;
config.scale = 0.03f;
config.heightMultiplier = 20.0f;
config.minHeight = -10;
config.generateCaves = false;

// Progress-Callback
auto progressCallback = [](float progress, const std::string& message) {
    terrainGenerationProgress = progress;
    terrainGenerationMessage = message;
    std::cout << "Terrain: " << (int)(progress * 100) << "% - " << message << std::endl;
};

// Generiere Terrain
terrainGenerator->generateTerrainBatched(voxelWorld, config, progressCallback, 512);
```

### Fortschrittsanzeige in ImGui

```cpp
// Im Render-Loop
ImGui::Begin("Voxel Settings");

ImGui::Text("Terrain Generation");
if (terrainGenerationInProgress) {
    ImGui::ProgressBar(terrainGenerationProgress, ImVec2(-1, 0));
    ImGui::Text("%s", terrainGenerationMessage.c_str());
} else {
    ImGui::Text("Terrain: Complete");
}

ImGui::End();
```

## Konfigurationsbeispiele

### Kleine Test-Welt (schnell)

```cpp
TerrainConfig config;
config.sizeX = 32;
config.sizeZ = 32;
config.scale = 0.1f;
config.heightMultiplier = 8.0f;
config.minHeight = -5;
config.generateCaves = false;
```

### Mittlere Welt (Standard)

```cpp
TerrainConfig config;
config.sizeX = 128;
config.sizeZ = 128;
config.scale = 0.03f;
config.heightMultiplier = 20.0f;
config.minHeight = -10;
config.generateCaves = false;
```

### Große Welt mit Höhlen

```cpp
TerrainConfig config;
config.sizeX = 256;
config.sizeZ = 256;
config.scale = 0.02f;
config.heightMultiplier = 32.0f;
config.minHeight = -20;
config.generateCaves = true;  // Achtung: Reduziert Performance
config.seed = 42;  // Reproduzierbare Welt
```

### Flaches Terrain

```cpp
TerrainConfig config;
config.sizeX = 128;
config.sizeZ = 128;
config.scale = 0.1f;
config.heightMultiplier = 4.0f;
config.minHeight = 0;
config.generateCaves = false;
```

### Extreme Berge

```cpp
TerrainConfig config;
config.sizeX = 128;
config.sizeZ = 128;
config.scale = 0.01f;  // Sehr kleine Scale = große Features
config.heightMultiplier = 50.0f;  // Sehr hoch
config.minHeight = -15;
config.generateCaves = false;
```

## Block-Typen-Logik

Die Generierung verwendet folgende Logik:

```cpp
if (y == maxY && maxY > 0) {
    return BlockType::Grass;  // Oberste Schicht
}
else if (y > maxY - 4 && y < maxY) {
    return BlockType::Dirt;   // 3 Blöcke unter Oberfläche
}
else {
    return BlockType::Stone;  // Alles darunter
}
```

### Eigene Block-Logik

```cpp
// In TerrainGenerator.cpp, getBlockTypeAtHeight():
BlockType TerrainGenerator::getBlockTypeAtHeight(int y, int maxY, float caveValue) const {
    if (caveValue > 0.6f) {
  return BlockType::Air;  // Höhle
    }
    
  // Wasser bei Y = 0
    if (y == 0 && maxY < 0) {
        return BlockType::Water;
    }
    
    // Sand an Küsten
    if (y > maxY - 2 && maxY < 2) {
        return BlockType::Sand;
    }
    
    // Schnee auf hohen Bergen
    if (y == maxY && maxY > 30) {
      return BlockType::Snow;  // Needs to be added to enum
    }
    
    // Standard-Logik
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
```

## Performance

### Generierungszeiten (ca.)

| Terrain-Größe | Blöcke | Zeit (ohne Batching) | Zeit (mit Batching) |
|---------------|--------|---------------------|---------------------|
| 32×32     | ~5K    | ~50ms          | ~30ms |
| 64×64         | ~20K   | ~200ms   | ~100ms        |
| 128×128       | ~80K   | ~800ms  | ~400ms       |
| 256×256       | ~320K  | ~3200ms             | ~1500ms     |

### Optimierungen

**1. Batch-Size anpassen**

```cpp
// Kleiner Batch = mehr Updates, aber flüssiger
terrainGenerator->generateTerrainBatched(voxelWorld, config, callback, 256);

// Großer Batch = weniger Updates, aber schneller
terrainGenerator->generateTerrainBatched(voxelWorld, config, callback, 1024);
```

**2. Höhlen deaktivieren**

Höhlen-Generierung ist teuer (zusätzliches Perlin Noise pro Block):

```cpp
config.generateCaves = false;  // Spart ~30% Zeit
```

**3. Kleinere Batch-Updates für Progress**

```cpp
// In TerrainGenerator.cpp
if (callback && processedColumns % 25 == 0) {  // Öfter updaten
    callback(progress, "Generiere Terrain...");
}
```

## Multi-Threading (Fortgeschritten)

Für sehr große Terrains kann Multi-Threading verwendet werden:

```cpp
#include <thread>
#include <mutex>

std::mutex progressMutex;
float threadProgress = 0.0f;

void generateTerrainThreaded(VoxelWorld* world, TerrainConfig config) {
    std::thread terrainThread([world, config]() {
        auto callback = [](float progress, const std::string& msg) {
    std::lock_guard<std::mutex> lock(progressMutex);
     threadProgress = progress;
      };
        
        terrainGenerator->generateTerrainBatched(world, config, callback);
    });
    
    terrainThread.detach();
}

// Im Render-Loop
{
    std::lock_guard<std::mutex> lock(progressMutex);
    ImGui::ProgressBar(threadProgress, ImVec2(-1, 0));
}
```

**?? Achtung**: `VoxelWorld::setBlock()` ist nicht thread-safe!

## Reproduzierbare Welten

Verwende Seeds für gleiche Welten:

```cpp
config.seed = 12345;

// Später mit gleichem Seed:
config.seed = 12345;  // Generiert identische Welt
```

## Debugging

### Console-Output aktivieren

```cpp
// In TerrainGenerator.cpp
std::cout << "Terrain-Generierung abgeschlossen in " << duration.count() << "ms" << std::endl;
std::cout << "Generierte Spalten: " << totalColumns << std::endl;
std::cout << "Durchschnittliche Höhe: " << avgHeight << std::endl;
```

### Visualisiere Fortschritt

```cpp
// In Progress-Callback
auto progressCallback = [](float progress, const std::string& message) {
  // Console
 std::cout << "\r[";
    int barWidth = 50;
  int pos = static_cast<int>(barWidth * progress);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% " << message;
    std::cout.flush();
};
```

## Chunk-Update-Performance

Der langsamste Teil ist oft `updateAllChunks()`. Optimierungen:

### Inkrementelles Update

```cpp
// Statt updateAllChunks() am Ende:
// Update Chunks während der Generierung (alle N Spalten)
if (processedColumns % 64 == 0) {
    world->updateAllChunks();
    if (callback) {
        callback(progress, "Aktualisiere Chunks...");
    }
}
```

### Nur betroffene Chunks updaten

```cpp
// In VoxelWorld: Markiere dirty Chunks
std::set<glm::ivec3> dirtyChunks;

void VoxelWorld::setBlock(int x, int y, int z, BlockType type) {
    // ...existing code...
    glm::ivec3 chunkCoord = worldToChunkCoord(x, y, z);
  dirtyChunks.insert(chunkCoord);
}

void VoxelWorld::updateDirtyChunks() {
    for (const auto& coord : dirtyChunks) {
        updateChunkMesh(coord.x, coord.y, coord.z);
    }
    dirtyChunks.clear();
}
```

## Projekt-Dateien

- `src/TerrainGenerator.h` - Header
- `src/TerrainGenerator.cpp` - Implementierung
- `GLStudio.cpp` - Integration
- `docs/TerrainGenerator.md` - Diese Dokumentation

## Fehlerbehebung

**Problem**: Terrain wird nicht vollständig generiert  
**Lösung**: Stelle sicher, dass `updateAllChunks()` am Ende aufgerufen wird.

**Problem**: Performance-Probleme bei großen Terrains  
**Lösung**: Erhöhe `batchSize` oder deaktiviere Höhlen.

**Problem**: Fortschrittsbalken hängt  
**Lösung**: Reduziere Update-Intervall (z.B. alle 25 statt 100 Spalten).

**Problem**: Terrain sieht zu gleichmäßig aus  
**Lösung**: Verkleinere `scale` für größere Features oder erhöhe `heightMultiplier`.

## Lizenz

Frei verwendbar für persönliche und kommerzielle Projekte.
