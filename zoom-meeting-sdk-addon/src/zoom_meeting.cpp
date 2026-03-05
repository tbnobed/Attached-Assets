#include "zoom_addon.h"

#ifdef _WIN32
#include <windows.h>
#include "zoom_sdk.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"

using namespace ZOOMSDK;

IMeetingService* g_meetingService = nullptr;

class MeetingServiceEventListener : public IMeetingServiceEvent {
public:
    void onMeetingStatusChanged(MeetingStatus status, int iResult) override {
        std::string statusStr;
        switch (status) {
            case MEETING_STATUS_IDLE: statusStr = "MEETING_STATUS_IDLE"; break;
            case MEETING_STATUS_CONNECTING: statusStr = "MEETING_STATUS_CONNECTING"; break;
            case MEETING_STATUS_WAITINGFORHOST: statusStr = "MEETING_STATUS_WAITINGFORHOST"; break;
            case MEETING_STATUS_INMEETING: statusStr = "MEETING_STATUS_INMEETING"; break;
            case MEETING_STATUS_DISCONNECTING: statusStr = "MEETING_STATUS_DISCONNECTING"; break;
            case MEETING_STATUS_RECONNECTING: statusStr = "MEETING_STATUS_RECONNECTING"; break;
            case MEETING_STATUS_ENDED: statusStr = "MEETING_STATUS_ENDED"; break;
            case MEETING_STATUS_FAILED: statusStr = "MEETING_STATUS_FAILED"; break;
            default: statusStr = "MEETING_STATUS_UNKNOWN"; break;
        }
        printf("[ZoomNative] Meeting status: %s (result=%d)\n", statusStr.c_str(), iResult);
        fflush(stdout);
        ZoomAddon::Instance().OnMeetingStatusChanged(statusStr);

        if (status == MEETING_STATUS_INMEETING) {
            if (g_meetingService) {
                auto* pCtrl = g_meetingService->GetMeetingParticipantsController();
                if (pCtrl) {
                    auto* selfInfo = pCtrl->GetMySelfUser();
                    if (selfInfo) {
                        uint32_t selfId = selfInfo->GetUserID();
                        ZoomAddon::Instance().SetSelfUserId(selfId);
                        printf("[ZoomNative] Self userId detected: %u\n", selfId);
                        fflush(stdout);
                    }
                }
            }
            bool rawOk = ZoomAddon::Instance().StartRawRecording();
            printf("[ZoomNative] StartRawRecording: %s\n", rawOk ? "OK" : "PENDING (will retry)");
            fflush(stdout);
        }
    }

    void onMeetingStatisticsWarningNotification(StatisticsWarningType type) override {}
    void onMeetingParameterNotification(const MeetingParameter* param) override {}
    void onSuspendParticipantsActivities() override {}
    void onAICompanionActiveChangeNotice(bool bActive) override {}
    void onMeetingTopicChanged(const zchar_t* sTopic) override {}
    void onMeetingFullToWatchLiveStream(const zchar_t* sLiveStreamUrl) override {}
    void onUserNetworkStatusChanged(MeetingComponentType component, ConnectionQuality quality, unsigned int userId, bool bNotGood) override {}
    void onAppSignalPanelUpdated(IMeetingAppSignalHandler* pHandler) override {}
};

class ParticipantsEventListener : public IMeetingParticipantsCtrlEvent {
public:
    void onUserJoin(IList<unsigned int>* lstUserID, const zchar_t* strUserList = nullptr) override {
        if (!lstUserID) {
            printf("[ZoomNative] onUserJoin: lstUserID is null\n");
            fflush(stdout);
            return;
        }
        printf("[ZoomNative] onUserJoin: %d users\n", lstUserID->GetCount());
        fflush(stdout);
        for (int i = 0; i < lstUserID->GetCount(); i++) {
            unsigned int userId = lstUserID->GetItem(i);
            if (g_meetingService) {
                auto* pCtrl = g_meetingService->GetMeetingParticipantsController();
                if (pCtrl) {
                    auto* info = pCtrl->GetUserByUserID(userId);
                    if (info) {
                        const zchar_t* wName = info->GetUserName();
                        std::string name;
                        if (wName) {
                            std::wstring ws(wName);
                            name.assign(ws.begin(), ws.end());
                        } else {
                            name = "Participant";
                        }
                        printf("[ZoomNative] onUserJoin: userId=%u name=%s\n", userId, name.c_str());
                        fflush(stdout);
                        ZoomAddon::Instance().OnParticipantJoined(userId, name);
                    } else {
                        printf("[ZoomNative] onUserJoin: GetUserByUserID(%u) returned null\n", userId);
                        fflush(stdout);
                    }
                }
            }
        }
    }

    void onUserLeft(IList<unsigned int>* lstUserID, const zchar_t* strUserList = nullptr) override {
        if (!lstUserID) return;
        for (int i = 0; i < lstUserID->GetCount(); i++) {
            ZoomAddon::Instance().OnParticipantLeft(lstUserID->GetItem(i));
        }
    }

    void onHostChangeNotification(unsigned int userId) override {}
    void onLowOrRaiseHandStatusChanged(bool bLow, unsigned int userId) override {}
    void onCoHostChangeNotification(unsigned int userId, bool isCoHost) override {}
    void onInvalidReclaimHostkey() override {}
    void onAllHandsLowered() override {}
    void onLocalRecordingStatusChanged(unsigned int userId, RecordingStatus status) override {}
    void onAllowParticipantsRenameNotification(bool bAllow) override {}
    void onAllowParticipantsUnmuteSelfNotification(bool bAllow) override {}
    void onAllowParticipantsStartVideoNotification(bool bAllow) override {}
    void onAllowParticipantsShareWhiteBoardNotification(bool bAllow) override {}
    void onRequestLocalRecordingPrivilegeChanged(LocalRecordingRequestPrivilegeStatus status) override {}
    void onAllowParticipantsRequestCloudRecording(bool bAllow) override {}
    void onInMeetingUserAvatarPathUpdated(unsigned int userID) override {}
    void onParticipantProfilePictureStatusChange(bool bHidden) override {}
    void onFocusModeStateChanged(bool bEnabled) override {}
    void onFocusModeShareTypeChanged(FocusModeShareType type) override {}
    void onUserNamesChanged(IList<unsigned int>* lstUserID) override {}
    void onBotAuthorizerRelationChanged(unsigned int userId) override {}
    void onVirtualNameTagStatusChanged(bool bEnabled, unsigned int userId) override {}
    void onVirtualNameTagRosterInfoUpdated(unsigned int userId) override {}
    void onCreateCompanionRelation(unsigned int userId, unsigned int companionId) override {}
    void onRemoveCompanionRelation(unsigned int userId) override {}
    void onGrantCoOwnerPrivilegeChanged(bool bGranted) override {}
};

static MeetingServiceEventListener* g_meetingListener = nullptr;
static ParticipantsEventListener* g_participantsListener = nullptr;

bool ZoomAddon::JoinMeeting(const std::string& meetingId, const std::string& password, const std::string& displayName, const std::string& appPrivilegeToken) {
    if (state_ != AddonState::Authenticated) return false;

    SDKError err = CreateMeetingService(&g_meetingService);
    if (err != SDKERR_SUCCESS || !g_meetingService) return false;

    g_meetingListener = new MeetingServiceEventListener();
    g_meetingService->SetEvent(g_meetingListener);

    auto* pCtrl = g_meetingService->GetMeetingParticipantsController();
    if (pCtrl) {
        g_participantsListener = new ParticipantsEventListener();
        pCtrl->SetEvent(g_participantsListener);
    }

    JoinParam joinParam;
    joinParam.userType = SDK_UT_WITHOUT_LOGIN;

    JoinParam4WithoutLogin& param = joinParam.param.withoutloginuserJoin;

    std::wstring wMeetingId(meetingId.begin(), meetingId.end());
    std::wstring wPassword(password.begin(), password.end());
    std::wstring wDisplayName(displayName.begin(), displayName.end());

    param.meetingNumber = _wtoi64(wMeetingId.c_str());
    param.vanityID = nullptr;
    param.userName = wDisplayName.c_str();
    param.psw = wPassword.c_str();
    param.isVideoOff = true;
    param.isAudioOff = true;
    param.isDirectShareDesktop = false;

    if (!appPrivilegeToken.empty()) {
        appPrivilegeToken_.assign(appPrivilegeToken.begin(), appPrivilegeToken.end());
        param.app_privilege_token = appPrivilegeToken_.c_str();
        printf("[ZoomNative] JoinMeeting: app_privilege_token set (len=%zu)\n", appPrivilegeToken.size());
        fflush(stdout);
    } else {
        appPrivilegeToken_.clear();
        param.app_privilege_token = nullptr;
        printf("[ZoomNative] JoinMeeting: no app_privilege_token\n");
        fflush(stdout);
    }

    err = g_meetingService->Join(joinParam);
    printf("[ZoomNative] JoinMeeting: Join result=%d\n", (int)err);
    fflush(stdout);
    return err == SDKERR_SUCCESS;
}

void ZoomAddon::EnumerateParticipants() {
    printf("[ZoomNative] EnumerateParticipants called\n");
    fflush(stdout);
    if (!g_meetingService) {
        printf("[ZoomNative] EnumerateParticipants: no meeting service\n");
        fflush(stdout);
        return;
    }
    auto* pCtrl = g_meetingService->GetMeetingParticipantsController();
    if (!pCtrl) {
        printf("[ZoomNative] EnumerateParticipants: no participant controller\n");
        fflush(stdout);
        return;
    }

    auto* lstUserID = pCtrl->GetParticipantsList();
    if (!lstUserID) {
        printf("[ZoomNative] EnumerateParticipants: GetParticipantsList returned null\n");
        fflush(stdout);
        return;
    }

    printf("[ZoomNative] EnumerateParticipants: %d participants in SDK list\n", lstUserID->GetCount());
    fflush(stdout);

    for (int i = 0; i < lstUserID->GetCount(); i++) {
        unsigned int userId = lstUserID->GetItem(i);

        if (userId == selfUserId_) {
            printf("[ZoomNative] EnumerateParticipants: userId=%u is SELF — skipping\n", userId);
            fflush(stdout);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (participants_.count(userId)) {
                printf("[ZoomNative] EnumerateParticipants: userId=%u already known\n", userId);
                fflush(stdout);
                continue;
            }
        }

        auto* info = pCtrl->GetUserByUserID(userId);
        std::string name = "Participant";
        if (info) {
            const zchar_t* wName = info->GetUserName();
            if (wName) {
                std::wstring ws(wName);
                name.assign(ws.begin(), ws.end());
            }
        }
        printf("[ZoomNative] EnumerateParticipants: new userId=%u name=%s\n", userId, name.c_str());
        fflush(stdout);
        OnParticipantJoined(userId, name);
    }
}

bool ZoomAddon::LeaveMeeting() {
    if (!g_meetingService) return false;

    StopRawDataCapture();

    SDKError err = g_meetingService->Leave(LEAVE_MEETING);

    if (g_meetingService) {
        DestroyMeetingService(g_meetingService);
        g_meetingService = nullptr;
    }
    if (g_meetingListener) {
        delete g_meetingListener;
        g_meetingListener = nullptr;
    }
    if (g_participantsListener) {
        delete g_participantsListener;
        g_participantsListener = nullptr;
    }

    state_ = AddonState::Authenticated;
    participants_.clear();
    return err == SDKERR_SUCCESS;
}

#elif defined(__APPLE__)

#import <ZoomSDK/ZoomSDK.h>

@interface ZoomMeetingDelegateImpl : NSObject <ZoomSDKMeetingServiceDelegate>
@end

@implementation ZoomMeetingDelegateImpl

- (void)onMeetingStatusChange:(ZoomSDKMeetingStatus)state meetingError:(ZoomSDKMeetingError)error EndReason:(EndMeetingReason)reason {
    std::string statusStr;
    switch (state) {
        case ZoomSDKMeetingStatus_Idle: statusStr = "MEETING_STATUS_IDLE"; break;
        case ZoomSDKMeetingStatus_Connecting: statusStr = "MEETING_STATUS_CONNECTING"; break;
        case ZoomSDKMeetingStatus_WaitingForHost: statusStr = "MEETING_STATUS_WAITINGFORHOST"; break;
        case ZoomSDKMeetingStatus_InMeeting: statusStr = "MEETING_STATUS_INMEETING"; break;
        case ZoomSDKMeetingStatus_Disconnecting: statusStr = "MEETING_STATUS_DISCONNECTING"; break;
        case ZoomSDKMeetingStatus_Reconnecting: statusStr = "MEETING_STATUS_RECONNECTING"; break;
        case ZoomSDKMeetingStatus_Failed: statusStr = "MEETING_STATUS_FAILED"; break;
        case ZoomSDKMeetingStatus_Ended: statusStr = "MEETING_STATUS_ENDED"; break;
        default: statusStr = "MEETING_STATUS_UNKNOWN"; break;
    }
    printf("[ZoomNative] Meeting status: %s (error=%d)\n", statusStr.c_str(), (int)error);
    fflush(stdout);
    ZoomAddon::Instance().OnMeetingStatusChanged(statusStr);

    if (state == ZoomSDKMeetingStatus_InMeeting) {
        @autoreleasepool {
            ZoomSDKMeetingService* meetingSvc = [[ZoomSDK sharedSDK] getMeetingService];
            if (meetingSvc) {
                ZoomSDKMeetingActionController* actionCtrl = [meetingSvc getMeetingActionController];
                if (actionCtrl) {
                    ZoomSDKUserInfo* selfInfo = [actionCtrl getMyself];
                    if (selfInfo) {
                        uint32_t selfId = [selfInfo getUserID];
                        ZoomAddon::Instance().SetSelfUserId(selfId);
                        printf("[ZoomNative] Self userId detected: %u\n", selfId);
                        fflush(stdout);
                    }
                }
            }
        }
        bool rawOk = ZoomAddon::Instance().StartRawRecording();
        printf("[ZoomNative] StartRawRecording: %s\n", rawOk ? "OK" : "PENDING (will retry)");
        fflush(stdout);
    }
}

@end

@interface ZoomActionDelegateImpl : NSObject <ZoomSDKMeetingActionControllerDelegate>
@end

@implementation ZoomActionDelegateImpl

- (void)onUserJoin:(NSArray*)array {
    if (!array) return;
    printf("[ZoomNative] onUserJoin: %lu users\n", (unsigned long)array.count);
    fflush(stdout);

    @autoreleasepool {
        ZoomSDKMeetingService* meetingSvc = [[ZoomSDK sharedSDK] getMeetingService];
        ZoomSDKMeetingActionController* actionCtrl = meetingSvc ? [meetingSvc getMeetingActionController] : nil;

        for (NSNumber* uid in array) {
            unsigned int userId = [uid unsignedIntValue];
            std::string name = "Participant";
            if (actionCtrl) {
                ZoomSDKUserInfo* info = [actionCtrl getUserByUserID:userId];
                if (info) {
                    NSString* nsName = [info getUserName];
                    if (nsName) {
                        name = std::string([nsName UTF8String]);
                    }
                }
            }
            printf("[ZoomNative] onUserJoin: userId=%u name=%s\n", userId, name.c_str());
            fflush(stdout);
            ZoomAddon::Instance().OnParticipantJoined(userId, name);
        }
    }
}

- (void)onUserLeft:(NSArray*)array {
    if (!array) return;
    for (NSNumber* uid in array) {
        ZoomAddon::Instance().OnParticipantLeft([uid unsignedIntValue]);
    }
}

- (void)onVideoStatusChange:(ZoomSDKVideoStatus)videoStatus UserID:(unsigned int)userID {
    printf("[ZoomNative] onVideoStatusChange: userId=%u status=%d\n", userID, (int)videoStatus);
    fflush(stdout);
}

- (void)onUserAudioStatusChange:(NSArray*)userAudioStatusArray {}
- (void)onHostChange:(unsigned int)userID {}
- (void)onMeetingCoHostChanged:(unsigned int)userID isCoHost:(BOOL)isCoHost {}
- (void)onSpotlightVideoUserChange:(NSArray*)spotlightedUserList {}
- (void)onLowOrRaiseHandStatusChange:(BOOL)raise UserID:(unsigned int)userID {}
- (void)onJoinMeetingResponse:(ZoomSDKJoinMeetingHelper*)joinMeetingHelper {}
- (void)onActiveVideoUserChanged:(unsigned int)userID {}
- (void)onActiveSpeakerVideoUserChanged:(unsigned int)userID {}
- (void)onHostAskUnmute {}
- (void)onHostAskStartVideo {}
- (void)onUserActiveAudioChange:(NSArray*)useridArray {}
- (void)onUserNamesChanged:(NSArray<NSNumber*>*)userList {}
- (void)onInvalidReclaimHostKey {}
- (void)onHostVideoOrderUpdated:(NSArray*)orderList {}
- (void)onLocalVideoOrderUpdated:(NSArray*)localOrderList {}
- (void)onFollowHostVideoOrderChanged:(BOOL)follow {}
- (void)onAllHandsLowered {}
- (void)onUserVideoQualityChanged:(ZoomSDKVideoQuality)quality userID:(unsigned int)userID {}
- (void)onChatMsgDeleteNotification:(NSString*)msgID messageDeleteType:(ZoomSDKChatMessageDeleteType)deleteBy {}
- (void)onChatStatusChangedNotification:(ZoomSDKChatStatus*)chatStatus {}
- (void)onSuspendParticipantsActivities {}
- (void)onAllowParticipantsStartVideoNotification:(BOOL)allow {}
- (void)onAllowParticipantsRenameNotification:(BOOL)allow {}
- (void)onAllowParticipantsUnmuteSelfNotification:(BOOL)allow {}
- (void)onMeetingLockStatus:(BOOL)isLock {}
- (void)onRequestLocalRecordingPrivilegeChanged:(ZoomSDKLocalRecordingRequestPrivilegeStatus)status {}
- (void)onAllowParticipantsRequestCloudRecording:(BOOL)allow {}
- (void)onInMeetingUserAvatarPathUpdated:(unsigned int)userID {}
- (void)onAICompanionActiveChangeNotice:(BOOL)active {}
- (void)onParticipantProfilePictureStatusChange:(BOOL)hidden {}
- (void)onFocusModeStateChanged:(BOOL)on {}
- (void)onFocusModeShareTypeChanged:(ZoomSDKFocusModeShareType)shareType {}
- (void)onMuteOnEntryStatusChange:(BOOL)enable {}
- (void)onMeetingTopicChanged:(NSString*)topic {}

@end

static ZoomMeetingDelegateImpl* g_meetingDelegate = nil;
static ZoomActionDelegateImpl* g_actionDelegate = nil;

bool ZoomAddon::JoinMeeting(const std::string& meetingId, const std::string& password, const std::string& displayName, const std::string& appPrivilegeToken) {
    if (state_ != AddonState::Authenticated) return false;

    @autoreleasepool {
        ZoomSDKMeetingService* meetingSvc = [[ZoomSDK sharedSDK] getMeetingService];
        if (!meetingSvc) {
            printf("[ZoomNative] JoinMeeting: getMeetingService returned nil\n");
            fflush(stdout);
            return false;
        }

        g_meetingDelegate = [[ZoomMeetingDelegateImpl alloc] init];
        meetingSvc.delegate = g_meetingDelegate;

        ZoomSDKMeetingActionController* actionCtrl = [meetingSvc getMeetingActionController];
        if (actionCtrl) {
            g_actionDelegate = [[ZoomActionDelegateImpl alloc] init];
            actionCtrl.delegate = g_actionDelegate;
        }

        ZoomSDKJoinMeetingElements* joinParams = [[ZoomSDKJoinMeetingElements alloc] init];
        joinParams.userType = ZoomSDKUserType_WithoutLogin;
        joinParams.meetingNumber = std::stoll(meetingId);
        joinParams.displayName = [NSString stringWithUTF8String:displayName.c_str()];
        joinParams.password = [NSString stringWithUTF8String:password.c_str()];
        joinParams.isNoVideo = YES;
        joinParams.isNoAudio = YES;
        joinParams.isMyVoiceInMix = NO;
        joinParams.isAudioRawDataStereo = NO;
        joinParams.audioRawdataSamplingRate = ZoomSDKAudioRawdataSamplingRate_48K;

        if (!appPrivilegeToken.empty()) {
            appPrivilegeToken_str_ = appPrivilegeToken;
            joinParams.appPrivilegeToken = [NSString stringWithUTF8String:appPrivilegeToken.c_str()];
            printf("[ZoomNative] JoinMeeting: app_privilege_token set (len=%zu)\n", appPrivilegeToken.size());
            fflush(stdout);
        } else {
            printf("[ZoomNative] JoinMeeting: no app_privilege_token\n");
            fflush(stdout);
        }

        ZoomSDKError err = [meetingSvc joinMeeting:joinParams];
        printf("[ZoomNative] JoinMeeting: Join result=%d\n", (int)err);
        fflush(stdout);
        return err == ZoomSDKError_Success;
    }
}

void ZoomAddon::EnumerateParticipants() {
    printf("[ZoomNative] EnumerateParticipants called\n");
    fflush(stdout);

    @autoreleasepool {
        ZoomSDKMeetingService* meetingSvc = [[ZoomSDK sharedSDK] getMeetingService];
        if (!meetingSvc) {
            printf("[ZoomNative] EnumerateParticipants: no meeting service\n");
            fflush(stdout);
            return;
        }

        ZoomSDKMeetingActionController* actionCtrl = [meetingSvc getMeetingActionController];
        if (!actionCtrl) {
            printf("[ZoomNative] EnumerateParticipants: no action controller\n");
            fflush(stdout);
            return;
        }

        NSArray* participantList = [actionCtrl getParticipantsList];
        if (!participantList) {
            printf("[ZoomNative] EnumerateParticipants: getParticipantsList returned nil\n");
            fflush(stdout);
            return;
        }

        printf("[ZoomNative] EnumerateParticipants: %lu participants in SDK list\n", (unsigned long)participantList.count);
        fflush(stdout);

        for (NSNumber* uid in participantList) {
            unsigned int userId = [uid unsignedIntValue];

            if (userId == selfUserId_) {
                printf("[ZoomNative] EnumerateParticipants: userId=%u is SELF — skipping\n", userId);
                fflush(stdout);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (participants_.count(userId)) {
                    printf("[ZoomNative] EnumerateParticipants: userId=%u already known\n", userId);
                    fflush(stdout);
                    continue;
                }
            }

            ZoomSDKUserInfo* info = [actionCtrl getUserByUserID:userId];
            std::string name = "Participant";
            if (info) {
                NSString* nsName = [info getUserName];
                if (nsName) {
                    name = std::string([nsName UTF8String]);
                }
            }
            printf("[ZoomNative] EnumerateParticipants: new userId=%u name=%s\n", userId, name.c_str());
            fflush(stdout);
            OnParticipantJoined(userId, name);
        }
    }
}

bool ZoomAddon::LeaveMeeting() {
    StopRawDataCapture();

    @autoreleasepool {
        ZoomSDKMeetingService* meetingSvc = [[ZoomSDK sharedSDK] getMeetingService];
        if (meetingSvc) {
            [meetingSvc leaveMeetingWithCmd:LeaveMeetingCmd_Leave];
        }
    }

    g_meetingDelegate = nil;
    g_actionDelegate = nil;

    state_ = AddonState::Authenticated;
    participants_.clear();
    return true;
}

#else

bool ZoomAddon::JoinMeeting(const std::string& meetingId, const std::string& password, const std::string& displayName, const std::string& appPrivilegeToken) {
    state_ = AddonState::InMeeting;
    return true;
}

void ZoomAddon::EnumerateParticipants() {}

bool ZoomAddon::LeaveMeeting() {
    state_ = AddonState::Authenticated;
    participants_.clear();
    return true;
}

#endif
