#ifndef CAMERA_PATH_RECORDER_H
#define CAMERA_PATH_RECORDER_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

// Struktur für einen Kamera-Keyframe
struct CameraKeyframe {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float timestamp;  // Zeit in Sekunden seit Aufnahmestart
    
    CameraKeyframe() : position(0.0f), front(0.0f, 0.0f, -1.0f), up(0.0f, 1.0f, 0.0f), timestamp(0.0f) {}
    
    CameraKeyframe(const glm::vec3& pos, const glm::vec3& f, const glm::vec3& u, float time)
        : position(pos), front(f), up(u), timestamp(time) {}
};

// Kamera-Pfad Recorder
class CameraPathRecorder {
public:
    enum class State {
Idle,       // Nichts passiert
        Recording,  // Aufnahme läuft
        Playing     // Wiedergabe läuft
    };
    
    CameraPathRecorder();
    ~CameraPathRecorder();
    
    // Aufnahme-Steuerung
    void startRecording();
    void stopRecording();
    void clearRecording();
    
  // Wiedergabe-Steuerung
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    
    // Update-Funktionen (pro Frame aufrufen)
    void updateRecording(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up, float deltaTime);
    bool updatePlayback(glm::vec3& outPosition, glm::vec3& outFront, glm::vec3& outUp, float deltaTime);
 
    // Getters
    State getState() const { return currentState; }
    bool isRecording() const { return currentState == State::Recording; }
  bool isPlaying() const { return currentState == State::Playing; }
    bool isIdle() const { return currentState == State::Idle; }
    
    int getKeyframeCount() const { return static_cast<int>(keyframes.size()); }
    float getRecordingDuration() const { 
        // Während Recording: Zeige aktuelle Zeit, sonst finale Duration
        return (currentState == State::Recording) ? recordingTime : recordingDuration; 
    }
    float getPlaybackTime() const { return playbackTime; }
    float getPlaybackProgress() const;
    
  // Playback-Geschwindigkeit (1.0 = normal, 0.5 = halb, 2.0 = doppelt)
    void setPlaybackSpeed(float speed) { playbackSpeed = glm::max(0.1f, speed); }
    float getPlaybackSpeed() const { return playbackSpeed; }
    
    // Recording-Einstellungen
    void setRecordingRate(float framesPerSecond);
    float getRecordingRate() const { return recordingRate; }
 
    // Loop-Modus
    void setLooping(bool loop) { looping = loop; }
    bool isLooping() const { return looping; }
    
  // Speichern/Laden
    bool savePath(const std::string& filename) const;
    bool loadPath(const std::string& filename);
    
    // Debugging
    void printInfo() const;

private:
    State currentState;
    std::vector<CameraKeyframe> keyframes;
    
    // Recording-Parameter
    float recordingTime;
    float recordingDuration;
 float recordingRate;         // Keyframes pro Sekunde
    float timeSinceLastKeyframe;
    
    // Playback-Parameter
    float playbackTime;
    float playbackSpeed;
    bool looping;
    bool paused;
    int currentKeyframeIndex;
    
    // Hilfsfunktionen
    CameraKeyframe interpolateKeyframes(const CameraKeyframe& k1, const CameraKeyframe& k2, float t) const;
    int findKeyframeIndexAtTime(float time) const;
};

#endif // CAMERA_PATH_RECORDER_H
