/**
 * @file ZoomSDKErrors.h
 * @brief Definitions of error codes, enumerations, and constants used throughout the Zoom SDK.
 * 
 * This header file defines all error codes, status enumerations, user types, meeting states,
 * and other constants that are used across the Zoom SDK for macOS.
 */

#pragma once

#import <Foundation/Foundation.h>
/**
 * @brief Enumeration of user types.
 */
typedef enum {
    /** User logs in with working email. */
    ZoomSDKUserType_ZoomUser    = 100,
    /** Single-sign-on user. */
    ZoomSDKUserType_SSOUser     = 101,
    /** Users who are not logged in. */
    ZoomSDKUserType_WithoutLogin = 102
}ZoomSDKUserType;

/**
 * @brief Enumeration of the types of users.
 */
typedef enum {
	/** API user. */
    SDKUserType_APIUser,
	/** User logs in with email. */
    SDKUserType_EmailLogin,
	/** User logs in with Facebook account. */
    SDKUserType_FaceBook,
	/** User logs in with Google authentication. */
    SDKUserType_GoogleOAuth,
	/** User logs in with SSO token. */
    SDKUserType_SSO,
	/** Unknown user type. */
    SDKUserType_Unknown
}SDKUserType;

/**
 * @brief Enumeration of commands for leaving meeting.
 */
typedef enum {
    /** Command of leaving meeting. */
    LeaveMeetingCmd_Leave,
    /** Command of ending Meeting. */
    LeaveMeetingCmd_End
}LeaveMeetingCmd;

/**
 * @brief Enumeration of all the commands in the meeting.
 */
typedef enum{
    /** Mute the video. */
    ActionMeetingCmd_MuteVideo,
	/** Unmute the video. */
    ActionMeetingCmd_UnMuteVideo,
    /** Mute the audio. */
    ActionMeetingCmd_MuteAudio,
	/** Unmute the audio. */
    ActionMeetingCmd_UnMuteAudio,
    /** Enable the feature that user can unmute himself when muted. */
    ActionMeetingCmd_EnableUnmuteBySelf,
	/** Disable the feature that user can not unmute himself when muted. */
    ActionMeetingCmd_DisableUnmuteBySelf,
    /** Mute all participants in the meeting, available only for the host/co-host. */ 
    ActionMeetingCmd_MuteAll,
    /** Unmute all participants in the meeting, available only for the host/co-host. */ 
    ActionMeetingCmd_UnmuteAll,
    /** Lock the meeting, available only for the host/co-host. Once locked, the new participants can no longer join the meeting/co-host. */
    ActionMeetingCmd_LockMeeting,
	/** Unlock the meeting, available only for the host/co-host. */ 
    ActionMeetingCmd_UnLockMeeting,
    /** Adjust the display size fit to the window. */
    ActionMeetingCmd_ShareFitWindowMode,
    /** Pause sharing. */
    ActionMeetingCmd_PauseShare,
    /** Resume sharing. */
    ActionMeetingCmd_ResumeShare,
    /** Join meeting by VoIP. */
    ActionMeetingCmd_JoinVoip,
    /** Disconnect VoIP from meeting. */
    ActionMeetingCmd_LeaveVoip
}ActionMeetingCmd;

/**
 * @brief Get default information of meeting.
 */
typedef enum {
	/** The topic of meeting. */
    MeetingPropertyCmd_Topic,
	/** The template of email invitation. */
    MeetingPropertyCmd_InviteEmailTemplate,
	/** The title of email invitation. */
    MeetingPropertyCmd_InviteEmailTitle,
	/** The invitation URL. */
    MeetingPropertyCmd_JoinMeetingUrl,
	/** The default path to save the recording files. */
    MeetingPropertyCmd_DefaultRecordPath,
	/** The meeting number. */
    MeetingPropertyCmd_MeetingNumber,
	/** The tag of host. */
    MeetingPropertyCmd_HostTag, 
	/** Meeting ID. */
    MeetingPropertyCmd_MeetingID,
    /** Meeting password. */
    MeetingPropertyCmd_MeetingPassword
}MeetingPropertyCmd;

/**
 * @brief Type of annotation tools. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0067931>.
 */
typedef enum{
	/** Switch to mouse cursor. For initialization. */
    AnnotationToolType_None,
	/** Pen. */
    AnnotationToolType_Pen,
	/** Highlighter. */
    AnnotationToolType_HighLighter,
	/** A straight line changes automatically in pace with the mouse cursor. */
    AnnotationToolType_AutoLine,
	/** A rectangle changes automatically in pace with the mouse cursor. */
    AnnotationToolType_AutoRectangle,
	/** An ellipse changes automatically in pace with the mouse cursor. */
    AnnotationToolType_AutoEllipse,
	/** An arrow changes automatically in pace with the mouse cursor. */
    AnnotationToolType_AutoArrow,
	/** A filled rectangle. */
    AnnotationToolType_AutoRectangleFill,
	/** A filled ellipse. */
    AnnotationToolType_AutoEllipseFill,
	/** Only available if you started the shared screen or whiteboard. Displays your mouse pointer to all participants when your mouse is within the area being shared. Use this to point out parts of the screen to other participants. */
    AnnotationToolType_SpotLight,
	/** Displays a small arrow instead of your mouse pointer. Each subsequent click will remove the previous arrow placed. */
    AnnotationToolType_Arrow,
	/** Erase parts of your annotation. */
    AnnotationToolType_ERASER,
    /** Insert a textbox to input letters. */
    AnnotationToolType_Textbox,
    /** Only available if you started the shared screen or whiteboard. Select , move, or resize your annotations. */
    AnnotationToolType_Picker,
    /** A fair rectangle changes automatically in pace with the mouse cursor. */
    AnnotationToolType_AutoRectangleSemiFill,
    /** A fair ellipse changes automatically in pace with the mouse cursor. */
    AnnotationToolType_AutoEllipseSemiFill,
    /** A line with a double-arrow. */
    AnnotationToolType_AutoDoubleArrow,
    /** An unfilled rhombus. */
    AnnotationToolType_AutoDiamond,
    /** A fixed-size arrow for marking. */
    AnnotationToolType_AutoStampArrow,
    /** A sign marking that something is correct. */
    AnnotationToolType_AutoStampCheck,
    /** A sign marking that something is incorrect. */
    AnnotationToolType_AutoStampX,
    /** A star for marking. */
    AnnotationToolType_AutoStampStar,
    /** A heart for marking. */
    AnnotationToolType_AutoStampHeart,
    /** A sign for interrogation. */
    AnnotationToolType_AutoStampQm
}AnnotationToolType;

/**
 * @brief Types of clearing annotations.
 */
typedef enum{
	/** Clear all annotations. Hosts, cohost and shared meeting owners can use. */
    AnnotationClearType_All,
    /** Clear only your own annotations. Everyone can use. */
    AnnotationClearType_Self,
    /** Clear only the others' annotations. Only shared meeting owners can use. */
    AnnotationClearType_Other
}AnnotationClearType;

/**
 * @brief In-meeting UI components.
 */
typedef enum{
	/** Meeting window. */
    MeetingComponent_MainWindow,
	/** Audio. */
    MeetingComponent_Audio,
	/** Chat. */
    MeetingComponent_Chat,
	/** Participants. */
    MeetingComponent_Participants,
	/** Main toolbar at the bottom of meeting window. */
    MeetingComponent_MainToolBar,
	/** Main toolbar for sharing on the primary view. */
    MeetingComponent_MainShareToolBar,
	/** @deprecated This enum value is deprecated and will be removed in a future release. */
    MeetingComponent_AuxShareToolBar,
	/** Setting components. */
    MeetingComponent_Setting,
	/** Window for sharing options. */ 
    MeetingComponent_ShareOptionWindow,
	/** Thumbnail video layout. */
    MeetingComponent_ThumbnailVideo,
	/** Window for invite other into meeting. */
    MeetingComponent_InviteWindow,
	/** Window for sharing select. */
    MeetingComponent_ShareSelectWindow
}MeetingComponent;

/**
 * @brief Enumeration of Meeting settings.
 */
typedef enum{
	/** Dual screen mode. */
    MeetingSettingCmd_DualScreenMode,
	/** Enter full screen mode when user joins the meeting. */
    MeetingSettingCmd_AutoFullScreenWhenJoinMeeting,
	/** Enable to play chime when user joins or exits the meeting. */
    MeetingSettingCmd_EnablePlayChimeWhenEnterOrExit
}MeetingSettingCmd;

/**
 * @brief Enumeration of common errors of SDK.
 */
typedef enum{
	/** Success. */
    ZoomSDKError_Success,
	/** Failed. */
    ZoomSDKError_Failed,
	/** SDK is not initialize. */
    ZoomSDKError_Uninit,
	/** Service is failed. */
    ZoomSDKError_ServiceFailed,
	/** Incorrect usage of the feature. */ 
    ZoomSDKError_WrongUsage,
	/** Wrong parameter. */
    ZoomSDKError_InvalidParameter,
	/** No permission. */
    ZoomSDKError_NoPermission,
	/** There is no recording in process. */
    ZoomSDKError_NoRecordingInProgress,
    /** Api calls are too frequent. */
    ZoomSDKError_TooFrequentCall,
    /** Unsupported feature. */
    ZoomSDKError_UnSupportedFeature,
    /** Unsupport email login. */
    ZoomSDKError_EmailLoginIsDisabled,
    /** Module load fail. */
    ZoomSDKError_ModuleLoadFail,
    /** No video data. */
    ZoomSDKError_NoVideoData,
    /** No audio data. */
    ZoomSDKError_NoAudioData,
    /** No share data. */
    ZoomSDKError_NoShareData,
    /** Not found video device. */
    ZoomSDKError_NoVideoDeviceFound,
    /** Device error. */
    ZoomSDKError_DeviceError,
    /** Not in meeting. */
    ZoomSDKError_NotInMeeting,
    /** Init device. */
    ZoomSDKError_initDevice,
    /** Can't change virtual device. */
    ZoomSDKError_CanNotChangeVirtualDevice,
    /** Preprocess rawdata error. */
    ZoomSDKError_PreprocessRawdataError,
    /** No license. */
    ZoomSDKError_NoLicense,
    /** Malloc failed. */
    ZoomSDKError_Malloc_Failed,
    /** ShareCannotSubscribeMyself. */
    ZoomSDKError_ShareCannotSubscribeMyself,
    /** Need user confirm record disclaimer. */
    ZoomSDKError_NeedUserConfirmRecordDisclaimer,
    /** Unknown error. */
    ZoomSDKError_UnKnown,
    /** Not join audio. */
    ZoomSDKError_NotJoinAudio,
    /** The current device doesn't support the feature. */
    ZoomSDKError_HardwareDontSupport,
    /** Domain not support. */
    ZoomSDKError_DomainDontSupport,
    /** File transfer fail. */
    ZoomSDKError_FileTransferError
}ZoomSDKError;

/**
 * @brief Enumeration of SDK authentication results.
 */
typedef enum {
    /** Authentication is successful. */
    ZoomSDKAuthError_Success = 0,
    /** Key or secret is wrong. */
    ZoomSDKAuthError_KeyOrSecretWrong,
    /** Client account does not support. */
    ZoomSDKAuthError_AccountNotSupport,
    /** Client account does not enable SDK. */
    ZoomSDKAuthError_AccountNotEnableSDK,
    /** Auth timeout. */
    ZoomSDKAuthError_Timeout,
    /** Network issue. */
    ZoomSDKAuthError_NetworkIssue,
    /** Client incompatible. */
    ZoomSDKAuthError_Client_Incompatible,
    /** The jwt token to authenticate is wrong. */
    ZoomSDKAuthError_JwtTokenWrong,
    /** The key or secret to authenticate is empty. */
    ZoomSDKAuthError_KeyOrSecretEmpty,
    /** The authentication rate limit is exceeded. */
    ZoomSDKAuthError_LimitExceededException,
    /** Unknown error. */
    ZoomSDKAuthError_Unknown = 100
}ZoomSDKAuthError;

/**
 * @brief Enumeration of SDK pre-meeting errors.
 */
typedef enum {
    /** Calls SDK successfully. */
    ZoomSDKPremeetingError_Success,
    /** Calls SDK failed. */
    ZoomSDKPremeetingError_Failed,
    /** Timeout. */
    ZoomSDKPremeetingError_TimeOut,
    /** Unknown errors. */
    ZoomSDKPremeetingError_Unknown = 100
}ZoomSDKPremeetingError;

/**
 * @brief Enumeration of errors to start/join meeting.
 */
typedef enum {
    /** Start/Join meeting successfully. */
    ZoomSDKMeetingError_Success                         = 0,
    /** The connection with the backend service has errors. */
    ZoomSDKMeetingError_ConnectionError                 = 1,
    /** Failed to reconnect the meeting. */
    ZoomSDKMeetingError_ReconnectFailed                 = 2,
    /** MMR issue, please check MMR configuration. */
    ZoomSDKMeetingError_MMRError                        = 3,
    /** The meeting password is incorrect. */
    ZoomSDKMeetingError_PasswordError                   = 4,
    /** Failed to create video and audio data connection with MMR. */
    ZoomSDKMeetingError_SessionError                    = 5,
    /** Meeting is over. */
    ZoomSDKMeetingError_MeetingOver                     = 6,
    /** Meeting is not started. */
    ZoomSDKMeetingError_MeetingNotStart                 = 7,
    /** The meeting does not exist. */
    ZoomSDKMeetingError_MeetingNotExist                 = 8,
    /**
     * The number of participants exceeds the upper limit.
     * For users that can't join the meeting, they can go to watch the live stream with the interface \link ZoomSDKMeetingServiceDelegate::onMeetingFullToWatchLiveStream: \endlink if the host has started.
     */
    ZoomSDKMeetingError_UserFull                        = 9,
    /** The ZOOM SDK version is incompatible. */
    ZoomSDKMeetingError_ClientIncompatible              = 10,
    /** No MMR is valid. */
    ZoomSDKMeetingError_NoMMR                           = 11,
    /** The meeting is locked by the host. */
    ZoomSDKMeetingError_MeetingLocked                   = 12,
    /** The meeting is restricted. */
    ZoomSDKMeetingError_MeetingRestricted               = 13,
    /** The meeting is restricted to join before host. */
    ZoomSDKMeetingError_MeetingJBHRestricted            = 14,
    /** Failed to request the web server. */
    ZoomSDKMeetingError_EmitWebRequestFailed            = 15,
    /** Failed to start meeting with expired token. */
    ZoomSDKMeetingError_StartTokenExpired               = 16,
    /** The user's video does not work. */
    ZoomSDKMeetingError_VideoSessionError               = 17,
	/** The user's audio cannot auto-start. */
    ZoomSDKMeetingError_AudioAutoStartError             = 18,
	/** The amount of webinar attendees reaches the upper limit. */
    ZoomSDKMeetingError_RegisterWebinarFull             = 19,
    /** User needs to register a webinar account if he wants to start a webinar. */
    ZoomSDKMeetingError_RegisterWebinarHostRegister     = 20,
	/** User needs to register an account if he wants to join the webinar by the link. */
    ZoomSDKMeetingError_RegisterWebinarPanelistRegister = 21,
	/** The host has denied your webinar registration. */
    ZoomSDKMeetingError_RegisterWebinarDeniedEmail      = 22,
	/** Sign in with the specified account to join webinar. */
    ZoomSDKMeetingError_RegisterWebinarEnforceLogin     = 23,
    /** The certificate of ZC has been changed. */
    ZoomSDKMeetingError_ZCCertificateChanged            = 24,
    /** Vanity conference ID does not exist. */
    ZoomSDKMeetingError_vanityNotExist                  = 27,
    /** Join webinar with the same email. */
    ZoomSDKMeetingError_joinWebinarWithSameEmail        = 28,
    /** Meeting settings is not allowed to start a meeting. */
    ZoomSDKMeetingError_disallowHostMeeting             = 29,
    /** Failed to write configure file. */
    ZoomSDKMeetingError_ConfigFileWriteFailed           = 50,
    /** Forbidden to join the internal meeting. */
    ZoomSDKMeetingError_forbidToJoinInternalMeeting     = 60,
	/** User is removed from meeting by host. */
    ZoomSDKMeetingError_RemovedByHost                   = 61,
    /** Host disallow outside user join. */
    ZoomSDKMeetingError_HostDisallowOutsideUserJoin     = 62,
    /** To join a meeting hosted by an external Zoom account, your SDK app has to be published on Zoom Marketplace. You can refer to Section 6.1 of Zoom's API License Terms of Use. */
    ZoomSDKMeetingError_UnableToJoinExternalMeeting     = 63,
    /** Join failed because this Meeting SDK key is blocked by the host’s account admin. */
    ZoomSDKMeetingError_BlockedByAccountAdmin           = 64,
    /** Need sign in using the same account as the meeting organizer. */
    ZoomSDKMeetingError_NeedSigninForPrivateMeeting     = 82,
    /** Join meeting param vanityID is duplicated and needs to be confirmed. For more information about Vanity URLs, see https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0061540#multipleVanity. */
    ZoomSDKMeetingError_NeedConfirmPlink                = 88,
    /** Join meeting param vanityID does not exist in the current account. For more information about Vanity URLs, see https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0061540#multipleVanity. */
    ZoomSDKMeetingError_NeedInputPlink                  = 89,
    /** Unknown error. */
    ZoomSDKMeetingError_Unknown                         = 100,
	/** No error. */
    ZoomSDKMeetingError_None                            = 101,
    /** App privilege token error. */
    ZoomSDKMeetingError_AppPrivilegeTokenError          = 500,
    /** Authorized user not in meeting. */
    ZoomSDKMeetingError_AuthorizedUserNotInMeeting      = 501,
    /** On-behalf token error: conflict with login credentials. */
    ZoomSDKMeetingError_OnBehalfTokenConflictLoginError = 502,
    /** User level privilege token not have host zak/obf. */
    ZoomSDKMeetingError_UserLevelTokenNotHaveHostZakObf = 503,
    /** App can not anonymous join meeting. */
    ZoomSDKMeetingError_AppCanNotAnonymousJoinMeeting  = 504,
    /** On-behalf token invalid. */
    ZoomSDKMeetingError_OnBehalfTokenInvalid                = 505,
    /** On-behalf token meeting number not match. */
    ZoomSDKMeetingError_OnBehalfTokenNotMatchMeeting         = 506,
    /** Jmak user email not match. */
    ZoomSDKMeetingError_JmakUserEmailNotMatch           = 1143
}ZoomSDKMeetingError;


/**
 * @brief Enumeration of ZOOM SDK login status.
 */
typedef enum {
	/** User does not login. */
    ZoomSDKLoginStatus_Idle = 0,
	/** Login successfully. */
    ZoomSDKLoginStatus_Success = 1,
	/** Login failed. */
    ZoomSDKLoginStatus_Failed = 2,
	/** Login in progress. */
    ZoomSDKLoginStatus_Processing = 3
}ZoomSDKLoginStatus;


/**
 * @brief Enumeration of meeting status.
 */
typedef enum {
    /** No meeting is running. */
    ZoomSDKMeetingStatus_Idle             = 0,
    /** Connecting to the meeting server. */
    ZoomSDKMeetingStatus_Connecting       = 1,
    /** Waiting for the host to start the meeting. */
    ZoomSDKMeetingStatus_WaitingForHost   = 2,
    /** Meeting is ready, in meeting status. */
    ZoomSDKMeetingStatus_InMeeting        = 3,
    /** Disconnect the meeting server, leave meeting status. */
    ZoomSDKMeetingStatus_Disconnecting    = 4,
    /** Reconnecting meeting server status. */
    ZoomSDKMeetingStatus_Reconnecting     = 5,
    /** Join/Start meeting failed. */
    ZoomSDKMeetingStatus_Failed           = 6,
    /** Meeting ends. */
    ZoomSDKMeetingStatus_Ended            = 7,
    /** Audio is connected. */
    ZoomSDKMeetingStatus_AudioReady       = 8,
    /** There is another ongoing meeting on the server. */ 
    ZoomSDKMeetingStatus_OtherMeetingInProgress = 9,
	/** Participants who join the meeting before the start are in the waiting room. */
    ZoomSDKMeetingStatus_InWaitingRoom      = 10,
    /** Promote the attendees to panelist in webinar. */
    ZoomSDKMeetingStatus_Webinar_Promote = 12,
    /** Demote the attendees from the panelist. */
    ZoomSDKMeetingStatus_Webinar_Depromote = 13,
    /** Join breakout room. */
    ZoomSDKMeetingStatus_Join_Breakout_Room = 14,
    /** Leave breakout room. */
    ZoomSDKMeetingStatus_Leave_Breakout_Room = 15
}ZoomSDKMeetingStatus;

/**
 * @brief Enumeration of sharing status.
 */
typedef enum{
	/** For initialization. */
    ZoomSDKShareStatus_None,
	/** The current user begins the share. */
    ZoomSDKShareStatus_SelfBegin,
	/** The current user ends the share. */
    ZoomSDKShareStatus_SelfEnd,
	/** Other user begins the share. */
    ZoomSDKShareStatus_OtherBegin,
	/** Other user ends the share. */
    ZoomSDKShareStatus_OtherEnd,
	/** The current user is viewing the share by others. */
    ZoomSDKShareStatus_ViewOther,
	/** The share is paused. */
    ZoomSDKShareStatus_Pause,
	/** The share is resumed. */
    ZoomSDKShareStatus_Resume,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKShareStatus_ContentTypeChange,
	/** The current user begins to share the sounds of computer audio. */
    ZoomSDKShareStatus_SelfStartAudioShare,
	/** The current user stops sharing the sounds of computer audio. */
    ZoomSDKShareStatus_SelfStopAudioShare,
	/** Other user begins to share the sounds of computer audio. */
    ZoomSDKShareStatus_OtherStartAudioShare,
	/** Other user stops sharing the sounds of computer audio. */
    ZoomSDKShareStatus_OtherStopAudioShare,
    /** The share is disconnected. */
    ZoomSDKShareStatus_Disconnected
}ZoomSDKShareStatus;

/**
 * @brief Enumeration of Audio status.
 */
typedef enum{
	/** For initialization. */
    ZoomSDKAudioStatus_None = 0,
	/** The audio is muted. */
    ZoomSDKAudioStatus_Muted = 1,
	/** The audio is unmuted. */
    ZoomSDKAudioStatus_UnMuted = 2,
	/** The audio is muted by the host. */
    ZoomSDKAudioStatus_MutedByHost = 3,
	/** The audio is unmuted by the host. */
    ZoomSDKAudioStatus_UnMutedByHost = 4,
	/** Host mutes all participants. */
    ZoomSDKAudioStatus_MutedAllByHost = 5,
	/** Host unmutes all participants. */
    ZoomSDKAudioStatus_UnMutedAllByHost = 6
}ZoomSDKAudioStatus;

/**
 * @brief Enumeration of the status of a user's video.
 */
typedef enum{
    /** Video is turned off. */
    ZoomSDKVideoStatus_Off,
    /** Video is turned on. */
    ZoomSDKVideoStatus_On,
    /** Video is muted by the host. */
    ZoomSDKVideoStatus_MutedByHost,
    /** Video status is unknown. */
    ZoomSDKVideoStatus_None
}ZoomSDKVideoStatus;

/**
 * @brief Enumeration of the type of a user's audio connection.
 */
typedef enum{
	/** No audio connection. */
    ZoomSDKAudioType_None = 0,
	/** Connected via VoIP (Voice over Internet Protocol). */
    ZoomSDKAudioType_Voip = 1,
	/** Connected via phone call. */
    ZoomSDKAudioType_Phone = 2,
	/** Unknown audio connection type. */
    ZoomSDKAudioType_Unknown = 3
}ZoomSDKAudioType;

/**
 * @brief Enumeration of status of remote control.
 */
typedef enum{
	/** For initialization. */
    ZoomSDKRemoteControlStatus_None,
    /** Viewer can request to control the sharer remotely. */
    ZoomSDKRemoteControlStatus_CanRequestFromWho,
    /** Sharer receives the request from viewer. */
    ZoomSDKRemoteControlStatus_RequestFromWho,
    /** Sharer declines your request to be remote controlled. */
    ZoomSDKRemoteControlStatus_DeclineByWho,
    /** Sharer is remote controlled by viewer. */
    ZoomSDKRemoteControlStatus_RemoteControlledByWho,
    /** Notify user that controller of the shared content changes. */
    ZoomSDKRemoteControlStatus_StartRemoteControlWho,
	/** Remote control ends. */
    ZoomSDKRemoteControlStatus_EndRemoteControlWho,
    /** Viewer gets the privilege of remote control. */
    ZoomSDKRemoteControlStatus_HasPrivilegeFromWho,
    /** Viewer loses the privilege of remote control. */
    ZoomSDKRemoteControlStatus_LostPrivilegeFromWho,
    /** The status of remote control. I can be controlled by whom. */
    ZoomSDKRemoteControlStatus_WhoCanControlMe
}ZoomSDKRemoteControlStatus;

/**
 * @brief Enumeration of Recording status.
 */
typedef enum{
	/** For initialization. */
    ZoomSDKRecordingStatus_None,
	/** Start recording. */
    ZoomSDKRecordingStatus_Start,
	/** Stop recording. */
    ZoomSDKRecordingStatus_Stop,
	/** There is no more space to store recording. */
    ZoomSDKRecordingStatus_DiskFull,
    /** Pause recording. */
    ZoomSDKRecordingStatus_Pause,
    /** Connecting, only for cloud recording. */
    ZoomSDKRecordingStatus_Connecting,
    /** Saving the recording failed. */
    ZoomSDKRecordingStatus_Fail
}ZoomSDKRecordingStatus;

/**
 * @brief Enumeration of connection quality.
 */
typedef enum{
	/** Unknown connection status. */
    ZoomSDKConnectionQuality_Unknown,
	/** The connection quality is very poor. */
    ZoomSDKConnectionQuality_VeryBad,
	/** The connection quality is poor. */ 
    ZoomSDKConnectionQuality_Bad,
	/** The connection quality is not good. */
    ZoomSDKConnectionQuality_NotGood,
	/** The connection quality is normal. */
    ZoomSDKConnectionQuality_Normal,
	/** The connection quality is good. */
    ZoomSDKConnectionQuality_Good,
	/** The connection quality is excellent. */
    ZoomSDKConnectionQuality_Excellent
}ZoomSDKConnectionQuality;

/**
 * @brief Enumeration of video quality.
 */
typedef enum{
    /** Unknown video quality status. */
    ZoomSDKVideoQuality_Unknown,
    /** The video quality is poor. */
    ZoomSDKVideoQuality_Bad,
    /** The video quality is normal. */
    ZoomSDKVideoQuality_Normal,
    /** The video quality is good. */
    ZoomSDKVideoQuality_Good
}ZoomSDKVideoQuality;

/**
 * @brief Enumeration of H.323 device outgoing call status.
 * @note The order of enumeration members has been changed. H323CalloutStatus_Unknown has been moved.
 */
typedef enum
{
	/** Call out successfully. */
    H323CalloutStatus_Success,
	/** In process of ringing. */
    H323CalloutStatus_Ring,
	/** Timeout. */
    H323CalloutStatus_Timeout,
	/** Failed to call out. */
    H323CalloutStatus_Failed,
    /** Unknown status. */
    H323CalloutStatus_Unknown,
    /** Busy. */
    H323CalloutStatus_Busy,
    /** Declined. */
    H323CalloutStatus_Decline
}H323CalloutStatus;

/**
 * @brief Enumeration of H.323 device pairing Status.
 */
typedef enum
{
	/** Unknown status. */
    H323PairingResult_Unknown,
	/** Pairing successfully. */
    H323PairingResult_Success,
	/** Pairing meeting does not exist. */
    H323PairingResult_Meeting_Not_Exist,
	/** Pairing code does not exist. */
    H323PairingResult_Paringcode_Not_Exist,
	/** No pairing privilege. */
    H323PairingResult_No_Privilege,
	/** Other errors. */
    H323PairingResult_Other_Error
}H323PairingResult;

/**
 * @brief Enumeration of H.323 device types.
 */
typedef enum
{
	/** Unknown device type. */
    H323DeviceType_Unknown,
	/** H.323 device. */
    H323DeviceType_H323,
	/** SIP device. */
    H323DeviceType_SIP
}H323DeviceType;

/**
 * @brief Enumeration of screen types for multi-sharing.
 */
typedef enum
{
  /** Primary display. */
  ScreenType_First,
  /** Secondary display. */
  ScreenType_Second
}ScreenType;

/**
 * @brief Join meeting with required information.
 */
typedef enum
{
	/** For initialization. */
    JoinMeetingReqInfoType_None,
	/** Join meeting with password. */
    JoinMeetingReqInfoType_Password,
	/** The password for join meeting is incorrect. */
    JoinMeetingReqInfoType_Password_Wrong,
    /** The user needs to enter the screen name. */
    JoinMeetingReqInfoType_ScreenName
}JoinMeetingReqInfoType;

/**
 * @brief Enumeration of meeting types.
 */
typedef enum
{
	/** There is no meeting. */
    MeetingType_None,
	/** Normal meeting. */
    MeetingType_Normal,
	/** Breakout meeting. */
    MeetingType_BreakoutRoom,
	/** Webinar meeting. */
    MeetingType_Webinar
}MeetingType;

/**
 * @brief Enumeration of user roles.
 */
typedef enum
{
	/** For initialization. */
    UserRole_None,
	/** Host. */
    UserRole_Host,
	/** Co-host. */
    UserRole_CoHost,
	/** Attendee or webinar attendee. */
    UserRole_Attendee,
	/** Panelist. */
    UserRole_Panelist,
	/** Moderator of breakout room. */
    UserRole_BreakoutRoom_Moderator
}UserRole;

/**
 * @brief Enumeration of phone call status.
 */
typedef enum
{
	/** No status. */
    PhoneStatus_None,
	/** In process of calling out. */
    PhoneStatus_Calling,
	/** In process of ringing. */
    PhoneStatus_Ringing,
	/** The call is accepted. */
    PhoneStatus_Accepted,
	/** Call successful. */
    PhoneStatus_Success,
	/** Call failed. */
    PhoneStatus_Failed,
	/** In process of canceling the response to the previous state. */
    PhoneStatus_Canceling,
	/** Cancel successfully. */
    PhoneStatus_Canceled,
	/** Failed to cancel. */
    PhoneStatus_Cancel_Failed,
	/** Timeout. */
    PhoneStatus_Timeout
}PhoneStatus;

/**
 * @brief Enumeration of the reasons for the telephone call’s failure.
 */
typedef enum
{
	/** For initialization. */
    PhoneFailedReason_None,
	/** The telephone service is busy. */
    PhoneFailedReason_Busy,
	/** The telephone is out of service. */
    PhoneFailedReason_Not_Available,
	/** The phone is hung up. */
    PhoneFailedReason_User_Hangup,
	/** Other reasons. */
    PhoneFailedReason_Other_Fail,
	/** The call is not answered. */
    PhoneFailedReason_No_Answer,
	/** Disable the international callout function before the host joins the meeting. */
    PhoneFailedReason_Block_No_Host,
	/** The call-out is blocked by the system due to the high cost. */
    PhoneFailedReason_Block_High_Rate,
	/** All the users invited by the call should press one(1) to join the meeting. If many invitees do not press the button and instead are timed out, the call invitation for this meeting is blocked. */
    PhoneFailedReason_Block_Too_Frequent
}PhoneFailedReason;

/**
 * @brief Enumeration of types of shared content.
 */
typedef enum
{
	/** Type unknown. */
    ZoomSDKShareContentType_UNKNOWN,
	/** Type of sharing the application. */
    ZoomSDKShareContentType_AS,
	/** Type of sharing the desktop. */
    ZoomSDKShareContentType_DS,
	/** Type of sharing the white-board. */
    ZoomSDKShareContentType_WB,
	/** Type of sharing data from the device connected WIFI. */
    ZoomSDKShareContentType_AIRHOST,
	/** Type of sharing the camera. */
    ZoomSDKShareContentType_CAMERA,
	/** Type of sharing the data. */
    ZoomSDKShareContentType_DATA,
	/** Wired device, connect Mac and iPhone. */
    ZoomSDKShareContentType_WIRED_DEVICE,
	/** Share a portion of screen in the frame. */
    ZoomSDKShareContentType_FRAME,
	/** Share a document. */
    ZoomSDKShareContentType_DOCUMENT,
	/** Share only the audio sound of computer. */
    ZoomSDKShareContentType_COMPUTER_AUDIO,
    /** Type of sharing video file. */
    ZoomSDKShareContentType_VIDEO_FILE
}ZoomSDKShareContentType;

/**
 * @brief Enumeration of the number types for calling to join the audio into a meeting.
 */
typedef enum
{
	/** For initialization. */
    CallInNumberType_None,
	/** Paid (toll) number. */
    CallInNumberType_Toll,
	/** Toll-free number. */
    CallInNumberType_TollFree
}CallInNumberType;

/**
 * @brief Enumeration of in-meeting buttons on the toolbar.
 */
typedef enum
{
	/** Audio button: manage in-meeting audio of the current user. */
    AudioButton,
	/** Video button: manage in-meeting video of the current user. */
    VideoButton,
	/** Participant button: manage or check the participants. */
    ParticipantButton,
	/** Share button: share screen or application, etc. */
    FitBarNewShareButton,
	/** Remote control button when sharing or viewing the share. */ 
    FitBarRemoteControlButton,
	/** Pause the share. */
    FitBarPauseShareButton,
	/** Annotation button. */
    FitBarAnnotateButton,
	/** Question and answer(QA) button. Available only in webinar. */
    QAButton,
	/** Broadcast the webinar so user can join the webinar. */
    FitBarBroadcastButton,
	/** Poll button: questionnaire. */
    PollingButton,
	/** More: other functions in the menu. */
    FitBarMoreButton,
	/** Exit full screen. */
    MainExitFullScreenButton,
	/** Button for getting host. */
    ClaimHostButton,
	/** Upgarde button of free meeting remain time tooltip view. */
    UpgradeButtonInFreeMeetingRemainTimeTooltip,
    /** Swap share and video button: swap to display share or video. */
    SwapShareContentAndVideoButton,
    /** Chat button: manage in-meeting chat of the current user. */
    ChatButton,
    /** Reaction Button on tool bar. */
    ToolBarReactionsButton,
    /** Share button on tool bar. */
    ToolBarShareButton,
    /** Recording button. */
    RecordButton
}SDKButton;

/**
 * @brief Enumeration of security session types.
 * @deprecated This enum is deprecated and will be removed in a future release.
 */
typedef enum
{
	/** Unknown component. */
    SecuritySessionComponent_Unknown,
	/** Chat. */
    SecuritySessionComponent_Chat,
	/** File Transfer. */
    SecuritySessionComponent_FT,
	/** Audio. */
    SecuritySessionComponent_Audio,
	/** Video. */
    SecuritySessionComponent_Video,
	/** Share application. */
    SecuritySessionComponent_AS
}SecuritySessionComponet;

/**
 * @brief Enumeration of warning types.
 */
typedef enum
{
	/** No warnings. */
    StatisticWarningType_None,
	/** The quality of the network connection is very poor. */
    StatisticWarningType_NetworkBad,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    StatisticWarningType_CPUHigh,
	/** The system is busy. */
    StatisticWarningType_SystemBusy
}StatisticWarningType;

/**
 * @brief Enumeration of component types.
 */
typedef enum{
	/** For initialization. */
    ConnectionComponent_None,
	/** Share. */
    ConnectionComponent_Share,
	/** Video. */
    ConnectionComponent_Video,
	/** Audio. */
    ConnectionComponent_Audio
}ConnectionComponent;

/**
 * @brief Enumeration of ending meeting errors.
 */
typedef enum{
	/** For initialization. */
    EndMeetingReason_None = 0,
	/** The user is kicked off by the host and leaves the meeting. */
    EndMeetingReason_KickByHost = 1,
	/** Host ends the meeting. */
    EndMeetingReason_EndByHost = 2,
	/** Join the meeting before host (JBH) timeout. */
    EndMeetingReason_JBHTimeOut = 3,
	/** Meeting is ended for there is no attendee joins it. */
    EndMeetingReason_NoAttendee = 4,
	/** Host ends the meeting for he will start another meeting. */
    EndMeetingReason_HostStartAnotherMeeting = 5,
	/** Meeting is ended for the free meeting timeout. */
    EndMeetingReason_FreeMeetingTimeOut = 6,
    /** Represents an undefined end meeting reason, typically used for new error codes introduced by the backend after client release. */
    EndMeetingReason_Undefined = 7,
    /* Authorized user left. */
    EndMeetingReason_DueToAuthorizedUserLeave = 8,
}EndMeetingReason;

/**
 * @brief Enumeration of H.323/SIP encryption types.
 */
typedef enum
{
	/** Meeting room system is not encrypted. */
    EncryptType_NO,
	/** Meeting room system is encrypted. */
    EncryptType_YES,
	/** Meeting room system is encrypted automatically. */
    EncryptType_Auto
}EncryptType;

/**
 * @brief Enumeration of connection types.
 */
typedef enum{
	/** Unknown connection types. */
    SettingConnectionType_Unknown,
	/** Peer to peer. */
    SettingConnectionType_P2P,
	/** Connect to the cloud. */
    SettingConnectionType_Cloud
}SettingConnectionType;

/**
 * @brief Enumeration of network types.
 */
typedef enum{
	/** Unknown network type. */
    SettingNetworkType_Unknown,
	/** Wired LAN. */
    SettingNetworkType_Wired,
	/** WIFI. */
    SettingNetworkType_WiFi,
	/** PPP. */
    SettingNetworkType_PPP,
	/* 3G. */
    SettingNetworkType_3G,
	/** Other network types. */
    SettingNetworkType_Other
}SettingNetworkType;

/**
 * @brief Enumeration of video render element types.
 */
typedef enum{
	/** For initialization. */
    VideoRenderElementType_None,
	/** Preview the video of user himself. */
    VideoRenderElementType_Preview,
	/** Render the video of active speaker. */
    VideoRenderElementType_Active,
	/** Render normal video. */ 
    VideoRenderElementType_Normal
}VideoRenderElementType;

/**
 * @brief Enumeration of video render data types.
 */
typedef enum{
	/** For initialization. */
    VideoRenderDataType_None,
	/** Video data. */
    VideoRenderDataType_Video,
	/** Avatar data. */
    VideoRenderDataType_Avatar
}VideoRenderDataType;

/**
 * @brief Enumeration of video aspect modes.
 */
typedef enum{
	/** Stretch both horizontally and vertically to fill the display (may cause distortion). */
    ViewShareMode_FullFill,
    /** Add black bars to maintain aspect ratio (e.g., 16:9 content on a 4:3 display or vice versa). */
    ViewShareMode_LetterBox
}ViewShareMode;

/**
 * @brief Enumeration of annotation status.
 */
typedef enum{
	/** Ready to annotate. */
    AnnotationStatus_Ready,
	/** Annotation is closed. */
    AnnotationStatus_Close,
	/** For initialization. */
    AnnotationStatus_None
}AnnotationStatus;

/**
 * @brief Enumeration of live stream status.
 */
typedef enum{
	/** Only for initialization. */
    LiveStreamStatus_None,
	/** Live stream in process. */
    LiveStreamStatus_InProgress,
	/** Be connecting. */
    LiveStreamStatus_Connecting,
	/** Connect timeout. */
    LiveStreamStatus_StartFailedTimeout,
	/** Connect failed to the live streaming. */ 
    LiveStreamStatus_StartFailed,
	/** End. */
    LiveStreamStatus_Ended
}LiveStreamStatus;


/**
 * @brief Enumeration indicating reasons why a free meeting needs an upgrade.
 */
typedef enum{
    FreeMeetingNeedUpgradeType_NONE,
    /** Upgrade required due to admin payment reminder. */
    FreeMeetingNeedUpgradeType_BY_ADMIN,
    /** Upgrade triggered by gift URL. */
    FreeMeetingNeedUpgradeType_BY_GIFTURL
}FreeMeetingNeedUpgradeType;

/**
 * @brief Enumeration of direct sharing status.
 */
typedef enum{
	/** For initialization. */
    DirectShareStatus_None = 0,
	/** Waiting for enabling the direct sharing. */
    DirectShareStatus_Connecting = 1,
	/** In direct sharing mode. */
    DirectShareStatus_InProgress = 2,
	/** End the direct sharing. */
    DirectShareStatus_Ended = 3,
	/** Input the meeting ID/pairing code. */
    DirectShareStatus_NeedMeetingIDOrSharingKey = 4,
	/** The meeting ID or pairing code is wrong. */
    DirectShareStatus_WrongMeetingIDOrSharingKey = 5,
	/** Network issue. Reconnect later. */ 
    DirectShareStatus_NetworkError = 6,
    /** Need input new paring code. */
    DirectShareStatus_NeedInputNewPairingCode,
    /** Prepared. */
    DirectShareStatus_Prepared,
	/** Unknown share status. */
    DirectShareStatus_Unknown = 100
}DirectShareStatus;

/**
 * @brief Enumeration of types to register webinar.
 */
typedef enum
{
	/** For initialization. */
    WebinarRegisterType_None,
	/** Register webinar with URL. */
    WebinarRegisterType_URL,
	/** Register webinar with email. */
    WebinarRegisterType_Email
}WebinarRegisterType;

/**
 * @brief Enumeration of microphone test types.
 */
typedef enum{
	/** Normal status. */
    testMic_Normal = 0,
	/** Recording in progress. */
    testMic_Recording,
	/** Recording has stopped. */
    testMic_RecrodingStopped,
	/** Playing back the recorded audio. */
    testMic_Playing
}ZoomSDKTestMicStatus;

/**
 * @brief Enumeration of device status.
 */
typedef enum{
	/** Unknown device error. */
    Device_Error_Unknown,
	/** New device detected by the system. */
    New_Device_Found,
	/** Device not found. */
    Device_Error_Found,
	/** No device. */
    No_Device,
	/** No audio input detected from the microphone. */
    Audio_No_Input,
	/** Audio is muted. Press Command+Shift+A to unmute. */
    Audio_Error_Be_Muted,
	/** The device list has been updated. */
    Device_List_Update,
	/** Audio disconnected due to detected echo. */
    Audio_Disconnect_As_Detected_Echo
}ZoomSDKDeviceStatus;

/**
 * @brief Enumeration of sharing types.
 */
typedef enum{
	/** Anyone can share, but only one can share at a  moment, and only the host can start sharing when another user is sharing. The previous share will be ended once the host grabs the sharing. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0058730#h_01GCCD82NKKECQ6QJNH6F4CTWA>. */
    ShareSettingType_OnlyHostCanGrab = 0,
	/** Only host can share. */
    ShareSettingType_OnlyHostCanShare = 1,
	/** Only one participant can share at a time. And anyone can start sharing when someone else is sharing. */
    ShareSettingType_AnyoneCanGrab = 2,
	/** Multi participant can share at a moment. */
    ShareSettingType_MultiShare = 3,
    ShareSettingType_None = 4
}ZoomSDKShareSettingType;

/**
 * @brief Enumeration of General setting about share
 */
typedef enum {
    /** When user share screen will enter full screen. */
    shareSettingCmd_enterFullScreen,
    /** When user to share screen will enter max window. */
    shareSettingCmd_enterMaxWindow,
    /** Display the shared screen and participants' video side by side. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0067526>. */
    shareSettingCmd_sideToSideMode,
    /** Keep current size. */
    shareSettingCmd_MaintainCurrentSize,
    /** Automatically scale the shared screen to fit the size of the Zoom window. */
    shareSettingCmd_AutoFitWindowWhenViewShare
}shareSettingCmd;

/**
 * @brief Enumeration of attendee view question type.
 */
typedef enum {
    /** Attendee only view the answered question. */
    ViewType_OnlyAnswered_Question = 0,
    /** Attendee view the all question. */
    ViewType_All_Question
}AttendeeViewQuestionType;

/**
 * @brief Enumeration of question status.
 * @deprecated This enum is deprecated and will be removed in a future release.
 */
typedef enum {
    /** The question state is init. */
    QAQuestionState_Init = 0,
    /** The question is sent. */
    QAQuestionState_Sent,
    /** The question is received. */
    QAQuestionState_Received,
    /** The question send fail. */
    QAQuestionState_SendFail,
    /** The question is sending. */
    QAQuestionState_Sending,
    /** The question state is unknown for init. */
    QAQuestionState_Unknown
}ZoomSDKQAQuestionState;

/**
 * @brief Enumeration of Q&A connection statuses.
 */
typedef enum {
    /** The Q&A is connecting. */
    QAConnectStatus_Connecting = 0,
    /** The Q&A is connected. */
    QAConnectStatus_Connected,
    /** The Q&A is disonnected. */
    QAConnectStatus_Disconnected,
    /** The Q&A disconnected due to conflict. */
    QAConnectStatus_Disconnect_Conflict
}ZoomSDKQAConnectStatus;

/**
 * @brief Enumeration of audio button action info.
 */
typedef enum {
    /** The audio button action info is none. */
    ZoomSDKAudioActionInfo_none = 0,
    /** Need to join VoIP audio. */
    ZoomSDKAudioActionInfo_needJoinVoip,
    /** Need to mute or unmute audio. */
    ZoomSDKAudioActionInfo_muteOrUnmenuAudio,
    /** No audio device connected. */
    ZoomSDKAudioActionInfo_noAudioDeviceConnected,
    /** Computer audio device error. */
    ZoomSDKAudioActionInfo_computerAudioDeviceError
}ZoomSDKAudioActionInfo;

/**
 * @brief Enumeration of breakout meeting user status.
 */
typedef enum{
    /** The breakout meeting status is unknown. */
    ZoomSDKBOUserStatus_Unknown = 0,
    /** The user is unassigned to any breakout meeting. */
    ZoomSDKBOUserStatus_UnAssigned,
    /** The user is assigned but has not joined the breakout meeting. */
    ZoomSDKBOUserStatus_Assigned_Not_Join,
    /** The user is currently in the breakout meeting. */
    ZoomSDKBOUserStatus_InBreakOutMeeting
}ZoomSDKBOUserStatus;

/**
 * @brief Enumeration of limited FPS (frames per second) values.
 */
typedef enum {
    /** FPS value is 1. */
    ZoomSDKFPSValue_One,
    /** FPS value is 2. */
    ZoomSDKFPSValue_Two,
    /** TFPS value is 4. */
    ZoomSDKFPSValue_Four,
    /** TFPS value is 6. */
    ZoomSDKFPSValue_Six,
    /** FPS value is 8. */
    ZoomSDKFPSValue_Eight,
    /** FPS value is 10. */
    ZoomSDKFPSValue_Ten,
    /** FPS value is 15. */
    ZoomSDKFPSValue_Fifteen
}ZoomSDKFPSValue;

/**
 * @brief Enumeration of attendee request for help results.
 */
typedef enum {
    /** Host is handling other's request with the request dialog, no chance to show dialog for this request. */
    ZoomSDKRequest4HelpResult_Busy,
    /** Host click "later" button or close the request dialog directly. */
    ZoomSDKRequest4HelpResult_Ignore,
    /** Host already in your breakout meeting. */
    ZoomSDKRequest4HelpResult_HostAlreadyInBO,
    /** For initialization (Host receive the help request and there is no other one currently requesting for help). */
    ZoomSDKRequest4HelpResult_Idle
}ZoomSDKRequest4HelpResult;

/**
 * @brief Enumeration of memory modes for raw data handling.
 */
typedef enum
{
    /** Use stack memory. */
    ZoomSDKRawDataMemoryMode_Stack,
    /** Use heap memory. */
    ZoomSDKRawDataMemoryMode_Heap
}ZoomSDKRawDataMemoryMode;

/**
 * @brief Enumeration of video resolution options.
 */
typedef enum
{
    /** Resolution 90P. */
    ZoomSDKResolution_90P,
    /** Resolution 180P. */
    ZoomSDKResolution_180P,
    /** Resolution 360P. */
    ZoomSDKResolution_360P,
    /** Resolution 720P. (Video resolution might be 1080P based on the user's network condition and device specs). */
    ZoomSDKResolution_720P,
    /** Resolution 1080P (Video resolution might be 720P based on the user's network condition and device specs). */
    ZoomSDKResolution_1080P,
    /** Not used. */
    ZoomSDKResolution_NoUse = 100
}ZoomSDKResolution;

/**
 * @brief Enumeration of local video device rotation actions.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKLOCAL_DEVICE_ROTATION_ACTION_UNKnown,
    /** Rotation 0 degrees. */
    ZoomSDKLOCAL_DEVICE_ROTATION_ACTION_0,
    /** Rotation clockwise 90 degrees. */
    ZoomSDKLOCAL_DEVICE_ROTATION_ACTION_CLOCK90,
    /** Rotation clockwise 180 degrees. */
    ZoomSDKLOCAL_DEVICE_ROTATION_ACTION_CLOCK180,
    /** Rotation counter-clockwise 90 degrees. */
    ZoomSDKLOCAL_DEVICE_ROTATION_ACTION_ANTI_CLOCK90
}ZoomSDKLocalVideoDeviceRotation;

/**
 * @brief Enumeration of raw data types.
 */
typedef enum
{
    /** Video raw data. */
    ZoomSDKRawDataType_Video = 1,
    /** Share raw data. */
    ZoomSDKRawDataType_Share
}ZoomSDKRawDataType;

/**
 * @brief Enumeration of file types for saving screenshots.
 */
typedef enum
{
    /** PNG file format. */
    ZoomSDKAnnotationSavedType_PNG,
    /** PDF file format. */
    ZoomSDKAnnotationSavedType_PDF
}ZoomSDKAnnotationSavedType;

/**
 * @brief Enumeration of the privilege for attendee chat.
 */
typedef enum
{
    /** Allow attendee to chat with everyone. [meeting & webinar]. */
    ZoomSDKChatPrivilegeType_To_Everyone,
    /** Allow attendee to chat with all panelists only.[for webinar]. */
    ZoomSDKChatPrivilegeType_To_All_Panelist,
    /** Allow attendee to chat with host only [meeting]. */
    ZoomSDKChatPrivilegeType_To_Host,
    /** Allow attendee to chat with no one [meeting & webinar]. */
    ZoomSDKChatPrivilegeType_Disable_Attendee_Chat,
    /** Allow attendee to chat with host and public [meeting]. */
    ZoomSDKChatPrivilegeType_Host_Public
}ZoomSDKChatPrivilegeType;

/**
 * @brief Enumeration of the panelist chat privilege in webinar meeting.
 */
typedef enum
{
    /** Allow panelists only to chat with each other.[for webinar]. */
    ZoomSDKPanelistChatPrivilege_PanelistOnly,
    /** Allow panelist to chat with everyone.[for webinar]. */
    ZoomSDKPanelistChatPrivilege_All
}ZoomSDKPanelistChatPrivilege;

/**
 * @brief Enumeration of the type for chat message.
 */
typedef enum
{
    /** For initialize. */
    ZoomSDKChatMessageType_To_None,
    /** Chat message is send to all in normal meeting ,also means to all panelist and attendees when webinar meeting. */
    ZoomSDKChatMessageType_To_All,
    /** Chat message is send to all panelists. */
    ZoomSDKChatMessageType_To_All_Panelist,
    /** Chat message is send to individual attendee and cc panelists. */
    ZoomSDKChatMessageType_To_Individual_Panelist,
    /** Chat message is send to individual user. */
    ZoomSDKChatMessageType_To_Individual,
    /** Chat message is send to waiting room user. */
    ZoomSDKChatMessageType_To_WaitingRoomUsers
}ZoomSDKChatMessageType;

/**
 * @brief Enumeration of background noise suppression levels. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0059985>.
 */
typedef enum
{
    /** For initialize. */
    ZoomSDKSuppressBackgroundNoiseLevel_None,
    /** Automatically adjust noise suppression. */
    ZoomSDKSuppressBackgroundNoiseLevel_Auto,
    /** Low level suppression. Allows faint background. */
    ZoomSDKSuppressBackgroundNoiseLevel_Low,
    /** Medium level suppression. Filters out moderate noise like computer fan or desk taps. */
    ZoomSDKSuppressBackgroundNoiseLevel_Medium,
    /** High level suppression. Eliminates most background speech and persistent noise. */
    ZoomSDKSuppressBackgroundNoiseLevel_High
}ZoomSDKSuppressBackgroundNoiseLevel;

/**
 * @brief Enumeration of the mode for screen capture. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0063824>.
 */
typedef enum
{
    /** Automatically choose the best method to use for screen share. */
    ZoomSDKScreenCaptureMode_Auto,
    /** This mode can be applicable if you are not on the latest operating systems, or don't have certain video drivers. If this option isn't enabled, a blank screen may appear on participants' screens while the host shares their screen. */
    ZoomSDKScreenCaptureMode_Legacy,
    /** This mode will share your screen without showing windows from the app. */
    ZoomSDKScreenCaptureMode_GPU_Copy_Filter,
    /** This mode will share your screen, include motion detection (when you move a window or play a movie), and will not show windows from the app. */
    ZoomSDKScreenCaptureMode_ADA_Copy_Filter,
    /** This mode will share your screen, include motion detection (when you move a window or play a movie), and will show windows from the app. */
    ZoomSDKScreenCaptureMode_ADA_Copy_Without_Filter
}ZoomSDKScreenCaptureMode;

/**
 * @brief Enumeration of video light adjustment modes. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0060612>.
 */
typedef enum
{
    /** Light adaption is none. */
    ZoomSDKSettingVideoLightAdaptionModel_None,
    /** Light adjustment is performed automatically based on current lighting conditions. */
    ZoomSDKSettingVideoLightAdaptionModel_Auto,
    /** Light adjustment is controlled manually. */
    ZoomSDKSettingVideoLightAdaptionModel_Manual
}ZoomSDKSettingVideoLightAdaptionModel;

/**
 * @brief Enumeration of virtual background video errors.
 */
typedef enum
{
    ZoomSDKSettingVBVideoError_None = 0,
    /** Unsupported or unrecognized video format. */
    ZoomSDKSettingVBVideoError_UnknownFormat,
    /** Video resolution is too large to be used as a virtual background. */
    ZoomSDKSettingVBVideoError_ResolutionBig,
    /** Video resolution exceeds 720p. */
    ZoomSDKSettingVBVideoError_ResolutionHigh720P,
    /** Video resolution is too low for virtual background requirements. */
    ZoomSDKSettingVBVideoError_ResolutionLow,
    /** Error occurred while trying to play the video. */
    ZoomSDKSettingVBVideoError_PlayError,
    /** Failed to open the video file. */
    ZoomSDKSettingVBVideoError_OpenError
}ZoomSDKSettingVBVideoError;

/**
 * @brief Enumeration of video effect types.
 */
typedef enum
{
    ZoomSDKVideoEffectType_None = 0,
    /** Filter effect. */
    ZoomSDKVideoEffectType_Filter = 1,
    /** Decorative frame effect added around the video. */
    ZoomSDKVideoEffectType_Frame = 2,
    /** Custom video filter. */
    ZoomSDKVideoEffectType_CustomFilter = 3,
    /** Fun or themed stickers overlaid on the video. */
    ZoomSDKVideoEffectType_Sticker = 4
}ZoomSDKVideoEffectType;

/**
 * @brief Enumeration of UI appearance modes.
 */
typedef enum
{
    /** Follow the system default appearance (light or dark mode). */
    ZoomSDKUIAppearance_System,
    /** Force the UI to use light mode, regardless of system setting. */
    ZoomSDKUIAppearance_Light,
    /** Force the UI to use dark mode, regardless of system setting. */
    ZoomSDKUIAppearance_Dark
}ZoomSDKUIAppearance;

/**
 * @brief Enumeration of available UI themes.
 */
typedef enum
{
    /** Bloom theme. */
    ZoomSDKUITheme_Bloom,
    /** Rose theme. */
    ZoomSDKUITheme_Rose,
    /** Agave theme. */
    ZoomSDKUITheme_Agave,
    /** Classic theme. */
    ZoomSDKUITheme_Classic
}ZoomSDKUITheme;

/**
 * @brief Enumeration of available emoji reaction types. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0060612>.
 */
typedef enum
{
    ZoomSDKEmojiReactionType_Unknown = 0,
    /** Clap emoji reaction. */
    ZoomSDKEmojiReactionType_Clap,
    /** Thumbs-up emoji reaction. */
    ZoomSDKEmojiReactionType_Thumbsup,
    /** Heart emoji reaction. */
    ZoomSDKEmojiReactionType_Heart,
    /** Joy emoji reaction. */
    ZoomSDKEmojiReactionType_Joy,
    /** Open-mouth emoji reaction. */
    ZoomSDKEmojiReactionType_Openmouth,
    /** Tada emoji reaction. */
    ZoomSDKEmojiReactionType_Tada
}ZoomSDKEmojiReactionType;

/**
 * @brief Enumeration of available emoji reaction skin tones.
 */
typedef enum
{
    ZoomSDKEmojiReactionSkinTone_Unknown = 0,
    /** Default skin tone. */
    ZoomSDKEmojiReactionSkinTone_Default,
    /** Light skin tone. */
    ZoomSDKEmojiReactionSkinTone_Light,
    /** Medium-light skin tone. */
    ZoomSDKEmojiReactionSkinTone_MediumLight,
    /** Medium skin tone. */
    ZoomSDKEmojiReactionSkinTone_Medium,
    /** Medium-dark skin tone. */
    ZoomSDKEmojiReactionSkinTone_MediumDark,
    /** Dark skin tone. */
    ZoomSDKEmojiReactionSkinTone_Dark
}ZoomSDKEmojiReactionSkinTone;

/**
 * @brief Enumeration of echo cancellation. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0066398>.
 */
typedef enum
{
    /** Automatically adjust echo cancellation, balancing CPU and performance. */
    ZoomSDKAudioEchoCancellationLevel_Auto = 0,
    /** Better echo limitation, taking into account multiple people talking at the same time, low CPU utilization. */
    ZoomSDKAudioEchoCancellationLevel_Low,
    /** Best experience when multiple people are talking at the same time. Enabling this option may increase CPU utilization. */
    ZoomSDKAudioEchoCancellationLevel_High
}ZoomSDKAudioEchoCancellationLevel;

/**
 * @brief Enumeration of screen sharing options when setting the page share screen item. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0060612>.
 */
typedef enum
{
    /** Share individual Window. Only for set share application. */
    ZoomSDKSettingShareScreenShareOption_IndividualWindow,
    /** Share all window from a specific application. Only for set share application. */
    ZoomSDKSettingShareScreenShareOption_AllWindowFromApplication,
    /** Automatically share desktop. Applicable for meeting share or direct share. */
    ZoomSDKSettingShareScreenShareOption_AutoShareDesktop,
    /** Show all sharing options. Applicable for meeting share or direct share. */
    ZoomSDKSettingShareScreenShareOption_AllOption
}ZoomSDKSettingShareScreenShareOption;

/**
 * @brief Enumeration of the spotlight operation result.
 */
typedef enum
{
    /** Spotlight operation succeeded. */
    ZoomSDKSpotlightResult_Success = 0,
    /** Failed to spotlight: not enough users in the meeting (less than 2 users). */
    ZoomSDKSpotlightResult_Fail_NotEnoughUsers,
    /** Failed to spotlight: too many users already spotlighted (more than 9 users). */
    ZoomSDKSpotlightResult_Fail_ToMuchSpotlightedUsers,
    /** Failed to spotlight: the user is in view-only mode, silent mode, or currently active in some other capacity. */
    ZoomSDKSpotlightResult_Fail_UserCannotBeSpotlighted,
    /** Failed to spotlight: the user has not turned on their video. */
    ZoomSDKSpotlightResult_Fail_UserWithoutVideo,
    /** Failed to spotlight: the current user has no privilege to spotlight another user. */
    ZoomSDKSpotlightResult_Fail_NoPrivilegeToSpotlight,
    /** Failed to spotlight: the user was not successfully spotlighted. */
    ZoomSDKSpotlightResult_Fail_UserNotSpotlighted,
    /** Unknown error occurred during the spotlight operation. */
    ZoomSDKSpotlightResult_Unknown = 100
}ZoomSDKSpotlightResult;

/**
 * @brief Enumeration of the result of a pin operation.
 */
typedef enum
{
    /** Pin operation succeeded. */
    ZoomSDKPinResult_Success = 0,
    /** Failed to pin: not enough users in the meeting (less than 2 users). */
    ZoomSDKPinResult_Fail_NotEnoughUsers,
    /** Failed to pin: too many users already pinned (more than 9 users). */
    ZoomSDKPinResult_Fail_ToMuchPinnedUsers,
    /** Failed to pin: the user is in view-only mode, silent mode, or active mode. */
    ZoomSDKPinResult_Fail_UserCannotBePinned,
    /** Failed to pin: the current video mode does not support pinning (other unspecified reasons). */
    ZoomSDKPinResult_Fail_VideoModeDoNotSupport, 
    /** Failed to pin: the current user has no privilege to pin another user. */
    ZoomSDKPinResult_Fail_NoPrivilegeToPin,
    /** Failed to pin: the meeting type (e.g., webinar or view-only meeting) does not support pinning. */
    ZoomSDKPinResult_Fail_MeetingDoNotSupport,
    /** Too many users in the meeting to allow pinning. */
    ZoomSDKPinResult_Fail_TooManyUsers,
    /** Unknown error occurred during the pin operation. */
    ZoomSDKPinResult_Unknown = 100
}ZoomSDKPinResult;

/**
 * @brief Enumeration of the login failure reasons.
 */
typedef enum
{
    /** No login failure reason (success case). */
    ZoomSDKLoginFailReason_None = 0,
    /** Login failed: email login is disabled for the account. */
    ZoomSDKLoginFailReason_EmailLoginDisabled,
    /** Login failed: the user does not exist. */
    ZoomSDKLoginFailReason_UserNotExist,
    /** Login failed: the password is incorrect. */
    ZoomSDKLoginFailReason_WrongPassword,
    /** Login failed: the account is locked. */
    ZoomSDKLoginFailReason_AccountLocked,
    /** Login failed: the SDK needs to be updated to the new version. */
    ZoomSDKLoginFailReason_SDKNeedUpdate,
    /** Login failed: too many failed login attempts. */
    ZoomSDKLoginFailReason_TooManyFailedAttempts,
    /** Login failed: the entered SMS code is incorrect. */
    ZoomSDKLoginFailReason_SMSCodeError,
    /** Login failed: the SMS code has expired. */
    ZoomSDKLoginFailReason_SMSCodeExpired,
    /** Login failed: the phone number format is invalid. */
    ZoomSDKLoginFailReason_PhoneNumberFormatInValid,
    /** Login failed: the login token is invalid or expired. */
    ZoomSDKLoginFailReason_LoginTokenInvalid,
    /** Login failed: the user disagreed with the login disclaimer. */
    ZoomSDKLoginFailReason_UserDisagreeLoginDisclaimer,
    /** Login failed: multi-factor authentication (MFA) is required. */
    ZoomSDKLoginFailReason_MFARequired,
    /** Login failed: requires the user to provide their birthday. */
    ZoomSDKLoginFailReason_NeedBirthdayAsk,
    /** Login failed due to other unspecified reasons. */
    ZoomSDKLoginFailReason_Other_Issue = 100
}ZoomSDKLoginFailReason;

/**
 * @brief Enumeration of setting page URLs.
 */
typedef enum{
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKSettingPageURL_General_ViewMoreSetting,
    /** URL for the "Learn More" link on the Audio settings page. */
    ZoomSDKSettingPageURL_Audio_LearnMore,
    /** URL for the "Learn More" link on the Virtual Background (VB) settings page. */
    ZoomSDKSettingPageURL_VB_LearnMore,
    /** URL for the "Learn More" link on the Share Screen settings page. */
    ZoomSDKSettingPageURL_ShareScreen_LearnMore
} ZoomSDKSettingPageURL;

/**
 * @brief Enumeration of the errors related to the Breakout Room (BO) controller operations.
 */
typedef enum{
    /** The pointer is null. */
    ZoomSDKBOControllerError_Null_Pointer = 0,
    /** Invalid action due to current BO status (e.g., already started or stopped). */
    ZoomSDKBOControllerError_Wrong_Current_Status,
    /** BO token is not ready yet. */
    ZoomSDKBOControllerError_Token_Not_Ready,
    /** Only the host has the privilege to create, start, or stop breakout rooms. */
    ZoomSDKBOControllerError_No_Privilege,
    /** Breakout room list is currently being uploaded. */
    ZoomSDKBOControllerError_BO_List_Is_Uploading,
    /** Failed to upload the breakout room list to the conference attributes. */
    ZoomSDKBOControllerError_Upload_Fail,
    /** No participants have been assigned to breakout rooms. */
    ZoomSDKBOControllerError_No_One_Has_Been_Assigned,
    /** Unknown error. */
    ZoomSDKBOControllerError_Unknown = 100
} ZoomSDKBOControllerError;

/**
 * @brief Enumeration of video render resolution.
 */
typedef enum{
    /** No resolution specified (typically used for initialization). */
    ZoomSDKVideoRenderResolution_None = 0,
    /* 90p resolution. */
    ZoomSDKVideoRenderResolution_90p,
    /* 180p resolution. */
    ZoomSDKVideoRenderResolution_180p,
    /* 360p resolution. */
    ZoomSDKVideoRenderResolution_360p,
    /* 720p resolution. */
    ZoomSDKVideoRenderResolution_720p,
    /* 1080p resolution. */
    ZoomSDKVideoRenderResolution_1080p
} ZoomSDKVideoRenderResolution;

/**
 * @brief Enumeration of Breakout Room (BO) stop countdown durations.
 */
typedef enum{
    /** No countdown. Breakout Rooms stop immediately. */
    ZoomSDKBOStopCountDown_Not = 0,
    /** Countdown duration: 10 seconds before BO stops. */
    ZoomSDKBOStopCountDown_Seconds_10,
    /** Countdown duration: 15 seconds before BO stops. */
    ZoomSDKBOStopCountDown_Seconds_15,
    /** Countdown duration: 30 seconds before BO stops. */
    ZoomSDKBOStopCountDown_Seconds_30,
    /** Countdown duration: 60 seconds before BO stops. */
    ZoomSDKBOStopCountDown_Seconds_60,
    /** Countdown duration: 120 seconds before BO stops. */
    ZoomSDKBOStopCountDown_Seconds_120
} ZoomSDKBOStopCountDown;

/**
 * @brief Enumeration of Breakout Room (BO) status.
 */
typedef enum{
    /** BO status is invalid. */
    ZoomSDKBOStatus_Invalid = 0,
    /** Editing and assigning participants to Breakout Rooms. */
    ZoomSDKBOStatus_Edit = 1,
    /** Breakout Rooms have started. */
    ZoomSDKBOStatus_Started = 2,
    /** Breakout Rooms are currently stopping. */
    ZoomSDKBOStatus_Stopping = 3,
    /** Breakout Rooms have ended. */
    ZoomSDKBOStatus_Ended = 4
} ZoomSDKBOStatus;

/**
 * @brief Enumeration of the type for video subscribe failed reason.
 */
typedef enum{
    /** No failure. */
    ZoomSDKVideoSubscribe_Fail_None = 0,
    /** The user is in view-only mode and cannot subscribe to video. */
    ZoomSDKVideoSubscribe_Fail_ViewOnly,
    /** The user is not currently in the meeting. */
    ZoomSDKVideoSubscribe_Fail_NotInMeeting,
    /** Already subscribed to a 1080p or 720p video stream. Cannot subscribe to additional streams at that quality. */
    ZoomSDKVideoSubscribe_Fail_HasSubscribe1080POr720P,
    /** Already subscribed to a 720p video stream. */
    ZoomSDKVideoSubscribe_Fail_HasSubscribe720P,
    /** Already subscribed to two 720p video streams. Maximum limit reached. */
    ZoomSDKVideoSubscribe_Fail_HasSubscribeTwo720P,
    /** The number of video subscriptions has exceeded the allowed limit. */
    ZoomSDKVideoSubscribe_Fail_HasSubscribeExceededLimit,
    /** The subscription requests were made too frequently in a short period of time. */
    ZoomSDKVideoSubscribe_Fail_TooFrequentCall
} ZoomSDKVideoSubscribeFailReason;

/**
 * @brief Enumeration of live transcription status in a meeting.
 */
typedef enum
{
    /** Live transcription has not started. */
    ZoomSDK_LiveTranscription_Status_Stop = 0,
    /** Live transcription is active. */
    ZoomSDK_LiveTranscription_Status_Start = 1,
    /** The user has subscribed to live transcription. */
    ZoomSDK_LiveTranscription_Status_User_Sub = 2,
    /** Live transcription service is connecting. */
    ZoomSDK_LiveTranscription_Status_Connecting = 10
}ZoomSDKLiveTranscriptionStaus;

/**
 * @brief Enumeration of live transcription operation types.
 */
typedef enum
{
    /** No operation. Typically used for initialization. */
    ZoomSDK_LiveTranscription_OperationType_None = 0,
    /** Add a new live transcription entry. */
    ZoomSDK_LiveTranscription_OperationType_Add,
    /** Update an existing live transcription entry. */
    ZoomSDK_LiveTranscription_OperationType_Update,
    /** Delete a live transcription entry. */
    ZoomSDK_LiveTranscription_OperationType_Delete,
    /** The transcription entry complete. */
    ZoomSDK_LiveTranscription_OperationType_Complete,
    /** The operation is not supported. */
    ZoomSDK_LiveTranscription_OperationType_NotSupported
}ZoomSDKLiveTranscriptionOperationType;

/**
 * @brief Enumeration of zoom ratios of the shared content view.
 */
typedef enum
{
    /** Zoom ratio set to 50%. */
    ZoomSDKShareViewZoomRatio_50 = 0,
    /** Zoom ratio set to 100% (actual size). */
    ZoomSDKShareViewZoomRatio_100,
    /** Zoom ratio set to 150%. */
    ZoomSDKShareViewZoomRatio_150,
    /** Zoom ratio set to 200%. */
    ZoomSDKShareViewZoomRatio_200,
    /** Zoom ratio set to 300%. */
    ZoomSDKShareViewZoomRatio_300
}ZoomSDKShareViewZoomRatio;

/**
 * @brief Enumeration of the chat message delete type.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDK_Chat_Delete_By_None,
    /** Message is deleted by the sender (self deletion). */
    ZoomSDK_Chat_Delete_By_Self,
    /** Message is deleted by the meeting host. */
    ZoomSDK_Chat_Delete_By_Host,
    /** Message is deleted by Data Loss Prevention (DLP) system for violating compliance policies. */
    ZoomSDK_Chat_Delete_By_Dlp
}ZoomSDKChatMessageDeleteType;

/**
 * @brief Enumeration of the audio share modes.
 */
typedef enum
{
    /** Mono audio share mode. Single channel audio. */
    ZoomSDKAudioShareMode_Mono,
    /** Stereo audio share mode. Two-channel audio. */
    ZoomSDKAudioShareMode_Stereo
}ZoomSDKAudioShareMode;

/**
 * @brief Enumeration of waiting room layout type. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0059359>.
 */
typedef enum
{
    /** Default layout. */
    ZoomSDKWaitingRoomLayoutType_Default,
    /** Layout displaying a logo. */
    ZoomSDKWaitingRoomLayoutType_Logo,
    /** Layout displaying a video. */
    ZoomSDKWaitingRoomLayoutType_Video
}ZoomSDKWaitingRoomLayoutType;

/**
 * @brief Enumeration of the status of custom waiting room data.
 */
typedef enum
{
    /** Initial state, before any download has started. */
    ZoomSDKCustomWaitingRoomDataStatus_Init,
    /** Custom waiting room data is currently being downloaded. */
    ZoomSDKCustomWaitingRoomDataStatus_Downloading,
    /** Custom waiting room data has been successfully downloaded. */
    ZoomSDKCustomWaitingRoomDataStatus_Download_OK,
    /** Failed to download custom waiting room data. */
    ZoomSDKCustomWaitingRoomDataStatus_Download_Failed
}ZoomSDKCustomWaitingRoomDataStatus;

/**
 * @brief Enumeration of the status of sign language interpretation.
 */
typedef enum
{
    /** The initial status. */
    ZoomSDKSignInterpretationStatus_Initial,
    /** Sign language interpretation has started. */
    ZoomSDKSignInterpretationStatus_Started,
    /** Sign language interpretation has been stopped. */
    ZoomSDKSignInterpretationStatus_Stopped
}ZoomSDKSignInterpretationStatus;

/**
 * @brief Enumeration of the status of the notification service.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKNotificationServiceStatus_None = 0,
    /** Starting. */
    ZoomSDKNotificationServiceStatus_Starting,
    /** Started. */
    ZoomSDKNotificationServiceStatus_Started,
    /** Failed to start. */
    ZoomSDKNotificationServiceStatus_StartFailed,
    ZoomSDKNotificationServiceStatus_Closed
}ZoomSDKNotificationServiceStatus;

/**
 * @brief Enumeration of notification service error codes.
 */
typedef enum
{
    /** Operation completed successfully. */
    ZoomSDKNotificationServiceError_Success = 0,
    /** Unknown error occurred. */
    ZoomSDKNotificationServiceError_Unknown,
    /** Internal error occurred. Retry may be required. */
    ZoomSDKNotificationServiceError_Internal_Error,
    /** Token is invalid. */
    ZoomSDKNotificationServiceError_Invalid_Token,
    /** Multiple logins detected on the same device with the same user/resource. The previous login session will receive this error. */
    ZoomSDKNotificationServiceError_Multi_Connect,
    /** Network connection issue encountered. */
    ZoomSDKNotificationServiceError_Network_Issue,
    /** Connection exceeded maximum allowed duration (24 hours). Client must reconnect or log in again. */
    ZoomSDKNotificationServiceError_Max_Duration
}ZoomSDKNotificationServiceError;

/**
 * @brief Enumeration of the types of face makeup effects.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKFaceMakeupType_None,
    /** A virtual mustache. */
    ZoomSDKFaceMakeupType_Mustache,
    /** Enhances or changes eyebrow appearance. */
    ZoomSDKFaceMakeupType_Eyebrow,
    /** Applies lip makeup. */
    ZoomSDKFaceMakeupType_Lip
}ZoomSDKFaceMakeupType;

/**
 * @brief Enumeration of the status of a local recording permission request.
 */
typedef enum
{
    /** The request for local recording permission was granted. */
    ZoomSDKRequestLocalRecordingStatus_Granted,
    /** The request for local recording permission was denied. */
    ZoomSDKRequestLocalRecordingStatus_Denied,
    /** The request timed out without a response. */
    ZoomSDKRequestLocalRecordingStatus_Timeout
}ZoomSDKRequestLocalRecordingStatus;

/**
 * @brief Enumeration of attendee view modes in a Zoom meeting. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0063672>.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKAttendeeViewMode_None,
    /** Attendee view follows the host's view setting. */
    ZoomSDKAttendeeViewMode_FollowHost,
    /** Speaker view: shows the active speaker. */
    ZoomSDKAttendeeViewMode_Speaker,
    /** Gallery view: displays all participants in a grid. */
    ZoomSDKAttendeeViewMode_Gallery,
    /** Standard screen sharing view. */
    ZoomSDKAttendeeViewMode_Sharing_Standard,
    /** Side-by-side view with shared content and speaker. */
    ZoomSDKAttendeeViewMode_Sharing_SidebysideSpeaker,
    /** Side-by-side view with shared content and gallery. */
    ZoomSDKAttendeeViewMode_Sharing_SidebysideGallery
}ZoomSDKAttendeeViewMode;

/**
 * @brief Enumeration of emoji reaction display types in the meeting UI.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKEmojiReactionDisplayType_None,
    /** Display all emoji reactions in full detail. */
    ZoomSDKEmojiReactionDisplayType_Full,
    /** Display emoji reactions in a medium form. */
    ZoomSDKEmojiReactionDisplayType_Medium,
    /** Do not display emoji reactions. */
    ZoomSDKEmojiReactionDisplayType_Hidden
}ZoomSDKEmojiReactionDisplayType;

/**
 * @brief Enumeration of supported audio types in a Zoom meeting. This is a bitmask enumeration; multiple values can be combined using bitwise OR.
 */
typedef enum
{
    /** No audio support. */
    ZoomSDKMeetingSupportAudioType_None = 0,
    /** VoIP (Voice over Internet Protocol) audio supported. */
    ZoomSDKMeetingSupportAudioType_Voip = 1,
    /** Telephony (phone dial-in/out) audio supported. */
    ZoomSDKMeetingSupportAudioType_Telephony = 1 << 1
}ZoomSDKMeetingSupportAudioType;

/**
 * @brief Enumeration of local recording request privilege settings.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKLocalRecordingRequestPrivilege_None = 0,
    /** Allow participants to send recording privilege requests. */
    ZoomSDKLocalRecordingRequestPrivilege_AllowRequest,
    /** Automatically grant all recording privilege requests. */
    ZoomSDKLocalRecordingRequestPrivilege_AutoGrant,
    /** Automatically deny all recording privilege requests. */
    ZoomSDKLocalRecordingRequestPrivilege_AutoDeny
}ZoomSDKLocalRecordingRequestPrivilegeStatus;

/**
 * @brief Enumeration of video frame data format.
 */
typedef enum
{
    /** I420 format with limited (TV) range color space. */
    ZoomSDKFrameDataFormat_I420_Limited,
    /** I420 format with full (PC) range color space. */
    ZoomSDKFrameDataFormat_I420_Full
}ZoomSDKFrameDataFormat;

/**
 * @brief Enumeration of user's presence status. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0065488>.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKPresenceStatus_None,
    /** User is available. */
    ZoomSDKPresenceStatus_Available,
    /** User is offline or not available. */
    ZoomSDKPresenceStatus_UnAvailable,
    /** User is currently in a meeting. */
    ZoomSDKPresenceStatus_InMeeting,
    /** User is marked as busy. */
    ZoomSDKPresenceStatus_Busy,
    /** User does not want to be disturbed. */
    ZoomSDKPresenceStatus_DoNotDisturb,
    /** User is away from the device. */
    ZoomSDKPresenceStatus_Away,
    /** User is on a phone call. */
    ZoomSDKPresenceStatus_PhoneCall,
    /** User is currently presenting content. */
    ZoomSDKPresenceStatus_Presenting,
    /** User is in a calendar-scheduled event. */
    ZoomSDKPresenceStatus_Calendar,
    /** User is marked as out of office. */
    ZoomSDKPresenceStatus_OutOfOffice
}ZoomSDKPresenceStatus;

/**
 * @brief Enumeration of reminder types.
 */
typedef enum
{
    /** Reminder type of login. */
    ZoomSDKReminderType_LoginRequired = 0,
    /** Reminder type of start or join meeting. */
    ZoomSDKReminderType_StartOrJoinMeeting,
    /** Reminder type of record reminder. */
    ZoomSDKReminderType_RecordReminder,
    /** Reminder type of record reminder. */
    ZoomSDKReminderType_RecordDisclaimer,
    /** Reminder type of live stream reminder. */
    ZoomSDKReminderType_LiveStreamDisclaimer,
    /** Reminder type of archive reminder. */
    ZoomSDKReminderType_ArchiveDisclaimer,
    /** Reminder type of join webinar as panelist. */
    ZoomSDKReminderType_WebinarAsPanelistJoin,
    /** Reminder type of terms of service or privacy statement changed. */
    ZoomSDKReminderType_TermsService,
    /** Reminder type of smart summary. */
    ZoomSDKReminderType_SmartSummaryDisclaimer,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKReminderType_SmartSummaryEnableRequestReminder,
    /** Reminder type of query disclaimer. */
    ZoomSDKReminderType_QueryDisclaimer,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKReminderType_QueryEnableRequestReminder,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKReminderType_EnableSmartSummaryReminder,
    /** Reminder type of webinar promote attendee. */
    ZoomSDKReminderType_WebinarAttendeePromoteReminder,
    /** Reminder type of join meeting with private mode. */
    ZoomSDKReminderType_JoinMeetingPrivateModeReminder,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKReminderType_SmartRecordingEnableRequestReminder,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKReminderType_EnableSmartRecordingReminder,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    ZoomSDKReminderType_AICompanionPlusDisclaimer,
    /** Reminder type of ClosedCaption disclaimer. */
    ZoomSDKReminderType_ClosedCaptionDisclamier,
    /** Reminder type of disclaimers combination. */
    ZoomSDKReminderType_MultiDisclamier,
    /** Reminder type for join meeting connector with guest mode. */
    ZoomSDKReminderType_JoinMeetingConnectorAsGuestReminder,
    /** Reminder type of common disclaimer. */
    ZoomSDKReminderType_CommonDisclaimer,
    /** Reminder type of custom AI Companion disclaimer. */
    ZoomSDKReminderType_CustomAICompanionDisclamier,
    /** Reminder type of AI Companion restrict notify disclaimer. */
    ZoomSDKReminderType_AICRestrictNotifyDisclamier,
}ZoomSDKReminderType;

/**
 * @brief Enumeration of the pre-assign breakout room data download status.
 */
typedef enum
{
    /** Initial status, no request was sent. */
    ZoomSDKPreAssignBODataStatus_None,
    /** Download in progress. */
    ZoomSDKPreAssignBODataStatus_Downloading,
    /** Download success. */
    ZoomSDKPreAssignBODataStatus_Download_Ok,
    /** Download fail. */
    ZoomSDKPreAssignBODataStatus_Download_Fail
}ZoomSDKPreAssignBODataStatus;

/**
 * @brief Enumeration of auto framing modes in video.
 */
typedef enum
{
    /** No auto framing. */
    ZoomSDKAutoFramingMode_None,
    /** Use the video frame’s center point as the center to zoom in. */
    ZoomSDKAutoFramingMode_Center_Coordinates,
    /** Use the detected face in the video frame as the center to zoom in. */
    ZoomSDKAutoFramingMode_Face_Recognition
}ZoomSDKAutoFramingMode;


/**
 * @brief Enumeration of face recognition failure strategies.
 */
typedef enum
{
    /** No use of the fail strategy. */
    ZoomSDKFaceRecognitionFailStrategy_None,
    /** After face recognition fails, do nothing until face recognition succeeds again. */
    ZoomSDKFaceRecognitionFailStrategy_Remain,
    /** After face recognition fails, use the video frame’s center point as the center for zoom in. */
    ZoomSDKFaceRecognitionFailStrategy_Using_Center_Coordinates,
    /** After face recognition fails, use original video. */
    ZoomSDKFaceRecognitionFailStrategy_Using_Original_Video
}ZoomSDKFaceRecognitionFailStrategy;


/**
 * @brief Enumeration of emoji feedback types.
 */
typedef enum
{
    /** No emoji feedback. */
    ZoomSDKEmojiFeedbackType_None,
    /** Yes emoji feedback. */
    ZoomSDKEmojiFeedbackType_Yes,
    /** No emoji feedback. */
    ZoomSDKEmojiFeedbackType_No,
    /** Speed up emoji feedback. */
    ZoomSDKEmojiFeedbackType_SpeedUp,
    /** Slow down emoji feedback. */
    ZoomSDKEmojiFeedbackType_SlowDown,
    /** Away emoji feedback. */
    ZoomSDKEmojiFeedbackType_Away
}ZoomSDKEmojiFeedbackType;


/**
 * @brief Enumeration of video file share play errors.
 */
typedef enum
{
    /** No error. */
    ZoomSDKVideoFileSharePlayError_None,
    /** Video file format not supported. */
    ZoomSDKVideoFileSharePlayError_Not_Supported,
    /** Resolution is too high to play. */
    ZoomSDKVideoFileSharePlayError_Resolution_Too_High,
    /** Failed to open the video file. */
    ZoomSDKVideoFileSharePlayError_Open_Fail,
    /** Failed to play the video file. */
    ZoomSDKVideoFileSharePlayError_Play_Fail,
    /** Ailed to seek to the desired position in the video. */
    ZoomSDKVideoFileSharePlayError_Seek_Fail
}ZoomSDKVideoFileSharePlayError;


/**
 * @brief Enumeration of audio channel types.
 */
typedef enum
{
    /** Mono audio channel. */
    ZoomSDKAudioChannel_Mono,
    /** Stereo audio channel. */
    ZoomSDKAudioChannel_Stereo
}ZoomSDKAudioChannel;


/**
 * @brief Enumeration of the content font style type for chat message. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0064400>.
 */
typedef enum
{
    /** Chat message content font style normal. */
    ZoomSDKRichTextStyle_None,
    /** Chat message content font style bold. */
    ZoomSDKRichTextStyle_Bold,
    /** Chat message content font style italic. */
    ZoomSDKRichTextStyle_Italic,
    /** Chat message content font style strikethrough. */
    ZoomSDKRichTextStyle_Strikethrough,
    /** Chat message content font style bulletedlist. */
    ZoomSDKRichTextStyle_BulletedList,
    /** Chat message content font style numberedlist. */
    ZoomSDKRichTextStyle_NumberedList,
    /** The style is under line. */
    ZoomSDKRichTextStyle_Underline,
    /** The font size. */
    ZoomSDKRichTextStyle_FontSize,
    /** The font color. */
    ZoomSDKRichTextStyle_FontColor,
    /** Chat message content background color. */
    ZoomSDKRichTextStyle_BackgroundColor,
    /** Chat message content font style indent. */
    ZoomSDKRichTextStyle_Indent,
    /** Chat message content font style paragraph. */
    ZoomSDKRichTextStyle_Paragraph,
    /** Chat message content font style quote. */
    ZoomSDKRichTextStyle_Quote,
    /** Chat message content font style insert link. */
    ZoomSDKRichTextStyle_InsertLink
}ZoomSDKRichTextStyle;


/**
 * @brief Enumeration of the whiteboard share options.
 */
typedef enum
{
    /** Only the host can share a whiteboard. */
    ZoomSDKWhiteboardShareOption_HostShare,
    /** Anyone can share a whiteboard, but only one can share at a time, and only the host can take another's sharing role. */
    ZoomSDKWhiteboardShareOption_HostGrabShare,
    /** Anyone can share whiteboard, but one share at a time, and anyone can take another's sharing role. */
    ZoomSDKWhiteboardShareOption_AllGrabShare
}ZoomSDKWhiteboardShareOption;


/**
 * @brief Enumeration of the whiteboard create options.
 */
typedef enum
{
    /** Only the host can initiate a new whiteboard. */
    ZoomSDKWhiteboardCreateOption_HostOnly,
    /** Users under the same account as the meeting owner can initiate a new whiteboard. */
    ZoomSDKWhiteboardCreateOption_AccountUsers,
    /** All participants can initiate a new whiteboard. */
    ZoomSDKWhiteboardCreateOption_All
}ZoomSDKWhiteboardCreateOption;


/**
 * @brief Enumeration of the whiteboard status.
 */
typedef enum
{
    /** The user has started sharing their whiteboard. */
    ZoomSDKWhiteboardStatus_Started,
    /** The user has stopped sharing their whiteboard. */
    ZoomSDKWhiteboardStatus_Stopped
}ZoomSDKWhiteboardStatus;

/**
 * @brief Enumeration of the reminder action type.
 */
typedef enum
{
    /** No action required. */
    ZoomSDKReminderActionType_None,
    /** User needs to sign in. */
    ZoomSDKReminderActionType_NeedSignin,
    /** User needs to switch accounts. */
    ZoomSDKReminderActionType_NeedSwitchAccount
}ZoomSDKReminderActionType;


/**
 * @brief Enumeration of the focus mode share type. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0063004>.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKFocusModeShareType_None,
    /** Only the host can view share content in focus mode. */
    ZoomSDKFocusModeShareType_HostOnly,
    /** All participants can view share content in focus mode. */
    ZoomSDKFocusModeShareType_AllParticipants
}ZoomSDKFocusModeShareType;


/**
 * @brief Enumeration of the remote support type. For more information, please visit <https://support.zoom.com/hc/en/article?id=zm_kb&sysparm_article=KB0068720>.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKRemoteSupportType_None,
    /** Remote support for desktop, allowing control over the desktop. */
    ZoomSDKRemoteSupportType_Desktop,
    /** Remote support for application, allowing control over an application window. */
    ZoomSDKRemoteSupportType_Application
}ZoomSDKRemoteSupportType;


/**
 * @brief Enumeration of the remote support status.
 */
typedef enum
{
    /** Request was denied by the other user. */
    ZoomSDKRemoteSupportStatus_RequestDeny,
    /** Request was granted by the other user. */
    ZoomSDKRemoteSupportStatus_RequestGrant,
    /** The user stops the remote support. */
    ZoomSDKRemoteSupportStatus_StopBySupportedUser,
    /** The supporter stops the remote support. */
    ZoomSDKRemoteSupportStatus_StopBySupporter
}ZoomSDKRemoteSupportStatus;


/**
 * @brief Enumeration of the response status when a participant requests the host to start cloud recording.
 */
typedef enum
{
    /** The host approved the cloud recording request. */
    ZoomSDKRequestStartCloudRecordingStatus_Granted,
    /** The host denied the cloud recording request. */
    ZoomSDKRequestStartCloudRecordingStatus_Denied,
    /** The request timed out due to no response from the host. */
    ZoomSDKRequestStartCloudRecordingStatus_TimedOut
}ZoomSDKRequestStartCloudRecordingStatus;


/**
 * @brief Enumeration of options to enable a specific meeting feature.
 */
typedef enum
{
    /** Do not enable. */
    ZoomSDKMeetingFeatureEnableOption_None,
    /** Enable the feature for the current meeting only. */
    ZoomSDKMeetingFeatureEnableOption_Once,
    /** Enable the feature for the current and all future meetings. */
    ZoomSDKMeetingFeatureEnableOption_Always
}ZoomSDKMeetingFeatureEnableOption;


/**
 * @brief Enumeration of reasons why sharing is not allowed.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKCannotShareReasonType_None,
    /** Only the host is allowed to share. */
    ZoomSDKCannotShareReasonType_Locked,
    /** Sharing is disabled. */
    ZoomSDKCannotShareReasonType_Disabled,
    /** Another participant is currently sharing their screen. */
    ZoomSDKCannotShareReasonType_Other_Screen_Sharing,
    /** Another participant is currently sharing their whiteboard. */
    ZoomSDKCannotShareReasonType_Other_WB_Sharing,
    /** The user is sharing screen, and can grab. To grab, call 'EnableGrabShareWithoutReminder:true' before starting the share. */
    ZoomSDKCannotShareReasonType_Need_Grab_Myself_Screen_Sharing,
    /** Another is sharing screen, and can grab. To grab, call 'EnableGrabShareWithoutReminder:true' before starting the share. */
    ZoomSDKCannotShareReasonType_Need_Grab_Other_Screen_Sharing,
    /** Another is sharing pure computer audio, and can grab. To grab, call 'EnableGrabShareWithoutReminder:true' before starting the share. */
    ZoomSDKCannotShareReasonType_Need_Grab_Audio_Sharing,
    /** Other or myself is sharing cloud whiteboard, and can grab. To grab, call 'EnableGrabShareWithoutReminder:true' before starting the share. */
    ZoomSDKCannotShareReasonType_Need_Grap_WB_Sharing,
    /** The meeting has reached the maximum allowed screen share sessions. */
    ZoomSDKCannotShareReasonType_Reach_Maximum,
    /** Other share screen in main session. */
    ZoomSDKCannotShareReasonType_Have_Share_From_Mainsession,
    /** Another participant is sharing zoom docs. */
    ZoomSDKCannotShareReasonType_Other_Docs_Sharing,
    /** Other or myself is sharing docs, can grab. To grab, call 'EnableGrabShareWithoutReminder:true' before starting the share. */
    ZoomSDKCannotShareReasonType_Need_Grab_Docs_Sharing,
    /** UnKnown reason. */
    ZoomSDKCannotShareReasonType_UnKnown
}ZoomSDKCannotShareReasonType;


/**
 * @brief Enumeration of AI Companion features in a meeting.
 */
typedef enum
{
    /** Meeting summary with AI Companion generates summary assets. */
    ZoomSDKAICompanionFeature_SmartSummary,
    /** Meeting summary with AI Companion generates transcript assets. */
    ZoomSDKAICompanionFeature_Query,
    /** Meeting summary with AI Companion generates recording assets. */
    ZoomSDKAICompanionFeature_SmartRecording
}ZoomSDKAICompanionFeature;


/**
 * @brief Enumeration of camera control requests types.
 */
typedef enum
{
    /** Request to take control of the camera. */
    ZoomSDKCameraControlRequestType_RequestControl,
    /** Give up control of the camera. */
    ZoomSDKCameraControlRequestType_GiveUpControl
}ZoomSDKCameraControlRequestType;


/**
 * @brief Enumeration of the results of a camera control request.
 */
typedef enum
{
    /** The camera control request was approved. */
    ZoomSDKCameraControlRequestResult_Approve,
    /** The camera control request was declined. */
    ZoomSDKCameraControlRequestResult_Decline,
    /** The previously approved camera control was revoked. */
    ZoomSDKCameraControlRequestResult_Revoke
}ZoomSDKCameraControlRequestResult;


/**
 * @brief Enumeration of the status of a file transfer.
 */
typedef enum
{
    /** The file transfer has no state. */
    ZoomSDKFileTransferStatus_None = 0,
    /** The file transfer is ready to start. */
    ZoomSDKFileTransferStatus_ReadyToTransfer,
    /** The file transfer is currently in progress. */
    ZoomSDKFileTransferStatus_Transfering,
    /** The file transfer has failed. */
    ZoomSDKFileTransferStatus_TransferFailed,
    /** The file transfer completed successfully. */
    ZoomSDKFileTransferStatus_TransferDone
}ZoomSDKFileTransferStatus;


/**
 * @brief Enumeration representing the sampling rate of acquired raw audio data.
 */
typedef enum
{
    /** The sampling rate of the acquired raw audio data is 32K. */
    ZoomSDKAudioRawdataSamplingRate_32K,
    /** The sampling rate of the acquired raw audio data is 48K. */
    ZoomSDKAudioRawdataSamplingRate_48K
}ZoomSDKAudioRawdataSamplingRate;


/**
 * @brief Enumeration of wallpaper layout modes.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKWallpaperLayoutMode_None,
    /** The wallpaper is scaled to completely fill the display area. */
    ZoomSDKWallpaperLayoutMode_Fill,
    /** The wallpaper is scaled to fit entirely within the display area. */
    ZoomSDKWallpaperLayoutMode_Fit
}ZoomSDKWallpaperLayoutMode;


/**
 * @brief Enumeration of the wallpaper setting status.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKWallpaperSettingStatus_None,
    /** Wallpaper is currently being downloaded. */
    ZoomSDKWallpaperSettingStatus_Downloading,
    /** Wallpaper was downloaded successfully. */
    ZoomSDKWallpaperSettingStatus_Downloaded,
    /** Wallpaper download failed. */
    ZoomSDKWallpaperSettingStatus_DownloadFail
}ZoomSDKWallpaperSettingStatus;


/**
 * @brief Enumeration of video preference modes.
 */
typedef enum
{
    /** Balance (Default preference with no additional parameters needed): Zoom will do what is best under the current bandwidth situation and make adjustments as needed. */
    ZoomSDKVideoPreferenceMode_Balance,
    /** Sharpness: Prioritizes a smooth video frame transition by preserving the frame rate as much as possible. */
    ZoomSDKVideoPreferenceMode_Sharpness,
    /** Smoothness: Prioritizes a sharp video image by preserving the resolution as much as possible. */
    ZoomSDKVideoPreferenceMode_Smoothness,
    /** Custom: Allows customization by providing the minimum and maximum frame rate. Use this mode if you have an understanding of your network behavior and a clear idea on how to adjust the frame rate to achieve the desired video quality. */
    ZoomSDKVideoPreferenceMode_Custom
}ZoomSDKVideoPreferenceMode;


/**
 * @brief Enumeration of meeting transfer modes.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKTransferMeetingMode_None,
    /** Try to transfer meeting to current device. */
    ZoomSDKTransferMeetingMode_Transfer,
    /** Try to join meeting with companion mode.If the meeting is successfully joined, both video and audio will be unavailable. */
    ZoomSDKTransferMeetingMode_Companion
}ZoomSDKTransferMeetingMode;


/**
 * @brief Enumerate of the various components in the settings UI.
 */
typedef enum{
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    SettingComponent_AdvancedFeatureButton,
    /** The advanced settings tab. */
    SettingComponent_AdvancedFeatureTab,
    /** The general settings tab. */
    SettingComponent_GeneralFeatureTab,
    /** @deprecated Use SettingComponent_VideoAndVirtualBackgroundFeatureTab instead. */
    SettingComponent_VideoFeatureTab,
    /** The audio settings tab. */
    SettingComponent_AudioFeatureTab,
    /** @deprecated Use SettingComponent_VideoAndVirtualBackgroundFeatureTab instead. */
    SettingComponent_VirtualBackgroundFeatureTab,
    /** The recording settings tab. */
    SettingComponent_RecordingFeatureTab,
    /** The statistics settings tab. */
    SettingComponent_StatisticsFeatureTab,
    /** @deprecated This enum value is deprecated and will be removed in a future release. */
    SettingComponent_FeedbackFeatureTab,
    /** The accessibility settings tab. */
    SettingComponent_AccessibilityFeatureTab,
    /** The screen sharing settings tab. */
    SettingComponent_ScreenShareFeatureTab,
    /** The shortcut settings tab. */
    SettingComponent_ShortCutFeatureTab,
    /** The video & virtual background settings tab. */
    SettingComponent_VideoAndVirtualBackgroundFeatureTab
}SettingComponent;


/**
 * @brief Enumeration of document sharing status in a meeting.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKDocsStatus_None,
    /** User starts sharing docs. */
    ZoomSDKDocsStatus_Start,
    /** User stops sharing docs. */
    ZoomSDKDocsStatus_Stop
}ZoomSDKDocsStatus;


/**
 * @brief Enumeration of document sharing permission options.
 */
typedef enum
{
    /** Invalid option, such as the meeting not supporting document sharing. */
    ZoomSDKDocsShareOption_None,
    /** Only Only the host or co-host can share documents. */
    ZoomSDKDocsShareOption_HostCoHostShare,
    /** Anyone can share docs, but only one doc can be shared at a time, and the host or co-host can take over another'sharing. */
    ZoomSDKDocsShareOption_HostCoHostGrabShare,
    /** Anyone can share docs, but only one doc can be shared at a time, and any participant can take over another'sharing. */
    ZoomSDKDocsShareOption_AllGrabShare
}ZoomSDKDocsShareOption;


/**
 * @brief Enumeration of document creation permission options.
 */
typedef enum
{
    /** Invalid option, possibly because the meeting does not support docs. */
    ZoomSDKDocsCreateOption_None,
    /** Only the host can initiate new docs. */
    ZoomSDKDocsCreateOption_HostOnly,
    /** Users under the same account can initiate new docs. */
    ZoomSDKDocsCreateOption_AccountUsers,
    /** All participants can initiate new docs. */
    ZoomSDKDocsCreateOption_All
}ZoomSDKDocsCreateOption;

/**
 * @brief Enumeration of asset type.
 */
typedef enum
{
    /** For initialization. */
    ZoomSDKGrantCoOwnerAssetsType_None,
    /** Summary. */
    ZoomSDKGrantCoOwnerAssetsType_Summary,
    /** CloudRecording. */
    ZoomSDKGrantCoOwnerAssetsType_CloudRecording
}ZoomSDKGrantCoOwnerAssetsType;

/**
 * @brief The colorspace of video rawdata.
 */
typedef enum
{
    /** For standard definition TV (SDTV)  Y[16,235], Cb/Cr[16,240]. */
    ZoomSDKVideoRawdataColorspace_BT601_L,
    /** For standard definition TV (SDTV) full range version: [0,255]. */
    ZoomSDKVideoRawdataColorspace_BT601_F,
    /** For high definition TV (HDTV) Y[16,235], Cb/Cr[16,240] */
    ZoomSDKVideoRawdataColorspace_BT709_L,
    /** For high definition TV (HDTV) full range version: [0,255] */
    ZoomSDKVideoRawdataColorspace_BT709_F
}ZoomSDKVideoRawdataColorspace;

/**
 * @brief Enumeration of custom 3D avatar element image types.
 */
typedef enum
{
    /** None. */
    ZoomSDKCustom3DAvatarElementImageType_None = 0,
    /** Skin. */
    ZoomSDKCustom3DAvatarElementImageType_Skin,
    /** Face. */
    ZoomSDKCustom3DAvatarElementImageType_Face,
    /** Hair. */
    ZoomSDKCustom3DAvatarElementImageType_Hair,
    /** Eyes. */
    ZoomSDKCustom3DAvatarElementImageType_Eyes,
    /** Eye color. */
    ZoomSDKCustom3DAvatarElementImageType_EyeColor,
    /** Eyelashes. */
    ZoomSDKCustom3DAvatarElementImageType_Eyelashes,
    /** Eyebrows. */
    ZoomSDKCustom3DAvatarElementImageType_Eyebrows,
    /** Nose. */
    ZoomSDKCustom3DAvatarElementImageType_Nose,
    /** Mouth. */
    ZoomSDKCustom3DAvatarElementImageType_Mouth,
    /** Lip color. */
    ZoomSDKCustom3DAvatarElementImageType_LipColor,
    /** Age. */
    ZoomSDKCustom3DAvatarElementImageType_Age,
    /** Facial hair. */
    ZoomSDKCustom3DAvatarElementImageType_FacialHair,
    /** Body. */
    ZoomSDKCustom3DAvatarElementImageType_Body,
    /** Clothing. */
    ZoomSDKCustom3DAvatarElementImageType_Clothing,
    /** Head covering. */
    ZoomSDKCustom3DAvatarElementImageType_HeadCovering,
    /** Glasses. */
    ZoomSDKCustom3DAvatarElementImageType_Glasses,
}ZoomSDKCustom3DAvatarElementImageType;

/**
 * @brief Enumeration of custom 3D avatar element color types.
 */
typedef enum
{
    /** None. */
    ZoomSDKCustom3DAvatarElementColorType_None = 0,
    /** Eyebrow. */
    ZoomSDKCustom3DAvatarElementColorType_Eyebrow,
    /** Mustache. */
    ZoomSDKCustom3DAvatarElementColorType_Mustache,
    /** Hair. */
    ZoomSDKCustom3DAvatarElementColorType_Hair,
    /** Eyelash. */
    ZoomSDKCustom3DAvatarElementColorType_Eyelash,
}ZoomSDKCustom3DAvatarElementColorType;
