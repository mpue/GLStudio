# Block Outline System - Dokumentation

## Übersicht

Das Block Outline System zeigt visuell an, welcher Block beim Klicken gelöscht und wo ein neuer Block platziert werden würde. Es verwendet farbige Wireframe-Outlines:

- **Rot**: Block, der bei Mittelklick gelöscht wird
- **Grün**: Position, wo bei Linksklick ein neuer Block platziert wird

## Features

- ? **Immer sichtbar** - Outline wird über andere Objekte gerendert
- ? **Farbcodiert** - Rot für Löschen, Grün für Platzieren
- ? **Performance-optimiert** - Minimal Draw Calls
- ? **Gut sichtbar** - 3px dicke Linien mit Transparenz

## Implementierung

### Klassen

#### BlockOutline

Verwaltet das Rendering der Wireframe-Outlines um Blöcke.

```cpp
class BlockOutline {
public:
    BlockOutline();
    ~BlockOutline();

    // Initialisiert den Outline-Shader
  void init(const char* vertPath, const char* fragPath);

  // Rendert eine einzelne Outline
    void renderOutline(const glm::ivec3& blockPos, const glm::mat4& projection, 
        const glm::mat4& view, const glm::vec3& color, float alpha = 0.8f);
    
    // Rendert beide Outlines (Remove = Rot, Place = Grün)
    void renderDualOutline(const glm::ivec3& removePos, const glm::ivec3& placePos,
     const glm::mat4& projection, const glm::mat4& view);
};
```

### Shader

**shaders/outline.vert**
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

**shaders/outline.frag**
```glsl
#version 330 core
out vec4 FragColor;

uniform vec3 outlineColor;
uniform float alpha;

void main()
{
    FragColor = vec4(outlineColor, alpha);
}
```

### Integration in GLStudio.cpp

```cpp
// Globale Variable
BlockOutline* blockOutline = nullptr;

// In main() nach Character Controller
blockOutline = new BlockOutline();
blockOutline->init("shaders/outline.vert", "shaders/outline.frag");

// Im Render-Loop (nach Voxel-Welt-Rendering)
if (hasTargetBlock && blockOutline) {
    blockOutline->renderDualOutline(
        currentTargetBlock.blockPos,   // Rot: Löschen
        currentTargetBlock.placePos,   // Grün: Platzieren
 projection,
        view
    );
}

// Cleanup
if (blockOutline) {
    delete blockOutline;
    blockOutline = nullptr;
}
```

## Verwendung

### Einzelne Outline rendern

```cpp
// Rendert eine rote Outline um Block (5, 10, 5)
glm::ivec3 blockPos(5, 10, 5);
glm::vec3 color(1.0f, 0.0f, 0.0f); // Rot
float alpha = 0.8f;

blockOutline->renderOutline(blockPos, projection, view, color, alpha);
```

### Duale Outlines (Standard)

```cpp
// Rendert beide Outlines basierend auf Raycast
if (hasTargetBlock) {
    blockOutline->renderDualOutline(
      currentTargetBlock.blockPos,  // Rot
        currentTargetBlock.placePos,  // Grün
        projection,
     view
    );
}
```

## Render-State

Die `renderOutline()`-Funktion konfiguriert temporär den OpenGL-State:

```cpp
// Aktiviert für Outline-Rendering:
glLineWidth(3.0f);   // Dickere Linien
glEnable(GL_BLEND);       // Transparenz
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable(GL_DEPTH_TEST);   // Immer sichtbar

// Nach Rendering wiederhergestellt:
glEnable(GL_DEPTH_TEST);
glDisable(GL_BLEND);
glLineWidth(1.0f);
```

## Geometrie

Die Outline besteht aus 12 Linien (Kanten eines Würfels):

- 4 Linien für untere Fläche
- 4 Linien für obere Fläche
- 4 vertikale Linien

Insgesamt: **24 Vertices** (12 Linien × 2 Punkte)

### Würfel-Vertices

```cpp
// 8 Ecken eines 1x1x1 Würfels, zentriert um (0, 0, 0)
float vertices[] = {
    // Untere 4 Ecken
    -0.5f, -0.5f, -0.5f,  // 0
     0.5f, -0.5f, -0.5f,  // 1
     0.5f, -0.5f,  0.5f,  // 2
    -0.5f, -0.5f,  0.5f,  // 3
  // Obere 4 Ecken
    -0.5f,  0.5f, -0.5f,  // 4
     0.5f,  0.5f, -0.5f,  // 5
     0.5f,  0.5f,  0.5f,  // 6
    -0.5f,  0.5f,  0.5f   // 7
};
```

### Skalierung

Die Outline wird um **0.5%** größer skaliert (`scale = 1.005f`), damit sie sichtbar über dem Block liegt und nicht z-fighting verursacht.

## Farben

### Standard-Farben

- **Löschen (Remove)**: RGB(1.0, 0.2, 0.2) - Rot mit 90% Alpha
- **Platzieren (Place)**: RGB(0.2, 1.0, 0.2) - Grün mit 70% Alpha

### Eigene Farben

```cpp
// Blaue Outline
glm::vec3 blue(0.2f, 0.5f, 1.0f);
blockOutline->renderOutline(blockPos, projection, view, blue, 0.6f);

// Gelbe Outline
glm::vec3 yellow(1.0f, 1.0f, 0.0f);
blockOutline->renderOutline(blockPos, projection, view, yellow, 0.9f);
```

## Performance

**Kosten pro Frame:**
- 1× VAO Bind
- 1× Shader Use
- 4× Uniform Sets (projection, view, model, color, alpha)
- 2× glDrawElements (24 indices × 2 für beide Outlines)

**Geschätzte GPU-Belastung:** < 0.1ms

## Debugging

### ImGui-Integration

Das System zeigt automatisch Debug-Info im "Voxel Lighting Settings" Fenster:

```cpp
ImGui::Text("Target Block: (%d, %d, %d)", 
    currentTargetBlock.blockPos.x, 
    currentTargetBlock.blockPos.y, 
    currentTargetBlock.blockPos.z);
ImGui::Text("Place Position: (%d, %d, %d)", 
    currentTargetBlock.placePos.x, 
    currentTargetBlock.placePos.y, 
    currentTargetBlock.placePos.z);
```

### Console Output

Bei Block-Interaktion wird die Position ausgegeben:

```
Block platziert bei: (5, 10, 12)
Block entfernt bei: (5, 10, 11)
```

## Anpassungen

### Liniendicke ändern

```cpp
// In BlockOutline.cpp, Funktion renderOutline():
glLineWidth(5.0f);  // Dickere Linien
```

### Transparenz anpassen

```cpp
// In BlockOutline.cpp, Funktion renderDualOutline():
renderOutline(removePos, projection, view, glm::vec3(1.0f, 0.2f, 0.2f), 1.0f);  // 100% undurchsichtig
renderOutline(placePos, projection, view, glm::vec3(0.2f, 1.0f, 0.2f), 0.3f);    // 30% durchsichtig
```

### Depth-Testing aktivieren

Wenn die Outline durch Blöcke verdeckt werden soll:

```cpp
// In BlockOutline.cpp, Funktion renderOutline():
// glDisable(GL_DEPTH_TEST);  // Auskommentieren
```

## Erweiterungen

### Animierte Outlines

```cpp
// In render loop
float time = glfwGetTime();
float pulse = 0.5f + 0.5f * sin(time * 3.0f);
glm::vec3 color = glm::mix(
    glm::vec3(1.0f, 0.2f, 0.2f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    pulse
);
blockOutline->renderOutline(blockPos, projection, view, color);
```

### Mehrere Outlines

```cpp
std::vector<glm::ivec3> highlightedBlocks = {
    glm::ivec3(5, 10, 5),
    glm::ivec3(6, 10, 5),
    glm::ivec3(7, 10, 5)
};

for (const auto& pos : highlightedBlocks) {
    blockOutline->renderOutline(pos, projection, view, 
 glm::vec3(1.0f, 1.0f, 0.0f), 0.5f);
}
```

### Selektions-Box (Multi-Block)

```cpp
void renderSelectionBox(glm::ivec3 start, glm::ivec3 end) {
    // Rendere Outline für jeden Block in der Selektion
    for (int x = start.x; x <= end.x; ++x) {
        for (int y = start.y; y <= end.y; ++y) {
            for (int z = start.z; z <= end.z; ++z) {
  blockOutline->renderOutline(
     glm::ivec3(x, y, z), 
         projection, view,
       glm::vec3(1.0f, 1.0f, 0.0f), 0.4f
      );
            }
        }
    }
}
```

## Projektdateien

- `src/BlockOutline.h` - Header-Datei
- `src/BlockOutline.cpp` - Implementierung
- `shaders/outline.vert` - Vertex Shader
- `shaders/outline.frag` - Fragment Shader
- `GLStudio.cpp` - Integration

## Lizenz

Frei verwendbar für persönliche und kommerzielle Projekte.
