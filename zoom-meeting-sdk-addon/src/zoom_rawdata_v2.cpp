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

    void onRawDataStatusChanged(RawDataStatus status) override {}
    void onRendererBeDestroyed() override {}

    uint32_t GetUserId() const { return userId_; }

private:
    uint32_t userId_;
};

class AudioRawDataListener : public IZoomSDKAudioRawDataDelegate {
public:
    void onMixedAudioRawDataReceived(AudioRawData* data) override {}

    void onOneWayAudioRawDataReceived(AudioRawData* data, uint32_t node_id) override {
        if (!data) return;

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
    if (g_videoRenderers.count(userId)) return;

    auto* listener = new PerUserVideoListener(userId);
    IZoomSDKRenderer* renderer = nullptr;
    auto err = createRenderer(&renderer, listener);
    if (err == SDKERR_SUCCESS && renderer) {
        renderer->setRawDataResolution(ZoomSDKResolution_1080P);
        renderer->subscribe(userId, RAW_DATA_TYPE_VIDEO);
        g_videoRenderers[userId] = { renderer, listener };
    } else {
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
    if (g_rawDataActive) return true;

    g_audioListener = new AudioRawDataListener();
    auto* audioHelper = GetAudioRawdataHelper();
    if (audioHelper) {
        audioHelper->subscribe(g_audioListener);
    }

    g_rawDataActive = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [userId, info] : participants_) {
            subscribeUserVideo(userId);
        }
    }

    return true;
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
        auto* audioHelper = GetAudioRawdataHelper();
        if (audioHelper) {
            audioHelper->unSubscribe();
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
void ZoomAddon::StopRawDataCapture() {}

#endif
