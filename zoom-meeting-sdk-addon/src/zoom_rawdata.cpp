#include "zoom_addon.h"

#ifdef _WIN32
#include <windows.h>
#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"
#include "rawdata/zoom_rawdata_api.h"
#include "rawdata/rawdata_renderer_interface.h"
#include "rawdata/rawdata_audio_helper_interface.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_recording_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"

using namespace ZOOMSDK;

extern IMeetingService* g_meetingService;

class PerUserVideoListener : public IZoomSDKRendererDelegate {
public:
    explicit PerUserVideoListener(uint32_t userId) : userId_(userId), frameLogCount_(0) {}

    void onRawDataFrameReceived(YUVRawDataI420* data) override {
        if (!data) return;

        int width = data->GetStreamWidth();
        int height = data->GetStreamHeight();
        if (width <= 0 || height <= 0) return;

        if (data->CanAddRef()) {
            data->AddRef();
        }

        const unsigned char* yPlane = reinterpret_cast<const unsigned char*>(data->GetYBuffer());
        const unsigned char* uPlane = reinterpret_cast<const unsigned char*>(data->GetUBuffer());
        const unsigned char* vPlane = reinterpret_cast<const unsigned char*>(data->GetVBuffer());

        if (!yPlane || !uPlane || !vPlane) {
            const unsigned char* rawBuf = reinterpret_cast<const unsigned char*>(data->GetBuffer());
            unsigned int rawLen = data->GetBufferLen();

            if (frameLogCount_ < 5) {
                printf("[ZoomNative] Video frame: userId=%u %dx%d Y/U/V null, GetBuffer=%p len=%u\n",
                       userId_, width, height, (void*)rawBuf, rawLen);
                fflush(stdout);
            }

            unsigned int expectedI420 = (unsigned int)(width * height * 3 / 2);
            if (rawBuf && rawLen >= expectedI420) {
                yPlane = rawBuf;
                uPlane = rawBuf + (width * height);
                vPlane = rawBuf + (width * height) + (width * height / 4);
            } else {
                if (frameLogCount_ < 3) {
                    printf("[ZoomNative] Video frame: userId=%u skipping — no usable buffer\n", userId_);
                    fflush(stdout);
                }
                frameLogCount_++;
                if (data->CanAddRef()) data->Release();
                return;
            }
        }

        if (frameLogCount_ < 5 || frameLogCount_ % 300 == 0) {
            printf("[ZoomNative] Video frame: userId=%u %dx%d (frame #%d)\n", userId_, width, height, frameLogCount_);
            fflush(stdout);
        }
        frameLogCount_++;

        int bgraSize = width * height * 4;
        auto* bgraData = new uint8_t[bgraSize];

        int yStride = width;
        int uStride = width / 2;

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

                int idx = (j * width + i) * 4;
                bgraData[idx + 0] = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
                bgraData[idx + 1] = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
                bgraData[idx + 2] = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
                bgraData[idx + 3] = 255;
            }
        }

        ZoomAddon::Instance().OnVideoFrame(userId_, bgraData, width, height);
        delete[] bgraData;

        if (data->CanAddRef()) data->Release();
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
    int frameLogCount_;
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

class RecordingEventListener : public IMeetingRecordingCtrlEvent {
public:
    void onRecordingStatus(RecordingStatus status) override {
        const char* statusName = "UNKNOWN";
        switch (status) {
            case Recording_Start: statusName = "Recording_Start"; break;
            case Recording_Stop: statusName = "Recording_Stop"; break;
            case Recording_Pause: statusName = "Recording_Pause"; break;
            case Recording_DiskFull: statusName = "Recording_DiskFull"; break;
            default: break;
        }
        printf("[ZoomNative] onRecordingStatus: %s (%d)\n", statusName, (int)status);
        fflush(stdout);

        if (status == Recording_Start) {
            printf("[ZoomNative] Raw recording started — now subscribing raw data\n");
            fflush(stdout);
            ZoomAddon::Instance().OnRawRecordingStarted();
        }
    }

    void onCloudRecordingStatus(RecordingStatus status) override {}
    void onRecordPrivilegeChanged(bool bCanRec) override {
        printf("[ZoomNative] onRecordPrivilegeChanged: canRecord=%d\n", bCanRec);
        fflush(stdout);
        if (bCanRec) {
            ZoomAddon::Instance().OnRecordingPermissionGranted();
        }
    }
    void onLocalRecordingPrivilegeRequestStatus(RequestLocalRecordingStatus status) override {
        printf("[ZoomNative] onLocalRecordingPrivilegeRequestStatus: %d\n", (int)status);
        fflush(stdout);
    }
    void onRequestCloudRecordingResponse(RequestStartCloudRecordingStatus status) override {}
    void onLocalRecordingPrivilegeRequested(IRequestLocalRecordingPrivilegeHandler* handler) override {
        printf("[ZoomNative] onLocalRecordingPrivilegeRequested\n");
        fflush(stdout);
    }
    void onStartCloudRecordingRequested(IRequestStartCloudRecordingHandler* handler) override {}
    void onCustomizedLocalRecordingSourceNotification(ICustomizedLocalRecordingLayoutHelper* pHelper) override {}
    void onCloudRecordingStorageFull(time_t gracePeriodDate) override {}
    void onRecording2MP4Done(bool bSuccess, int iResult, const zchar_t* szPath) override {}
    void onRecording2MP4Processing(int iPercentage) override {}
    void onEnableAndStartSmartRecordingRequested(IRequestEnableAndStartSmartRecordingHandler* handler) override {}
    void onSmartRecordingEnableActionCallback(ISmartRecordingEnableActionHandler* handler) override {}
};

static AudioRawDataListener* g_audioListener = nullptr;
static RecordingEventListener* g_recordingListener = nullptr;
static std::map<uint32_t, std::pair<IZoomSDKRenderer*, PerUserVideoListener*>> g_videoRenderers;
static bool g_rawDataActive = false;
static bool g_rawRecordingStarted = false;

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

bool ZoomAddon::StartRawRecording() {
    if (g_rawRecordingStarted) {
        printf("[ZoomNative] StartRawRecording: already started\n");
        fflush(stdout);
        return true;
    }

    if (!g_meetingService) {
        printf("[ZoomNative] StartRawRecording: no meeting service\n");
        fflush(stdout);
        return false;
    }

    auto* recCtrl = g_meetingService->GetMeetingRecordingController();
    if (!recCtrl) {
        printf("[ZoomNative] StartRawRecording: GetMeetingRecordingController returned null\n");
        fflush(stdout);
        return false;
    }

    if (!g_recordingListener) {
        g_recordingListener = new RecordingEventListener();
        recCtrl->SetEvent(g_recordingListener);
        printf("[ZoomNative] StartRawRecording: recording event listener set\n");
        fflush(stdout);
    }

    auto err = recCtrl->StartRawRecording();
    printf("[ZoomNative] StartRawRecording: result=%d\n", (int)err);
    fflush(stdout);

    if (err == SDKERR_SUCCESS) {
        printf("[ZoomNative] StartRawRecording succeeded — activating raw data capture\n");
        fflush(stdout);
        OnRawRecordingStarted();
        return true;
    }

    if (err == SDKERR_NO_PERMISSION) {
        printf("[ZoomNative] StartRawRecording: no permission — requesting local recording privilege\n");
        fflush(stdout);
        auto reqErr = recCtrl->RequestLocalRecordingPrivilege();
        printf("[ZoomNative] RequestLocalRecordingPrivilege: result=%d\n", (int)reqErr);
        fflush(stdout);
    } else {
        printf("[ZoomNative] StartRawRecording: unexpected error %d — will retry via RetryVideoSubscriptions\n", (int)err);
        fflush(stdout);
        if (eventCallback_) {
            int errCode = (int)err;
            eventCallback_.NonBlockingCall([errCode](Napi::Env env, Napi::Function jsCallback) {
                auto obj = Napi::Object::New(env);
                obj.Set("type", Napi::String::New(env, "meeting-status"));
                obj.Set("status", Napi::String::New(env, "RAW_RECORDING_ERROR"));
                obj.Set("errorCode", Napi::Number::New(env, errCode));
                jsCallback.Call({ obj });
            });
        }
    }

    return false;
}

void ZoomAddon::OnRecordingPermissionGranted() {
    if (g_rawRecordingStarted) {
        printf("[ZoomNative] OnRecordingPermissionGranted: already started, ignoring\n");
        fflush(stdout);
        return;
    }
    printf("[ZoomNative] Recording permission granted — attempting StartRawRecording\n");
    fflush(stdout);
    if (g_meetingService) {
        auto* recCtrl = g_meetingService->GetMeetingRecordingController();
        if (recCtrl) {
            auto err = recCtrl->StartRawRecording();
            printf("[ZoomNative] StartRawRecording after permission: result=%d\n", (int)err);
            fflush(stdout);
        }
    }
}

void ZoomAddon::OnRawRecordingStarted() {
    g_rawRecordingStarted = true;

    if (eventCallback_) {
        eventCallback_.NonBlockingCall([](Napi::Env env, Napi::Function jsCallback) {
            auto obj = Napi::Object::New(env);
            obj.Set("type", Napi::String::New(env, "meeting-status"));
            obj.Set("status", Napi::String::New(env, "RAW_RECORDING_STARTED"));
            jsCallback.Call({ obj });
        });
    }

    StartRawDataCapture();
}

bool ZoomAddon::StartRawDataCapture() {
    if (g_rawDataActive) {
        printf("[ZoomNative] StartRawDataCapture: already active\n");
        fflush(stdout);
        return true;
    }

    if (!g_rawRecordingStarted) {
        printf("[ZoomNative] StartRawDataCapture: raw recording not started yet — deferring\n");
        fflush(stdout);
        return false;
    }

    printf("[ZoomNative] StartRawDataCapture: attempting audio subscribe + video renderers\n");
    fflush(stdout);

    if (g_meetingService) {
        auto* audioCtrl = g_meetingService->GetMeetingAudioController();
        if (audioCtrl) {
            auto joinErr = audioCtrl->JoinVoip();
            printf("[ZoomNative] StartRawDataCapture: JoinVoip result=%d\n", (int)joinErr);
            fflush(stdout);
        }
    }

    g_audioListener = new AudioRawDataListener();
    SDKError subErr = SDKERR_WRONG_USAGE;

    auto* rawDataHelper = GetAudioRawdataHelper();
    if (rawDataHelper) {
        subErr = rawDataHelper->subscribe(g_audioListener);
        printf("[ZoomNative] StartRawDataCapture: audio subscribe result=%d\n", (int)subErr);
        fflush(stdout);
    } else {
        printf("[ZoomNative] StartRawDataCapture: GetAudioRawdataHelper returned null\n");
        fflush(stdout);
    }

    if (subErr != SDKERR_SUCCESS) {
        printf("[ZoomNative] StartRawDataCapture: audio subscribe FAILED (err=%d) — will retry\n", (int)subErr);
        fflush(stdout);
        delete g_audioListener;
        g_audioListener = nullptr;
    } else {
        printf("[ZoomNative] StartRawDataCapture: audio subscribe SUCCESS\n");
        fflush(stdout);
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
    if (!g_rawRecordingStarted) {
        printf("[ZoomNative] RetryVideoSubscriptions: raw recording not started, trying StartRawRecording\n");
        fflush(stdout);
        StartRawRecording();
        return;
    }

    if (!g_rawDataActive) {
        printf("[ZoomNative] RetryVideoSubscriptions: raw data not active, trying StartRawDataCapture\n");
        fflush(stdout);
        StartRawDataCapture();
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    printf("[ZoomNative] RetryVideoSubscriptions: %zu participants\n", participants_.size());
    fflush(stdout);
    for (const auto& [userId, info] : participants_) {
        if (g_videoRenderers.count(userId) == 0) {
            subscribeUserVideo(userId);
        }
    }

    if (!g_audioListener) {
        g_audioListener = new AudioRawDataListener();
        SDKError subErr = SDKERR_WRONG_USAGE;

        auto* rawDataHelper = GetAudioRawdataHelper();
        if (rawDataHelper) {
            subErr = rawDataHelper->subscribe(g_audioListener);
            printf("[ZoomNative] RetryVideoSubscriptions: audio subscribe=%d\n", (int)subErr);
            fflush(stdout);
        }
        if (subErr != SDKERR_SUCCESS) {
            printf("[ZoomNative] RetryVideoSubscriptions: audio subscribe FAILED (err=%d)\n", (int)subErr);
            fflush(stdout);
            delete g_audioListener;
            g_audioListener = nullptr;
        } else {
            printf("[ZoomNative] RetryVideoSubscriptions: audio subscribe SUCCESS\n");
            fflush(stdout);
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

    if (g_meetingService && g_rawRecordingStarted) {
        auto* recCtrl = g_meetingService->GetMeetingRecordingController();
        if (recCtrl) {
            recCtrl->StopRawRecording();
            printf("[ZoomNative] StopRawRecording called\n");
            fflush(stdout);
        }
    }

    if (g_recordingListener) {
        delete g_recordingListener;
        g_recordingListener = nullptr;
    }

    g_rawDataActive = false;
    g_rawRecordingStarted = false;
}

#else

bool ZoomAddon::StartRawDataCapture() {
    return true;
}
bool ZoomAddon::StartRawRecording() {
    return true;
}

void ZoomAddon::SubscribeParticipantVideo(uint32_t userId) {}
void ZoomAddon::UnsubscribeParticipantVideo(uint32_t userId) {}
void ZoomAddon::RetryVideoSubscriptions() {}
void ZoomAddon::OnRecordingPermissionGranted() {}
void ZoomAddon::OnRawRecordingStarted() {}
void ZoomAddon::StopRawDataCapture() {}

#endif
