#import <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <napi.h>
#include <map>
#include <string>
#include <mutex>

#include "DeckLinkAPI.h"

static IDeckLinkIterator* CreateDeckLinkIteratorInstance_Compat() {
    static void* cached_func = nullptr;
    static bool tried = false;
    if (!tried) {
        tried = true;
        void* h = dlopen("/Library/Frameworks/DeckLinkAPI.framework/DeckLinkAPI", RTLD_LAZY | RTLD_GLOBAL);
        if (h) {
            const char* names[] = {
                "CreateDeckLinkIteratorInstance_0004",
                "CreateDeckLinkIteratorInstance_0003",
                "CreateDeckLinkIteratorInstance_0002",
                "CreateDeckLinkIteratorInstance_0001",
                "CreateDeckLinkIteratorInstance",
                nullptr
            };
            for (int i = 0; names[i]; i++) {
                cached_func = dlsym(h, names[i]);
                if (cached_func) break;
            }
        }
    }
    if (!cached_func) return nullptr;
    typedef IDeckLinkIterator* (*Func)(void);
    return ((Func)cached_func)();
}

struct OutputState {
    IDeckLink* device = nullptr;
    IDeckLinkOutput* output = nullptr;
    IDeckLinkMutableVideoFrame* videoFrame = nullptr;
    int width = 0;
    int height = 0;
    int rowBytes = 0;
    bool audioEnabled = false;
    bool videoEnabled = false;
};

static std::map<int, OutputState*> g_outputs;
static int g_nextHandle = 1;
static std::mutex g_mutex;

static std::string CFStringToStd(CFStringRef cfStr) {
    if (!cfStr) return "Unknown";
    char buf[256];
    if (CFStringGetCString(cfStr, buf, sizeof(buf), kCFStringEncodingUTF8)) {
        return std::string(buf);
    }
    return "Unknown";
}

Napi::Value GetDevices(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env);

    IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance_Compat();
    if (!iterator) {
        return result;
    }

    IDeckLink* deckLink = nullptr;
    int index = 0;
    while (iterator->Next(&deckLink) == S_OK) {
        Napi::Object device = Napi::Object::New(env);
        device.Set("index", Napi::Number::New(env, index));

        CFStringRef nameRef = nullptr;
        if (deckLink->GetDisplayName(&nameRef) == S_OK && nameRef) {
            device.Set("name", Napi::String::New(env, CFStringToStd(nameRef)));
            CFRelease(nameRef);
        } else {
            device.Set("name", Napi::String::New(env, "DeckLink Device"));
        }

        CFStringRef modelRef = nullptr;
        if (deckLink->GetModelName(&modelRef) == S_OK && modelRef) {
            device.Set("modelName", Napi::String::New(env, CFStringToStd(modelRef)));
            CFRelease(modelRef);
        }

        IDeckLinkOutput* output = nullptr;
        bool hasOutput = (deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&output) == S_OK);
        device.Set("hasOutput", Napi::Boolean::New(env, hasOutput));

        if (hasOutput && output) {
            Napi::Array modes = Napi::Array::New(env);
            IDeckLinkDisplayModeIterator* modeIter = nullptr;
            if (output->GetDisplayModeIterator(&modeIter) == S_OK && modeIter) {
                IDeckLinkDisplayMode* mode = nullptr;
                int modeIdx = 0;
                while (modeIter->Next(&mode) == S_OK) {
                    Napi::Object modeObj = Napi::Object::New(env);

                    CFStringRef modeNameRef = nullptr;
                    if (mode->GetName(&modeNameRef) == S_OK && modeNameRef) {
                        modeObj.Set("name", Napi::String::New(env, CFStringToStd(modeNameRef)));
                        CFRelease(modeNameRef);
                    }

                    modeObj.Set("width", Napi::Number::New(env, (double)mode->GetWidth()));
                    modeObj.Set("height", Napi::Number::New(env, (double)mode->GetHeight()));

                    BMDTimeValue frameDuration;
                    BMDTimeScale timeScale;
                    if (mode->GetFrameRate(&frameDuration, &timeScale) == S_OK) {
                        double fps = (double)timeScale / (double)frameDuration;
                        modeObj.Set("fps", Napi::Number::New(env, fps));
                        modeObj.Set("frameDuration", Napi::Number::New(env, (double)frameDuration));
                        modeObj.Set("timeScale", Napi::Number::New(env, (double)timeScale));
                    }

                    modeObj.Set("displayMode", Napi::Number::New(env, (double)mode->GetDisplayMode()));

                    modes.Set(modeIdx++, modeObj);
                    mode->Release();
                }
                modeIter->Release();
            }
            device.Set("displayModes", modes);
            output->Release();
        }

        result.Set(index, device);
        deckLink->Release();
        index++;
    }
    iterator->Release();
    return result;
}

class OpenOutputWorker : public Napi::AsyncWorker {
public:
    OpenOutputWorker(Napi::Env env,
                     int deviceIndex,
                     BMDDisplayMode displayMode,
                     BMDPixelFormat pixelFormat,
                     uint32_t audioSampleRate,
                     uint32_t audioSampleType,
                     uint32_t audioChannels)
        : Napi::AsyncWorker(env),
          deferred_(Napi::Promise::Deferred::New(env)),
          deviceIndex_(deviceIndex),
          displayMode_(displayMode),
          pixelFormat_(pixelFormat),
          audioSampleRate_(audioSampleRate),
          audioSampleType_(audioSampleType),
          audioChannels_(audioChannels),
          handle_(-1) {}

    Napi::Promise Promise() { return deferred_.Promise(); }

    void Execute() override {
        IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance_Compat();
        if (!iterator) {
            SetError("Failed to create DeckLink iterator - drivers not installed?");
            return;
        }

        IDeckLink* deckLink = nullptr;
        int idx = 0;
        while (iterator->Next(&deckLink) == S_OK) {
            if (idx == deviceIndex_) break;
            deckLink->Release();
            deckLink = nullptr;
            idx++;
        }
        iterator->Release();

        if (!deckLink) {
            SetError("Device index " + std::to_string(deviceIndex_) + " not found");
            return;
        }

        IDeckLinkOutput* output = nullptr;
        HRESULT hr = deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&output);
        if (hr != S_OK || !output) {
            deckLink->Release();
            SetError("Device does not support output");
            return;
        }

        hr = output->EnableVideoOutput(displayMode_, bmdVideoOutputFlagDefault);
        if (hr != S_OK) {
            output->Release();
            deckLink->Release();
            SetError("EnableVideoOutput failed (HRESULT=" + std::to_string(hr) + ")");
            return;
        }

        BMDAudioSampleRate sampleRate = (BMDAudioSampleRate)audioSampleRate_;
        BMDAudioSampleType sampleType = (BMDAudioSampleType)audioSampleType_;
        hr = output->EnableAudioOutput(sampleRate, sampleType, audioChannels_,
                                        bmdAudioOutputStreamContinuous);
        if (hr != S_OK) {
            output->DisableVideoOutput();
            output->Release();
            deckLink->Release();
            SetError("EnableAudioOutput failed (HRESULT=" + std::to_string(hr) + ")");
            return;
        }

        IDeckLinkDisplayModeIterator* modeIter = nullptr;
        int frameWidth = 1920, frameHeight = 1080;
        if (output->GetDisplayModeIterator(&modeIter) == S_OK && modeIter) {
            IDeckLinkDisplayMode* mode = nullptr;
            while (modeIter->Next(&mode) == S_OK) {
                if (mode->GetDisplayMode() == displayMode_) {
                    frameWidth = (int)mode->GetWidth();
                    frameHeight = (int)mode->GetHeight();
                    mode->Release();
                    break;
                }
                mode->Release();
            }
            modeIter->Release();
        }

        int bytesPerPixel = 2;
        if (pixelFormat_ == bmdFormat8BitBGRA) bytesPerPixel = 4;
        else if (pixelFormat_ == bmdFormat10BitYUV) bytesPerPixel = 0;
        int rowBytes = frameWidth * bytesPerPixel;
        if (pixelFormat_ == bmdFormat10BitYUV) {
            rowBytes = ((frameWidth + 47) / 48) * 128;
        }

        IDeckLinkMutableVideoFrame* videoFrame = nullptr;
        hr = output->CreateVideoFrame(frameWidth, frameHeight, rowBytes,
                                       pixelFormat_, bmdFrameFlagDefault, &videoFrame);
        if (hr != S_OK || !videoFrame) {
            output->DisableAudioOutput();
            output->DisableVideoOutput();
            output->Release();
            deckLink->Release();
            SetError("CreateVideoFrame failed (HRESULT=" + std::to_string(hr) + ")");
            return;
        }

        OutputState* state = new OutputState();
        state->device = deckLink;
        state->output = output;
        state->videoFrame = videoFrame;
        state->width = frameWidth;
        state->height = frameHeight;
        state->rowBytes = rowBytes;
        state->videoEnabled = true;
        state->audioEnabled = true;

        std::lock_guard<std::mutex> lock(g_mutex);
        handle_ = g_nextHandle++;
        g_outputs[handle_] = state;
    }

    void OnOK() override {
        Napi::Env env = Env();
        Napi::Object result = Napi::Object::New(env);
        result.Set("handle", Napi::Number::New(env, handle_));

        OutputState* state = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_outputs.find(handle_);
            if (it != g_outputs.end()) state = it->second;
        }
        if (state) {
            result.Set("width", Napi::Number::New(env, state->width));
            result.Set("height", Napi::Number::New(env, state->height));
        }

        deferred_.Resolve(result);
    }

    void OnError(const Napi::Error& err) override {
        deferred_.Reject(err.Value());
    }

private:
    Napi::Promise::Deferred deferred_;
    int deviceIndex_;
    BMDDisplayMode displayMode_;
    BMDPixelFormat pixelFormat_;
    uint32_t audioSampleRate_;
    uint32_t audioSampleType_;
    uint32_t audioChannels_;
    int handle_;
};

Napi::Value OpenOutput(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected options object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object opts = info[0].As<Napi::Object>();

    int deviceIndex = opts.Has("deviceIndex") ? opts.Get("deviceIndex").As<Napi::Number>().Int32Value() : 0;
    BMDDisplayMode displayMode = opts.Has("displayMode")
        ? (BMDDisplayMode)opts.Get("displayMode").As<Napi::Number>().Uint32Value()
        : bmdModeHD1080i5994;
    BMDPixelFormat pixelFormat = opts.Has("pixelFormat")
        ? (BMDPixelFormat)opts.Get("pixelFormat").As<Napi::Number>().Uint32Value()
        : bmdFormat8BitYUV;
    uint32_t audioSampleRate = opts.Has("audioSampleRate")
        ? opts.Get("audioSampleRate").As<Napi::Number>().Uint32Value()
        : 48000;
    uint32_t audioSampleType = opts.Has("audioSampleType")
        ? opts.Get("audioSampleType").As<Napi::Number>().Uint32Value()
        : 16;
    uint32_t audioChannels = opts.Has("audioChannels")
        ? opts.Get("audioChannels").As<Napi::Number>().Uint32Value()
        : 2;

    auto worker = new OpenOutputWorker(env, deviceIndex, displayMode, pixelFormat,
                                        audioSampleRate, audioSampleType, audioChannels);
    worker->Queue();
    return worker->Promise();
}

class DisplayFrameWorker : public Napi::AsyncWorker {
public:
    DisplayFrameWorker(Napi::Env env, int handle,
                       Napi::Buffer<uint8_t> videoBuf,
                       Napi::Buffer<uint8_t> audioBuf)
        : Napi::AsyncWorker(env),
          deferred_(Napi::Promise::Deferred::New(env)),
          handle_(handle),
          videoData_(videoBuf.Data()),
          videoSize_(videoBuf.Length()),
          audioData_(audioBuf.Data()),
          audioSize_(audioBuf.Length()),
          samplesWritten_(0) {
        videoRef_ = Napi::Persistent(videoBuf);
        audioRef_ = Napi::Persistent(audioBuf);
    }

    Napi::Promise Promise() { return deferred_.Promise(); }

    void Execute() override {
        OutputState* state = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_outputs.find(handle_);
            if (it != g_outputs.end()) state = it->second;
        }

        if (!state || !state->output || !state->videoFrame) {
            SetError("Invalid output handle " + std::to_string(handle_));
            return;
        }

        void* frameBytes = nullptr;
        HRESULT hr = state->videoFrame->GetBytes(&frameBytes);
        if (hr != S_OK || !frameBytes) {
            SetError("GetBytes failed");
            return;
        }

        size_t frameSize = (size_t)state->height * (size_t)state->rowBytes;
        size_t copySize = videoSize_ < frameSize ? videoSize_ : frameSize;
        memcpy(frameBytes, videoData_, copySize);
        if (copySize < frameSize) {
            memset((uint8_t*)frameBytes + copySize, 0, frameSize - copySize);
        }

        hr = state->output->DisplayVideoFrameSync(state->videoFrame);
        if (hr != S_OK) {
            SetError("DisplayVideoFrameSync failed (HRESULT=" + std::to_string(hr) + ")");
            return;
        }

        if (state->audioEnabled && audioSize_ > 0) {
            uint32_t bytesPerSample = 4;
            uint32_t sampleCount = (uint32_t)(audioSize_ / bytesPerSample);
            if (sampleCount > 0) {
                hr = state->output->WriteAudioSamplesSync(
                    audioData_, sampleCount, &samplesWritten_);
                if (hr != S_OK) {
                    SetError("WriteAudioSamplesSync failed (HRESULT=" + std::to_string(hr) + ")");
                    return;
                }
            }
        }
    }

    void OnOK() override {
        Napi::Env env = Env();
        Napi::Object result = Napi::Object::New(env);
        result.Set("samplesWritten", Napi::Number::New(env, (double)samplesWritten_));
        deferred_.Resolve(result);
    }

    void OnError(const Napi::Error& err) override {
        deferred_.Reject(err.Value());
    }

private:
    Napi::Promise::Deferred deferred_;
    int handle_;
    uint8_t* videoData_;
    size_t videoSize_;
    uint8_t* audioData_;
    size_t audioSize_;
    uint32_t samplesWritten_;
    Napi::Reference<Napi::Buffer<uint8_t>> videoRef_;
    Napi::Reference<Napi::Buffer<uint8_t>> audioRef_;
};

Napi::Value DisplayFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Expected (handle, videoBuf, audioBuf)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int handle = info[0].As<Napi::Number>().Int32Value();
    Napi::Buffer<uint8_t> videoBuf = info[1].As<Napi::Buffer<uint8_t>>();
    Napi::Buffer<uint8_t> audioBuf = info[2].As<Napi::Buffer<uint8_t>>();

    auto worker = new DisplayFrameWorker(env, handle, videoBuf, audioBuf);
    worker->Queue();
    return worker->Promise();
}

Napi::Value CloseOutput(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int handle = info[0].As<Napi::Number>().Int32Value();

    OutputState* state = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_outputs.find(handle);
        if (it != g_outputs.end()) {
            state = it->second;
            g_outputs.erase(it);
        }
    }

    if (state) {
        if (state->videoFrame) {
            state->videoFrame->Release();
        }
        if (state->output) {
            if (state->audioEnabled) state->output->DisableAudioOutput();
            if (state->videoEnabled) state->output->DisableVideoOutput();
            state->output->Release();
        }
        if (state->device) {
            state->device->Release();
        }
        delete state;
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value IsAvailable(const Napi::CallbackInfo& info) {
    IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance_Compat();
    if (iterator) {
        iterator->Release();
        return Napi::Boolean::New(info.Env(), true);
    }
    return Napi::Boolean::New(info.Env(), false);
}

static uint32_t fourcc(const char* s) {
    return ((uint32_t)s[0] << 24) | ((uint32_t)s[1] << 16) |
           ((uint32_t)s[2] << 8) | (uint32_t)s[3];
}

static void CleanupOutputs(void* /*arg*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& pair : g_outputs) {
        OutputState* state = pair.second;
        if (state) {
            if (state->videoFrame) state->videoFrame->Release();
            if (state->output) {
                if (state->audioEnabled) state->output->DisableAudioOutput();
                if (state->videoEnabled) state->output->DisableVideoOutput();
                state->output->Release();
            }
            if (state->device) state->device->Release();
            delete state;
        }
    }
    g_outputs.clear();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    napi_add_env_cleanup_hook(env, CleanupOutputs, nullptr);
    exports.Set("getDevices", Napi::Function::New(env, GetDevices));
    exports.Set("openOutput", Napi::Function::New(env, OpenOutput));
    exports.Set("displayFrame", Napi::Function::New(env, DisplayFrame));
    exports.Set("closeOutput", Napi::Function::New(env, CloseOutput));
    exports.Set("isAvailable", Napi::Function::New(env, IsAvailable));

    exports.Set("bmdModeNTSC",           Napi::Number::New(env, fourcc("ntsc")));
    exports.Set("bmdModeNTSC2398",       Napi::Number::New(env, fourcc("nt23")));
    exports.Set("bmdModePAL",            Napi::Number::New(env, fourcc("pal ")));
    exports.Set("bmdModeNTSCp",          Napi::Number::New(env, fourcc("ntsp")));
    exports.Set("bmdModePALp",           Napi::Number::New(env, fourcc("palp")));
    exports.Set("bmdModeHD1080p2398",    Napi::Number::New(env, fourcc("23ps")));
    exports.Set("bmdModeHD1080p24",      Napi::Number::New(env, fourcc("24ps")));
    exports.Set("bmdModeHD1080p25",      Napi::Number::New(env, fourcc("Hp25")));
    exports.Set("bmdModeHD1080p2997",    Napi::Number::New(env, fourcc("Hp29")));
    exports.Set("bmdModeHD1080p30",      Napi::Number::New(env, fourcc("Hp30")));
    exports.Set("bmdModeHD1080i50",      Napi::Number::New(env, fourcc("Hi50")));
    exports.Set("bmdModeHD1080i5994",    Napi::Number::New(env, fourcc("Hi59")));
    exports.Set("bmdModeHD1080i6000",    Napi::Number::New(env, fourcc("Hi60")));
    exports.Set("bmdModeHD1080p50",      Napi::Number::New(env, fourcc("Hp50")));
    exports.Set("bmdModeHD1080p5994",    Napi::Number::New(env, fourcc("Hp59")));
    exports.Set("bmdModeHD1080p6000",    Napi::Number::New(env, fourcc("Hp60")));
    exports.Set("bmdModeHD720p50",       Napi::Number::New(env, fourcc("hp50")));
    exports.Set("bmdModeHD720p5994",     Napi::Number::New(env, fourcc("hp59")));
    exports.Set("bmdModeHD720p60",       Napi::Number::New(env, fourcc("hp60")));

    exports.Set("bmdFormat8BitYUV",      Napi::Number::New(env, fourcc("2vuy")));
    exports.Set("bmdFormat8BitBGRA",     Napi::Number::New(env, fourcc("BGRA")));
    exports.Set("bmdFormat10BitYUV",     Napi::Number::New(env, fourcc("v210")));

    exports.Set("bmdAudioSampleRate48kHz",        Napi::Number::New(env, 48000));
    exports.Set("bmdAudioSampleType16bitInteger",  Napi::Number::New(env, 16));
    exports.Set("bmdAudioSampleType32bitInteger",  Napi::Number::New(env, 32));

    return exports;
}

NODE_API_MODULE(decklink_output, Init)
