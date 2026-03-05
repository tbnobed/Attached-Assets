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

static HWND g_hiddenHwnd = nullptr;

static void CreateHiddenWindow() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ZoomSDKHiddenWindow";
    RegisterClassW(&wc);

    g_hiddenHwnd = CreateWindowW(
        L"ZoomSDKHiddenWindow",
        L"ZoomSDK",
        WS_OVERLAPPEDWINDOW,
        0, 0, 1, 1,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (g_hiddenHwnd) {
        printf("[ZoomNative] Hidden HWND created: %p\n", (void*)g_hiddenHwnd);
    } else {
        printf("[ZoomNative] WARNING: Failed to create hidden HWND (err=%lu)\n", GetLastError());
    }
    fflush(stdout);
}

bool ZoomAddon::Initialize(const ZoomConfig& config) {
    config_ = config;

    CreateHiddenWindow();

    InitParam initParam;
    memset(&initParam, 0, sizeof(initParam));
    initParam.strWebDomain = L"https://zoom.us";
    initParam.enableLogByDefault = true;
    initParam.hResInstance = GetModuleHandle(nullptr);
    initParam.uiWindowIconSmallID = 0;
    initParam.uiWindowIconBigID = 0;

    initParam.rawdataOpts.enableRawdataIntermediateMode = false;

    printf("[ZoomNative] Initializing SDK (rawdata intermediate mode DISABLED, hResInstance set)\n");
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

    if (g_hiddenHwnd) {
        DestroyWindow(g_hiddenHwnd);
        g_hiddenHwnd = nullptr;
        UnregisterClassW(L"ZoomSDKHiddenWindow", GetModuleHandle(nullptr));
    }

    state_ = AddonState::Uninitialized;
    participants_.clear();
}

#elif defined(__APPLE__)

#import <ZoomSDK/ZoomSDK.h>

@interface ZoomAuthDelegateImpl : NSObject <ZoomSDKAuthDelegate>
@end

@implementation ZoomAuthDelegateImpl

- (void)onZoomSDKAuthReturn:(ZoomSDKAuthError)returnValue {
    if (returnValue == ZoomSDKAuthError_Success) {
        printf("[ZoomNative] Auth SUCCESS\n");
        fflush(stdout);
        ZoomAddon::Instance().OnMeetingStatusChanged("AUTH_SUCCESS");
    } else {
        printf("[ZoomNative] Auth FAILED: result=%d\n", (int)returnValue);
        fflush(stdout);
        ZoomAddon::Instance().OnMeetingStatusChanged("AUTH_FAILED");
    }
}

- (void)onZoomAuthIdentityExpired {
    printf("[ZoomNative] Auth identity expired\n");
    fflush(stdout);
}

@end

static ZoomAuthDelegateImpl* g_authDelegate = nil;

bool ZoomAddon::Initialize(const ZoomConfig& config) {
    config_ = config;

    @autoreleasepool {
        ZoomSDK* sdk = [ZoomSDK sharedSDK];
        sdk.videoRawDataMode = ZoomSDKRawDataMemoryMode_Heap;
        sdk.audioRawDataMode = ZoomSDKRawDataMemoryMode_Heap;
        sdk.shareRawDataMode = ZoomSDKRawDataMemoryMode_Heap;
        sdk.enableRawdataIntermediateMode = NO;

        ZoomSDKInitParams* params = [[ZoomSDKInitParams alloc] init];
        params.zoomDomain = @"https://zoom.us";
        params.enableLog = YES;
        params.needCustomizedUI = YES;

        printf("[ZoomNative] Initializing macOS SDK (customUI=YES, rawdata heap mode)\n");
        fflush(stdout);

        ZoomSDKError err = [sdk initSDKWithParams:params];
        printf("[ZoomNative] InitSDK result=%d\n", (int)err);
        fflush(stdout);

        if (err != ZoomSDKError_Success) {
            state_ = AddonState::Error;
            return false;
        }
    }

    state_ = AddonState::Initialized;
    return true;
}

bool ZoomAddon::Authenticate() {
    if (state_ != AddonState::Initialized) return false;

    @autoreleasepool {
        ZoomSDKAuthService* authService = [[ZoomSDK sharedSDK] getAuthService];
        if (!authService) {
            printf("[ZoomNative] getAuthService returned nil\n");
            fflush(stdout);
            state_ = AddonState::Error;
            return false;
        }

        g_authDelegate = [[ZoomAuthDelegateImpl alloc] init];
        authService.delegate = g_authDelegate;

        printf("[ZoomNative] JWT token length: %zu\n", config_.jwtToken.length());
        fflush(stdout);
        if (config_.jwtToken.length() > 20) {
            printf("[ZoomNative] JWT prefix: %.20s...\n", config_.jwtToken.c_str());
            fflush(stdout);
        }

        ZoomSDKAuthContext* ctx = [[ZoomSDKAuthContext alloc] init];
        NSString* jwtNS = [NSString stringWithUTF8String:config_.jwtToken.c_str()];
        ctx.jwtToken = jwtNS;

        printf("[ZoomNative] AuthContext.jwtToken length: %lu\n", (unsigned long)[ctx.jwtToken length]);
        printf("[ZoomNative] SDK initialized state: %d\n", (int)[[ZoomSDK sharedSDK] isSDKInitialized]);
        fflush(stdout);

        ZoomSDKError err = [authService sdkAuth:ctx];
        printf("[ZoomNative] SDKAuth result=%d (0=Success, 1=Failed, 6=WrongUsage)\n", (int)err);
        fflush(stdout);

        if (err != ZoomSDKError_Success) {
            state_ = AddonState::Error;
            return false;
        }
    }

    return true;
}

void ZoomAddon::Cleanup() {
    StopRawDataCapture();

    @autoreleasepool {
        [[ZoomSDK sharedSDK] unInitSDK];
    }

    g_authDelegate = nil;
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
