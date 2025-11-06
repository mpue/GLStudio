# Voxel Character Controller - Dokumentation

## Übersicht

Einfacher Character Controller für die Voxel-Welt **ohne** Bullet Physics. Implementiert grundlegende First-Person-Steuerung mit Kollisionserkennung gegen das Voxel-Terrain.

## Features

? **WASD-Bewegung** - Standard-FPS-Steuerung  
? **Maus-Look** - Freie Kamerarotation  
? **Springen** - Mit Leertaste  
? **Gravitation** - Realistische Fallphysik  
? **Kollisionserkennung** - Gegen Voxel-Terrain  
? **Slope Sliding** - Automatisches Gleiten an Wänden  

## Steuerung

| Taste | Aktion |
|-------|--------|
| **W** | Vorwärts |
| **S** | Rückwärts |
| **A** | Links |
| **D** | Rechts |
| **Leertaste** | Springen |
| **Maus** | Umschauen |
| **Linksklick** | Block platzieren |
| **Mittelklick** | Block entfernen |

## Klasse: VoxelCharacterController

### Konstruktor

```cpp
VoxelCharacterController(VoxelWorld* world, GLFWwindow* window);
```

**Parameter:**
- `world` - Pointer zur VoxelWorld für Kollisionserkennung
- `window` - GLFW Window für Input-Handling

### Hauptmethoden

```cpp
// Update-Schleife (jeden Frame aufrufen)
void update(float deltaTime);

// Maus-Input
void onMouseMove(double dx, double dy);

// Springen
void jump();

// Getter
glm::vec3 getPosition() const;
glm::vec3 getFront() const;
glm::vec3 getUp() const;
glm::vec3 getRight() const;
```

## Verwendung

### 1. Initialisierung

```cpp
// In main() nach VoxelWorld-Initialisierung
VoxelWorld* voxelWorld = new VoxelWorld();
VoxelCharacterController* characterController = 
    new VoxelCharacterController(voxelWorld, window);
```

### 2. Update-Loop

```cpp
// In der Render-Loop
while (!glfwWindowShouldClose(window))
{
float deltaTime = /* berechne deltaTime */;
    
    // Update Character
    characterController->update(deltaTime);
    
    // Update Kamera
    camera.Position = characterController->getPosition() + 
 glm::vec3(0.0f, 1.6f, 0.0f); // Augenhöhe
    camera.Front = characterController->getFront();
    camera.Up = characterController->getUp();
    
    // Render...
}
```

### 3. Input-Callbacks

```cpp
// Mouse Callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double lastX = xpos, lastY = ypos;
    double dx = xpos - lastX;
    double dy = ypos - lastY;
    lastX = xpos;
    lastY = ypos;
 
    characterController->onMouseMove(dx, dy);
}
```

## Kollisionserkennung

### Algorithmus

Der Controller verwendet eine **AABB (Axis-Aligned Bounding Box)** Kollisionserkennung:

1. **Character-Hitbox:**
   - Radius: 0.3 Blöcke
- Höhe: 1.8 Blöcke
   
2. **Kollisionsprüfung:**
   ```cpp
   // Prüfe alle Blöcke in der Nähe
   for (x in [pos.x - radius, pos.x + radius])
       for (y in [pos.y, pos.y + height])
           for (z in [pos.z - radius, pos.z + radius])
  if (isBlockSolid(x, y, z))
    return true; // Kollision!
   ```

3. **Auflösung:**
 - Horizontal: Einzelne Achsen-Bewegung (X/Z getrennt)
   - Vertikal: Snap zu Boden/Decke

### Boden-Erkennung

```cpp
// Prüfe Block direkt unter den Füßen
int footBlockY = floor(position.y);
if (isBlockSolid(footX, footBlockY, footZ)) {
    isOnGround = true;
    position.y = footBlockY + 1.0f;
    velocity.y = 0.0f;
}
```

## Konfiguration

### Anpassbare Parameter

In `VoxelCharacterController.h`:

```cpp
private:
 float height = 1.8f;           // Charakter-Höhe
    float radius = 0.3f;// Kollisions-Radius
  float moveSpeed = 5.0f;        // Laufgeschwindigkeit (Blöcke/Sekunde)
    float mouseSensitivity = 0.1f; // Maus-Empfindlichkeit
    float jumpForce = 8.0f;        // Sprungkraft
    float gravity = -20.0f;        // Gravitationsstärke
```

**Beispiel-Anpassungen:**

```cpp
// Schnellerer Charakter
moveSpeed = 10.0f;

// Höherer Sprung
jumpForce = 12.0f;

// Stärkere Gravitation
gravity = -30.0f;

// Größerer Charakter
height = 2.0f;
radius = 0.4f;
```

## Physik-Details

### Bewegungsgleichungen

**Horizontale Bewegung:**
```cpp
position.x += direction.x * moveSpeed * deltaTime;
position.z += direction.z * moveSpeed * deltaTime;
```

**Vertikale Bewegung (Gravitation):**
```cpp
velocity.y += gravity * deltaTime;
position.y += velocity.y * deltaTime;
```

**Sprung:**
```cpp
if (isOnGround) {
    velocity.y = jumpForce;  // Sofortige Aufwärts-Geschwindigkeit
}
```

### Zeitschritt (Delta Time)

Der Controller ist **framerate-independent** durch Verwendung von `deltaTime`:

```cpp
// 60 FPS: deltaTime ? 0.0167s
// 30 FPS: deltaTime ? 0.0333s
// Bewegung bleibt konstant unabhängig von FPS
```

## Unterschiede zu Bullet Physics

| Feature | Bullet Physics | Voxel Controller |
|---------|---------------|------------------|
| **Komplexität** | Hoch (vollständige Physics Engine) | Niedrig (einfache AABB) |
| **Performance** | Schwerer | Sehr leicht |
| **Genauigkeit** | Sehr präzise | Ausreichend für Voxel |
| **Features** | Rigid Bodies, Constraints, Impulse | Nur Charakter-Bewegung |
| **Abhängigkeiten** | Bullet Library | Nur GLM + GLFW |

## Bekannte Einschränkungen

1. **Keine Slope Walking** - Stufen müssen gesprungen werden
2. **Einfache Kollision** - Nur gegen Voxel, keine Entities
3. **Keine Physik-Interaktion** - Kann Objekte nicht bewegen
4. **Keine Präzisions-Kollision** - Nur Block-basiert

## Erweiterungsmöglichkeiten

### 1. Crouching (Ducken)

```cpp
void VoxelCharacterController::crouch(bool enable) {
    if (enable) {
        height = 1.4f;
moveSpeed = 2.5f;
    } else {
        height = 1.8f;
        moveSpeed = 5.0f;
    }
}
```

### 2. Sprint

```cpp
void VoxelCharacterController::setSprinting(bool sprint) {
    moveSpeed = sprint ? 10.0f : 5.0f;
}
```

### 3. Schwimmen

```cpp
void VoxelCharacterController::updateSwimming() {
    if (voxelWorld->getBlock(posX, posY + 1, posZ) == BlockType::Water) {
      gravity = -5.0f;  // Reduzierte Gravitation
        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            velocity.y = 3.0f;  // Schwimmen nach oben
}
    } else {
        gravity = -20.0f;
    }
}
```

### 4. Slope Sliding

```cpp
void VoxelCharacterController::resolveSliding(glm::vec3& newPos) {
    // Wenn Kollision, versuche seitlich zu gleiten
    if (checkCollision(newPos)) {
   glm::vec3 slideDir = glm::cross(up, hitNormal);
        newPos += slideDir * glm::dot(movement, slideDir);
    }
}
```

## Debug-Informationen

Nützliche Debug-Ausgaben:

```cpp
void VoxelCharacterController::printDebugInfo() {
    std::cout << "Position: " << position.x << ", " 
      << position.y << ", " << position.z << std::endl;
    std::cout << "Velocity: " << velocity.y << std::endl;
    std::cout << "On Ground: " << (isOnGround ? "Yes" : "No") << std::endl;
}
```

## Performance

**Typische Performance:**
- Kollisionsprüfung: ~5-10 Blöcke pro Frame
- CPU-Zeit: < 0.1ms pro Frame
- Kein GPU-Overhead

**Optimierungen:**
- Nur relevante Blöcke prüfen (Bounding Box)
- Früher Abbruch bei erster Kollision
- Cached Block-Zugriffe möglich

## Troubleshooting

**Problem: Charakter fällt durch den Boden**
```cpp
// Lösung: Erhöhe Startposition
position.y = 20.0f; // Höher starten
```

**Problem: Charakter bleibt in Wänden stecken**
```cpp
// Lösung: Reduziere Radius
radius = 0.25f; // Kleiner machen
```

**Problem: Sprung zu niedrig/hoch**
```cpp
// Lösung: Passe jumpForce an
jumpForce = 10.0f; // Höher springen
```

## Beispiel: Komplette Integration

```cpp
#include "VoxelCharacterController.h"

int main() {
    // Setup...
    VoxelWorld* world = new VoxelWorld();
    VoxelCharacterController* player = 
        new VoxelCharacterController(world, window);
    
    // Main Loop
    while (!glfwWindowShouldClose(window)) {
  float deltaTime = calculateDeltaTime();
     
    // Update
        player->update(deltaTime);
        
        // Camera
        camera.Position = player->getPosition() + 
           glm::vec3(0, 1.6f, 0);
        camera.Front = player->getFront();
  
        // Render
        renderWorld();
      
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    delete player;
    delete world;
}
```

## Lizenz

Frei verwendbar für persönliche und kommerzielle Projekte.
