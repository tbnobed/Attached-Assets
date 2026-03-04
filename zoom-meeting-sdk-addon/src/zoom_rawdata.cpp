#include "zoom_addon.h"

#ifdef _WIN32
#include <windows.h>
#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"
#include "rawdata/zoom_rawdata_api.h"
#include "rawdata/rawdata_renderer_interface.h"
#include "rawdata/rawdata_audio_helper_interface.h"

using namespace ZOOMSDK;

class PerUserVideoListener : public IZoomSDKRendererDelegate {
public:
    explicit PerUserVideoListener(uint32_t userId) : userId_(userId) {}

    void onRawDataFrameReceived(YUVRawDataI420* data) override {
        if (!data) return;

        int width = data->GetStreamWidth();
        int height = data->GetStreamHeight();
        if (width <= 0 || height <= 0) return;

        static int frameLogCount = 0;
        if (frameLogCount < 5 || frameLogCount % 300 == 0) {
            printf("[ZoomNative] Video frame: userId=%u %dx%d (frame #%d)\n", userId_, width, height, frameLogCount);
            fflush(stdout);
        }
        frameLogCount++;

        int rgbaSize = width * height * 4;
        auto* rgbaData = new uint8_t[rgbaSize];

        const unsigned char* yPlane = reinterpret_cast<const unsigned char*>(data->GetYBuffer());
        const unsigned char* uPlane = reinterpret_cast<const unsigned char*>(data->GetUBuffer());
        const unsigned char* vPlane = reinterpret_cast<const unsigned char*>(data->GetVBuffer());

        int yStride = width;
        int uStride = width / 2;
        int vStride = width / 2;

        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                int yIdx = j * yStride + i;
                int uvIdx = (j / 2) * uStride + (i / 2);

                int y = yPlane[yIdx];
                int u = uPlane[uvIdx] - 128;
                int v = vPlane[uvIdx] - 128;

                int r = y + (int)(1.402 * v);
                int g = y - (int)(0.344 * u) - (int)(0.714 * v);
                int b = y + (int)(1.772 * u);

                int rgbaIdx = (j * width + i) * 4;
                rgbaData[rgbaIdx + 0] = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
                rgbaData[rgbaIdx + 1] = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
                rgbaData[rgbaIdx + 2] = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
                rgbaData[rgbaIdx + 3] = 255;
            }
        }

        ZoomAddon::Instance().OnVideoFrame(userId_, rgbaData, width, height);
        delete[] rgbaData;
    }

    void onRawDataStatusChanged(RawDataStatus status) override {
        const char* statusName = "UNKNOWN";
        switch (status) {
            case RawData_On: statusName = "RawData_On"; break;
            case RawData_Off: statusName = "RawData_Off"; break;
            default: break;
        }
        printf("[ZoomNative] onRawDataStatusChanged: userId=%u status=%s\n", userId_, statusName);
        fflush(stdout);
    }
    void onRendererBeDestroyed() override {
        printf("[ZoomNative] onRendererBeDestroyed: userId=%u\n", userId_);
        fflush(stdout);
    }

    uint32_t GetUserId() const { return userId_; }

private:
    uint32_t userId_;
};

class AudioRawDataListener : public IZoomSDKAudioRawDataDelegate {
public:
    void onMixedAudioRawDataReceived(AudioRawData* data) override {}

    void onOneWayAudioRawDataReceived(AudioRawData* data, uint32_t node_id) override {
        if (!data) return;

        static int audioLogCount = 0;
        if (audioLogCount < 5 || audioLogCount % 1000 == 0) {
            printf("[ZoomNative] Audio frame: nodeId=%u len=%d sr=%d ch=%d (frame #%d)\n",
                   node_id, data->GetBufferLen(), data->GetSampleRate(), data->GetChannelNum(), audioLogCount);
            fflush(stdout);
        }
        audioLogCount++;

        ZoomAddon::Instance().OnAudioFrame(
            node_id,
            (const uint8_t*)data->GetBuffer(),
            data->GetBufferLen(),
            data->GetSampleRate(),
            data->GetChannelNum()
        );
    }

    void onShareAudioRawDataReceived(AudioRawData* data, uint32_t node_id) override {}
    void onOneWayInterpreterAudioRawDataReceived(AudioRawData* data, const zchar_t* pLanguageName) override {}
};

static AudioRawDataListener* g_audioListener = nullptr;
static std::map<uint32_t, std::pair<IZoomSDKRenderer*, PerUserVideoListener*>> g_videoRenderers;
static bool g_rawDataActive = false;

void subscribeUserVideo(uint32_t userId) {
    if (g_videoRenderers.count(userId)) {
        printf("[ZoomNative] subscribeUserVideo: userId=%u already subscribed\n", userId);
        fflush(stdout);
        return;
    }

    auto* listener = new PerUserVideoListener(userId);
    IZoomSDKRenderer* renderer = nullptr;
    auto err = createRenderer(&renderer, listener);
    if (err == SDKERR_SUCCESS && renderer) {
        auto resErr = renderer->setRawDataResolution(ZoomSDKResolution_1080P);
        auto subErr = renderer->subscribe(userId, RAW_DATA_TYPE_VIDEO);
        g_videoRenderers[userId] = { renderer, listener };
        printf("[ZoomNative] subscribeUserVideo: userId=%u renderer created (res=%d sub=%d)\n", userId, (int)resErr, (int)subErr);
        fflush(stdout);
    } else {
        printf("[ZoomNative] subscribeUserVideo: userId=%u FAILED (err=%d renderer=%p)\n", userId, (int)err, (void*)renderer);
        fflush(stdout);
        delete listener;
    }
}

void unsubscribeUserVideo(uint32_t userId) {
    auto it = g_videoRenderers.find(userId);
    if (it != g_videoRenderers.end()) {
        auto* renderer = it->second.first;
        auto* listener = it->second.second;
        renderer->unSubscribe();
        destroyRenderer(renderer);
        delete listener;
        g_videoRenderers.erase(it);
    }
}

void unsubscribeAllVideo() {
    for (auto& [userId, pair] : g_videoRenderers) {
        pair.first->unSubscribe();
        destroyRenderer(pair.first);
        delete pair.second;
    }
    g_videoRenderers.clear();
}

bool ZoomAddon::StartRawDataCapture() {
    if (g_rawDataActive) {
        printf("[ZoomNative] StartRawDataCapture: already active\n");
        fflush(stdout);
        return true;
    }

    auto* rawDataHelper = GetAudioRawdataHelper();
    if (!rawDataHelper) {
        printf("[ZoomNative] StartRawDataCapture: GetAudioRawdataHelper returned null\n");
        fflush(stdout);
        return false;
    }

    g_audioListener = new AudioRawDataListener();
    auto subErr = rawDataHelper->subscribe(g_audioListener);
    printf("[ZoomNative] StartRawDataCapture: audio subscribe result=%d\n", (int)subErr);
    fflush(stdout);

    if (subErr != SDKERR_SUCCESS) {
        printf("[ZoomNative] StartRawDataCapture: audio subscribe FAILED — raw data may not be permitted\n");
        printf("[ZoomNative] Check: 1) Zoom Marketplace app has 'Raw Data' enabled\n");
        printf("[ZoomNative]        2) Meeting SDK app type allows raw data access\n");
        fflush(stdout);
        delete g_audioListener;
        g_audioListener = nullptr;
    }

    g_rawDataActive = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        printf("[ZoomNative] StartRawDataCapture: subscribing video for %zu existing participants\n", participants_.size());
        fflush(stdout);
        for (const auto& [userId, info] : participants_) {
            subscribeUserVideo(userId);
        }
    }

    return true;
}

void ZoomAddon::RetryVideoSubscriptions() {
    if (!g_rawDataActive) return;
    std::lock_guard<std::mutex> lock(mutex_);
    printf("[ZoomNative] RetryVideoSubscriptions: %zu participants\n", participants_.size());
    fflush(stdout);
    for (const auto& [userId, info] : participants_) {
        if (g_videoRenderers.count(userId) == 0) {
            subscribeUserVideo(userId);
        }
    }

    if (!g_audioListener) {
        auto* rawDataHelper = GetAudioRawdataHelper();
        if (rawDataHelper) {
            g_audioListener = new AudioRawDataListener();
            auto subErr = rawDataHelper->subscribe(g_audioListener);
            printf("[ZoomNative] RetryVideoSubscriptions: audio re-subscribe result=%d\n", (int)subErr);
            fflush(stdout);
            if (subErr != SDKERR_SUCCESS) {
                delete g_audioListener;
                g_audioListener = nullptr;
            }
        }
    }
}

void ZoomAddon::SubscribeParticipantVideo(uint32_t userId) {
    if (g_rawDataActive) {
        subscribeUserVideo(userId);
    }
}

void ZoomAddon::UnsubscribeParticipantVideo(uint32_t userId) {
    unsubscribeUserVideo(userId);
}

void ZoomAddon::StopRawDataCapture() {
    unsubscribeAllVideo();

    if (g_audioListener) {
        auto* rawDataHelper = GetAudioRawdataHelper();
        if (rawDataHelper) {
            rawDataHelper->unSubscribe();
        }
        delete g_audioListener;
        g_audioListener = nullptr;
    }

    g_rawDataActive = false;
}

#else

bool ZoomAddon::StartRawDataCapture() {
    return true;
}

void ZoomAddon::SubscribeParticipantVideo(uint32_t userId) {}
void ZoomAddon::UnsubscribeParticipantVideo(uint32_t userId) {}
void ZoomAddon::RetryVideoSubscriptions() {}
void ZoomAddon::StopRawDataCapture() {}

#endif
