# Erweiterte Terrain-Generierung - Vollständiger Guide

## Übersicht

Der Terrain Generator wurde massiv erweitert, um deutlich mehr Varianz und realistische Terrain-Features zu erzeugen. Das System nutzt Multi-Oktaven Perlin Noise und ein Biom-System für komplexe, abwechslungsreiche Landschaften.

## Kernkonzepte

### 1. Multi-Oktaven Perlin Noise (Fractal Brownian Motion)

Mehrere Schichten von Noise werden übereinandergelegt, um detaillierte Strukturen zu erzeugen.

**Oktaven** (`octaves: 1-8`):
- Anzahl der Noise-Schichten
- 1-2: Sehr glatt, einfache Formen
- 3-4: Ausgewogen, natürlich aussehend (**Standard: 4**)
- 5-6: Sehr detailliert, komplex
- 7-8: Extrem detailliert, kann chaotisch wirken

**Persistence** (`persistence: 0.1-1.0`):
- Kontrolliert die Amplitude-Abnahme pro Oktave
- 0.2-0.3: Sehr glatte Landschaften
- 0.4-0.6: Natürlich aussehend (**Standard: 0.5**)
- 0.7-0.9: Sehr rau und zerklüftet

**Lacunarity** (`lacunarity: 1.5-4.0`):
- Kontrolliert die Frequenz-Zunahme pro Oktave
- 1.5-1.8: Sanfte Übergänge
- 2.0-2.5: Ausgewogen (**Standard: 2.0**)
- 3.0-4.0: Scharfe, dramatische Details

```cpp
// Implementierung
float octaveNoise3D(x, y, z, octaves, persistence, lacunarity) {
    total = 0
    frequency = 1.0
  amplitude = 1.0
    maxValue = 0
    
    for (i = 0 to octaves) {
        total += noise3D(x * frequency, y * frequency, z * frequency) * amplitude
      maxValue += amplitude
    amplitude *= persistence
     frequency *= lacunarity
    }
    
    return total / maxValue  // Normalisiert auf 0-1
}
```

### 2. Biom-System

Verschiedene Noise-Layer werden kombiniert für komplexe Landschaften:

#### A. Kontinentalität (Continentalness)
Erzeugt große Landmassen und Ozeane.

**Parameter**: `continentalnessScale: 0.001-0.05`
- 0.003-0.007: Große Kontinente, weite Ozeane
- 0.01-0.02: Ausgewogene Land/Wasser-Verteilung (**Standard: 0.01**)
- 0.03-0.05: Viele kleine Inseln

**Effekt**: Beeinflusst die Basis-Höhe (40% des Höhenwerts)

#### B. Erosion
Fügt Rauheit und realistische Details hinzu.

**Parameter**: `erosionScale: 0.01-0.1`
- 0.01-0.02: Sehr glatte Hügel
- 0.03-0.05: Natürliche Rauheit (**Standard: 0.03**)
- 0.06-0.1: Stark zerklüftete Landschaft

**Effekt**: Addiert/subtrahiert bis zu 30% der Höhe basierend auf Erosions-Noise

#### C. Berg-System
Erzeugt dramatische Gebirgsketten.

**Berg-Scale** (`mountainScale: 0.005-0.05`):
- 0.005-0.01: Sehr große Bergmassive
- 0.02: Ausgewogene Berggröße (**Standard**)
- 0.03-0.05: Viele kleinere Berge

**Berg-Schwellwert** (`mountainThreshold: 0.3-0.9`):
- 0.3-0.4: Sehr viele Berge (60-70% der Welt)
- 0.5-0.6: Ausgewogen (**Standard: 0.6**)
- 0.7-0.9: Seltene, isolierte Berge

**Berg-Höhen-Multiplikator** (`mountainHeightMultiplier: 1.0-5.0`):
- 1.0-1.5: Sanfte Hügel
- 2.0-3.0: Hohe Berge (**Standard: 2.5**)
- 3.5-5.0: Extreme, dramatische Gipfel

**Berechnung**:
```cpp
if (mountainFactor > mountainThreshold) {
    // Quadratische Kurve für dramatischere Spitzen
    intensity = (mountainFactor - threshold) / (1.0 - threshold)
    intensity = intensity * intensity
    height += intensity * mountainHeightMultiplier
}
```

### 3. Höhlen-System

Verbesserte 3D-Höhlen-Generierung mit organischen Formen.

**Höhlen-Scale** (`caveScale: 0.01-0.1`):
- 0.01-0.03: Große, offene Kavernen
- 0.05: Ausgewogen (**Standard**)
- 0.07-0.1: Enge, verschlungene Tunnel

**Höhlen-Schwellwert** (`caveThreshold: 0.4-0.7`):
- 0.40-0.50: Sehr viele, große Höhlen
- 0.55: Ausgewogen (**Standard**)
- 0.60-0.70: Wenige, kleine Höhlen

**Minimale Tiefe** (`caveMinDepth: 1-20`):
- Verhindert Löcher direkt an der Oberfläche
- **Standard: 5** Blöcke unter Oberfläche

**3D-Noise mit Stretch**:
```cpp
caveNoise = octaveNoise3D(
    x * caveScale,
    y * caveScale * 1.5,  // Y-Achse gestreckt für organischere Formen
    z * caveScale,
    2 octaves  // Weniger Oktaven für glattere Höhlen
)
```

### 4. Strand-System

Automatische Generierung von Stränden an Wasser-Grenzen.

**Parameter**:
- `generateBeaches: true/false` - Aktiviert Strände
- `waterLevel: -10 bis 10` - Höhe der Wasserlinie (**Standard: 0**)

**Logik**:
- Strände werden 2 Blöcke über und unter Wasserlinie generiert
- Verwendet Dirt-Blöcke (kann später zu Sand erweitert werden)
- Verhindert Gras/Stone direkt am Wasser

### 5. Seed-System

Vollständig reproduzierbare Welten.

**Parameter**: `seed: Integer`
- Initialisiert die Perlin-Permutationstabelle
- Gleicher Seed + gleiche Parameter = identische Welt
- **Standard: 12345**
- "Zufälliger Seed"-Button nutzt aktuelle Zeit

```cpp
// Seed-basierte Permutation
if (seed != 0) {
    std::mt19937 rng(seed)
    for (i = 255 to 0) {
        j = random(0 to i)
     swap(permutation[i], permutation[j])
    }
}
```

## Benutzeroberfläche (UI)

### Collapsing Headers

Die Parameter sind in 5 logische Gruppen unterteilt:

#### 1. Basis-Parameter (Default: Offen)
```
- Größe X/Z:  32-512 Blöcke
- Scale:      0.001-0.1 (kleiner = größere Features)
- Höhen-Multiplikator: 5-100
- Min-Höhe:   -50 bis 0
- Seed:       Integer + "Zufälliger Seed" Button
```

#### 2. Noise-Parameter
```
- Oktaven: 1-8
- Persistence: 0.1-1.0
- Lacunarity:  1.5-4.0
```

#### 3. Terrain-Features
```
- Kontinental-Scale:   0.001-0.05
- Erosion-Scale:    0.01-0.1
- Berg-Scale:          0.005-0.05
- Berg-Schwellwert:    0.3-0.9
- Berg-Höhe:  1.0-5.0
```

#### 4. Höhlen-Parameter
```
- Höhlen generieren:   Checkbox
- Höhlen-Scale:        0.01-0.1
- Höhlen-Schwellwert:  0.4-0.7
- Min-Tiefe:    1-20
```

#### 5. Erweiterte Features
```
- Strände generieren:  Checkbox
- Wasser-Level:        -10 bis 10
- Anzahl Threads:      1-32
```

### Preset-Buttons

5 vordefinierte Landschafts-Typen:

#### **Flaches Land**
```cpp
heightMultiplier = 10.0
mountainThreshold = 0.9  // Fast keine Berge
erosionScale = 0.05// Sanfte Hügel
```
**Einsatz**: Starter-Gebiete, Bauland, Farmen

#### **Hügelland**
```cpp
heightMultiplier = 25.0
mountainThreshold = 0.7
mountainHeightMultiplier = 1.5
```
**Einsatz**: Ausgewogene Landschaft, natürlich aussehend

#### **Gebirge**
```cpp
heightMultiplier = 40.0
mountainThreshold = 0.5  // 50% der Welt sind Berge
mountainHeightMultiplier = 3.5
```
**Einsatz**: Dramatische Landschaften, Bergwanderungen

#### **Extreme Berge**
```cpp
heightMultiplier = 60.0
mountainThreshold = 0.4
mountainHeightMultiplier = 5.0
octaves = 6  // Maximale Details
```
**Einsatz**: Fantasy-Welten, extreme Herausforderungen

#### **Inselwelt**
```cpp
continentalnessScale = 0.005  // Große Ozeane
heightMultiplier = 20.0
waterLevel = 5
generateBeaches = true
```
**Einsatz**: Archipel, Insel-Hopping, Ozean-Navigation

### Generierungs-Workflow

1. **Parameter einstellen** oder **Preset wählen**
2. **"Terrain Generieren"** Button klicken
3. **Fortschritts-Anzeige** beobachten:
   - Progress Bar (0-100%)
   - Status-Text mit aktueller Phase
4. **Warten** (typisch 1-3 Sekunden)
5. **Erkunden** des neuen Terrains

**Hinweis**: Das UI bleibt während der Generierung reaktiv (separater Thread).

## Höhen-Berechnungs-Algorithmus

Vollständiger Algorithmus zur Höhen-Bestimmung:

```cpp
float getTerrainHeight(x, z, config) {
    // 1. BASIS: Multi-Oktaven Noise
    baseHeight = octaveNoise3D(
  x * scale, 0, z * scale,
        octaves, persistence, lacunarity
  )
    
    // 2. KONTINENTALITÄT: Große Landmassen
    continentalness = octaveNoise3D(
        x * continentalnessScale, 1000, z * continentalnessScale,
   2, 0.5, 2.0
    )
    
    // 3. EROSION: Rauheit
    erosion = octaveNoise3D(
        x * erosionScale, 2000, z * erosionScale,
        3, 0.6, 2.0
    )
    
    // 4. BERGE: Dramatische Höhen
    mountainFactor = octaveNoise3D(
        x * mountainScale, 3000, z * mountainScale,
        2, 0.5, 2.0
    )
    
    // 5. KOMBINIERE ALLE FAKTOREN
height = baseHeight
    
    // Kontinentalität beeinflusst Basis (60/40 Mix)
    height = height * 0.6 + continentalness * 0.4
    
    // Erosion addiert Rauheit (±30%)
    height += (erosion - 0.5) * 0.3
    
    // Berge: Nur über Schwellwert
 if (mountainFactor > mountainThreshold) {
        intensity = (mountainFactor - mountainThreshold) / (1.0 - mountainThreshold)
 intensity = intensity * intensity  // Quadratisch = dramatischere Spitzen
        height += intensity * mountainHeightMultiplier
    }
    
    // 6. FINALE HÖHE
    return height * heightMultiplier
}
```

## Block-Typ-Bestimmung

Logik zur Auswahl des Block-Typs basierend auf Position und Features:

```cpp
BlockType getBlockType(y, maxY, caveValue, isMountain, isBeach) {
    // 1. HÖHLEN-CHECK (Priorität 1)
    if (caveValue > 0.6) {
      return Air  // Loch in der Höhle
    }
    
  // 2. STRAND-CHECK (Priorität 2)
    if (isBeach && y >= maxY - 2 && y <= maxY) {
        return Dirt  // Sand-Ersatz (TODO: Sand-Block)
    }
    
    // 3. BERG-CHECK (Priorität 3)
    if (isMountain && y >= maxY - 2 && y <= maxY) {
        return Stone  // Felsige Berggipfel
    }
    
    // 4. STANDARD-SCHICHTEN
    if (y == maxY && maxY > 0) {
    return Grass  // Oberste Schicht
    }
    else if (y > maxY - 4 && y < maxY) {
        return Dirt  // 1-3 Blöcke unter Oberfläche
    }
    else {
        return Stone  // Alles darunter
    }
}
```

## Performance-Optimierung

### Multi-Threading
- Nutzt alle CPU-Kerne automatisch
- Work-Distribution über X-Koordinaten
- Lock-free während Generierung (nur ein Lock am Ende pro Thread)

### Speicher-Management
- Pre-Allokation basierend auf geschätzter Block-Anzahl
- Move-Semantics beim Buffer-Merge
- Minimale Allocations während Generierung

### Batch-Updates
```cpp
world->beginBatchUpdate()    // Deaktiviert Mesh-Updates
// ... setze alle Blöcke ...
world->endBatchUpdate()       // Updated nur dirty Chunks
```

### Typische Performance (256x256 Terrain, 16 Threads)

| Phase          | Zeit      | Anteil |
|---------------------|-----------|--------|
| Block-Generierung   | 500-800ms | 40-50% |
| Block-Platzierung   | 200-400ms | 15-25% |
| Chunk-Update        | 300-600ms | 25-35% |
| **GESAMT**    | **1-2s**  | 100%   |

### Performance-Tipps

**Für schnellste Generierung:**
```
- Terrain-Größe: 128x128 oder kleiner
- Oktaven: 2-3
- Höhlen: Deaktiviert
- Threads: Hardware-Anzahl
```

**Für beste Qualität:**
```
- Terrain-Größe: 512x512
- Oktaven: 5-6
- Höhlen: Aktiviert
- Threads: Hardware-Anzahl * 2
```

## Best Practices

### Realistische Landschaften
```yaml
Scale: 0.02-0.04
Height Multiplier: 25-40
Octaves: 4-5
Persistence: 0.5-0.6
Lacunarity: 2.0-2.2
Mountain Threshold: 0.55-0.65
Mountain Height: 2.0-3.0
Erosion Scale: 0.03-0.05
Cave Threshold: 0.55
```

### Fantasy-Welten (Extreme)
```yaml
Scale: 0.01-0.03
Height Multiplier: 60-100
Octaves: 6-7
Persistence: 0.6-0.7
Lacunarity: 2.5-3.0
Mountain Threshold: 0.4-0.5
Mountain Height: 4.0-5.0
Domain Warp: Hoch
```

### Minecraft-Stil
```yaml
Scale: 0.03-0.05
Height Multiplier: 20-30
Octaves: 4
Persistence: 0.5
Lacunarity: 2.0
Mountain Threshold: 0.65
Caves: Aktiviert
```

### Wüsten-Landschaft
```yaml
Scale: 0.04-0.06
Height Multiplier: 15-25
Octaves: 3-4
Persistence: 0.4
Erosion Scale: 0.02  // Sanfte Dünen
Mountain Threshold: 0.8  // Seltene Felsformationen
```

### Insel-Archipel
```yaml
Continentalness Scale: 0.003-0.007
Height Multiplier: 15-25
Water Level: 3-8
Beaches: True
Mountain Threshold: 0.7  // Bergige Inseln
```

## Troubleshooting

### Problem: Terrain ist zu flach
**Lösungen:**
- ? Erhöhe `heightMultiplier` (30-60)
- ? Erhöhe `mountainHeightMultiplier` (3-5)
- ? Senke `mountainThreshold` (0.4-0.5)
- ? Erhöhe `octaves` (5-6)
- ? Erhöhe `persistence` (0.6-0.7)

### Problem: Terrain ist zu chaotisch/rau
**Lösungen:**
- ? Reduziere `octaves` (2-3)
- ? Reduziere `persistence` (0.3-0.4)
- ? Reduziere `erosionScale` (0.01-0.02)
- ? Erhöhe `scale` für größere Features (0.05-0.1)
- ? Reduziere `lacunarity` (1.5-1.8)

### Problem: Zu viele Höhlen
**Lösungen:**
- ? Erhoehe `caveThreshold` (0.60-0.70)
- ? Erhoehe `caveScale` (0.07-0.1) für kleinere Höhlen
- ? Erhoehe `caveMinDepth` (8-12)

### Problem: Zu wenige Höhlen
**Lösungen:**
- ? Senke `caveThreshold` (0.45-0.50)
- ? Senke `caveScale` (0.03-0.04) für größere Höhlen
- ? Senke `caveMinDepth` (2-4)

### Problem: Zu wenig Variation
**Lösungen:**
- ? Erhöhe `octaves` (5-7)
- ? Erhöhe `lacunarity` (2.5-3.0)
- ? Senke `mountainThreshold` (mehr Berge)
- ? Erhöhe `erosionScale` (mehr Rauheit)
- Kombiniere mehrere Features

### Problem: Zu viele Berge
**Lösungen:**
- ? Erhöhe `mountainThreshold` (0.7-0.8)
- ? Reduziere `mountainHeightMultiplier` (1.5-2.0)
- ? Erhöhe `mountainScale` für seltenere Berge

### Problem: Performance-Probleme
**Lösungen:**
- ? Reduziere Terrain-Größe (128x128)
- ? Reduziere `octaves` (3-4)
- Deaktiviere `generateCaves`
- Setze `numThreads` auf Hardware-Anzahl (nicht höher)
- ? Erhöhe `scale` (weniger Details = schneller)

## Zukünftige Erweiterungen

### In Entwicklung
1. **Biome mit unterschiedlichen Blöcken**
   - Wüsten (Sand, Sandstein)
   - Schnee-Biome (Schnee, Eis)
   - Sümpfe (Wasser, Schlamm)
   - Temperatur-basierte Verteilung

2. **Vegetation**
   - Prozedural generierte Bäume
   - Biom-spezifische Pflanzen
   - Gras, Blumen, Pilze
   - Dichtebasierte Verteilung

3. **Ore-Generierung**
   - Verschiedene Erz-Typen (Kohle, Eisen, Gold, Diamant)
   - Höhen-basierte Verteilung
   - Cluster-Algorithmus für Adern
   - Seltene Erze in Höhlen

4. **Echtes Wasser-System**
   - Wasser-Blöcke unter waterLevel
   - Flüsse basierend auf Erosion
   - Seen in Senken
   - Wasserfall-Generierung

5. **Verbesserte Höhlen**
   - Große Kavernen mit hohen Decken
   - Verbundene Höhlen-Systeme
   - Unterirdische Seen/Lava
   - Stalaktiten/Stalagmiten

6. **Strukturen**
   - Ruinen, Häuser, Burgen
   - Template-basierte Platzierung
   - Dorf-Generierung
   - Dungeons in Höhlen

### Geplante Features
- **Dynamic LOD**: Terrain-Vereinfachung in Ferne
- **Chunk-Streaming**: Unendliche Welten
- **Biom-Übergänge**: Sanfte Blends zwischen Biomen
- **Klima-System**: Temperatur, Feuchtigkeit
- **Wetter**: Regen, Schnee basierend auf Höhe/Biom

## Technische Referenz

### Perlin.h - Neue Funktionen
```cpp
class Perlin {
 Perlin(int seed = 0)  // Konstruktor mit Seed
    
    void initializePermutation(int seed)  // Seed-basierte Init
    
    float noise3D(x, y, z)  // Basis Perlin Noise
    
    float octaveNoise3D(x, y, z, octaves, persistence, lacunarity)
    // Multi-Oktaven FBM
}
```

### TerrainConfig - Alle Parameter
```cpp
struct TerrainConfig {
    // Basis
    int sizeX, sizeZ
    float scale
    float heightMultiplier
    int minHeight
    int seed
    int numThreads
    
    // Noise
    int octaves
  float persistence
    float lacunarity
    
    // Biome
    float continentalnessScale
    float erosionScale
    float mountainScale
 float mountainThreshold
    float mountainHeightMultiplier
    
    // Höhlen
    bool generateCaves
    float caveScale
    float caveThreshold
    int caveMinDepth
    
    // Features
    bool generateBeaches
    int waterLevel
}
```

### API-Verwendung
```cpp
// Erstelle Generator
TerrainGenerator* gen = new TerrainGenerator()

// Konfiguriere
TerrainConfig config
config.sizeX = 256
config.seed = 12345
// ... weitere Parameter ...

// Generiere mit Callback
auto callback = [](float progress, string msg) {
    cout << progress * 100 << "% - " << msg << endl
}

gen->generateTerrainParallel(voxelWorld, config, callback)

// Hole aktuellen Seed
int currentSeed = gen->getCurrentSeed()
```

## Fazit

Das erweiterte Terrain-System bietet:
- ? **Massive Varianz** durch Multi-Oktaven Noise
- ? **Realistische Biome** durch kombinierte Noise-Layer
- ? **Dramatische Berge** mit quadratischer Intensitäts-Kurve
- ? **Organische Höhlen** mit 3D-Noise
- ? **Reproduzierbarkeit** durch Seed-System
- ? **Hohe Performance** durch Multi-Threading
- ? **Intuitive UI** mit Presets und Kategorien
- ? **Echtzeit-Regenerierung** im separaten Thread

Experimentiere mit verschiedenen Parameter-Kombinationen für einzigartige Welten!
