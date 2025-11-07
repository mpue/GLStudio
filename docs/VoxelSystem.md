# Voxel Chunk System - Dokumentation

## Übersicht

Dieses Minecraft-ähnliche Voxel-System implementiert:
- **Chunk-basierte Welt** (16x16x16 Blöcke pro Chunk)
- **Face Culling** (nur sichtbare Flächen werden gerendert)
- **Textur-Atlas-Unterstützung** (verschiedene Texturen pro Blockseite)
- **Optimiertes Rendering** mit OpenGL VBO/VAO/EBO
- **Nachbar-Chunk-Integration** für nahtlose Übergänge
- **Voxel-optimierte Beleuchtung** mit direktionaler Sonne und Ambient Occlusion

## Beleuchtungssystem

Das Voxel-System verwendet ein speziell optimiertes Beleuchtungsmodell für Minecraft-ähnliche Welten:

### Komponenten

**1. Direktionale Sonne (Sunlight)**
- Einfache, performante Richtungslichtquelle
- Keine teuren Shadow-Maps benötigt
- Simuliert Tageslicht von oben/schräg

**2. Ambient Light**
- Bläuliches Umgebungslicht für Schatten und dunkle Bereiche
- Sorgt dafür, dass keine komplett schwarzen Flächen entstehen

**3. Ambient Occlusion (AO)**
- Vertex-basiert, keine zusätzlichen Berechnungen zur Laufzeit
- Macht Ecken und nach unten zeigende Flächen dunkler
- Erhöht Tiefenwahrnehmung erheblich

**4. Distanz-Fog**
- Blendet ferne Blöcke sanft in Himmelfarbe über
- Versteckt Chunk-Lade-Grenzen

### Shader-Konfiguration

```cpp
// In GLStudio.cpp
glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f);  // Warmes Sonnenlicht
glm::vec3 ambientColor = glm::vec3(0.3f, 0.35f, 0.4f); // Bläuliches Umgebungslicht

// Im Render-Loop
voxelShader.setVec3("sunDirection", sunDirection);
voxelShader.setVec3("sunColor", sunColor);
voxelShader.setVec3("ambientColor", ambientColor);
voxelShader.setVec3("viewPos", camera.Position);
```

### Tag/Nacht-Zyklus implementieren

```cpp
// In main() vor Render-Loop
float timeOfDay = 0.0f;

// Im Render-Loop
timeOfDay += deltaTime * 0.1f; // Geschwindigkeit anpassen
if (timeOfDay > 1.0f) timeOfDay = 0.0f;

// Sonne rotiert über den Himmel
float angle = timeOfDay * 2.0f * 3.14159f;
sunDirection = glm::normalize(glm::vec3(
    sin(angle) * 0.5f,
    -cos(angle),
    0.3f
));

// Sonnenfarbe ändert sich (Sonnenuntergang = Orange)
float sunIntensity = glm::max(0.0f, -sunDirection.y);
sunColor = glm::mix(
    glm::vec3(1.0f, 0.4f, 0.1f), // Sonnenuntergang
    glm::vec3(1.0f, 0.95f, 0.8f), // Mittag
    sunIntensity
);

// Ambient wird nachts dunkler
ambientColor = glm::vec3(0.3f, 0.35f, 0.4f) * (0.2f + 0.8f * sunIntensity);

// Himmelfarbe anpassen
glm::vec3 skyColor = glm::mix(
    glm::vec3(0.05f, 0.05f, 0.1f), // Nacht
    glm::vec3(0.5f, 0.5f, 0.8f),   // Tag
sunIntensity
);
glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
```

## Integration in GLStudio.cpp

### Initialisierung

Das Voxel-System ist bereits in GLStudio.cpp integriert mit **Voxel-basierter Kollisionserkennung**:

```cpp
// In main() nach Shader-Kompilierung:
Shader voxelShader("shaders/voxel.vert", "shaders/voxel.frag");

// Textur-Atlas laden
unsigned int voxelAtlasTexture = loadTexture("resources/textures/atlas.png");

// Voxel-Welt erstellen
voxelWorld = new VoxelWorld();
characterController = new VoxelCharacterController(voxelWorld, window);

// Beispiel-Terrain erstellen
createVoxelTerrain(voxelWorld, 16, 0.1f, 8.0f);

// Beispiel-Struktur
for (int x = 0; x < 8; x++) {
    for (int z = 0; z < 8; z++) {
      voxelWorld->setBlock(x, 0, z, BlockType::Grass);
        voxelWorld->setBlock(x, -1, z, BlockType::Dirt);
        voxelWorld->setBlock(x, -2, z, BlockType::Stone);
    }
}

voxelWorld->updateAllChunks();
```

### Rendering

Das Voxel-System wird mit optimiertem Shader gerendert:

```cpp
// In der Render-Loop
characterController->update(deltaTime);

// Update Kamera
camera.Position = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
camera.Front = characterController->getFront();
camera.Up = characterController->getUp();

// Render mit Voxel-Shader
if (voxelWorld) {
    voxelShader.use();
    voxelShader.setMat4("projection", projection);
    voxelShader.setMat4("view", view);
    voxelShader.setMat4("model", glm::mat4(1.0f));
    
    // Lighting
    voxelShader.setVec3("sunDirection", sunDirection);
    voxelShader.setVec3("sunColor", sunColor);
    voxelShader.setVec3("ambientColor", ambientColor);
    voxelShader.setVec3("viewPos", camera.Position);
  
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, voxelAtlasTexture);
    
    voxelWorld->render();
}
```

### Steuerung

**WASD** - Bewegen  
**Leertaste** - Springen  
**Maus** - Umschauen  
**Linksklick** - Block platzieren  
**Mittelklick (Mausrad)** - Block entfernen

### Interaktion

**Linksklick**: Block platzieren  
**Mittelklick (Mausrad)**: Block entfernen

```cpp
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
   glm::vec3 pos = characterController->getPosition();
        glm::vec3 front = characterController->getFront();
 glm::vec3 blockPos = pos + front * 3.0f;

        voxelWorld->setBlock(
            static_cast<int>(std::round(blockPos.x)),
       static_cast<int>(std::round(blockPos.y)),
       static_cast<int>(std::round(blockPos.z)),
            BlockType::Stone
     );
    }
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

**Vertex Shader Output (zu Fragment Shader):**
```glsl
out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    float AmbientOcclusion; // Vertex-basiertes AO
} vs_out;
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

### Voxel-spezifische Beleuchtung
Keine teuren Shadow-Maps oder Point-Lights. Stattdessen:
- Einfache direktionale Sonne (1 dot-Product pro Fragment)
- Vertex-basiertes Ambient Occlusion (keine Runtime-Berechnung)
- Distance Fog statt komplexem Atmospheric Scattering

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
- `shaders/voxel.vert` - Voxel Vertex Shader mit AO
- `shaders/voxel.frag` - Voxel Fragment Shader mit Beleuchtung
- `GLStudio.cpp` - Integration in bestehende Engine
- `docs/VoxelSystem.md` - Ausführliche Dokumentation

## Fehlerbehebung

**Problem**: Blöcke werden nicht angezeigt  
**Lösung**: Stelle sicher, dass `updateAllChunks()` nach dem Setzen der Blöcke aufgerufen wird und dass der Voxel-Shader gebunden ist.

**Problem**: Blöcke haben keine Textur  
**Lösung**: Überprüfe, dass ein Textur-Atlas geladen und gebunden ist (Texture Unit 0).

**Problem**: Blöcke sind zu dunkel/hell  
**Lösung**: Passe `sunColor` und `ambientColor` in GLStudio.cpp an. Verwende das ImGui-Fenster "Voxel Lighting Settings" zur Laufzeit-Anpassung.

**Problem**: Performance-Probleme bei vielen Chunks  
**Lösung**: Implementiere Frustum Culling um nur sichtbare Chunks zu rendern.

**Problem**: Lücken zwischen Chunks  
**Lösung**: `generateMeshWithNeighbors()` verwenden statt `generateMesh()`.

## Lizenz

Dieses Voxel-System ist frei verwendbar für persönliche und kommerzielle Projekte.
