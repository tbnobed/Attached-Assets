#include <napi.h>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "DeckLinkAPI.h"
#include "LinuxCOM.h"

static bool IsEqualIID(REFIID a, REFIID b) {
    return memcmp(&a, &b, sizeof(IID)) == 0;
}

class DeckLinkVideoFrame : public IDeckLinkVideoFrame {
public:
    DeckLinkVideoFrame(long width, long height, long rowBytes, BMDPixelFormat pixelFormat)
        : width_(width), height_(height), rowBytes_(rowBytes),
          pixelFormat_(pixelFormat), refCount_(1) {
        bufferSize_ = (size_t)height * (size_t)rowBytes;
        buffer_ = (uint8_t*)calloc(1, bufferSize_);
    }

    ~DeckLinkVideoFrame() {
        if (buffer_) free(buffer_);
    }

    uint8_t* RawPtr() { return buffer_; }
    size_t BufferSize() { return bufferSize_; }

    long STDMETHODCALLTYPE GetWidth() override { return width_; }
    long STDMETHODCALLTYPE GetHeight() override { return height_; }
    long STDMETHODCALLTYPE GetRowBytes() override { return rowBytes_; }
    BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat() override { return pixelFormat_; }
    BMDFrameFlags STDMETHODCALLTYPE GetFlags() override { return bmdFrameFlagDefault; }

    HRESULT STDMETHODCALLTYPE GetBytes(void** buffer) override {
        if (!buffer) return E_FAIL;
        *buffer = buffer_;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTimecode(BMDTimecodeFormat, IDeckLinkTimecode**) override { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE GetAncillaryData(IDeckLinkVideoFrameAncillary**) override { return S_FALSE; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID* ppv) override {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (IsEqualIID(iid, IID_IDeckLinkVideoFrame)) {
            *ppv = static_cast<IDeckLinkVideoFrame*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++refCount_; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG c = --refCount_;
        if (c == 0) delete this;
        return c;
    }

private:
    uint8_t* buffer_;
    size_t bufferSize_;
    long width_;
    long height_;
    long rowBytes_;
    BMDPixelFormat pixelFormat_;
    std::atomic<ULONG> refCount_;
};

struct OutputState {
    IDeckLink* device = nullptr;
    IDeckLinkOutput* output = nullptr;
    DeckLinkVideoFrame* videoFrame = nullptr;
    int width = 0;
    int height = 0;
    int rowBytes = 0;
    size_t frameBufferSize = 0;
    bool audioEnabled = false;
    bool videoEnabled = false;
    std::atomic<int> activeWorkers{0};
    std::atomic<bool> closing{false};
};

static std::map<int, OutputState*> g_outputs;
static int g_nextHandle = 1;
static std::mutex g_mutex;

Napi::Value EnumerateDevices(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env);

    IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance();
    if (!iterator) {
        return result;
    }

    IDeckLink* deckLink = nullptr;
    uint32_t index = 0;
    while (iterator->Next(&deckLink) == S_OK) {
        const char* nameStr = nullptr;
        Napi::Object devObj = Napi::Object::New(env);
        devObj.Set("index", Napi::Number::New(env, index));

        if (deckLink->GetDisplayName(&nameStr) == S_OK && nameStr) {
            devObj.Set("name", Napi::String::New(env, nameStr));
            free((void*)nameStr);
        } else {
            devObj.Set("name", Napi::String::New(env, "Unknown"));
        }

        bool supportsOutput = false;
        IDeckLinkOutput* testOutput = nullptr;
        if (deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&testOutput) == S_OK && testOutput) {
            supportsOutput = true;
            testOutput->Release();
        }
        devObj.Set("supportsOutput", Napi::Boolean::New(env, supportsOutput));

        result.Set(index, devObj);
        deckLink->Release();
        index++;
    }

    iterator->Release();
    return result;
}

class OpenDeviceWorker : public Napi::AsyncWorker {
public:
    OpenDeviceWorker(Napi::Env env, int deviceIndex,
                     BMDDisplayMode displayMode, BMDPixelFormat pixelFormat,
                     uint32_t audioSampleRate, uint32_t audioSampleType,
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
        IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance();
        if (!iterator) {
            SetError("DeckLink drivers not installed");
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

        int rowBytes = 0;
        int bytesPerPixel = 2;
        if (pixelFormat_ == bmdFormat8BitBGRA) bytesPerPixel = 4;
        else if (pixelFormat_ == bmdFormat10BitYUV) bytesPerPixel = 0;

        if (pixelFormat_ == bmdFormat10BitYUV) {
            rowBytes = ((frameWidth + 47) / 48) * 128;
        } else {
            rowBytes = frameWidth * bytesPerPixel;
        }

        DeckLinkVideoFrame* videoFrame = new DeckLinkVideoFrame(
            frameWidth, frameHeight, rowBytes, pixelFormat_);

        if (!videoFrame->RawPtr()) {
            videoFrame->Release();
            output->DisableAudioOutput();
            output->DisableVideoOutput();
            output->Release();
            deckLink->Release();
            SetError("Failed to allocate video buffer");
            return;
        }

        OutputState* state = new OutputState();
        state->device = deckLink;
        state->output = output;
        state->videoFrame = videoFrame;
        state->width = frameWidth;
        state->height = frameHeight;
        state->rowBytes = rowBytes;
        state->frameBufferSize = videoFrame->BufferSize();
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

Napi::Value OpenDevice(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected options object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object opts = info[0].As<Napi::Object>();

    int deviceIndex = opts.Has("deviceIndex") ? opts.Get("deviceIndex").As<Napi::Number>().Int32Value() : 0;
    BMDDisplayMode displayMode = opts.Has("displayMode")
        ? static_cast<BMDDisplayMode>(opts.Get("displayMode").As<Napi::Number>().Uint32Value())
        : bmdModeHD1080i5994;
    BMDPixelFormat pixelFormat = opts.Has("pixelFormat")
        ? static_cast<BMDPixelFormat>(opts.Get("pixelFormat").As<Napi::Number>().Uint32Value())
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

    auto worker = new OpenDeviceWorker(env, deviceIndex, displayMode, pixelFormat,
                                        audioSampleRate, audioSampleType, audioChannels);
    worker->Queue();
    return worker->Promise();
}

class DisplayFrameWorker : public Napi::AsyncWorker {
public:
    DisplayFrameWorker(Napi::Env env, int handle, Napi::Buffer<uint8_t> videoBuf)
        : Napi::AsyncWorker(env),
          deferred_(Napi::Promise::Deferred::New(env)),
          handle_(handle),
          videoData_(videoBuf.Data()),
          videoSize_(videoBuf.Length()) {
        videoRef_ = Napi::Persistent(videoBuf);
    }

    Napi::Promise Promise() { return deferred_.Promise(); }

    void Execute() override {
        OutputState* state = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_outputs.find(handle_);
            if (it != g_outputs.end()) {
                state = it->second;
                if (state->closing.load()) {
                    state = nullptr;
                } else {
                    state->activeWorkers.fetch_add(1);
                    state_ = state;
                }
            }
        }

        if (!state || !state->output || !state->videoFrame) {
            SetError("Invalid output handle " + std::to_string(handle_));
            return;
        }

        uint8_t* framePtr = state->videoFrame->RawPtr();
        size_t copySize = videoSize_ < state->frameBufferSize ? videoSize_ : state->frameBufferSize;
        memcpy(framePtr, videoData_, copySize);
        if (copySize < state->frameBufferSize) {
            memset(framePtr + copySize, 0, state->frameBufferSize - copySize);
        }

        HRESULT hr = state->output->DisplayVideoFrameSync(state->videoFrame);
        if (hr != S_OK) {
            SetError("DisplayVideoFrameSync failed (HRESULT=" + std::to_string(hr) + ")");
        }
    }

    void OnOK() override {
        if (state_) state_->activeWorkers.fetch_sub(1);
        deferred_.Resolve(Napi::Boolean::New(Env(), true));
    }

    void OnError(const Napi::Error& err) override {
        if (state_) state_->activeWorkers.fetch_sub(1);
        deferred_.Reject(err.Value());
    }

private:
    Napi::Promise::Deferred deferred_;
    int handle_;
    uint8_t* videoData_;
    size_t videoSize_;
    OutputState* state_ = nullptr;
    Napi::Reference<Napi::Buffer<uint8_t>> videoRef_;
};

Napi::Value DisplayFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected (handle, videoBuf)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int handle = info[0].As<Napi::Number>().Int32Value();
    Napi::Buffer<uint8_t> videoBuf = info[1].As<Napi::Buffer<uint8_t>>();

    auto worker = new DisplayFrameWorker(env, handle, videoBuf);
    worker->Queue();
    return worker->Promise();
}

class WriteAudioWorker : public Napi::AsyncWorker {
public:
    WriteAudioWorker(Napi::Env env, int handle, Napi::Buffer<uint8_t> audioBuf)
        : Napi::AsyncWorker(env),
          deferred_(Napi::Promise::Deferred::New(env)),
          handle_(handle),
          audioData_(audioBuf.Data()),
          audioSize_(audioBuf.Length()),
          samplesWritten_(0) {
        audioRef_ = Napi::Persistent(audioBuf);
    }

    Napi::Promise Promise() { return deferred_.Promise(); }

    void Execute() override {
        OutputState* state = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_outputs.find(handle_);
            if (it != g_outputs.end()) {
                state = it->second;
                if (state->closing.load()) {
                    state = nullptr;
                } else {
                    state->activeWorkers.fetch_add(1);
                    state_ = state;
                }
            }
        }

        if (!state || !state->output) {
            SetError("Invalid output handle " + std::to_string(handle_));
            return;
        }

        if (!state->audioEnabled || audioSize_ == 0) {
            return;
        }

        uint32_t bytesPerSample = 4;
        uint32_t sampleCount = (uint32_t)(audioSize_ / bytesPerSample);
        if (sampleCount > 0) {
            HRESULT hr = state->output->WriteAudioSamplesSync(
                audioData_, sampleCount, &samplesWritten_);
            if (hr != S_OK) {
                SetError("WriteAudioSamplesSync failed (HRESULT=" + std::to_string(hr) + ")");
            }
        }
    }

    void OnOK() override {
        if (state_) state_->activeWorkers.fetch_sub(1);
        Napi::Env env = Env();
        Napi::Object result = Napi::Object::New(env);
        result.Set("samplesWritten", Napi::Number::New(env, (double)samplesWritten_));
        deferred_.Resolve(result);
    }

    void OnError(const Napi::Error& err) override {
        if (state_) state_->activeWorkers.fetch_sub(1);
        deferred_.Reject(err.Value());
    }

private:
    Napi::Promise::Deferred deferred_;
    int handle_;
    uint8_t* audioData_;
    size_t audioSize_;
    uint32_t samplesWritten_;
    OutputState* state_ = nullptr;
    Napi::Reference<Napi::Buffer<uint8_t>> audioRef_;
};

Napi::Value WriteAudio(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected (handle, audioBuf)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int handle = info[0].As<Napi::Number>().Int32Value();
    Napi::Buffer<uint8_t> audioBuf = info[1].As<Napi::Buffer<uint8_t>>();

    auto worker = new WriteAudioWorker(env, handle, audioBuf);
    worker->Queue();
    return worker->Promise();
}

Napi::Value CloseDevice(const Napi::CallbackInfo& info) {
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
            state->closing.store(true);
            g_outputs.erase(it);
        }
    }

    if (state) {
        int spins = 0;
        while (state->activeWorkers.load() > 0 && spins < 1000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            spins++;
        }

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
    IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance();
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

static void CleanupOutputs(void*) {
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

    exports.Set("enumerateDevices", Napi::Function::New(env, EnumerateDevices));
    exports.Set("openDevice", Napi::Function::New(env, OpenDevice));
    exports.Set("displayFrame", Napi::Function::New(env, DisplayFrame));
    exports.Set("writeAudio", Napi::Function::New(env, WriteAudio));
    exports.Set("closeDevice", Napi::Function::New(env, CloseDevice));
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
