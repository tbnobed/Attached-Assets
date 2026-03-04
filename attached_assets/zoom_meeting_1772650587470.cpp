#include "zoom_addon.h"

#ifdef _WIN32
#include <windows.h>
#include "zoom_sdk.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"

using namespace ZOOMSDK;

static IMeetingService* g_meetingService = nullptr;

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
        ZoomAddon::Instance().OnMeetingStatusChanged(statusStr);

        if (status == MEETING_STATUS_INMEETING) {
            ZoomAddon::Instance().StartRawDataCapture();
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
        if (!lstUserID) return;
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
                        ZoomAddon::Instance().OnParticipantJoined(userId, name);
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
};

static MeetingServiceEventListener* g_meetingListener = nullptr;
static ParticipantsEventListener* g_participantsListener = nullptr;

bool ZoomAddon::JoinMeeting(const std::string& meetingId, const std::string& password, const std::string& displayName) {
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

    err = g_meetingService->Join(joinParam);
    return err == SDKERR_SUCCESS;
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

#else

bool ZoomAddon::JoinMeeting(const std::string& meetingId, const std::string& password, const std::string& displayName) {
    state_ = AddonState::InMeeting;
    return true;
}

bool ZoomAddon::LeaveMeeting() {
    state_ = AddonState::Authenticated;
    participants_.clear();
    return true;
}

#endif
