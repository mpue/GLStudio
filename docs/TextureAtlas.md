# Textur-Atlas Layout - Dokumentation

## 4x4 Textur-Atlas (1024x1024px)

### Atlas-Struktur
- **Gesamtgröße**: 1024x1024 Pixel
- **Grid**: 4x4 Texturen
- **Tile-Größe**: 256x256 Pixel pro Textur
- **UV-Bereich pro Tile**: 0.25 x 0.25 (1/4)

---

## ??? Atlas-Layout

```
     [0]    [1]    [2]    [3]  <- X-Koordinate
    ?????????????????????????????
[0] ? GRASS? DIRT ?STONE ? WOOD ? <- Reihe 0: TOP-Texturen
    ? TOP  ?      ?      ? TOP  ?
    ?????????????????????????????
[1] ??SIDE?? ???  ? ???  ? ???  ? <- Reihe 1: SEITEN-Texturen
    ? ALL  ??      ?  ?    (Alle Seiten verwenden [0,1])
    ?????????????????????????????
[2] ? SAND ?WATER ? ???  ? ???  ? <- Reihe 2: Weitere Blöcke
    ?      ?      ?      ?      ?
    ?????????????????????????????
[3] ? ???  ? ???? ???  ? ???  ? <- Reihe 3: Reserve
    ?  ?      ?      ?      ?
    ?????????????????????????????
     Y-Koordinate (vertikal)
```

---

## ?? Block-Typ Mapping

### **Grass (BlockType::Grass)**
| Face | Atlas Position | UV-Koordinaten |
|------|---------------|----------------|
| **Top** | [0, 0] | (0.00-0.25, 0.00-0.25) |
| **Bottom** | [1, 0] | (0.25-0.50, 0.00-0.25) |
| **Sides** (N/S/E/W) | **[0, 1]** | **(0.00-0.25, 0.25-0.50)** |

### **Stone (BlockType::Stone)**
| Face | Atlas Position | UV-Koordinaten |
|------|---------------|----------------|
| **All** | [2, 0] | (0.50-0.75, 0.00-0.25) |
| **Sides** | **[0, 1]** | **(0.00-0.25, 0.25-0.50)** |

### **Dirt (BlockType::Dirt)**
| Face | Atlas Position | UV-Koordinaten |
|------|---------------|----------------|
| **All** | [1, 0] | (0.25-0.50, 0.00-0.25) |
| **Sides** | **[0, 1]** | **(0.00-0.25, 0.25-0.50)** |

### **Wood (BlockType::Wood)**
| Face | Atlas Position | UV-Koordinaten |
|------|---------------|----------------|
| **Top/Bottom** | [3, 0] | (0.75-1.00, 0.00-0.25) |
| **Sides** | **[0, 1]** | **(0.00-0.25, 0.25-0.50)** |

### **Sand (BlockType::Sand)**
| Face | Atlas Position | UV-Koordinaten |
|------|---------------|----------------|
| **All** | [0, 2] | (0.00-0.25, 0.50-0.75) |
| **Sides** | **[0, 1]** | **(0.00-0.25, 0.25-0.50)** |

### **Water (BlockType::Water)**
| Face | Atlas Position | UV-Koordinaten |
|------|---------------|----------------|
| **All** | [1, 2] | (0.25-0.50, 0.50-0.75) |
| **Sides** | **[0, 1]** | **(0.00-0.25, 0.25-0.50)** |

---

## ?? Wichtige Regel

### **ALLE Seiten-Faces verwenden [0, 1]**

```cpp
// Im Code:
if (isSideFace) {  // North, South, East, West
    atlasX = 0;
    atlasY = 1;  // <- IMMER Reihe 1, Position 0
}
```

**Seiten-Faces sind:**
- ? **North** (FaceDirection::North)
- ? **South** (FaceDirection::South)
- ? **East** (FaceDirection::East)
- ? **West** (FaceDirection::West)

**NICHT Seiten-Faces:**
- ? **Top** (FaceDirection::Top)
- ? **Bottom** (FaceDirection::Bottom)

---

## ?? UV-Koordinaten Berechnung

### Formel
```cpp
float tileSize = 1.0f / 4.0f;  // 0.25 für 4x4 Grid
float uMin = atlasX * tileSize;
float vMin = atlasY * tileSize;
float uMax = uMin + tileSize;
float vMax = vMin + tileSize;
```

### Pixel-zu-UV Umrechnung
- **1 Tile** = 256x256 Pixel
- **UV pro Pixel** = 1.0 / 1024 = 0.0009765625
- **UV pro Tile** = 256 / 1024 = 0.25

### Vollständige Atlas-Tabelle

| Position | Atlas-Koordinaten | UV-Bereich | Pixel-Bereich (1024x1024) |
|----------|-------------------|------------|---------------------------|
| **[0, 0]** | (0, 0) | (0.00-0.25, 0.00-0.25) | (0-256, 0-256) |
| **[1, 0]** | (1, 0) | (0.25-0.50, 0.00-0.25) | (256-512, 0-256) |
| **[2, 0]** | (2, 0) | (0.50-0.75, 0.00-0.25) | (512-768, 0-256) |
| **[3, 0]** | (3, 0) | (0.75-1.00, 0.00-0.25) | (768-1024, 0-256) |
| **[0, 1]** | (0, 1) | **(0.00-0.25, 0.25-0.50)** | **(0-256, 256-512)** ? **SEITEN** |
| **[1, 1]** | (1, 1) | (0.25-0.50, 0.25-0.50) | (256-512, 256-512) |
| **[2, 1]** | (2, 1) | (0.50-0.75, 0.25-0.50) | (512-768, 256-512) |
| **[3, 1]** | (3, 1) | (0.75-1.00, 0.25-0.50) | (768-1024, 256-512) |
| **[0, 2]** | (0, 2) | (0.00-0.25, 0.50-0.75) | (0-256, 512-768) |
| **[1, 2]** | (1, 2) | (0.25-0.50, 0.50-0.75) | (256-512, 512-768) |
| **[2, 2]** | (2, 2) | (0.50-0.75, 0.50-0.75) | (512-768, 512-768) |
| **[3, 2]** | (3, 2) | (0.75-1.00, 0.50-0.75) | (768-1024, 512-768) |
| **[0, 3]** | (0, 3) | (0.00-0.25, 0.75-1.00) | (0-256, 768-1024) |
| **[1, 3]** | (1, 3) | (0.25-0.50, 0.75-1.00) | (256-512, 768-1024) |
| **[2, 3]** | (2, 3) | (0.50-0.75, 0.75-1.00) | (512-768, 768-1024) |
| **[3, 3]** | (3, 3) | (0.75-1.00, 0.75-1.00) | (768-1024, 768-1024) |

### Beispiele

**Position [0, 0] (Gras-Top):**
```
uMin = 0 * 0.25 = 0.00
vMin = 0 * 0.25 = 0.00
uMax = 0.00 + 0.25 = 0.25
vMax = 0.00 + 0.25 = 0.25
? UV-Bereich: (0.00-0.25, 0.00-0.25)
? Pixel-Bereich: (0-256, 0-256)
```

**Position [0, 1] (ALLE Seiten):**
```
uMin = 0 * 0.25 = 0.00
vMin = 1 * 0.25 = 0.25
uMax = 0.00 + 0.25 = 0.25
vMax = 0.25 + 0.25 = 0.50
? UV-Bereich: (0.00-0.25, 0.25-0.50)
? Pixel-Bereich: (0-256, 256-512)
```

**Position [2, 0] (Stein):**
```
uMin = 2 * 0.25 = 0.50
vMin = 0 * 0.25 = 0.00
uMax = 0.50 + 0.25 = 0.75
vMax = 0.00 + 0.25 = 0.25
? UV-Bereich: (0.50-0.75, 0.00-0.25)
? Pixel-Bereich: (512-768, 0-256)
```

---

## ??? Textur-Atlas Erstellen

### Photoshop/GIMP Layout

```
??????????????????????????????????????????
? 0,0     256,0    512,0    768,0   ? Y=0
? GRASS   DIRT     STONE    WOOD-TOP    ?
?  TOP              ?
??????????????????????????????????????????
? 0,256   256,256  512,256  768,256     ? Y=256
??SIDES?  ???      ???      ???         ?
?  ALL    (WICHTIG!)        ?
??????????????????????????????????????????
? 0,512   256,512  512,512  768,512  ? Y=512
? SAND    WATER    ???      ???       ?
?              ?
??????????????????????????????????????????
? 0,768   256,768  512,768  768,768     ? Y=768
? ???     ???    ???      ???         ?
?             ?
??????????????????????????????????????????
 X=0     X=256    X=512  X=768
```

### Wichtig beim Erstellen:
1. **Gesamtgröße**: Exakt 1024x1024 Pixel
2. **Tile-Grid**: 4x4 mit je 256x256 Pixel
3. **Position [0,1]**: Muss die Seiten-Textur enthalten
4. **Keine Lücken**: Tiles müssen nahtlos aneinander grenzen
5. **Power of 2**: 1024 = 2¹? (GPU-optimal)

---

## ?? Anpassung im Code

### Neues Block-Typ hinzufügen

```cpp
// In VoxelChunk.cpp, in addFace():
case BlockType::MyNewBlock:
    if (direction == FaceDirection::Top) {
        atlasX = 3;  // Spalte 3
        atlasY = 2;  // Reihe 2
    } else if (direction == FaceDirection::Bottom) {
      atlasX = 2;
      atlasY = 3;
    }
    // Seiten werden automatisch auf [0,1] gesetzt!
    break;
```

### Atlas-Position ändern

```cpp
// Seiten-Position ändern (aktuell [0,1]):
if (isSideFace) {
    atlasX = 1;  // Neue Spalte
    atlasY = 1;  // Neue Reihe
}
```

---

## ?? Performance-Vorteile

### Textur-Atlas vs. Einzeltexturen

| Aspekt | Einzeltexturen | Textur-Atlas |
|--------|---------------|--------------|
| **Texture Binds** | 1 pro Block-Typ | 1 für alle |
| **Draw Calls** | Viele | Wenige |
| **GPU Memory** | Fragmentiert | Zusammenhängend |
| **Batch-Rendering** | Schwierig | Optimal |
| **FPS-Einfluss** | Negativ | Positiv |

### Warum 4x4 mit 256x256px Tiles?
- ? **16 verschiedene Texturen** ausreichend für Minecraft-Style
- ? **1024x1024px** = Power-of-2 (GPU-optimal)
- ? **256x256px pro Tile** = hohe Detailstufe, scharfe Texturen
- ? **Einfache Berechnung** (Division durch 4)
- ? **Moderne GPUs** verarbeiten 1024x1024 ohne Probleme

---

## ?? Beispiel-Atlas-Inhalt

### Vorgeschlagenes Layout:

```
Reihe 0 (Top-Texturen):
[0,0] Gras-Top (grün)
[1,0] Erde (braun)
[2,0] Stein (grau)
[3,0] Holz-Ringe (braun mit Ringen)

Reihe 1 (Seiten-Texturen):
[0,1] ? Universal-Seiten-Textur ? (WICHTIG!)
[1,1] Alternative Seiten
[2,1] Reserve
[3,1] Reserve

Reihe 2 (Weitere Blöcke):
[0,2] Sand (beige)
[1,2] Wasser (blau)
[2,2] Kies
[3,2] Lehm

Reihe 3 (Reserve):
[0,3] Glas
[1,3] Laub
[2,3] Erz
[3,3] Ziegel
```

---

## ?? Debugging

### Problem: Falsche Texturen
```cpp
// Überprüfe Atlas-Koordinaten:
std::cout << "Block: " << (int)type 
       << " Face: " << (int)direction 
        << " Atlas: [" << atlasX << "," << atlasY << "]"
          << " UV: (" << uMin << "," << vMin 
          << ") - (" << uMax << "," << vMax << ")" 
     << std::endl;
```

### Problem: Textur-Bleeding
- **Ursache**: Mipmapping zieht Nachbar-Tiles
- **Lösung**: Padding zwischen Tiles oder Clamp-to-Edge

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

### Problem: Unscharfe Texturen
```cpp
// Deaktiviere Mipmaps für pixel-perfektes Rendering:
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
```

---

## ?? Zusammenfassung

### Was wurde implementiert:
? **4x4 Textur-Atlas** (256x256px)  
? **64x64px pro Tile**  
? **Alle Seiten-Faces verwenden [0,1]**  
? **Top/Bottom-Faces haben eigene Texturen**  
? **Automatische UV-Berechnung**  

### Nächste Schritte:
1. **Textur-Atlas-Datei erstellen** (256x256px PNG)
2. **Textur in Position [0,1] platzieren** (für Seiten)
3. **Textur im Spiel laden**
4. **Optional**: Weitere Block-Typen hinzufügen

---

## ?? Verwendete Dateien

- `src/VoxelChunk.h` - Atlas-Konfiguration
- `src/VoxelChunk.cpp` - UV-Mapping-Logik
- `shaders/voxel.frag` - Textur-Sampling

---

**Letzte Aktualisierung**: Implementiert in VoxelChunk v2.0
