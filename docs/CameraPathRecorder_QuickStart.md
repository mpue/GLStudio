# Camera Path Recorder - Quick Start

## ?? Was ist das?

Der Camera Path Recorder ermöglicht es dir, Kamerabewegungen aufzuzeichnen und wieder abzuspielen - perfekt für Cinematics, Demos und automatisierte Durchflüge!

## ? Schnellstart

### 1. Aufnahme starten
1. Drücke **F** um Free Fly Modus zu aktivieren
2. Öffne das **"Camera Path Recorder"** Fenster
3. Klicke **"Start Recording"**
4. Fliege mit WASD + Maus durch die Szene
5. Klicke **"Stop Recording"**

### 2. Abspielen
1. Klicke **"Play"**
2. Optional: Ändere **"Playback Speed"** (0.1x - 5.0x)
3. Optional: Aktiviere **"Loop Playback"**

### 3. Speichern/Laden
1. Gib einen **Dateinamen** ein (z.B. `my_camera_path.bin`)
2. Klicke **"Save Path"** zum Speichern
3. Klicke **"Load Path"** zum Laden

## ?? Steuerung während Recording

| Taste | Aktion |
|-------|--------|
| **W** | Vorwärts |
| **S** | Rückwärts |
| **A** | Links |
| **D** | Rechts |
| **Space** | Hoch |
| **Shift** | Schneller |
| **Strg** | Langsamer |
| **Maus** | Kamera drehen |
| **F** | Free Fly Ein/Aus |

## ?? Einstellungen

### Recording Rate
- **5-10 fps**: Kleine Dateien, für langsame Bewegungen
- **30 fps**: Standard, gute Balance
- **60-120 fps**: Sehr smooth, größere Dateien

### Playback Speed
- **0.5x**: Zeitlupe
- **1.0x**: Normal
- **2.0x**: Doppelte Geschwindigkeit
- **5.0x**: Schnelldurchlauf

## ?? Dateiformat

Pfade werden als kompakte Binärdateien gespeichert:
- Standard: `camera_path.bin`
- Größe: ~40 Bytes pro Keyframe
- Beispiel: 30 fps × 60s = ~70 KB

## ?? UI-Übersicht

```
?? Camera Path Recorder ???????????
?      ?
? Status: Idle/Recording/Playing   ?
?  ?
? === Recording ===           ?
? [Start Recording]                ?
? Recording Rate: [30] fps ?
?               ?
? === Playback ===   ?
? Keyframes: 1800                  ?
? Duration: 60.0 seconds           ?
? [??????????] 80%        ?
? [Play]       ?
? Playback Speed: [1.0]x           ?
? [?] Loop Playback        ?
?   ?
? === File Operations ===          ?
? Filename: [camera_path.bin]     ?
? [Save Path] [Load Path]   ?
? [Clear Recording]          ?
?            ?
????????????????????????????????????
```

## ?? Beispiel-Workflow

### Cinematic erstellen
```
1. [F] ? Free Fly aktivieren
2. [Start Recording] ? Aufnahme starten
3. Fliege eine interessante Route
4. [Stop Recording] ? Aufnahme beenden
5. [Save Path] ? Als "intro.bin" speichern
6. [Play] ? Abspielen und bewundern!
```

### Mehrere Takes
```
Take 1: [Record] ? [Stop] ? [Save] "take1.bin"
Take 2: [Clear] ? [Record] ? [Stop] ? [Save] "take2.bin"
Take 3: [Clear] ? [Record] ? [Stop] ? [Save] "take3.bin"

Später: [Load] "take2.bin" ? [Play]
```

## ?? Tipps

### Für smooth Recordings
- ? Bewege die Kamera langsam und gleichmäßig
- ? Verwende höhere Recording Rate (60 fps) bei schnellen Bewegungen
- ? Plane deine Route vorher

### Für beste Performance
- ?? 30 fps Recording Rate ist meist ausreichend
- ?? Speichere nur finale Takes
- ?? Lösche unbenutzte Recordings

### Kreative Ideen
- ?? Zeitlupe (0.3x) für dramatische Effekte
- ?? Loop für repetitive Animationen
- ?? Mehrere Pfade für verschiedene Szenen kombinieren

## ?? Wichtig

- **Recording nur im Free Fly Modus** (F-Taste drücken!)
- **Während Playback** kannst du nicht manuell steuern
- **Kamera fliegt durch Objekte** während Playback (keine Collision)

## ?? Technische Details

Siehe vollständige Dokumentation: [`docs/CameraPathRecorder.md`](docs/CameraPathRecorder.md)

## ?? Probleme?

**Recording startet nicht?**
? Free Fly Modus mit [F] aktivieren!

**Playback ist ruckelig?**
? Erhöhe Recording Rate auf 60 fps

**Datei kann nicht geladen werden?**
? Überprüfe Dateiname und Format

---

Viel Spaß beim Erstellen von Cinematics! ???
