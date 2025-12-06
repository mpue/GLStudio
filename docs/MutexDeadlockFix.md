# Mutex Deadlock Fix - VoxelWorld

## Problem
Beim Hinzufügen neuer Blöcke trat ein **Deadlock** auf, der zu einem Absturz führte.

### Ursache
Die Methode `VoxelWorld::setBlock()` hatte folgenden Ablauf:
1. Erwarb einen Lock auf `chunkMutex` 
2. Rief `updateChunkMesh()` auf, während der Lock gehalten wurde
3. `updateChunkMesh()` versuchte, den gleichen `chunkMutex` erneut zu locken
4. **Deadlock**: Ein Thread kann nicht den gleichen non-recursive Mutex zweimal locken

### Stack Trace
```
std::_Mutex_base::lock()
std::lock_guard<std::mutex>::lock_guard<std::mutex>()
VoxelWorld::updateChunkMesh()
VoxelWorld::setBlock()
mouse_button_callback()
```

## Lösung
Die Lösung bestand darin, eine **interne Version** der `updateChunkMesh()` Methode zu erstellen, die keine Locks verwendet:

### Änderungen in `VoxelWorld.h`
```cpp
private:
    // Internal unlocked versions (assumes caller holds lock)
    void updateChunkMeshInternal(int chunkX, int chunkY, int chunkZ);
```

### Änderungen in `VoxelWorld.cpp`

#### Öffentliche Methode (mit Lock)
```cpp
void VoxelWorld::updateChunkMesh(int chunkX, int chunkY, int chunkZ) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    updateChunkMeshInternal(chunkX, chunkY, chunkZ);
}
```

#### Interne Methode (ohne Lock)
```cpp
void VoxelWorld::updateChunkMeshInternal(int chunkX, int chunkY, int chunkZ) {
    // Internal version - assumes caller holds chunkMutex lock!
    // ... original implementation ...
}
```

#### Verwendung in setBlock()
```cpp
void VoxelWorld::setBlock(...) {
    std::lock_guard<std::mutex> lock(chunkMutex);
 
    // ... block setting code ...
  
    // Use internal version that doesn't lock (we already hold the lock)
    updateChunkMeshInternal(chunkCoord.x, chunkCoord.y, chunkCoord.z);
    
    // Update neighbor chunks also with internal version
    if (localCoord.x == 0) {
        updateChunkMeshInternal(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z);
    }
    // ... etc ...
}
```

## Vorteile dieser Lösung
1. **Keine Deadlocks**: Der Lock wird nur einmal erworben
2. **Bessere Performance**: Weniger Lock-Overhead
3. **Klare Trennung**: Öffentliche API (mit Locks) vs. interne API (ohne Locks)
4. **Best Practice**: Vermeidet recursive mutex (die Performance-Nachteile haben)

## Alternative Ansätze (nicht gewählt)
- **std::recursive_mutex**: Würde funktionieren, hat aber Performance-Nachteile und ist generell schlechte Praxis
- **Lock Release**: Lock freigeben vor updateChunkMesh() - würde zu Race Conditions führen

## Testing
Nach dem Fix:
- ? Build erfolgreich kompiliert
- ? Keine Compiler-Fehler
- ? Thread-Safety bleibt gewahrt
- ?? Bitte testen: Blöcke hinzufügen/entfernen sollte ohne Crash funktionieren
