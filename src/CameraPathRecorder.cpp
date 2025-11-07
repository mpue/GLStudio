#include "CameraPathRecorder.h"
#include <iostream>
#include <fstream>
#include <algorithm>

CameraPathRecorder::CameraPathRecorder()
    : currentState(State::Idle)
    , recordingTime(0.0f)
    , recordingDuration(0.0f)
    , recordingRate(30.0f)  // 30 Keyframes pro Sekunde
    , timeSinceLastKeyframe(0.0f)
    , playbackTime(0.0f)
    , playbackSpeed(1.0f)
    , looping(false)
    , paused(false)
    , currentKeyframeIndex(0)
{
}

CameraPathRecorder::~CameraPathRecorder()
{
    clearRecording();
}

void CameraPathRecorder::startRecording()
{
    if (currentState != State::Idle) {
    std::cout << "Cannot start recording: recorder is not idle" << std::endl;
        return;
    }
    
    clearRecording();
currentState = State::Recording;
    recordingTime = 0.0f;
    timeSinceLastKeyframe = 0.0f;
  
    std::cout << "Camera path recording started" << std::endl;
}

void CameraPathRecorder::stopRecording()
{
    if (currentState != State::Recording) {
        return;
    }
    
    recordingDuration = recordingTime;
  currentState = State::Idle;
    
    std::cout << "Camera path recording stopped. " 
   << keyframes.size() << " keyframes recorded over " 
 << recordingDuration << " seconds" << std::endl;
}

void CameraPathRecorder::clearRecording()
{
    keyframes.clear();
 recordingTime = 0.0f;
    recordingDuration = 0.0f;
    timeSinceLastKeyframe = 0.0f;
    playbackTime = 0.0f;
    currentKeyframeIndex = 0;
}

void CameraPathRecorder::startPlayback()
{
    if (keyframes.empty()) {
std::cout << "Cannot start playback: no keyframes recorded" << std::endl;
     return;
    }
    
    if (currentState == State::Recording) {
 std::cout << "Cannot start playback: recording is in progress" << std::endl;
  return;
    }
    
    currentState = State::Playing;
    playbackTime = 0.0f;
currentKeyframeIndex = 0;
    paused = false;
    
    std::cout << "Camera path playback started" << std::endl;
}

void CameraPathRecorder::stopPlayback()
{
    if (currentState != State::Playing) {
      return;
 }
    
    currentState = State::Idle;
    playbackTime = 0.0f;
    currentKeyframeIndex = 0;
    paused = false;
    
    std::cout << "Camera path playback stopped" << std::endl;
}

void CameraPathRecorder::pausePlayback()
{
    if (currentState == State::Playing) {
     paused = true;
        std::cout << "Camera path playback paused" << std::endl;
    }
}

void CameraPathRecorder::resumePlayback()
{
    if (currentState == State::Playing && paused) {
        paused = false;
 std::cout << "Camera path playback resumed" << std::endl;
    }
}

void CameraPathRecorder::updateRecording(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up, float deltaTime)
{
    if (currentState != State::Recording) {
        return;
    }
    
    recordingTime += deltaTime;
    timeSinceLastKeyframe += deltaTime;
    
    // Keyframe-Rate basiert auf eingestellter Frequenz
    float keyframeInterval = 1.0f / recordingRate;
    
    if (timeSinceLastKeyframe >= keyframeInterval) {
        keyframes.emplace_back(position, front, up, recordingTime);
        timeSinceLastKeyframe = 0.0f;
    }
}

bool CameraPathRecorder::updatePlayback(glm::vec3& outPosition, glm::vec3& outFront, glm::vec3& outUp, float deltaTime)
{
    if (currentState != State::Playing || paused || keyframes.empty()) {
        return false;
    }
    
    // Update playback time mit Geschwindigkeitsmultiplikator
    playbackTime += deltaTime * playbackSpeed;
    
    // Check if playback finished
    if (playbackTime >= recordingDuration) {
        if (looping) {
            playbackTime = 0.0f;
    currentKeyframeIndex = 0;
        } else {
      stopPlayback();
       return false;
        }
    }
    
    // Finde die beiden Keyframes für Interpolation
  int nextIndex = findKeyframeIndexAtTime(playbackTime);
    
    if (nextIndex <= 0) {
        // Vor dem ersten Keyframe
        const CameraKeyframe& kf = keyframes[0];
        outPosition = kf.position;
        outFront = kf.front;
        outUp = kf.up;
    return true;
    }
    
    if (nextIndex >= static_cast<int>(keyframes.size())) {
   // Nach dem letzten Keyframe
        const CameraKeyframe& kf = keyframes.back();
      outPosition = kf.position;
        outFront = kf.front;
        outUp = kf.up;
  return true;
    }
  
    // Interpoliere zwischen zwei Keyframes
    const CameraKeyframe& k1 = keyframes[nextIndex - 1];
    const CameraKeyframe& k2 = keyframes[nextIndex];
    
    float timeDiff = k2.timestamp - k1.timestamp;
    float t = (timeDiff > 0.0001f) ? (playbackTime - k1.timestamp) / timeDiff : 0.0f;
  t = glm::clamp(t, 0.0f, 1.0f);
    
    CameraKeyframe interpolated = interpolateKeyframes(k1, k2, t);
    
    outPosition = interpolated.position;
    outFront = interpolated.front;
 outUp = interpolated.up;
    
    currentKeyframeIndex = nextIndex;
    return true;
}

float CameraPathRecorder::getPlaybackProgress() const
{
    if (recordingDuration <= 0.0f) {
        return 0.0f;
    }
    return glm::clamp(playbackTime / recordingDuration, 0.0f, 1.0f);
}

void CameraPathRecorder::setRecordingRate(float framesPerSecond)
{
    recordingRate = glm::clamp(framesPerSecond, 1.0f, 120.0f);
}

CameraKeyframe CameraPathRecorder::interpolateKeyframes(const CameraKeyframe& k1, const CameraKeyframe& k2, float t) const
{
    CameraKeyframe result;
    
    // Lineare Interpolation für Position
    result.position = glm::mix(k1.position, k2.position, t);
    
 // Slerp (spherical linear interpolation) für Richtungsvektoren wäre besser,
    // aber für einfache Anwendung reicht lineare Interpolation
    result.front = glm::normalize(glm::mix(k1.front, k2.front, t));
result.up = glm::normalize(glm::mix(k1.up, k2.up, t));
 
    result.timestamp = glm::mix(k1.timestamp, k2.timestamp, t);
    
    return result;
}

int CameraPathRecorder::findKeyframeIndexAtTime(float time) const
{
    if (keyframes.empty()) {
        return -1;
    }
    
    // Binäre Suche für effizienten Zugriff
    for (size_t i = 0; i < keyframes.size(); ++i) {
        if (keyframes[i].timestamp > time) {
            return static_cast<int>(i);
  }
    }
    
    return static_cast<int>(keyframes.size());
}

bool CameraPathRecorder::savePath(const std::string& filename) const
{
    if (keyframes.empty()) {
        std::cout << "No keyframes to save" << std::endl;
     return false;
    }
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to open file for writing: " << filename << std::endl;
 return false;
    }
    
    // Header schreiben
    int version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    size_t count = keyframes.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
file.write(reinterpret_cast<const char*>(&recordingDuration), sizeof(recordingDuration));
    
    // Keyframes schreiben
    for (const auto& kf : keyframes) {
        file.write(reinterpret_cast<const char*>(&kf.position), sizeof(kf.position));
        file.write(reinterpret_cast<const char*>(&kf.front), sizeof(kf.front));
        file.write(reinterpret_cast<const char*>(&kf.up), sizeof(kf.up));
        file.write(reinterpret_cast<const char*>(&kf.timestamp), sizeof(kf.timestamp));
    }
    
    file.close();
    std::cout << "Camera path saved to " << filename << " (" << count << " keyframes)" << std::endl;
    return true;
}

bool CameraPathRecorder::loadPath(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
  std::cout << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }
    
    clearRecording();
    
    // Header lesen
    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (version != 1) {
std::cout << "Unsupported file version: " << version << std::endl;
file.close();
   return false;
    }
    
    size_t count;
file.read(reinterpret_cast<char*>(&count), sizeof(count));
    file.read(reinterpret_cast<char*>(&recordingDuration), sizeof(recordingDuration));
    
    // Keyframes lesen
    keyframes.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      CameraKeyframe kf;
        file.read(reinterpret_cast<char*>(&kf.position), sizeof(kf.position));
        file.read(reinterpret_cast<char*>(&kf.front), sizeof(kf.front));
        file.read(reinterpret_cast<char*>(&kf.up), sizeof(kf.up));
        file.read(reinterpret_cast<char*>(&kf.timestamp), sizeof(kf.timestamp));
        keyframes.push_back(kf);
    }
    
    file.close();
    std::cout << "Camera path loaded from " << filename << " (" << count << " keyframes)" << std::endl;
    return true;
}

void CameraPathRecorder::printInfo() const
{
    std::cout << "=== Camera Path Recorder Info ===" << std::endl;
    std::cout << "State: ";
    switch (currentState) {
        case State::Idle: std::cout << "Idle"; break;
   case State::Recording: std::cout << "Recording"; break;
        case State::Playing: std::cout << "Playing"; break;
    }
    std::cout << std::endl;
    
    std::cout << "Keyframes: " << keyframes.size() << std::endl;
    std::cout << "Duration: " << recordingDuration << " seconds" << std::endl;
    std::cout << "Playback Speed: " << playbackSpeed << "x" << std::endl;
    std::cout << "Recording Rate: " << recordingRate << " fps" << std::endl;
    std::cout << "Looping: " << (looping ? "Yes" : "No") << std::endl;
    
    if (currentState == State::Playing) {
   std::cout << "Playback Progress: " << (getPlaybackProgress() * 100.0f) << "%" << std::endl;
    }
}
