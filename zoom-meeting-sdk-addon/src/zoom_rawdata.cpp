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
#include "meeting_service_components/meeting_participants_ctrl_interface.h"
#include "meeting_service_components/meeting_video_interface.h"

using namespace ZOOMSDK;

extern IMeetingService* g_meetingService;

void subscribeUserVideo(uint32_t userId);
extern std::mutex g_videoMutex;
extern std::set<uint32_t> g_videoPendingResubscribe;

class PerUserVideoListener : public IZoomSDKRendererDelegate {
public:
    explicit PerUserVideoListener(uint32_t userId) : userId_(userId), frameLogCount_(0), subscribed_(false) {}

    void onRawDataFrameReceived(YUVRawDataI420* data) override {
        if (!data) return;

        unsigned int width = data->GetStreamWidth();
        unsigned int height = data->GetStreamHeight();
        if (width == 0 || height == 0) return;

        bool isLimited = data->IsLimitedI420();
        bool canRef = data->CanAddRef();

        if (canRef) {
            data->AddRef();
        }

        unsigned int yuvSize = width * height * 3 / 2;
        const unsigned char* yPlane = nullptr;
        const unsigned char* uPlane = nullptr;
        const unsigned char* vPlane = nullptr;
        uint8_t* localCopy = nullptr;

        const char* yBuf = data->GetYBuffer();
        const char* uBuf = data->GetUBuffer();
        const char* vBuf = data->GetVBuffer();
        const char* rawBuf = data->GetBuffer();
        unsigned int rawLen = data->GetBufferLen();

        if (yBuf && uBuf && vBuf) {
            localCopy = new uint8_t[yuvSize];
            memcpy(localCopy, yBuf, width * height);
            memcpy(localCopy + width * height, uBuf, width * height / 4);
            memcpy(localCopy + width * height + width * height / 4, vBuf, width * height / 4);
            yPlane = localCopy;
            uPlane = localCopy + (width * height);
            vPlane = localCopy + (width * height) + (width * height / 4);
        } else if (rawBuf && rawLen >= yuvSize) {
            localCopy = new uint8_t[yuvSize];
            memcpy(localCopy, rawBuf, yuvSize);
            yPlane = localCopy;
            uPlane = localCopy + (width * height);
            vPlane = localCopy + (width * height) + (width * height / 4);
        }

        if (!yPlane) {
            if (frameLogCount_ < 10 || frameLogCount_ % 500 == 0) {
                printf("[ZoomNative] Video frame: userId=%u %ux%u NO DATA (frame #%d) Y=%p U=%p V=%p buf=%p len=%u canRef=%d limited=%d\n",
                       userId_, width, height, frameLogCount_,
                       (void*)yBuf, (void*)uBuf, (void*)vBuf,
                       (void*)rawBuf, rawLen, (int)canRef, (int)isLimited);
                fflush(stdout);
            }
            frameLogCount_++;
            delete[] localCopy;
            if (canRef) data->Release();
            return;
        }

        if (frameLogCount_ < 5 || frameLogCount_ % 300 == 0) {
            printf("[ZoomNative] Video frame: userId=%u %ux%u (frame #%d) limited=%d src=%s\n",
                   userId_, width, height, frameLogCount_, (int)isLimited,
                   (yBuf && uBuf && vBuf) ? "Y/U/V" : "GetBuffer");
            fflush(stdout);
        }
        frameLogCount_++;

        unsigned int bgraSize = width * height * 4;
        auto* bgraData = new uint8_t[bgraSize];

        unsigned int yStride = width;
        unsigned int uStride = width / 2;

        for (unsigned int j = 0; j < height; j++) {
            for (unsigned int i = 0; i < width; i++) {
                unsigned int yIdx = j * yStride + i;
                unsigned int uvIdx = (j / 2) * uStride + (i / 2);

                int y = yPlane[yIdx];
                int u = uPlane[uvIdx] - 128;
                int v = vPlane[uvIdx] - 128;

                int r = y + (int)(1.402 * v);
                int g = y - (int)(0.344 * u) - (int)(0.714 * v);
                int b = y + (int)(1.772 * u);

                unsigned int idx = (j * width + i) * 4;
                bgraData[idx + 0] = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
                bgraData[idx + 1] = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
                bgraData[idx + 2] = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
                bgraData[idx + 3] = 255;
            }
        }

        ZoomAddon::Instance().OnVideoFrame(userId_, bgraData, width, height);
        delete[] bgraData;
        delete[] localCopy;
        if (canRef) data->Release();
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

        if (status == RawData_Off) {
            subscribed_ = false;
        }

        if (status == RawData_On && !subscribed_) {
            subscribed_ = true;
            printf("[ZoomNative] onRawDataStatusChanged: userId=%u video is ON — marking pending resubscribe\n", userId_);
            fflush(stdout);
            {
                std::lock_guard<std::mutex> lock(g_videoMutex);
                g_videoPendingResubscribe.insert(userId_);
            }
        }
    }

    void onRendererBeDestroyed() override {
        printf("[ZoomNative] onRendererBeDestroyed: userId=%u\n", userId_);
        fflush(stdout);
    }

    uint32_t GetUserId() const { return userId_; }

private:
    uint32_t userId_;
    int frameLogCount_;
    bool subscribed_;
};

class AudioRawDataListener : public IZoomSDKAudioRawDataDelegate {
public:
    void onMixedAudioRawDataReceived(AudioRawData* data) override {}

    void onOneWayAudioRawDataReceived(AudioRawData* data, uint32_t node_id) override {
        if (!data) return;

        auto& count = audioFrameCounts_[node_id];
        if (count < 5 || count % 1000 == 0) {
            printf("[ZoomNative] Audio frame: nodeId=%u len=%d sr=%d ch=%d (frame #%d)\n",
                   node_id, data->GetBufferLen(), data->GetSampleRate(), data->GetChannelNum(), count);
            fflush(stdout);
        }
        count++;

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

private:
    std::map<uint32_t, int> audioFrameCounts_;
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
std::mutex g_videoMutex;
static std::map<uint32_t, std::pair<IZoomSDKRenderer*, PerUserVideoListener*>> g_videoRenderers;
static std::set<uint32_t> g_videoSubscribedOK;
std::set<uint32_t> g_videoPendingResubscribe;
static bool g_rawDataActive = false;
static bool g_rawRecordingStarted = false;

bool isUserVideoOn(uint32_t userId) {
    if (!g_meetingService) return false;
    auto* pCtrl = g_meetingService->GetMeetingParticipantsController();
    if (!pCtrl) return false;
    auto* info = pCtrl->GetUserByUserID(userId);
    if (!info) return false;
    return !info->IsVideoOn() ? false : true;
}

void subscribeUserVideo(uint32_t userId) {
    std::lock_guard<std::mutex> lock(g_videoMutex);

    if (g_videoSubscribedOK.count(userId)) {
        return;
    }

    if (!g_rawDataActive) {
        printf("[ZoomNative] subscribeUserVideo: userId=%u deferred — raw data not active yet\n", userId);
        fflush(stdout);
        return;
    }

    if (g_videoRenderers.count(userId)) {
        auto* oldRenderer = g_videoRenderers[userId].first;
        auto* oldListener = g_videoRenderers[userId].second;
        oldRenderer->unSubscribe();
        oldRenderer->setRawDataResolution(ZoomSDKResolution_1080P);
        auto subErr = oldRenderer->subscribe(userId, RAW_DATA_TYPE_VIDEO);
        printf("[ZoomNative] subscribeUserVideo: userId=%u RE-subscribe result=%d (1080P)\n", userId, (int)subErr);
        fflush(stdout);
        if (subErr == SDKERR_SUCCESS) {
            g_videoSubscribedOK.insert(userId);
            return;
        }
        printf("[ZoomNative] subscribeUserVideo: userId=%u RE-subscribe failed — destroying old renderer, creating fresh\n", userId);
        fflush(stdout);
        oldRenderer->unSubscribe();
        destroyRenderer(oldRenderer);
        delete oldListener;
        g_videoRenderers.erase(userId);
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
        if (subErr == SDKERR_SUCCESS) {
            g_videoSubscribedOK.insert(userId);
        } else {
            printf("[ZoomNative] subscribeUserVideo: userId=%u sub failed — will retry on RawData_On or video status change\n", userId);
            fflush(stdout);
        }
    } else {
        printf("[ZoomNative] subscribeUserVideo: userId=%u createRenderer FAILED (err=%d)\n", userId, (int)err);
        fflush(stdout);
        delete listener;
    }
}

class MeetingVideoCtrlEventListener : public IMeetingVideoCtrlEvent {
public:
    void onUserVideoStatusChange(unsigned int userId, VideoStatus status) override {
        const char* statusName = "UNKNOWN";
        switch (status) {
            case Video_ON: statusName = "Video_ON"; break;
            case Video_OFF: statusName = "Video_OFF"; break;
            default: break;
        }
        printf("[ZoomNative] onUserVideoStatusChange: userId=%u status=%s\n", userId, statusName);
        fflush(stdout);

        if (status == Video_OFF) {
            std::lock_guard<std::mutex> lock(g_videoMutex);
            g_videoSubscribedOK.erase(userId);
            printf("[ZoomNative] onUserVideoStatusChange: userId=%u video OFF — cleared subscription OK state\n", userId);
            fflush(stdout);
        }

        if (status == Video_ON && g_rawDataActive) {
            {
                std::lock_guard<std::mutex> lock(g_videoMutex);
                g_videoSubscribedOK.erase(userId);
            }
            printf("[ZoomNative] onUserVideoStatusChange: userId=%u video ON — (re)subscribing\n", userId);
            fflush(stdout);
            subscribeUserVideo(userId);
        }
    }
    void onSpotlightedUserListChangeNotification(IList<unsigned int>* lstSpotlightedUserID) override {}
    void onHostRequestStartVideo(IRequestStartVideoHandler* handler) override {}
    void onActiveSpeakerVideoUserChanged(unsigned int userid) override {}
    void onActiveVideoUserChanged(unsigned int userid) override {}
    void onUserVideoQualityChanged(VideoConnectionQuality quality, unsigned int userid) override {}
    void onHostVideoOrderUpdated(IList<unsigned int>* lst) override {}
    void onLocalVideoOrderUpdated(IList<unsigned int>* lst) override {}
    void onFollowHostVideoOrderChanged(bool bFollow) override {}
    void onVideoAlphaChannelStatusChanged(bool bEnabled) override {}
    void onCameraControlRequestReceived(unsigned int userId, CameraControlRequestType reqType, ICameraControlRequestHandler* handler) override {}
    void onCameraControlRequestResult(unsigned int userId, CameraControlRequestResult result) override {}
};

static MeetingVideoCtrlEventListener* g_videoCtrlListener = nullptr;

void unsubscribeUserVideo(uint32_t userId) {
    std::lock_guard<std::mutex> lock(g_videoMutex);
    g_videoSubscribedOK.erase(userId);
    g_videoPendingResubscribe.erase(userId);
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
    std::lock_guard<std::mutex> lock(g_videoMutex);
    for (auto& [userId, pair] : g_videoRenderers) {
        pair.first->unSubscribe();
        destroyRenderer(pair.first);
        delete pair.second;
    }
    g_videoRenderers.clear();
    g_videoSubscribedOK.clear();
    g_videoPendingResubscribe.clear();
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

    if (!g_videoCtrlListener && g_meetingService) {
        auto* videoCtrl = g_meetingService->GetMeetingVideoController();
        if (videoCtrl) {
            g_videoCtrlListener = new MeetingVideoCtrlEventListener();
            videoCtrl->SetEvent(g_videoCtrlListener);
            printf("[ZoomNative] StartRawDataCapture: video controller event listener set\n");
            fflush(stdout);
        }
    }

    printf("[ZoomNative] StartRawDataCapture: NO upfront video subscriptions — waiting for onUserVideoStatusChange / onRawDataStatusChanged events\n");
    fflush(stdout);

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

    std::set<uint32_t> pending;
    {
        std::lock_guard<std::mutex> lock(g_videoMutex);
        pending = g_videoPendingResubscribe;
        g_videoPendingResubscribe.clear();
    }
    for (auto uid : pending) {
        printf("[ZoomNative] RetryVideoSubscriptions: processing pending resubscribe for userId=%u\n", uid);
        fflush(stdout);
        subscribeUserVideo(uid);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    int needSub = 0;
    for (const auto& [userId, info] : participants_) {
        {
            std::lock_guard<std::mutex> vlock(g_videoMutex);
            if (g_videoSubscribedOK.count(userId)) continue;
        }
        if (isUserVideoOn(userId)) {
            needSub++;
            printf("[ZoomNative] RetryVideoSubscriptions: userId=%u video ON but not subscribed — subscribing\n", userId);
            fflush(stdout);
            subscribeUserVideo(userId);
        }
    }
    {
        std::lock_guard<std::mutex> vlock(g_videoMutex);
        printf("[ZoomNative] RetryVideoSubscriptions: %zu participants, %d needed retry, %zu already OK\n",
               participants_.size(), needSub, g_videoSubscribedOK.size());
        fflush(stdout);
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

    if (g_videoCtrlListener) {
        delete g_videoCtrlListener;
        g_videoCtrlListener = nullptr;
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
