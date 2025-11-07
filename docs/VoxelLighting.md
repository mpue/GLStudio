# Voxel Lichtsystem - Dokumentation

## Übersicht

Einfaches, **performantes Lichtsystem** speziell für Voxel-Rendering optimiert. Ersetzt das rechenintensive Point-Light-System mit Cubemap-Shadows durch ein effizientes direktionales Lichtsystem.

## Features

? **Direktionale Beleuchtung** - Sonne als Hauptlichtquelle  
? **Ambient Occlusion** - Faces nach unten sind dunkler  
? **Fog-System** - Atmosphärische Perspektive  
? **Keine Shadow Maps** - Höhere Performance  
? **Anpassbare Parameter** - Echtzeit-Steuerung via ImGui  

---

## Komponenten

### 1. Shader-Dateien

#### `shaders/voxel.vert`
```glsl
// Vertex Shader
- Transformiert Vertices
- Berechnet Ambient Occlusion basierend auf Normalen
- Übergibt Daten an Fragment Shader
```

#### `shaders/voxel.frag`
```glsl
// Fragment Shader
- Berechnet finale Beleuchtung
- Komponenten:
  1. Ambiente Beleuchtung
  2. Diffuse Beleuchtung (Sonne)
  3. Ambient Occlusion
  4. Fog-Effekt
```

---

## Beleuchtungs-Komponenten

### ?? **1. Direktionale Sonne**

**Beschreibung:**  
Simuliert Sonnenlicht aus einer bestimmten Richtung.

**Parameter:**
```cpp
glm::vec3 sunDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.5f));
glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f); // Warmes Sonnenlicht
```

**Berechnung:**
```glsl
float diff = max(dot(normal, -sunDirection), 0.0);
vec3 diffuse = diff * sunColor * blockColor;
```

**Effekt:**
- Faces die zur Sonne zeigen sind heller
- Schafft klare Tag/Nacht-Unterschiede
- Einfaches, schnelles Shading

---

### ?? **2. Ambiente Beleuchtung**

**Beschreibung:**  
Basis-Helligkeit, sodass auch Schatten nicht komplett schwarz sind.

**Parameter:**
```cpp
glm::vec3 ambientColor = glm::vec3(0.3f, 0.3f, 0.4f); // Kühles Umgebungslicht
```

**Berechnung:**
```glsl
vec3 ambient = ambientColor * blockColor;
```

**Effekt:**
- Sorgt für Grundhelligkeit
- Verhindert komplett schwarze Bereiche
- Kühlerer Ton für Kontrast zur warmen Sonne

---

### ?? **3. Ambient Occlusion (AO)**

**Beschreibung:**  
Vereinfachte AO basierend auf Flächennormalen.

**Berechnung (Vertex Shader):**
```glsl
vs_out.AmbientOcclusion = 0.5 + 0.5 * dot(aNormal, vec3(0.0, 1.0, 0.0));
```

**Effekt:**
- **Top-Faces** (zeigen nach oben): Heller (AO = 1.0)
- **Side-Faces** (horizontal): Mittel (AO ? 0.5)
- **Bottom-Faces** (zeigen nach unten): Dunkler (AO = 0.0)

**Finale Anwendung:**
```glsl
vec3 lighting = (ambient + diffuse) * ao;
```

---

### ??? **4. Fog-System**

**Beschreibung:**  
Distanz-basierter Nebel für atmosphärische Tiefe.

**Berechnung:**
```glsl
float distance = length(viewPos - fs_in.FragPos);
float fogFactor = clamp(1.0 - (distance - 20.0) / 80.0, 0.0, 1.0);
vec3 fogColor = vec3(0.5, 0.5, 0.8); // Himmel-Farbe

vec3 finalColor = mix(fogColor, lighting, fogFactor);
```

**Parameter:**
- **Fog Start:** 20 Einheiten
- **Fog Range:** 80 Einheiten
- **Fog End:** 100 Einheiten
- **Fog Color:** Himmelsblau (0.5, 0.5, 0.8)

**Effekt:**
- Weiche Sicht-Reichweiten-Begrenzung
- Versteckt Chunk-Pop-In
- Atmosphärische Perspektive

---

## Verwendung in GLStudio.cpp

### Initialisierung

```cpp
// Shader laden
Shader voxelShader("shaders/voxel.vert", "shaders/voxel.frag");

// Beleuchtungs-Parameter
glm::vec3 sunDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.5f));
glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f);
glm::vec3 ambientColor = glm::vec3(0.3f, 0.3f, 0.4f);
```

### Rendering

```cpp
voxelShader.use();
voxelShader.setMat4("projection", projection);
voxelShader.setMat4("view", view);
voxelShader.setMat4("model", model);

// Beleuchtungs-Uniforms
voxelShader.setVec3("sunDirection", sunDirection);
voxelShader.setVec3("sunColor", sunColor);
voxelShader.setVec3("ambientColor", ambientColor);
voxelShader.setVec3("viewPos", camera.Position);

voxelWorld->render();
```

---

## ImGui-Steuerung

### UI-Controls

```cpp
ImGui::CollapsingHeader("Voxel Beleuchtung")
{
    ImGui::DragFloat3("Sonnen-Richtung", (float*)&sunDirection);
    ImGui::ColorEdit3("Sonnen-Farbe", (float*)&sunColor);
    ImGui::ColorEdit3("Ambiente Farbe", (float*)&ambientColor);
    
    if (ImGui::Button("Sonnen-Richtung normalisieren")) {
        sunDirection = glm::normalize(sunDirection);
    }
}
```

### Live-Anpassung

Sie können in Echtzeit folgende Parameter ändern:
- **Sonnen-Richtung**: Ändert Lichteinfall
- **Sonnen-Farbe**: Ändert Farbton des Tageslichts
- **Ambiente Farbe**: Ändert Schatten-Helligkeit

---

## Performance-Vorteile

### Vergleich: Alt vs. Neu

| Feature | Point Light + Shadow Maps | Voxel Lighting |
|---------|---------------------------|----------------|
| **Shadow Maps** | 6 Cubemap Faces (1024²) | Keine |
| **Render Passes** | 2 (Depth + Color) | 1 (nur Color) |
| **Texture Samples** | 20+ pro Fragment | 0 |
| **GPU Memory** | ~25 MB | ~0 MB |
| **Fragment Shader** | Komplex (100+ Zeilen) | Einfach (30 Zeilen) |
| **FPS (geschätzt)** | Baseline | +50-100% |

### Warum ist es schneller?

1. **Keine Shadow Maps**: Spart Render-Passes und GPU-Memory
2. **Einfache Berechnungen**: Nur Dot-Products, keine Texture-Samples
3. **Weniger Uniforms**: Keine Cubemap-Textur
4. **Besseres Caching**: Linearer Shader-Code

---

## Erweiterungsmöglichkeiten

### 1. Tag/Nacht-Zyklus

```cpp
float time = glfwGetTime() * 0.1f;
float sunHeight = sin(time);

sunDirection = glm::normalize(glm::vec3(
    cos(time),
    sunHeight,
    sin(time)
));

// Farbe ändert sich mit Sonnenstand
if (sunHeight > 0) {
    sunColor = glm::vec3(1.0f, 0.95f, 0.8f); // Tag
} else {
    sunColor = glm::vec3(0.3f, 0.3f, 0.5f); // Nacht
}
```

### 2. Block-Farben (Textur-Atlas)

```glsl
// In voxel.frag
uniform sampler2D blockAtlas;

vec3 getBlockColor() {
    return texture(blockAtlas, fs_in.TexCoords).rgb;
}
```

### 3. Bessere Ambient Occlusion

Berechne AO pro Vertex basierend auf Nachbar-Blöcken:

```cpp
// In VoxelChunk::addFace()
float calculateAO(int x, int y, int z, FaceDirection face) {
int occludedNeighbors = 0;
    // Prüfe 8 Nachbarn um Vertex
 // ... Code für Nachbar-Prüfung
    return 1.0f - (occludedNeighbors / 8.0f) * 0.5f;
}
```

### 4. Dynamische Lichter (optional)

```glsl
// Zusätzliche Punkt-Lichter (Fackeln, Lava)
uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform int numLights;

for (int i = 0; i < numLights; i++) {
    vec3 lightDir = normalize(lightPositions[i] - fs_in.FragPos);
    float dist = length(lightPositions[i] - fs_in.FragPos);
    float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);
    
    float diff = max(dot(normal, lightDir), 0.0);
    lighting += diff * lightColors[i] * attenuation;
}
```

---

## Beispiel-Szenarien

### Heller Mittag
```cpp
sunDirection = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
sunColor = glm::vec3(1.0f, 1.0f, 0.95f);
ambientColor = glm::vec3(0.6f, 0.6f, 0.7f);
```

### Sonnenaufgang/Sonnenuntergang
```cpp
sunDirection = glm::normalize(glm::vec3(-0.8f, -0.3f, -0.5f));
sunColor = glm::vec3(1.0f, 0.6f, 0.3f);
ambientColor = glm::vec3(0.3f, 0.2f, 0.3f);
```

### Nacht
```cpp
sunDirection = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
sunColor = glm::vec3(0.1f, 0.1f, 0.2f);
ambientColor = glm::vec3(0.05f, 0.05f, 0.1f);
```

### Bewölkter Tag
```cpp
sunDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.5f));
sunColor = glm::vec3(0.7f, 0.7f, 0.75f);
ambientColor = glm::vec3(0.5f, 0.5f, 0.55f);
```

---

## Troubleshooting

### Problem: Alles zu dunkel
```cpp
// Lösung: Erhöhe ambientes Licht
ambientColor = glm::vec3(0.5f, 0.5f, 0.6f);
```

### Problem: Zu flache Beleuchtung
```cpp
// Lösung: Stärkere Sonnen-Farbe
sunColor = glm::vec3(1.5f, 1.4f, 1.2f);
```

### Problem: Zu viel Fog
```glsl
// In voxel.frag: Erhöhe Distanzen
float fogFactor = clamp(1.0 - (distance - 50.0) / 150.0, 0.0, 1.0);
```

### Problem: Faces zu unterschiedlich hell
```glsl
// Reduziere AO-Einfluss
vs_out.AmbientOcclusion = 0.7 + 0.3 * dot(aNormal, vec3(0.0, 1.0, 0.0));
```

---

## Technische Details

### Vertex Shader Output
```glsl
out VS_OUT {
    vec3 FragPos;           // Welt-Position
    vec3 Normal;    // Welt-Normale
 vec2 TexCoords;         // UV-Koordinaten
    float AmbientOcclusion; // Vorberechnete AO
} vs_out;
```

### Fragment Shader Uniforms
```glsl
uniform vec3 sunDirection;   // Normalisierter Vektor
uniform vec3 sunColor;       // RGB [0-2] (kann > 1 sein für HDR)
uniform vec3 ambientColor; // RGB [0-1]
uniform vec3 viewPos;        // Kamera-Position für Fog
```

### Beleuchtungs-Gleichung

```
finalColor = mix(fogColor, lighting, fogFactor)

where:
  lighting = (ambient + diffuse) * ao
  ambient = ambientColor * blockColor
  diffuse = max(dot(normal, -sunDirection), 0) * sunColor * blockColor
  ao = 0.5 + 0.5 * dot(normal, up)
  fogFactor = clamp(1.0 - (distance - fogStart) / fogRange, 0.0, 1.0)
```

---

## Migration von Point Light

### Was wurde entfernt:
- ? Point Light Position (`lightPos`)
- ? Shadow Map Rendering (6 Cubemap Faces)
- ? Depth Shader (`ps_depth.vert/frag/geom`)
- ? Shadow Calculation im Fragment Shader
- ? Cubemap Texture Sampling

### Was wurde hinzugefügt:
- ? Direktionale Sonne (`sunDirection`, `sunColor`)
- ? Ambient Occlusion (vorberechnet im Vertex Shader)
- ? Fog-System (distanzbasiert)
- ? ImGui-Controls für Echtzeit-Anpassung

---

## Best Practices

### Performance
1. Normalisiere `sunDirection` nur wenn geändert
2. Verwende `const` Variablen im Shader wo möglich
3. Vermeide `if`-Statements im Fragment Shader

### Ästhetik
1. Halte `ambientColor` < `sunColor` für Kontrast
2. Verwende leicht warme Sonnenfarben (mehr Rot als Blau)
3. Verwende leicht kühle Ambient-Farben (mehr Blau als Rot)

### Debugging
1. Deaktiviere AO temporär: `ao = 1.0`
2. Deaktiviere Fog temporär: `fogFactor = 1.0`
3. Visualisiere Normalen: `FragColor = vec4(normal * 0.5 + 0.5, 1.0)`

---

## Lizenz

Frei verwendbar für persönliche und kommerzielle Projekte.

---

## Changelog

### Version 1.0 (Aktuell)
- ? Direktionale Beleuchtung
- ? Einfache Ambient Occlusion
- ? Distanz-basierter Fog
- ? ImGui-Integration
- ? Performance-Optimierung

### Geplant (Future)
- ?? Tag/Nacht-Zyklus
- ?? Textur-Atlas Support
- ?? Vertex-basierte AO
- ?? Dynamische Punkt-Lichter (optional)
- ?? Wetter-Effekte
