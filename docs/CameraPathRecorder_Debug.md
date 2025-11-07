# Camera Path Recorder - Debug Guide

## Problem: Playback funktioniert nicht

### Mögliche Ursachen und Lösungen

#### 1. ? **BEHOBEN: Character Controller überschreibt Kamera während Playback**

**Problem:** Der Character Controller aktualisierte die Kamera-Position auch während des Playbacks.

**Lösung:** 
- Character Controller Update wird während Playback übersprungen
- Kamera-Position wird nur vom Playback-System gesetzt

```cpp
// In GLStudio.cpp - Update Loop
if (!cameraPathRecorder || !cameraPathRecorder->isPlaying()) {
    characterController->update(deltaTime);  // Nur wenn nicht im Playback
}

if (cameraPathRecorder->isPlaying()) {
    // Playback setzt Kamera direkt
    cameraPathRecorder->updatePlayback(...);
}
```

#### 2. ? **BEHOBEN: Doppelte Kamera-Position Updates**

**Problem:** Kamera wurde mehrfach pro Frame aktualisiert.

**Lösung:** Klare Trennung zwischen Controller- und Playback-Modus.

```cpp
// Nur EINE Kamera-Update-Stelle
if (!cameraPathRecorder || !cameraPathRecorder->isPlaying()) {
// Character Controller Mode
    camera.Position = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
    camera.Front = characterController->getFront();
    camera.Up = characterController->getUp();
}
// Playback setzt camera.Position/Front/Up bereits in updatePlayback()
```

#### 3. ? **BEHOBEN: Recording Duration Anzeige**

**Problem:** Während Recording wurde keine Zeit angezeigt.

**Lösung:** `getRecordingDuration()` zeigt jetzt `recordingTime` während Recording.

---

## Test-Schritte

### Basis-Funktionalität testen

1. **Recording starten:**
```
1. Starte Anwendung
2. Drücke [F] für Free Fly Mode
3. Drücke [Rechte Maustaste] gedrückt halten
4. Im UI: Klicke "Start Recording"
5. ? Status sollte "Recording" anzeigen
6. ? "REC" Indikator sollte rot leuchten
7. ? Zeit sollte hochzählen (0.00s, 0.03s, 0.06s, ...)
```

2. **Kamera bewegen:**
```
Während REC läuft:
- [W][A][S][D] = Bewegung
- [Space] = Hoch
- [Shift] = Runter
- Maus = Rotation (rechte Maustaste gedrückt!)
- ? Keyframes sollten sich erhöhen
```

3. **Recording stoppen:**
```
1. Klicke "Stop Recording"
2. ? Status sollte "Idle" anzeigen
3. ? Keyframes und Duration sollten angezeigt werden
```

4. **Playback starten:**
```
1. Klicke "Play"
2. ? Status sollte "Playing" anzeigen
3. ? Progress Bar sollte sich bewegen
4. ? Kamera sollte aufgezeichneten Pfad abfahren
5. ? Steuerung sollte NICHT funktionieren (Playback hat Kontrolle)
```

5. **Playback beenden:**
```
- Option A: Warte bis Ende ? Auto-Stop
- Option B: Klicke "Stop Playback" ? Manueller Stop
? Status sollte wieder "Idle" sein
? Steuerung sollte wieder funktionieren
```

---

## Debugging-Tipps

### Console-Output beobachten

Das System gibt hilfreiche Debug-Meldungen aus:

```
Camera path recording started         ? Recording begonnen
Camera path recording stopped. 1800 keyframes recorded over 60.0 seconds
Camera path playback started     ? Playback begonnen
Camera path playback stopped  ? Playback beendet
```

### Keyframe-Anzahl prüfen

**Problem:** Zu wenige Keyframes
```
Recording Rate: 30 fps
Recording Time: 10 Sekunden
Erwartete Keyframes: ~300

Wenn viel weniger:
- Free Fly Mode aktiv? (F-Taste)
- Rechte Maustaste gedrückt?
- Character Controller wird updated?
```

**Lösung:**
```cpp
// In GLStudio.cpp
if (cameraPathRecorder->isRecording() && characterController->isFreeFlyMode()) {
    // Dies muss TRUE sein während Recording!
    glm::vec3 camPos = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
    cameraPathRecorder->updateRecording(...);
}
```

### Playback läuft nicht

**Checkliste:**
1. ? Keyframes vorhanden? (Mindestens 2)
2. ? Duration > 0?
3. ? State == Playing?
4. ? updatePlayback() return true?

**Debug in Console:**
```cpp
// Temporär in GLStudio.cpp hinzufügen
if (cameraPathRecorder->isPlaying()) {
    std::cout << "Playback Time: " << cameraPathRecorder->getPlaybackTime() 
           << " / " << cameraPathRecorder->getRecordingDuration() << std::endl;
}
```

### Kamera bewegt sich nicht

**Mögliche Ursachen:**

1. **Character Controller überschreibt:**
```cpp
// FALSCH - Controller läuft während Playback
characterController->update(deltaTime);

// RICHTIG - Controller gestoppt während Playback
if (!cameraPathRecorder->isPlaying()) {
    characterController->update(deltaTime);
}
```

2. **Kamera wird doppelt gesetzt:**
```cpp
// FALSCH - Kamera wird nach Playback überschrieben
cameraPathRecorder->updatePlayback(...);
camera.Position = characterController->getPosition(); // ? Überschreibt Playback!

// RICHTIG - Nur eine Kamera-Quelle
if (cameraPathRecorder->isPlaying()) {
    cameraPathRecorder->updatePlayback(...); // Setzt camera.Position
} else {
    camera.Position = characterController->getPosition(); // Nur wenn nicht im Playback
}
```

3. **Interpolation fehlerhaft:**
```cpp
// Prüfe in CameraPathRecorder::findKeyframeIndexAtTime()
// Sollte Keyframe-Index zurückgeben, nicht -1

// Debug-Output in updatePlayback():
std::cout << "Next Index: " << nextIndex << " at time " << playbackTime << std::endl;
```

---

## Performance-Checks

### Recording Performance

**30 fps Recording Rate:**
- CPU-Last: Minimal (~0.1%)
- Memory pro Sekunde: ~1.2 KB (30 keyframes × 40 bytes)
- 60 Sekunden = ~70 KB

**120 fps Recording Rate:**
- CPU-Last: Minimal (~0.3%)
- Memory pro Sekunde: ~4.8 KB
- 60 Sekunden = ~280 KB

### Playback Performance

**Optimiert:**
- Binäre Suche: O(log n)
- Interpolation: O(1)
- Keine GPU-Operationen
- ~0.01 ms pro Frame

---

## Erweiterte Tests

### Test 1: Schnelle Kamera-Bewegung
```
1. Recording starten
2. Schnell durch Szene fliegen
3. Recording stoppen
4. Playback mit 0.5x Speed
? Bewegung sollte smooth sein
```

### Test 2: Looping
```
1. Recording machen (10 Sekunden)
2. "Loop Playback" aktivieren
3. Playback starten
? Sollte endlos wiederholen
? Kein Ruckeln am Loop-Punkt
```

### Test 3: Variable Speed
```
1. Recording machen
2. Playback mit Speed = 0.1x (Zeitlupe)
3. Playback mit Speed = 5.0x (Schnelldurchlauf)
? Interpolation sollte smooth bleiben
```

### Test 4: Speichern/Laden
```
1. Recording machen
2. "Save Path" ? Datei speichern
3. Anwendung NEU starten
4. "Load Path" ? Datei laden
5. Playback starten
? Sollte exakt gleiche Bewegung sein
```

---

## Known Issues (Gelöst)

### ? Issue #1: Kamera springt nach Playback
**Status:** BEHOBEN  
**Lösung:** Character Controller wird während Playback nicht mehr updated.

### ? Issue #2: Recording zeigt keine Zeit
**Status:** BEHOBEN  
**Lösung:** `getRecordingDuration()` gibt jetzt `recordingTime` während Recording zurück.

### ? Issue #3: Playback überspringt Frames
**Status:** BEHOBEN  
**Lösung:** Korrekte Interpolation zwischen Keyframes implementiert.

---

## Code-Checklist für Entwickler

Wenn Playback nicht funktioniert, prüfe diese Code-Stellen:

### GLStudio.cpp - Update Loop
```cpp
// ? Character Controller nur wenn NICHT im Playback
if (!cameraPathRecorder || !cameraPathRecorder->isPlaying()) {
characterController->update(deltaTime);
}

// ? Playback Update
if (cameraPathRecorder && cameraPathRecorder->isPlaying()) {
    glm::vec3 playbackPos, playbackFront, playbackUp;
    if (cameraPathRecorder->updatePlayback(playbackPos, playbackFront, playbackUp, deltaTime)) {
 camera.Position = playbackPos;
camera.Front = playbackFront;
        camera.Up = playbackUp;
    }
}

// ? Kamera Update nur wenn NICHT im Playback
if (!cameraPathRecorder || !cameraPathRecorder->isPlaying()) {
    camera.Position = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
    camera.Front = characterController->getFront();
    camera.Up = characterController->getUp();
}
```

### CameraPathRecorder.cpp - updatePlayback()
```cpp
// ? Prüfe State
if (currentState != State::Playing || paused || keyframes.empty()) {
    return false;
}

// ? Update Time
playbackTime += deltaTime * playbackSpeed;

// ? Interpolation
CameraKeyframe interpolated = interpolateKeyframes(k1, k2, t);
outPosition = interpolated.position;
outFront = interpolated.front;
outUp = interpolated.up;

return true; // ? Wichtig!
```

---

## Support

Bei weiteren Problemen:
1. Console-Output prüfen
2. UI-Status prüfen (Idle/Recording/Playing)
3. Keyframe-Count prüfen (sollte > 0 sein)
4. "Print Info to Console" Button klicken

**Debug-Output Beispiel:**
```
=== Camera Path Recorder Info ===
State: Playing
Keyframes: 1800
Duration: 60.000000 seconds
Playback Speed: 1.000000x
Recording Rate: 30.000000 fps
Looping: No
Playback Progress: 45%
```

Wenn alle Werte korrekt sind, aber Playback nicht funktioniert:
? Prüfe GLStudio.cpp Camera-Update Logik (siehe Checklist oben)
