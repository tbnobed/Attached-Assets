#include "zoom_addon.h"

ZoomAddon& ZoomAddon::Instance() {
    static ZoomAddon instance;
    return instance;
}

void ZoomAddon::SetVideoCallback(Napi::ThreadSafeFunction tsfn) {
    videoCallback_ = tsfn;
}

void ZoomAddon::SetAudioCallback(Napi::ThreadSafeFunction tsfn) {
    audioCallback_ = tsfn;
}

void ZoomAddon::SetEventCallback(Napi::ThreadSafeFunction tsfn) {
    eventCallback_ = tsfn;
}

std::map<uint32_t, ParticipantInfo> ZoomAddon::GetParticipants() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return participants_;
}

void ZoomAddon::OnVideoFrame(uint32_t userId, const uint8_t* data, int width, int height) {
    if (videoCallback_) {
        int bufSize = width * height * 4;
        auto buf = new uint8_t[bufSize];
        memcpy(buf, data, bufSize);

        videoCallback_.NonBlockingCall([userId, buf, width, height, bufSize](Napi::Env env, Napi::Function jsCallback) {
            auto buffer = Napi::Buffer<uint8_t>::Copy(env, buf, bufSize);
            delete[] buf;

            jsCallback.Call({
                Napi::Number::New(env, userId),
                buffer,
                Napi::Number::New(env, width),
                Napi::Number::New(env, height)
            });
        });
    }
}

void ZoomAddon::OnAudioFrame(uint32_t userId, const uint8_t* data, int length, int sampleRate, int channels) {
    if (audioCallback_) {
        auto buf = new uint8_t[length];
        memcpy(buf, data, length);

        audioCallback_.NonBlockingCall([userId, buf, length, sampleRate, channels](Napi::Env env, Napi::Function jsCallback) {
            auto buffer = Napi::Buffer<uint8_t>::Copy(env, buf, length);
            delete[] buf;

            jsCallback.Call({
                Napi::Number::New(env, userId),
                buffer,
                Napi::Number::New(env, sampleRate),
                Napi::Number::New(env, channels)
            });
        });
    }
}

void ZoomAddon::OnParticipantJoined(uint32_t userId, const std::string& name) {
    printf("[ZoomNative] OnParticipantJoined: userId=%u name=%s\n", userId, name.c_str());
    fflush(stdout);

    if (userId == selfUserId_) {
        printf("[ZoomNative] OnParticipantJoined: userId=%u is SELF (bot) — skipping video subscription\n", userId);
        fflush(stdout);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            participants_[userId] = { userId, name, false, false };
        }
        if (eventCallback_) {
            eventCallback_.NonBlockingCall([userId, name](Napi::Env env, Napi::Function jsCallback) {
                auto obj = Napi::Object::New(env);
                obj.Set("type", Napi::String::New(env, "participant-joined"));
                obj.Set("userId", Napi::Number::New(env, userId));
                obj.Set("displayName", Napi::String::New(env, name));
                obj.Set("isSelf", Napi::Boolean::New(env, true));
                jsCallback.Call({ obj });
            });
        }
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        participants_[userId] = { userId, name, false, false };
    }

    printf("[ZoomNative] OnParticipantJoined: userId=%u — deferring video subscription to onUserVideoStatusChange(Video_ON)\n", userId);
    fflush(stdout);

    if (eventCallback_) {
        eventCallback_.NonBlockingCall([userId, name](Napi::Env env, Napi::Function jsCallback) {
            auto obj = Napi::Object::New(env);
            obj.Set("type", Napi::String::New(env, "participant-joined"));
            obj.Set("userId", Napi::Number::New(env, userId));
            obj.Set("displayName", Napi::String::New(env, name));
            jsCallback.Call({ obj });
        });
    }
}

void ZoomAddon::OnParticipantLeft(uint32_t userId) {
    UnsubscribeParticipantVideo(userId);

    std::string name;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = participants_.find(userId);
        if (it != participants_.end()) {
            name = it->second.displayName;
            participants_.erase(it);
        }
    }

    if (eventCallback_) {
        eventCallback_.NonBlockingCall([userId, name](Napi::Env env, Napi::Function jsCallback) {
            auto obj = Napi::Object::New(env);
            obj.Set("type", Napi::String::New(env, "participant-left"));
            obj.Set("userId", Napi::Number::New(env, userId));
            obj.Set("displayName", Napi::String::New(env, name));
            jsCallback.Call({ obj });
        });
    }
}

void ZoomAddon::OnMeetingStatusChanged(const std::string& status) {
    if (eventCallback_) {
        eventCallback_.NonBlockingCall([status](Napi::Env env, Napi::Function jsCallback) {
            auto obj = Napi::Object::New(env);
            obj.Set("type", Napi::String::New(env, "meeting-status"));
            obj.Set("status", Napi::String::New(env, status));
            jsCallback.Call({ obj });
        });
    }

    if (status == "AUTH_SUCCESS") {
        state_ = AddonState::Authenticated;
    } else if (status == "MEETING_STATUS_INMEETING") {
        state_ = AddonState::InMeeting;
    } else if (status == "MEETING_STATUS_ENDED" || status == "MEETING_STATUS_FAILED") {
        state_ = AddonState::Authenticated;
    }
}

static Napi::Value InitSDK(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Config object required").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto config = info[0].As<Napi::Object>();
    ZoomConfig cfg;
    cfg.sdkKey = config.Get("sdkKey").As<Napi::String>().Utf8Value();
    cfg.sdkSecret = config.Get("sdkSecret").As<Napi::String>().Utf8Value();
    cfg.botName = config.Has("botName") ? config.Get("botName").As<Napi::String>().Utf8Value() : "PlexISO";
    cfg.jwtToken = config.Has("jwtToken") ? config.Get("jwtToken").As<Napi::String>().Utf8Value() : "";

    bool ok = ZoomAddon::Instance().Initialize(cfg);
    return Napi::Boolean::New(env, ok);
}

static Napi::Value AuthSDK(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool ok = ZoomAddon::Instance().Authenticate();
    return Napi::Boolean::New(env, ok);
}

static Napi::Value JoinMeeting(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2) {
        Napi::TypeError::New(env, "meetingId and password required").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string meetingId = info[0].As<Napi::String>().Utf8Value();
    std::string password = info[1].As<Napi::String>().Utf8Value();
    std::string displayName = info.Length() > 2 ? info[2].As<Napi::String>().Utf8Value() : "PlexISO";
    std::string appPrivilegeToken = info.Length() > 3 ? info[3].As<Napi::String>().Utf8Value() : "";

    bool ok = ZoomAddon::Instance().JoinMeeting(meetingId, password, displayName, appPrivilegeToken);
    return Napi::Boolean::New(env, ok);
}

static Napi::Value LeaveMeeting(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool ok = ZoomAddon::Instance().LeaveMeeting();
    return Napi::Boolean::New(env, ok);
}

static Napi::Value StartRawCapture(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool ok = ZoomAddon::Instance().StartRawDataCapture();
    return Napi::Boolean::New(env, ok);
}

static Napi::Value RetryVideoSubscriptions(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ZoomAddon::Instance().RetryVideoSubscriptions();
    return env.Undefined();
}

static Napi::Value OnVideoFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Callback function required").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto tsfn = Napi::ThreadSafeFunction::New(
        env, info[0].As<Napi::Function>(), "VideoFrameCallback", 0, 1
    );
    ZoomAddon::Instance().SetVideoCallback(tsfn);
    return env.Undefined();
}

static Napi::Value OnAudioFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Callback function required").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto tsfn = Napi::ThreadSafeFunction::New(
        env, info[0].As<Napi::Function>(), "AudioFrameCallback", 0, 1
    );
    ZoomAddon::Instance().SetAudioCallback(tsfn);
    return env.Undefined();
}

static Napi::Value OnEvent(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Callback function required").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto tsfn = Napi::ThreadSafeFunction::New(
        env, info[0].As<Napi::Function>(), "EventCallback", 0, 1
    );
    ZoomAddon::Instance().SetEventCallback(tsfn);
    return env.Undefined();
}

static Napi::Value EnumerateParticipants(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ZoomAddon::Instance().EnumerateParticipants();
    return env.Undefined();
}

static Napi::Value GetParticipants(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto participants = ZoomAddon::Instance().GetParticipants();
    auto result = Napi::Array::New(env, participants.size());

    int i = 0;
    for (const auto& [id, p] : participants) {
        auto obj = Napi::Object::New(env);
        obj.Set("userId", Napi::Number::New(env, p.userId));
        obj.Set("displayName", Napi::String::New(env, p.displayName));
        obj.Set("isVideoOn", Napi::Boolean::New(env, p.isVideoOn));
        obj.Set("isAudioOn", Napi::Boolean::New(env, p.isAudioOn));
        result.Set(i++, obj);
    }
    return result;
}

static Napi::Value GetState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto state = ZoomAddon::Instance().GetState();
    switch (state) {
        case AddonState::Uninitialized: return Napi::String::New(env, "uninitialized");
        case AddonState::Initialized: return Napi::String::New(env, "initialized");
        case AddonState::Authenticated: return Napi::String::New(env, "authenticated");
        case AddonState::InMeeting: return Napi::String::New(env, "in-meeting");
        case AddonState::Error: return Napi::String::New(env, "error");
        default: return Napi::String::New(env, "unknown");
    }
}

static Napi::Value CleanupSDK(const Napi::CallbackInfo& info) {
    ZoomAddon::Instance().Cleanup();
    return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("init", Napi::Function::New(env, InitSDK));
    exports.Set("auth", Napi::Function::New(env, AuthSDK));
    exports.Set("joinMeeting", Napi::Function::New(env, JoinMeeting));
    exports.Set("leaveMeeting", Napi::Function::New(env, LeaveMeeting));
    exports.Set("startRawCapture", Napi::Function::New(env, StartRawCapture));
    exports.Set("retryVideoSubscriptions", Napi::Function::New(env, RetryVideoSubscriptions));
    exports.Set("onVideoFrame", Napi::Function::New(env, OnVideoFrame));
    exports.Set("onAudioFrame", Napi::Function::New(env, OnAudioFrame));
    exports.Set("onEvent", Napi::Function::New(env, OnEvent));
    exports.Set("enumerateParticipants", Napi::Function::New(env, EnumerateParticipants));
    exports.Set("getParticipants", Napi::Function::New(env, GetParticipants));
    exports.Set("getState", Napi::Function::New(env, GetState));
    exports.Set("cleanup", Napi::Function::New(env, CleanupSDK));
    return exports;
}

NODE_API_MODULE(zoom_meeting_sdk, Init)
