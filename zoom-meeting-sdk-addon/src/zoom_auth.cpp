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
        ZoomSDKInitParams* params = [[ZoomSDKInitParams alloc] init];
        params.needCustomizedUI = YES;
        params.zoomDomain = @"zoom.us";
        params.enableLog = YES;

        printf("[ZoomNative] Initializing macOS SDK (customUI=YES, logging=YES)\n");
        fflush(stdout);

        ZoomSDKError err = [[ZoomSDK sharedSDK] initSDKWithParams:params];
        printf("[ZoomNative] InitSDK result=%d\n", (int)err);
        fflush(stdout);

        if (err != ZoomSDKError_Success) {
            state_ = AddonState::Error;
            return false;
        }

        ZoomSDK* sdk = [ZoomSDK sharedSDK];
        sdk.videoRawDataMode = ZoomSDKRawDataMemoryMode_Heap;
        sdk.audioRawDataMode = ZoomSDKRawDataMemoryMode_Heap;
        sdk.shareRawDataMode = ZoomSDKRawDataMemoryMode_Heap;
        sdk.enableRawdataIntermediateMode = NO;
        printf("[ZoomNative] Raw data modes set (heap, no intermediate)\n");
        fflush(stdout);

        NSString* sdkVer = [sdk getSDKVersionNumber];
        printf("[ZoomNative] SDK version: %s\n", sdkVer ? [sdkVer UTF8String] : "(null)");
        fflush(stdout);
    }

    state_ = AddonState::Initialized;
    return true;
}

bool ZoomAddon::Authenticate() {
    if (state_ != AddonState::Initialized) return false;

    @autoreleasepool {
        g_authDelegate = [[ZoomAuthDelegateImpl alloc] init];

        ZoomSDKAuthService* authService = [[ZoomSDK sharedSDK] getAuthService];
        if (!authService) {
            printf("[ZoomNative] getAuthService returned nil\n");
            fflush(stdout);
            state_ = AddonState::Error;
            return false;
        }
        authService.delegate = g_authDelegate;

        NSString* jwtNS = [NSString stringWithUTF8String:config_.jwtToken.c_str()];
        printf("[ZoomNative] JWT token length: %zu\n", config_.jwtToken.length());
        fflush(stdout);

        if (!jwtNS || jwtNS.length == 0) {
            printf("[ZoomNative] ERROR: JWT token is empty!\n");
            fflush(stdout);
            state_ = AddonState::Error;
            return false;
        }

        NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
        printf("[ZoomNative] Bundle ID: %s\n", bundleId ? [bundleId UTF8String] : "(null)");
        fflush(stdout);

        printf("[ZoomNative] JWT first 40 chars: %.40s\n", config_.jwtToken.c_str());
        fflush(stdout);

        BOOL isAlreadyAuthed = [authService isAuthorized];
        printf("[ZoomNative] isAuthorized before auth: %s\n", isAlreadyAuthed ? "YES" : "NO");
        fflush(stdout);

        printf("[ZoomNative] Attempting auth with JWT (immediate)...\n");
        fflush(stdout);

        ZoomSDKAuthContext* ctx = [[ZoomSDKAuthContext alloc] init];
        ctx.jwtToken = jwtNS;
        ZoomSDKError err = [authService sdkAuth:ctx];
        printf("[ZoomNative] Immediate SDKAuth result=%d (0=Success, 1=Failed)\n", (int)err);
        fflush(stdout);

        if (err == ZoomSDKError_Success) {
            printf("[ZoomNative] Immediate auth call accepted, waiting for delegate callback\n");
            fflush(stdout);
        } else {
            printf("[ZoomNative] Immediate auth failed. Trying with publicAppKey only...\n");
            fflush(stdout);

            NSString* sdkKeyNS = [NSString stringWithUTF8String:config_.sdkKey.c_str()];

            ZoomSDKAuthContext* ctx2 = [[ZoomSDKAuthContext alloc] init];
            ctx2.publicAppKey = sdkKeyNS;
            ZoomSDKError err2 = [authService sdkAuth:ctx2];
            printf("[ZoomNative] SDKAuth (publicAppKey only) result=%d\n", (int)err2);
            fflush(stdout);

            if (err2 != ZoomSDKError_Success) {
                printf("[ZoomNative] Trying with BOTH jwtToken + publicAppKey...\n");
                fflush(stdout);

                ZoomSDKAuthContext* ctx3 = [[ZoomSDKAuthContext alloc] init];
                ctx3.jwtToken = jwtNS;
                ctx3.publicAppKey = sdkKeyNS;
                ZoomSDKError err3 = [authService sdkAuth:ctx3];
                printf("[ZoomNative] SDKAuth (jwt+publicAppKey) result=%d\n", (int)err3);
                fflush(stdout);
            }

            printf("[ZoomNative] Scheduling 2s deferred retry...\n");
            fflush(stdout);

            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                @autoreleasepool {
                    ZoomSDKAuthService* svc = [[ZoomSDK sharedSDK] getAuthService];
                    if (!svc) return;
                    svc.delegate = g_authDelegate;

                    BOOL authed = [svc isAuthorized];
                    printf("[ZoomNative] isAuthorized after 2s: %s\n", authed ? "YES" : "NO");
                    fflush(stdout);

                    if (!authed) {
                        ZoomSDKAuthContext* ctx4 = [[ZoomSDKAuthContext alloc] init];
                        ctx4.jwtToken = jwtNS;
                        ZoomSDKError err4 = [svc sdkAuth:ctx4];
                        printf("[ZoomNative] Deferred SDKAuth result=%d\n", (int)err4);
                        fflush(stdout);
                    }
                }
            });
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
