#ifndef ZOOM_ADDON_H
#define ZOOM_ADDON_H

#include <napi.h>
#include <string>
#include <map>
#include <set>
#include <mutex>
#include <functional>

struct ZoomConfig {
    std::string sdkKey;
    std::string sdkSecret;
    std::string botName;
    std::string jwtToken;
};

struct ParticipantInfo {
    uint32_t userId;
    std::string displayName;
    bool isVideoOn;
    bool isAudioOn;
};

enum class AddonState {
    Uninitialized,
    Initialized,
    Authenticated,
    InMeeting,
    Error
};

class ZoomAddon {
public:
    static ZoomAddon& Instance();

    void SetVideoCallback(Napi::ThreadSafeFunction tsfn);
    void SetAudioCallback(Napi::ThreadSafeFunction tsfn);
    void SetEventCallback(Napi::ThreadSafeFunction tsfn);

    bool Initialize(const ZoomConfig& config);
    bool Authenticate();
    bool JoinMeeting(const std::string& meetingId, const std::string& password, const std::string& displayName, const std::string& appPrivilegeToken = "");
    bool LeaveMeeting();
    void EnumerateParticipants();
    bool StartRawRecording();
    bool StartRawDataCapture();
    void RetryVideoSubscriptions();
    void OnRecordingPermissionGranted();
    void OnRawRecordingStarted();
    void StopRawDataCapture();
    void SubscribeParticipantVideo(uint32_t userId);
    void UnsubscribeParticipantVideo(uint32_t userId);
    void Cleanup();

    AddonState GetState() const { return state_; }
    std::map<uint32_t, ParticipantInfo> GetParticipants() const;

    void OnVideoFrame(uint32_t userId, const uint8_t* data, int width, int height);
    void OnAudioFrame(uint32_t userId, const uint8_t* data, int length, int sampleRate, int channels);
    void OnParticipantJoined(uint32_t userId, const std::string& name);
    void OnParticipantLeft(uint32_t userId);
    void OnMeetingStatusChanged(const std::string& status);
    void SetSelfUserId(uint32_t userId) { selfUserId_ = userId; }
    uint32_t GetSelfUserId() const { return selfUserId_; }

private:
    ZoomAddon() : state_(AddonState::Uninitialized), selfUserId_(0) {}

    AddonState state_;
    uint32_t selfUserId_;
    ZoomConfig config_;
    std::map<uint32_t, ParticipantInfo> participants_;
    mutable std::mutex mutex_;
    std::wstring appPrivilegeToken_;

    Napi::ThreadSafeFunction videoCallback_;
    Napi::ThreadSafeFunction audioCallback_;
    Napi::ThreadSafeFunction eventCallback_;
};

#endif
