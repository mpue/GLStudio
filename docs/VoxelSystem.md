# Voxel Chunk System - Dokumentation

## Übersicht

Dieses Minecraft-ähnliche Voxel-System implementiert:
- **Chunk-basierte Welt** (16x16x16 Blöcke pro Chunk)
- **Face Culling** (nur sichtbare Flächen werden gerendert)
- **Textur-Atlas-Unterstützung** (verschiedene Texturen pro Blockseite)
- **Optimiertes Rendering** mit OpenGL VBO/VAO/EBO
- **Nachbar-Chunk-Integration** für nahtlose Übergänge

## Integration in GLStudio.cpp

### Initialisierung

Das Voxel-System ist bereits in GLStudio.cpp integriert mit **Voxel-basierter Kollisionserkennung** (ohne Bullet Physics):

```cpp
// In main() nach VoxelWorld-Initialisierung:
voxelWorld = new VoxelWorld();
characterController = new VoxelCharacterController(voxelWorld, window);

// Beispiel-Terrain erstellen
for (int x = 0; x < 8; x++) {
    for (int z = 0; z < 8; z++) {
     voxelWorld->setBlock(x, 0, z, BlockType::Grass);
        voxelWorld->setBlock(x, -1, z, BlockType::Dirt);
        voxelWorld->setBlock(x, -2, z, BlockType::Stone);
    }
}

voxelWorld->updateAllChunks();
```

### Steuerung

**WASD** - Bewegen  
**Leertaste** - Springen  
**Maus** - Umschauen  
**Linksklick** - Block platzieren  
**Mittelklick (Mausrad)** - Block entfernen

### Rendering

Das Voxel-System wird automatisch in der Render-Loop gerendert:

```cpp
// In der Render-Loop
characterController->update(deltaTime);

// Update Kamera
camera.Position = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
camera.Front = characterController->getFront();
camera.Up = characterController->getUp();

// Render
if (voxelWorld) {
    voxelWorld->render();
}
```

### Interaktion

**Linksklick**: Block platzieren  
**Mittelklick (Mausrad)**: Block entfernen

```cpp
// Platzieren
if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
  btTransform trans;
    fpsController.body->getMotionState()->getWorldTransform(trans);
    btVector3 pos = trans.getOrigin();
    
    glm::vec3 blockPos = glm::vec3(
        pos.x() + camera.Front.x * 3.0f,
      pos.y() + camera.Front.y * 3.0f,
        pos.z() + camera.Front.z * 3.0f
    );
    voxelWorld->setBlock(
    static_cast<int>(std::round(blockPos.x)),
      static_cast<int>(std::round(blockPos.y)),
        static_cast<int>(std::round(blockPos.z)),
        BlockType::Stone
    );
}
```

## Klassen

### VoxelChunk

Verwaltet einen einzelnen 16x16x16 Chunk.

**Hauptmethoden:**
- `void setBlock(int x, int y, int z, BlockType type)` - Block setzen (lokale Koordinaten 0-15)
- `BlockType getBlock(int x, int y, int z)` - Block abrufen
- `void generateMesh()` - Mesh generieren ohne Nachbarn
- `void generateMeshWithNeighbors(...)` - Mesh mit Nachbar-Chunks für nahtlose Grenzen
- `void setupOpenGL()` - Lädt Mesh-Daten in GPU (VAO/VBO/EBO)
- `void render()` - Rendert den Chunk

### VoxelWorld

Verwaltet mehrere Chunks in einer unendlichen Welt.

**Hauptmethoden:**
- `void setBlock(int worldX, int worldY, int worldZ, BlockType type)` - Block in Weltkoordinaten setzen
- `BlockType getBlock(int worldX, int worldY, int worldZ)` - Block abrufen
- `VoxelChunk* getOrCreateChunk(int chunkX, int chunkY, int chunkZ)` - Chunk holen/erstellen
- `void updateChunkMesh(int chunkX, int chunkY, int chunkZ)` - Einzelnen Chunk aktualisieren
- `void updateAllChunks()` - Alle Chunks aktualisieren
- `void render()` - Alle Chunks rendern

### BlockType (Enum)

Verfügbare Block-Typen:
- `Air` (0) - Luft/Leer
- `Stone` (1) - Stein
- `Grass` (2) - Gras (verschiedene Texturen pro Seite)
- `Dirt` (3) - Erde
- `Wood` (4) - Holz (Rinde seitlich, Ringe oben/unten)
- `Sand` (5) - Sand
- `Water` (6) - Wasser

## Verwendungsbeispiele

### 1. Terrain mit Perlin Noise generieren

```cpp
void createVoxelTerrain(VoxelWorld* world, int size, float scale, float heightMultiplier) {
    Perlin perlin;

  for (int x = -size; x < size; x++) {
     for (int z = -size; z < size; z++) {
            float height = perlin.noise3D(x * scale, 0.0f, z * scale) * heightMultiplier;
          int maxY = static_cast<int>(height);
   
     for (int y = -5; y <= maxY; y++) {
     BlockType blockType;
                
  if (y == maxY && maxY > 0) {
          blockType = BlockType::Grass;
          } else if (y > maxY - 3 && y < maxY) {
    blockType = BlockType::Dirt;
        } else {
            blockType = BlockType::Stone;
      }
    
             world->setBlock(x, y, z, blockType);
          }
        }
    }
    
  world->updateAllChunks();
}

// In main() aufrufen:
createVoxelTerrain(voxelWorld, 16, 0.1f, 8.0f);
```

### 2. Strukturen bauen

```cpp
// Würfel bauen (nur Außenwände)
void buildCube(VoxelWorld* world, int startX, int startY, int startZ, int size) {
    for (int x = 0; x < size; x++) {
   for (int y = 0; y < size; y++) {
        for (int z = 0; z < size; z++) {
           bool isEdge = (x == 0 || x == size-1 || 
 y == 0 || y == size-1 || 
        z == 0 || z == size-1);
                
  if (isEdge) {
             world->setBlock(startX + x, startY + y, startZ + z, BlockType::Stone);
    }
            }
   }
    }
    world->updateAllChunks();
}

// Turm bauen
void buildTower(VoxelWorld* world, int x, int z, int height) {
    for (int y = 0; y < height; y++) {
  for (int dx = -1; dx <= 1; dx++) {
for (int dz = -1; dz <= 1; dz++) {
      bool isWall = (dx == -1 || dx == 1 || dz == -1 || dz == 1);
        if (isWall) {
   world->setBlock(x + dx, y, z + dz, BlockType::Stone);
              }
 }
        }
    }
    world->updateAllChunks();
}
```

### 3. Einzelne Blöcke setzen/löschen

```cpp
// Block setzen
voxelWorld->setBlock(10, 5, 10, BlockType::Stone);

// Block löschen (zu Luft machen)
voxelWorld->setBlock(10, 5, 10, BlockType::Air);

// Block-Typ abfragen
BlockType type = voxelWorld->getBlock(10, 5, 10);
if (type == BlockType::Grass) {
    // Mach etwas...
}
```

## Vertex-Format

Jeder Vertex hat 8 float-Werte:
```
[x, y, z, nx, ny, nz, u, v]
 Position  Normal     UV
```

**Shader Vertex Attribute:**
```glsl
layout (location = 0) in vec3 aPos;      // Position
layout (location = 1) in vec3 aNormal;   // Normale
layout (location = 2) in vec2 aTexCoord; // UV-Koordinaten
```

## Textur-Atlas

Das System verwendet einen 16x16 Textur-Atlas. Die Textur-Zuordnung ist in `VoxelChunk.cpp` in der Funktion `getBlockTextures()` konfiguriert:

```cpp
switch (type) {
    case BlockType::Stone:
        atlasX = 0; atlasY = 0;
  break;
    case BlockType::Grass:
     if (face == FaceDirection::Top) {
      atlasX = 1; atlasY = 0;  // Gras oben
        } else if (face == FaceDirection::Bottom) {
            atlasX = 2; atlasY = 0;  // Erde unten
      } else {
      atlasX = 3; atlasY = 0;  // Gras-Seite
        }
    break;
    // ... weitere Block-Typen
}
```

**Um eigene Texturen hinzuzufügen:**
1. Erstelle einen Textur-Atlas (z.B. 256x256 Pixel mit 16x16 Feldern)
2. Passe die `atlasX` und `atlasY` Werte in `getBlockTextures()` an
3. Lade die Textur in `main()` und binde sie vor dem Rendering

## Performance-Optimierungen

### Face Culling
Das System rendert nur sichtbare Flächen. Eine Fläche wird nur generiert, wenn der angrenzende Block Luft ist.

### Chunk-Updates
Nach dem Setzen eines Blocks werden nur betroffene Chunks aktualisiert:
```cpp
voxelWorld->setBlock(x, y, z, type); // Automatisches Update des Chunks
```

### Batch-Updates
Für viele Block-Änderungen auf einmal:
```cpp
// Setze viele Blöcke
for (int i = 0; i < 1000; i++) {
    chunk->setBlock(x[i], y[i], z[i], type[i]);
}
// Dann einmalig Mesh generieren
chunk->generateMesh();
chunk->setupOpenGL();
```

## Koordinatensysteme

**Weltkoordinaten**: Absolute Position in der Welt  
**Chunk-Koordinaten**: Welcher Chunk (dividiert durch 16)  
**Lokale Koordinaten**: Position im Chunk (0-15)

```cpp
// Automatische Konvertierung in VoxelWorld
voxelWorld->setBlock(50, 10, 25, BlockType::Stone); // Weltkoordinaten

// Manuell:
glm::ivec3 chunkCoord = voxelWorld->worldToChunkCoord(50, 10, 25);  // (3, 0, 1)
glm::ivec3 localCoord = voxelWorld->worldToLocalCoord(50, 10, 25);// (2, 10, 9)
```

## Erweiterungen (für Fortgeschrittene)

### Greedy Meshing
Kombiniere benachbarte gleiche Flächen zu größeren Quads für bessere Performance.

### LOD (Level of Detail)
Rendere ferne Chunks mit weniger Details/niedrigerer Auflösung.

### Multithreading
Generiere Meshes in Background-Threads für flüssigere Framerate.

### Chunk-Persistenz
Speichere/Lade Chunks in/aus Dateien:
```cpp
void saveChunk(VoxelChunk* chunk, const std::string& filename);
VoxelChunk* loadChunk(const std::string& filename);
```

### Transparente Blöcke
Wasser/Glas benötigen spezielle Render-Queue (nach soliden Blöcken):
```cpp
bool isBlockTransparent(BlockType type) {
    return type == BlockType::Water || type == BlockType::Glass;
}
```

### Ambient Occlusion
Dunklere Ecken für bessere Tiefenwahrnehmung durch Vertex-Farben.

## Projektdateien

- `src/VoxelChunk.h` - Chunk-Klasse Header
- `src/VoxelChunk.cpp` - Chunk-Implementierung und Mesh-Generierung
- `src/VoxelWorld.h` - Welt-Manager Header
- `src/VoxelWorld.cpp` - Chunk-Verwaltung und Koordinaten-Konvertierung
- `GLStudio.cpp` - Integration in bestehende Engine
- `docs/VoxelSystem.md` - Ausführliche Dokumentation

## Fehlerbehebung

**Problem**: Blöcke werden nicht angezeigt  
**Lösung**: Stelle sicher, dass `updateAllChunks()` nach dem Setzen der Blöcke aufgerufen wird und dass ein Shader gebunden ist.

**Problem**: Blöcke haben keine Textur  
**Lösung**: Überprüfe, dass ein Textur-Atlas geladen und gebunden ist (Texture Unit 0).

**Problem**: Performance-Probleme bei vielen Chunks  
**Lösung**: Implementiere Frustum Culling um nur sichtbare Chunks zu rendern.

**Problem**: Lücken zwischen Chunks  
**Lösung**: `generateMeshWithNeighbors()` verwenden statt `generateMesh()`.

## Lizenz

Dieses Voxel-System ist frei verwendbar für persönliche und kommerzielle Projekte.
