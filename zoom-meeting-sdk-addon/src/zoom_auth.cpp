#include "zoom_addon.h"

#ifdef _WIN32
#include <windows.h>
#include "zoom_sdk.h"
#include "zoom_sdk_def.h"
#include "auth_service_interface.h"

using namespace ZOOMSDK;

class AuthServiceEventListener : public IAuthServiceEvent {
public:
    void onAuthenticationReturn(AuthResult ret) override {
        if (ret == AUTHRET_SUCCESS) {
            ZoomAddon::Instance().OnMeetingStatusChanged("AUTH_SUCCESS");
        } else {
            ZoomAddon::Instance().OnMeetingStatusChanged("AUTH_FAILED");
        }
    }

    void onLoginReturnWithReason(LOGINSTATUS ret, IAccountInfo* pAccountInfo, LoginFailReason reason) override {}
    void onLogout() override {}
    void onZoomIdentityExpired() override {}
    void onZoomAuthIdentityExpired() override {}
    void onNotificationServiceStatus(SDKNotificationServiceStatus status, SDKNotificationServiceError error) override {}
};

static AuthServiceEventListener* g_authListener = nullptr;
static IAuthService* g_authService = nullptr;

bool ZoomAddon::Initialize(const ZoomConfig& config) {
    config_ = config;

    InitParam initParam;
    memset(&initParam, 0, sizeof(initParam));
    initParam.strWebDomain = L"https://zoom.us";
    initParam.enableLogByDefault = true;

    initParam.rawdataOpts.enableRawdataIntermediateMode = false;
    initParam.rawdataOpts.videoRawdataMemoryMode = (ZoomSDKRawDataMemoryMode)1;
    initParam.rawdataOpts.audioRawdataMemoryMode = (ZoomSDKRawDataMemoryMode)1;
    initParam.rawdataOpts.shareRawdataMemoryMode = (ZoomSDKRawDataMemoryMode)1;

    printf("[ZoomNative] Initializing SDK (heap memory mode, intermediate OFF)\n");
    fflush(stdout);

    SDKError err = InitSDK(initParam);
    printf("[ZoomNative] InitSDK result=%d\n", (int)err);
    fflush(stdout);

    if (err != SDKERR_SUCCESS) {
        state_ = AddonState::Error;
        return false;
    }

    state_ = AddonState::Initialized;
    return true;
}

bool ZoomAddon::Authenticate() {
    if (state_ != AddonState::Initialized) return false;

    SDKError err = CreateAuthService(&g_authService);
    if (err != SDKERR_SUCCESS || !g_authService) {
        state_ = AddonState::Error;
        return false;
    }

    g_authListener = new AuthServiceEventListener();
    g_authService->SetEvent(g_authListener);

    AuthContext authCtx;
    std::string jwtStr = config_.jwtToken;
    std::wstring wJwt(jwtStr.begin(), jwtStr.end());
    authCtx.jwt_token = wJwt.c_str();

    err = g_authService->SDKAuth(authCtx);
    if (err != SDKERR_SUCCESS) {
        state_ = AddonState::Error;
        return false;
    }

    return true;
}

void ZoomAddon::Cleanup() {
    StopRawDataCapture();

    if (g_authService) {
        DestroyAuthService(g_authService);
        g_authService = nullptr;
    }
    if (g_authListener) {
        delete g_authListener;
        g_authListener = nullptr;
    }

    ZOOMSDK::CleanUPSDK();
    state_ = AddonState::Uninitialized;
    participants_.clear();
}

#else

bool ZoomAddon::Initialize(const ZoomConfig& config) {
    config_ = config;
    state_ = AddonState::Initialized;
    return true;
}

bool ZoomAddon::Authenticate() {
    state_ = AddonState::Authenticated;
    return true;
}

void ZoomAddon::Cleanup() {
    state_ = AddonState::Uninitialized;
    participants_.clear();
}

#endif
