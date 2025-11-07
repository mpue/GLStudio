# Camera Path Recorder

## Übersicht

Der **CameraPathRecorder** ermöglicht es, Kamerabewegungen im Free Fly Modus aufzuzeichnen und sie später mit einstellbarer Geschwindigkeit wieder abzuspielen. Dies ist nützlich für:

- Erstellen von Cinematics und Kamera-Fahrten
- Präsentationen und Demos
- Automatisierte Durchflüge durch die Voxel-Welt
- Testing und Debugging aus verschiedenen Perspektiven

## Features

### Aufnahme (Recording)
- **Automatische Keyframe-Erfassung**: Zeichnet Position, Blickrichtung und Up-Vektor der Kamera auf
- **Einstellbare Aufnahme-Rate**: 5-120 Keyframes pro Sekunde (Standard: 30 fps)
- **Nur im Free Fly Modus**: Aufnahme funktioniert nur wenn Free Fly Modus aktiviert ist (F-Taste)
- **Echtzeit-Feedback**: Zeigt Aufnahmezeit und REC-Indikator während der Aufnahme

### Wiedergabe (Playback)
- **Smooth Interpolation**: Weiche Übergänge zwischen Keyframes durch lineare Interpolation
- **Variable Geschwindigkeit**: 0.1x - 5.0x Wiedergabe-Geschwindigkeit
  - 0.5x = Zeitlupe
  - 1.0x = Normal-Geschwindigkeit
  - 2.0x = Doppelte Geschwindigkeit
- **Loop-Modus**: Optional endlose Wiederholung des Pfads
- **Fortschrittsanzeige**: Progress Bar zeigt aktuelle Position in der Wiedergabe

### Dateiverwaltung
- **Speichern/Laden**: Pfade als Binärdateien speichern und laden
- **Kompaktes Format**: Effiziente Speicherung mit Version-Header
- **Standard-Dateiname**: `camera_path.bin`

## Verwendung

### 1. Aufnahme starten

1. **Free Fly Modus aktivieren**: Drücke `F` um in den Free Fly Modus zu wechseln
2. **UI öffnen**: Das Fenster "Camera Path Recorder" öffnen
3. **Recording starten**: Klicke auf "Start Recording"
4. **Kamera bewegen**: Fliege mit der Kamera durch die Szene
   - WASD: Bewegung
   - Maus: Rotation
   - Shift: Schneller fliegen
   - Strg: Langsamer fliegen
5. **Recording stoppen**: Klicke auf "Stop Recording"

### 2. Wiedergabe

1. **Play starten**: Klicke auf "Play" im Playback-Bereich
2. **Geschwindigkeit anpassen**: Nutze den "Playback Speed" Slider
3. **Loop aktivieren**: Aktiviere "Loop Playback" für endlose Wiederholung
4. **Stoppen**: Klicke auf "Stop Playback" um die Wiedergabe zu beenden

### 3. Pfade speichern/laden

```cpp
// Im UI
1. Dateinamen eingeben im "Filename" Feld
2. "Save Path" klicken zum Speichern
3. "Load Path" klicken zum Laden

// Oder via Code
cameraPathRecorder->savePath("my_path.bin");
cameraPathRecorder->loadPath("my_path.bin");
```

## UI-Komponenten

### Recording Section
- **Status**: Zeigt aktuellen Status (Idle/Recording/Playing)
- **Start/Stop Recording**: Button zum Starten/Stoppen der Aufnahme
- **Recording Rate**: Slider für Keyframe-Rate (5-120 fps)
- **REC-Indikator**: Roter Text wenn Aufnahme läuft
- **Timer**: Zeigt Aufnahmezeit in Sekunden

### Playback Section
- **Keyframe Count**: Anzahl aufgezeichneter Keyframes
- **Duration**: Gesamtdauer der Aufnahme
- **Progress Bar**: Visueller Fortschritt der Wiedergabe
- **Play/Stop**: Buttons für Wiedergabe-Kontrolle
- **Playback Speed**: Slider für Geschwindigkeit (0.1x - 5.0x)
- **Loop Playback**: Checkbox für Loop-Modus

### File Operations
- **Filename**: Textfeld für Dateinamen
- **Save Path**: Speichert aktuellen Pfad
- **Load Path**: Lädt Pfad aus Datei
- **Clear Recording**: Löscht aktuellen Pfad

## API-Referenz

### Konstruktor
```cpp
CameraPathRecorder();
```

### Aufnahme-Funktionen
```cpp
void startRecording();      // Startet Aufnahme
void stopRecording();     // Stoppt Aufnahme
void clearRecording();    // Löscht alle Keyframes

// Pro Frame aufrufen während Recording
void updateRecording(const glm::vec3& position, 
   const glm::vec3& front, 
            const glm::vec3& up, 
         float deltaTime);
```

### Wiedergabe-Funktionen
```cpp
void startPlayback();      // Startet Wiedergabe
void stopPlayback();      // Stoppt Wiedergabe
void pausePlayback();             // Pausiert Wiedergabe
void resumePlayback();       // Setzt Wiedergabe fort

// Pro Frame aufrufen während Playback
bool updatePlayback(glm::vec3& outPosition, 
       glm::vec3& outFront, 
                   glm::vec3& outUp, 
       float deltaTime);
```

### Einstellungen
```cpp
// Wiedergabe-Geschwindigkeit
void setPlaybackSpeed(float speed);   // 0.1 - 10.0
float getPlaybackSpeed() const;

// Aufnahme-Rate
void setRecordingRate(float framesPerSecond);  // 1.0 - 120.0
float getRecordingRate() const;

// Loop-Modus
void setLooping(bool loop);
bool isLooping() const;
```

### Status-Abfragen
```cpp
bool isRecording() const;
bool isPlaying() const;
bool isIdle() const;

int getKeyframeCount() const;
float getRecordingDuration() const;
float getPlaybackTime() const;
float getPlaybackProgress() const;   // 0.0 - 1.0
```

### Dateiverwaltung
```cpp
bool savePath(const std::string& filename) const;
bool loadPath(const std::string& filename);
```

### Debugging
```cpp
void printInfo() const;  // Gibt Info in Konsole aus
```

## Technische Details

### Keyframe-Struktur
```cpp
struct CameraKeyframe {
    glm::vec3 position;   // Kamera-Position
    glm::vec3 front;      // Blickrichtung
  glm::vec3 up;         // Up-Vektor
    float timestamp;      // Zeit in Sekunden
};
```

### Interpolation
- **Position**: Lineare Interpolation zwischen Keyframes
- **Richtung**: Normalisierte lineare Interpolation (für echtes Slerp könnte Quaternion-Math verwendet werden)
- **Smooth Transitions**: Automatische Berechnung zwischen beliebigen Keyframes

### Dateiformat
```
[4 Bytes] Version (int32)
[8 Bytes] Keyframe Count (size_t)
[4 Bytes] Recording Duration (float)
[For each Keyframe]
    [12 Bytes] Position (3x float)
    [12 Bytes] Front (3x float)
    [12 Bytes] Up (3x float)
    [4 Bytes]  Timestamp (float)
```

### Performance
- **Memory**: ~40 Bytes pro Keyframe
- **Recording**: Minimal overhead, nur Speicherung von Vektoren
- **Playback**: O(log n) Suche + O(1) Interpolation pro Frame
- **Beispiel**: 30 fps × 60 Sekunden = 1800 Keyframes = ~70 KB

## Verwendungsbeispiele

### Einfache Kamera-Fahrt
```cpp
// Setup
cameraPathRecorder->setRecordingRate(30.0f);
cameraPathRecorder->setPlaybackSpeed(1.0f);

// Recording
cameraPathRecorder->startRecording();
// ... bewege Kamera ...
cameraPathRecorder->stopRecording();

// Playback
cameraPathRecorder->startPlayback();
```

### Zeitlupe mit Loop
```cpp
cameraPathRecorder->setPlaybackSpeed(0.5f);  // Halbe Geschwindigkeit
cameraPathRecorder->setLooping(true);
cameraPathRecorder->startPlayback();
```

### Cinematic mit mehreren Aufnahmen
```cpp
// Aufnahme 1
cameraPathRecorder->startRecording();
// ... erste Kamerafahrt ...
cameraPathRecorder->stopRecording();
cameraPathRecorder->savePath("scene1.bin");

// Aufnahme 2
cameraPathRecorder->clearRecording();
cameraPathRecorder->startRecording();
// ... zweite Kamerafahrt ...
cameraPathRecorder->stopRecording();
cameraPathRecorder->savePath("scene2.bin");

// Später abspielen
cameraPathRecorder->loadPath("scene1.bin");
cameraPathRecorder->startPlayback();
```

## Tipps und Best Practices

### Aufnahme-Qualität
- **30 fps**: Standard, gute Balance zwischen Qualität und Dateigröße
- **60 fps**: Sehr smooth, größere Dateien
- **10-15 fps**: Ausreichend für langsame Bewegungen, kleine Dateien

### Smooth Playback
- Verwende konstante Bewegungsgeschwindigkeiten während der Aufnahme
- Vermeide abrupte Richtungsänderungen
- Nutze höhere Recording-Rate für schnelle Bewegungen

### Performance
- Aufnahme hat minimalen Performance-Impact
- Playback ist sehr effizient (keine GPU-Operationen)
- Große Pfade (>10000 Keyframes) können beim Laden etwas Zeit brauchen

## Bekannte Limitierungen

1. **Nur Free Fly Modus**: Recording funktioniert nur im Free Fly Modus
2. **Keine Collision während Playback**: Kamera fliegt durch Objekte
3. **Lineare Interpolation**: Keine echte Spline-Interpolation (könnte mit Catmull-Rom verbessert werden)
4. **Keine Kamera-Einstellungen**: FOV, Near/Far Plane werden nicht aufgezeichnet

## Zukünftige Erweiterungen

Mögliche Verbesserungen:
- [ ] Spline-Interpolation (Catmull-Rom oder Bezier)
- [ ] Keyframe-Editor zum manuellen Anpassen
- [ ] Export als Video-File
- [ ] Kamera-Shake und Effekte
- [ ] Multiple Pfade gleichzeitig verwalten
- [ ] Timeline-UI mit Keyframe-Visualisierung
- [ ] Speed-Ramping (Geschwindigkeitsänderungen im Pfad)
- [ ] Bookmark-System für wichtige Positionen

## Integration in GLStudio

Der CameraPathRecorder ist vollständig in GLStudio integriert:

```cpp
// Globale Variable
CameraPathRecorder* cameraPathRecorder = nullptr;

// Initialisierung in main()
cameraPathRecorder = new CameraPathRecorder();
cameraPathRecorder->setRecordingRate(30.0f);
cameraPathRecorder->setLooping(false);

// Update in Render-Loop
if (cameraPathRecorder->isRecording() && characterController->isFreeFlyMode()) {
    glm::vec3 camPos = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
    cameraPathRecorder->updateRecording(camPos, 
        characterController->getFront(), 
        characterController->getUp(), 
   deltaTime);
}

if (cameraPathRecorder->isPlaying()) {
    glm::vec3 playbackPos, playbackFront, playbackUp;
    if (cameraPathRecorder->updatePlayback(playbackPos, playbackFront, playbackUp, deltaTime)) {
        camera.Position = playbackPos;
        camera.Front = playbackFront;
     camera.Up = playbackUp;
    }
}

// Cleanup
delete cameraPathRecorder;
```

## Troubleshooting

### Recording startet nicht
- **Problem**: Button ist deaktiviert
- **Lösung**: Free Fly Modus mit F-Taste aktivieren

### Playback ist ruckelig
- **Problem**: Zu wenige Keyframes
- **Lösung**: Erhöhe Recording Rate auf 60 fps

### Datei kann nicht geladen werden
- **Problem**: Version mismatch oder korrupte Datei
- **Lösung**: Überprüfe Dateiformat und Version

### Kamera bewegt sich nicht während Playback
- **Problem**: Keine Keyframes aufgezeichnet
- **Lösung**: Erst Recording durchführen, dann Playback starten

## Credits

Entwickelt für das GLStudio Voxel-Engine Projekt.
Verwendet GLM für Vektor-Mathematik und ImGui für die UI.
