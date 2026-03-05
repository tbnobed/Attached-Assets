/**
 * @file ZoomSDKMeetingActionController.h
 * @brief Interfaces for managing meeting actions  in Zoom meetings.
 */


#import <ZoomSDK/ZoomSDKErrors.h>
#import <ZoomSDK/ZoomSDKMeetingChatController.h>
NS_ASSUME_NONNULL_BEGIN

/**
 * @class ZoomSDKGrantCoOwnerAssetsInfo
 * @brief This class provides properties to specify and manage the privileges associated with different types of assets when assigning roles such as co-host or host in a meeting.
 * @note \link ZoomSDKGrantCoOwnerAssetsInfo \endlink objects must be obtained through the \link getGrantCoOwnerAssetsInfo \endlink interface provided by the SDK. Manual creation is not allowed.
 */
@interface ZoomSDKGrantCoOwnerAssetsInfo : NSObject
/**
 * @brief Indicates whether the specified privilege is granted for managing the asset. This is a writable property that can be set to YES or NO.
 */
@property (assign, nonatomic) BOOL isGranted;
/**
 * @brief Specifies the type of asset (such as smart summary, cloud recording) for which the privilege applies. This is a read-only property.
 */
@property (assign,nonatomic, readonly) ZoomSDKGrantCoOwnerAssetsType assetType;
/**
 * @brief Indicates whether the asset is locked, preventing any modifications. This is a read-only property.
 */
@property (assign,nonatomic, readonly) BOOL isAssetsLocked;
@end

/**
 * @class ZoomSDKMultiToSingleShareConfirmHandler
 * @brief Handles confirmation for switching from multi-share to single-share mode during screen sharing.
 */
@interface ZoomSDKMultiToSingleShareConfirmHandler : NSObject
/**
 * @brief Cancels to switch to single share from multi-share. All the shares are remained.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)cancelSwitch;
/**
 * @brief Confirms to switch to single share from multi-share. All the shares are stopped.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)confirmSwitch;
@end

/**
 * @class ZoomSDKChatInfo
 * @brief Represents a chat message in the meeting.
 */
@interface ZoomSDKChatInfo : NSObject
{
    unsigned int                      _sendID;
    unsigned int                      _receiverID;
    NSString*                         _sendName;
    NSString*                         _receiverName;
    NSString*                         _content;
    time_t                            _timestamp;
    BOOL                              _isChatToWaitingRoom;
    ZoomSDKChatMessageType            _chatMessageType;
}
/**
 * @brief Gets the user ID of whom sending message.
 * @return If the function succeeds, it returns the user ID of sender. Otherwise, returns -1.
 */
- (unsigned int)getSenderUserID;
/**
 * @brief Gets the sender's screen name.
 * @return If the function succeeds, it returns the screen name. Otherwise, this function fails and returns nil.
 */
- (NSString*)getSenderDisplayName;
/**
 * @brief Gets the user ID of whom receiving the message.
 * @return If the function succeeds, it returns the user ID. Otherwise, returns -1.
 */
- (unsigned int)getReceiverUserID;
/**
 * @brief Gets the screen name of receiver.
 * @return If the function succeeds, it returns the screen name. Otherwise, this function fails and returns nil.
 */
- (NSString*)getReceiverDisplayName;
/**
 * @brief Gets the content of message.
 * @return If the function succeeds, it returns the content of message. Otherwise, this function fails and returns nil.
 */
- (NSString*)getMsgContent;
/**
 * @brief Gets the current's timestamps message.
 * @return The current's timestamps message. 
 */
- (time_t)getTimeStamp;
/**
 * @brief The current message is send to waiting room.
 * @return YES if the message is send to waiting room. Otherwise, NO.
 */
- (BOOL)isChatToWaitingRoom;
/**
 * @brief Gets the current message's type.
 * @return The ZoomSDKChatMessageType.
 */
- (ZoomSDKChatMessageType)getChatMessageType;
/**
 * @brief Gets chat message ID.
 * @return If the function succeeds, it returns the ID of chat message. Otherwise, this function fails and returns nil.
 */
- (NSString*)getMessageID;

/**
 * @brief Determines if the current message is a reply to another message.
 * @return YES if the current message is a reply to another message. Otherwise, NO.
 */
- (BOOL)isComment;

/**
 * @brief Determines if the current message is part of a message thread, and can be directly replied to.
 * @return YES if the current message is a part of a message thread. Otherwise, NO.
 */
- (BOOL)isThread;

/**
 * @brief Gets the current message’s chat message font style list.
 * @deprecated This method is no longer used.
 */
- (NSArray<ZoomSDKRichTextStyleItem *> *_Nullable)getTextStyleItemList DEPRECATED_MSG_ATTRIBUTE("No longer used");

/**
 * @brief Gets the list of segment details in the current message.
 * @return If the function succeeds, it returns an array of ZoomSDKChatMsgSegmentDetails objects representing rich text segments in the message. Otherwise, this function fails and returns nil.
 */
- (NSArray<ZoomSDKChatMsgSegmentDetails *> *_Nullable)getSegmentDetails;

/**
 * @brief Gets the current message’s thread ID.
 * @return If the function succeeds, it returns the current message’s thread ID. Otherwise, this function fails and it returns the string of length zero(0).
 */
- (NSString*)getThreadID;
@end

/**
 * @class ZoomSDKNormalMeetingChatPrivilege
 * @brief Represents the chat privileges of a participant in a normal meeting.
 */
@interface ZoomSDKNormalMeetingChatPrivilege : NSObject
/**
 * YES indicates that the user owns the authority to send message to chat.
 */
@property (assign,nonatomic,readonly) BOOL canChat;
/**
 * YES indicates that the user owns the authority to send message to all.
 */
@property (assign,nonatomic,readonly) BOOL canChatToAll;
/**
 * YES indicates that the user owns the authority to send message to an individual attendee in the meeting.
 */
@property (assign,nonatomic,readonly) BOOL canChatToIndividual;
/**
 * YES indicates that the user owns the authority to send message only to the host.
 */
@property (assign,nonatomic,readonly) BOOL isOnlyCanChatToHost;
@end

/**
 * @class ZoomSDKWebinarAttendeeChatPrivilege
 * @brief Represents the chat privileges of a webinar attendee in a meeting.
 */
@interface ZoomSDKWebinarAttendeeChatPrivilege : NSObject
/**
 * YES indicates that the attendee can send message to chat.
 */
@property (assign,nonatomic,readonly) BOOL canChat;
/**
 * YES indicates that the user owns the authority to send message to all the panelists and attendees.
 */
@property (assign,nonatomic,readonly) BOOL canChatToAllPanellistAndAttendee;
/**
 * YES indicates that the user owns the authority to send message to all the panelists.
 */
@property (assign,nonatomic,readonly) BOOL canChatToAllPanellist;
@end

/**
 * @class ZoomSDKWebinarPanelistChatPrivilege
 * @brief Represents the chat privileges of a webinar panelist in a meeting.
 */
@interface ZoomSDKWebinarPanelistChatPrivilege : NSObject
/**
 * YES indicates that the user owns the authority to send message to all the panelists.
 */
@property (assign,nonatomic,readonly) BOOL canChatToAllPanellist;
/**
 * YES indicates that the user owns the authority to send message to all.
 */
@property (assign,nonatomic,readonly) BOOL canChatToAllPanellistAndAttendee;
/**
 * YES indicates that the user owns the authority to send message to individual attendee.
 */
@property (assign,nonatomic,readonly) BOOL canChatToIndividual;
@end

/**
 * @class ZoomSDKChatStatus
 * @brief Provides access to the chat privileges for different user roles in a meeting or webinar.
 */
@interface ZoomSDKChatStatus : NSObject
/**
 * @brief Gets the meeting participant's chat privilege.
 * @return The meeting participant's chat privilege. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKNormalMeetingChatPrivilege *)getNormalMeetingPrivilege;
/**
 * @brief Gets the webinar attendee's chat privilege.
 * @return The webinar attendee's chat privilege. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKWebinarAttendeeChatPrivilege *)getWebinarAttendeePrivilege;
/**
 * @brief Gets the webinar panelist's chat privilege.
 * @return The webinar panelist's chat privilege. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKWebinarPanelistChatPrivilege *)getWebinarPanelistPrivilege;
/**
 * @brief Current meeting support chat.
 * @return YES if current meeting support chat feature. Otherwise, NO.
 */
- (BOOL)isSupportChat;
/**
 * @brief Current role is a webinar attendee.
 * @return YES if current role is a webinar attendee. Otherwise, NO.
 */
- (BOOL)isWebinarAttendee;
/**
 * @brief Current meeting is a webinar meeting.
 * @return YES if current meeting is a webinar meeting. Otherwise, NO.
 */
- (BOOL)isWebinarMeeting;
@end

/**
 * @class ZoomSDKUserAudioStatus
 * @brief Represents a user's audio connection status and audio type in the meeting.
 */
@interface ZoomSDKUserAudioStatus : NSObject
{
    unsigned int _userID;
    ZoomSDKAudioStatus _status;
    ZoomSDKAudioType _type;
}
/**
 * @brief Gets the user ID.
 * @return If the function succeeds, it returns user ID. Otherwise, returns 0.
 */
- (unsigned int)getUserID;
/**
 * @brief Gets the user's audio status.
 * @return The audio status.
 */
- (ZoomSDKAudioStatus)getStatus;
/**
 * @brief Gets the user's audio type.
 * @return The audio type.
 */
- (ZoomSDKAudioType)getType;
@end


/**
 * @class ZoomSDKWebinarAttendeeStatus
 * @brief Represents the webinar attendee status in a meeting.
 */
@interface ZoomSDKWebinarAttendeeStatus : NSObject
{
    BOOL _isAttendeeCanTalk;
}
/**
 * @brief Indicates whether the webinar attendee is allowed to talk.
 */
@property(nonatomic, assign)BOOL  isAttendeeCanTalk;
@end


/**
 * @class ZoomSDKVirtualNameTag
 * @brief Represents a virtual name tag.
 */
@interface ZoomSDKVirtualNameTag : NSObject
/**
 * @brief Tag ID. tagID is the unique identifier. The range of tagID is 0-1024.
 */
@property (nonatomic, assign) int tagID;

/**
 * @brief Tag name.
 */
@property (nonatomic, copy, nullable) NSString *tagName;
@end


/**
 * @class ZoomSDKUserInfo
 * @brief Provides detailed information about a user in a  meeting.
 */
@interface ZoomSDKUserInfo :NSObject
{
    unsigned int _userID;
}
/**
 * @brief Determines if the information corresponds to the current user.
 * @return YES if the information corresponds to the current user. Otherwise, NO.
 */
- (BOOL)isMySelf;
/**
 * @brief Gets the username matched with the current user information.
 * @return If the function succeeds, it returns the username. Otherwise, this function fails and returns nil.
 */
- (NSString*_Nullable)getUserName;
/**
 * @brief Gets the user ID matched with the current user information.
 * @return The user ID.
 */
- (unsigned int)getUserID;

/**
 * @brief Gets the storage path of avatar.
 * @return If the function succeeds, it returns the path to store the head portrait. Otherwise, this function fails and returns nil.
 */
- (NSString*_Nullable)getAvatarPath;

/**
 * @brief Determines whether the member corresponding with the current information is the host or not.
 * @return YES if host. Otherwise, NO.
 */
- (BOOL)isHost;
/**
 * @brief Determines the user's video status specified by the current information.
 * @return YES if the video is turned on. Otherwise, NO.
 */
- (BOOL)isVideoOn;

/**
 * @brief Gets the audio status of user.
 * @return The audio status of user.
 */
- (ZoomSDKAudioStatus)getAudioStatus;
/**
 * @brief Gets the audio type of user.
 * @return The audio type of user.
 */
- (ZoomSDKAudioType)getAudioType;
/**
 * @brief Gets the user's type of role specified by the current information.
 * @return The user's role.
 */
- (UserRole)getUserRole;
/**
 * @brief Determines whether the user corresponding to the current information joins the meeting by telephone or not.
 * @return YES if the user joins the meeting by telephone. Otherwise, NO.
 */
- (BOOL)isPurePhoneUser;
/**
 * @brief Determines whether the user corresponding to the current information joins the meeting by h323 or not.
 * @return YES if the user joins the meeting by h323. Otherwise, NO.
 */
- (BOOL)isH323User;
/**
 * @brief Determines if it is able to change the specified user role as the co-host.
 * @return If the specified user can be the co-host, it returns YES. Otherwise, NO.
 */
- (BOOL)canBeCoHost;

/**
 * @brief Query if the user can be assigned as co-owner in meeting. Co-owner can be grant privilege to manage some assets after the meeting.
 * @return YES if that the user can be assigned as co-owner. Otherwise, NO.
 */
- (BOOL)canBeCoOwner;

/**
 * @brief Gets the user's webinar status specified by the current information.
 * @return If the function succeeds, it returns the object of ZoomSDKWebinarAttendeeStatus. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKWebinarAttendeeStatus*_Nullable)GetWebinarAttendeeStatus;

/**
 * @brief Gets the user is talking.
 * @return YES if the user is talking. Otherwise, NO.
 */
- (BOOL)isTalking;

/**
 * @brief Gets the customer Key matched with the current user information. The max length of customer Key is 35.
 * @return If the function succeeds, it returns the user customer Key. Otherwise, this function fails and returns nil.
 */
- (NSString *_Nullable)getCustomerKey;

/**
 * @brief Determines if user is interpreter.
 * @return YES if the user is interpreter. Otherwise, NO.
 */
- (BOOL)isInterpreter;

/**
 * @brief Determines whether the user specified by the current information is a sign language interpreter or not.
 * @return YES if the user is sign interpreter. Otherwise, NO.
 */
- (BOOL)isSignLanguageInterpreter;

/**
 * @brief Gets interpreter active language.
 * @return If the function succeeds, it returns the language ID. Otherwise, this function fails and returns nil.
 */
- (NSString *_Nullable)getInterpreterActiveLanguage;

/**
 * @brief Gets the raising hand status.
 * @return YES if the user is raising hand. Otherwise, NO.
 */
- (BOOL)isRaisingHand;

/**
 * @brief Gets the local recording status.
 * @return The local's status recording status.
 */
- (ZoomSDKRecordingStatus)getLocalRecordingStatus;

/**
 * @brief Gets the user persistent ID matched with the current user information. This ID persists for the main meeting's duration. Once the main meeting ends, the ID is discarded.
 * @return If the function succeeds, it returns the user persistent ID. Otherwise, this function fails and returns nil.
 */
- (NSString *_Nullable)getPersistentId;

/**
 * @brief Determines whether the user has started a raw live stream.
 * @return YES if the specified user has started a raw live stream. Otherwise, NO.
 */
- (BOOL)isRawLiveStreaming;

/**
 * @brief Determines whether the user has raw live stream privilege.
 * @return YES if the specified user has raw live stream privilege. Otherwise, NO.
 */
- (BOOL)hasRawLiveStreamPrivilege;

/**
 * @brief Gets the user's emoji feedback type.
 * @return The emoji feedback type.
 */
- (ZoomSDKEmojiFeedbackType)getEmojiFeedbackType;

/**
 * @brief Query if the participant has a camera.
 * @return YES if the user has a camera. Otherwise, NO.
 */
- (BOOL)hasCamera;

/**
 * @brief Query if the participant is in waiting room.
 * @return YES if the user in waiting room. Otherwise, NO.
 */
- (BOOL)isInWaitingRoom;

/**
 * @brief Determines whether the user specified by the current information is in the webinar backstage or not.
 * @return YES if the specified user is in the webinar backstage. Otherwise, NO.
 */
- (BOOL)isInWebinarBackstage;

/**
 * @brief Query if the participant is closedCaption sender.
 * @return YES if the user is closedCaption sender. Otherwise, NO.
 */
- (BOOL)isClosedCaptionSender;

/**
 * @brief Returns whether the user is production studio user.
 * @return YES if the user is production studio user. Otherwise, NO.
 */
- (BOOL)isProductionStudioUser;

/**
 * @brief Returns the parent user ID of this production user.
 * @return The parent user's ID.
 */
- (unsigned int)getProductionStudioParent;

/**
 * @brief Determines whether the user specified by the current information is bot user or not.
 * @return YES if the specified user is bot user. Otherwise, NO.
 */
- (BOOL)isBotUser;

/**
 * @brief Gets the bot app name.
 * @return If the function succeeds, it returns the bot app name. Otherwise, this function fails and returns nil.
 */
- (NSString * _Nullable)getBotAppName;

/**
 * @brief Query if the participant enabled virtual name tag.
 * @return YES if enabled. Otherwise, NO.
 */
- (BOOL)isVirtualNameTagEnabled;

/**
 * @brief Query the virtual name tag roster infomation.
 * @return If the function succeeds, it returns the list of user's virtual name tag roster info. Otherwise, this function fails and returns nil.
 */
- (nullable NSArray<ZoomSDKVirtualNameTag*> *)getVirtualNameTagArray;

/**
 * @brief Determines whether the user specified by the current information in companion mode or not.
 * @return YES if the specified user in companion mode. Otherwise, NO.
 */
- (BOOL)isCompanionModeUser;

/**
 * @brief Query the granted assets info when assign a co-owner.
 * @return If the function succeeds, it returns the list of user's grant assets info. Otherwise, this function fails and returns nil.
 * @note If not granted any assets privilege, the web's default configuration will be queried. If has granted assets privilege, the result after granting will be queried.
 */
- (NSArray <ZoomSDKGrantCoOwnerAssetsInfo *>* _Nullable)getGrantCoOwnerAssetsInfo;

/**
 * @brief Determines whether the specified user is an audio only user.
 * @return YES if the specified user is an audio only user. Otherwise, NO.
 */
- (BOOL)isAudioOnlyUser;
@end



/**
 * @class ZoomSDKJoinMeetingHelper
 * @brief Helper interface for handling the process of joining a meeting, such as inputting password or display name.
 */
@interface ZoomSDKJoinMeetingHelper :NSObject
{
    JoinMeetingReqInfoType   _reqInfoType;
}
/**
 * @brief Gets the type of registration information required to join the meeting.
 * @return The registration information's type.
 */
- (JoinMeetingReqInfoType)getReqInfoType;
/**
 * @brief Input the password to join meeting.
 * @param password The meeting's password.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)inputPassword:(NSString*)password;

/**
 * @brief Input the screen name to join meeting.
 * @param screenName The meeting's username.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)inputMeetingScreenName:(NSString*)screenName;

/**
 * @brief Cancels to join meeting.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)cancel;
@end


/**
 * @protocol ZoomSDKMeetingActionControllerDelegate
 * @brief Protocol for receiving meeting events such as user join, leave, and audio status changes.
 */
@protocol ZoomSDKMeetingActionControllerDelegate <NSObject>

/**
 * @brief Notification of audio status of the user changes. 
 * @param userAudioStatusArray An array contains ZoomSDKUserAudioStatus elements of each audio status of the user.
 */
- (void)onUserAudioStatusChange:(NSArray*)userAudioStatusArray;

/**
 * @brief Notification of user joins meeting.
 * @param array Array of users who join meeting. 
 *
 */
- (void)onUserJoin:(NSArray*)array;

/**
 * @brief Notification of user leaves meeting.
 * @param array Array of users leave meeting.
 */
- (void)onUserLeft:(NSArray*)array;

/**
 * @brief Upgrade the information of specified  user.
 * @param userID The specified' user's ID.
 * @deprecated This method is no longer used.
 */
- (void)onUserInfoUpdate:(unsigned int)userID DEPRECATED_MSG_ATTRIBUTE("No longer used");

/**
 * @brief Notification of virtual name tag status change.
 * @param bOn YES if virtual name tag is turned on, NO otherwise.
 * @param userID The ID of user who virtual name tag status changed.
 */
- (void)onVirtualNameTagStatusChanged:(BOOL)bOn userID:(unsigned int)userID;

/**
 * @brief Notification of virtual name tag roster info update.
 * @param userID The ID of user who virtual name tag status changed.
 */
- (void)onVirtualNameTagRosterInfoUpdated:(unsigned int)userID;

/**
 * @brief Notification of host changes.
 * @param userID User ID of new host.
 *
 */
- (void)onHostChange:(unsigned int)userID;

/**
 * @brief Notification of co-host changes.
 * @param userID User ID with coHost privilege changes.
 * @param isCoHost YES indicates that the specified user is co-host, NO otherwise.
 */
- (void)onMeetingCoHostChanged:(unsigned int)userID isCoHost:(BOOL)isCoHost;

/**
 * @brief Callback event for when the video spotlight user list changes.Spotlight user means that the view shows only the specified user and won't change the view even other users speak.
 * @param spotlightedUserList Spotlight user list.
 */
- (void)onSpotlightVideoUserChange:(NSArray*_Nullable)spotlightedUserList;

/**
 * @brief Notify that video status of the user changes.
 * @param videoStatus The video's status.
 * @param userID The ID of user who video status changes.
 *
 */
- (void)onVideoStatusChange:(ZoomSDKVideoStatus)videoStatus UserID:(unsigned int)userID;
/**
 * @brief Notification of hand status of the user changes.
 * @param raise YES if the specified user raises hand, NO if puts hand down.
 * @param userID The ID of user whose hand status changes.
 */
- (void)onLowOrRaiseHandStatusChange:(BOOL)raise UserID:(unsigned int)userID;

/**
 * @brief Callback event of user joins meeting. 
 * @param joinMeetingHelper An object for inputing password or canceling to join meeting.
 */
- (void)onJoinMeetingResponse:(ZoomSDKJoinMeetingHelper*_Nullable)joinMeetingHelper;

/**
 * @brief Notify user to confirm or cancel to switch to single share from multi-share.
 * @param confirmHandle An object that handles the action to switch to single share from multi-share.
 */
- (void)onMultiToSingleShareNeedConfirm:(ZoomSDKMultiToSingleShareConfirmHandler*_Nullable)confirmHandle;
/**
 * @brief Notify that video of active user changes.
 * @param userID The user ID.						  
 */
- (void)onActiveVideoUserChanged:(unsigned int)userID;
/**
 * @brief Notify that video of active speaker changes.
 * @param userID The user ID.
 */
- (void)onActiveSpeakerVideoUserChanged:(unsigned int)userID;
/**
 * @brief Notify that host ask you to unmute yourself.
 */
- (void)onHostAskUnmute;
/**
 * @brief Notify that host ask you to start video.
 */
- (void)onHostAskStartVideo;
/**
 * @brief Notification of in-meeting active speakers.
 * @param useridArray The active's array contain userid speakers.
 */
- (void)onUserActiveAudioChange:(NSArray *)useridArray;

/**
 * @brief Notification of user name changed.
 * @param userList The list of user whose user name have changed.
 * @note The old interface - (void)onUserNameChanged:(unsigned int)userid userName:(NSString *)userName; will be marked as deprecated, and all platforms will be using this new callbacks. This is because in a webinar, when the host renamed an attendee, only the attendee could receive the old callback, and the host, cohost, or panelist is not able to receive it, which leads to the developer not being able to update the UI.
 */
- (void)onUserNamesChanged:(NSArray<NSNumber*>*)userList;

/**
 * @brief Notification of reclaim host key is invalid.
 */
- (void)onInvalidReclaimHostKey;

/**
 * @brief Notification the video order updated.
 * @param orderList The order list contains the user ID of listed users.
 */
- (void)onHostVideoOrderUpdated:(NSArray*)orderList;

/**
 * @brief Notification the local video order updated.
 * @param localOrderList The lcoal vidoe order list contains the user ID of listed users.
 */
- (void)onLocalVideoOrderUpdated:(NSArray*)localOrderList;

/**
 * @brief Notification the status of following host's video order changed.
 * @param follow Yes means the option of following host's video order is on, NO otherwise.
 */
- (void)onFollowHostVideoOrderChanged:(BOOL)follow;

/**
 * @brief When the host calls the lower all hands interface, the host, cohost, or panelist receives this callback.
 */
- (void)onAllHandsLowered;

/**
 * @brief Notify that user's video quality changes.
 * @param quality The Video's quality.
 * @param userID The ID of user whose video quality changed.
 */
- (void)onUserVideoQualityChanged:(ZoomSDKVideoQuality)quality userID:(unsigned int)userID;

/**
 * @brief Notify that the chat message deleted.
 * @param msgID The ID of deleted chat message.
 * @param deleteBy Indicates the message was deleted by whom.
 */
- (void)onChatMsgDeleteNotification:(NSString*)msgID messageDeleteType:(ZoomSDKChatMessageDeleteType)deleteBy;

/**
 * @brief Callback to notify that the privilege of participants or webinar chat has changed.
 * @param chatStatus The privilege's info.
 */
- (void)onChatStatusChangedNotification:(ZoomSDKChatStatus *)chatStatus;

/**
 * @brief Notify that the share meeting chat status.
 * @param isStart YES if share meeting chat is started, NO otherwise.
 */
- (void)onShareMeetingChatStatusChanged:(BOOL)isStart;

/**
 * @brief Callback event when a meeting is suspended.
 */
- (void)onSuspendParticipantsActivities;

/**
 * @brief Callback event that lets participants start a video.
 * @param allow YES allow. If NO, disallow.
 */
- (void)onAllowParticipantsStartVideoNotification:(BOOL)allow;

/**
 * @brief Callback event that lets participants rename themself.
 * @param allow YES allow. If NO, participants may not rename themselves.
 */
- (void)onAllowParticipantsRenameNotification:(BOOL)allow;

/**
 * @brief Callback event that lets participants unmute themself.
 * @param allow YES allow. If NO, participants may not unmute themselves.
 */
- (void)onAllowParticipantsUnmuteSelfNotification:(BOOL)allow;

/**
 * @brief Callback event that lets participants share a new whiteboard.
 * @param allow YES allow. If NO, participants may not share new whiteboard.
 */
- (void)onAllowParticipantsShareWhiteBoardNotification:(BOOL)allow;

/**
 * @brief Callback event that allows a meeting lock status change.
 * @param isLock If YES, the status is locked. If NO, the status is unlocked.
 */
- (void)onMeetingLockStatus:(BOOL)isLock;

/**
 * @brief Callback event that requests local recording privilege changes.
 * @param status Value of request for local recording privilege status.
 */
- (void)onRequestLocalRecordingPrivilegeChanged:(ZoomSDKLocalRecordingRequestPrivilegeStatus)status;

/**
 * @brief Callback event that lets participants request that the host starts cloud recording.
 * @param allow YES  allow. If NO, disallow.
 */
- (void)onAllowParticipantsRequestCloudRecording:(BOOL)allow;
/**
 * @brief Callback event that user's avatar path updated when in the meeting.
 * @param userID The ID of user whose avatar path updated.
 */
- (void)onInMeetingUserAvatarPathUpdated:(unsigned int)userID;

/**
 * @brief Sink the event that AI Companion active status changed.
 * @param active YES if AI Companion is active, NO otherwise.
 */
- (void)onAICompanionActiveChangeNotice:(BOOL)active;

/**
 * @brief Sink the event that participant profile status change.
 * @param hidden YES to hide participant profile picture, NO to show participant profile picture.
 */
- (void)onParticipantProfilePictureStatusChange:(BOOL)hidden;

/**
 * @brief Callback event of alpha channel mode changes.
 * @param isAlphaModeOn YES if in alpha channel mode, NO otherwise.
 */
- (void)onVideoAlphaChannelStatusChanged:(BOOL)isAlphaModeOn;

/**
 * @brief Sink the event that focus mode changed by host or co-host.
 * @param on YES if the focus mode change to on, NO otherwise.
 */
- (void)onFocusModeStateChanged:(BOOL)on;

/**
 * @brief The focus mode share type was changed by the host or co-host.
 * @param shareType Share type change.
 */
- (void)onFocusModeShareTypeChanged:(ZoomSDKFocusModeShareType)shareType;

/**
 * @brief Callback event of meeting QA feature status changes.
 * @param isMeetingQAFeatureOn YES if meeting QA feature is on, NO otherwise.
 */
- (void)onMeetingQAStatusChanged:(BOOL)isMeetingQAFeatureOn;

/**
 * @brief Callback event that requests to join third party telephony audio.
 * @param audioInfo Instruction on how to join the meeting with third party audio.
 */
 - (void)notifyToJoin3rdPartyTelephonyAudio:(NSString*)audioInfo;

/**
 * @brief Callback for when the current user receives a camera control request. This callback triggers when another user requests control of the current user’s camera.
 * @param userId The user ID that sent the request.
 * @param requestType The request type.
 * @param actionApprove Execute this block to approve.
 * @param actionDecline Execute this block to decline.
 */
- (void)onCameraControlRequestReceived:(unsigned int)userId requestType:(ZoomSDKCameraControlRequestType)requestType actionApprove:(nullable ZoomSDKError(^)(void))actionApprove actionDecline:(nullable ZoomSDKError(^)(void))actionDecline;

/**
 * @brief Callback for when the current user is granted camera control access. Once the current user sends the camera control request, this callback triggers when they receive a request result.
 * @param userId The user ID that response the request.
 * @param resultType The result type.
 */
- (void)onCameraControlRequestResult:(unsigned int)userId resultType:(ZoomSDKCameraControlRequestResult)resultType;

/**
 * @brief Callback event for the mute on entry status change.
 * @param enable Specify whether mute on entry is enabled or not.
 */
- (void)onMuteOnEntryStatusChange:(BOOL)enable;

/**
 * @brief Callback event for the meeting topic changed.
 * @param topic The new meeting topic.
 */
- (void)onMeetingTopicChanged:(NSString *)topic;

/**
 * @brief Callback event that the bot relationship changed in the meeting.
 * @param authorizeUserID Specify the authorizer user ID.
 */
- (void)onBotAuthorizerRelationChanged:(unsigned int)authorizeUserID;

/**
 * @brief Callback event that the companion relationship created in the meeting.
 * @param parentUserID Specify the parent user ID.
 * @param childUserID Specify the child user ID.
 */
- (void)onCreateCompanionRelation:(unsigned int)parentUserID childUserID:(unsigned int)childUserID;

/**
 * @brief Callback event that the companion relationship removed in the meeting.
 * @param childUserID Specify the child user ID.
 */
- (void)onRemoveCompanionRelation:(unsigned int)childUserID;

/**
 * @brief Callback event when the grant co-owner permission changed.
 * @param canGrantOther YES if can grant others, NO otherwise.
 */
- (void)onGrantCoOwnerPrivilegeChanged:(BOOL)canGrantOther;
@end


/**
 * @class ZoomSDKMeetingActionController
 * @brief Interface for managing participant-related actions in a meeting, such as retrieving participant list, muting audio, or controlling roles.
 */
@interface ZoomSDKMeetingActionController : NSObject
{
    id<ZoomSDKMeetingActionControllerDelegate> _delegate;
}
/**
 * @brief Sets or get the delegate to receive meeting action events.
 */
@property(nonatomic, assign, nullable) id<ZoomSDKMeetingActionControllerDelegate> delegate;

/**
 * @brief Gets the list of participants.
 * @return If the function succeeds, it returns an array of participant ID. Otherwise, this function fails and returns nil.
 */
- (NSArray*_Nullable)getParticipantsList;
/**
 * @brief Commands in the meeting.
 * @param cmd The commands in the meeting.
 * @param userID The ID of user. Zero(0) means that the current user can control the commands. If it is other participant, it returns the corresponding user ID.
 * @param screen Specify the screen on which you want to do action.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)actionMeetingWithCmd:(ActionMeetingCmd)cmd userID:(unsigned int)userID onScreen:(ScreenType)screen;

/**
 * @brief Gets the specified user's information.
 * @param userID The specified user's ID.
 * @return If the function succeeds, it returns the object of ZoomSDKUserInfo. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKUserInfo*_Nullable)getUserByUserID:(unsigned int)userID;

/**
 * @brief Gets the information of myself.
 * @return If the function succeeds, it returns the object of ZoomSDKUserInfo. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKUserInfo*_Nullable)getMyself;

/**
 * @brief Change user's screen name in the meeting.
 * @param userID The ID of user whose screen name will be changed. Normal participants can change only their personal screen name while the host or co-host can change all participants' names.
 * @param name The new screen name. 
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)changeUserName:(unsigned int)userID newName:(NSString*)name;

/**
 * @brief Assign a participant to be a new host, or original host who loose the privilege reclaims to be the host.
 * @param userID User ID of new host. Zero(0) means that the original host takes back the privilege.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)makeHost:(unsigned int)userID;

/**
 * @brief Make the specified user to raise hand or lower hand.
 * @param raise YES to raise hand, NO to lower hand for this specified user.
 * @param userid User ID of the user to be modified.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)raiseHand:(BOOL)raise UserID:(unsigned int)userid;

/**
 * @brief Removes the specified user from meeting.
 * @param userid User ID of the user to be modified.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)expelUser:(unsigned int)userid;

/**
 * @brief Allow or disallow the specified user to local recording.
 * @param allow YES to allow, NO to disallow.
 * @param userid User ID of the user to be modified.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowLocalRecord:(BOOL)allow UserID:(unsigned int)userid;

/**
 * @brief Query whether the current user is the original host.
 * @return YES if the current user is the original host. Otherwise, NO.
 */
- (BOOL)isSelfOriginalHost;

/**
 * @brief Query if user can claim host(be host) or not. 
 * @return YES if able. Otherwise, NO.
 */
- (BOOL)canReclaimHost;

/**
 * @brief Reclaim the host's role.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)reclaimHost;
/**
 * @brief Normal participant claims host by host-key.
 * @param hostKey Host key.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)claimHostByKey:(NSString*)hostKey;
/**
 * @brief Assign a user as co-host in meeting. 
 * @param userid The ID of user to be a co-host.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note The co-host cannot be assigned as co-host by himself. And the user should have the power to assign the role.
 */
- (ZoomSDKError)assignCoHost:(unsigned int)userid;

/**
 * @brief Revoke co-host role of another user in meeting.
 * @param userid The ID of co-host who will loose the co-host privilege.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note Only meeting host can run the function.
 */
- (ZoomSDKError)revokeCoHost:(unsigned int)userid;

/**
 * @brief Sets sharing types for the host or co-host in meeting.
 * @param shareType The custom sharing type.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)setShareSettingType:(ZoomSDKShareSettingType)shareType;

/**
 * @brief Gets the sharing types for the host or co-host in meeting.
 * @param type The custom sharing type.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)getShareSettingType:(ZoomSDKShareSettingType*)type;

/**
 * @brief Determines if user's original sound is enabled.
 * @return YES if enabled. Otherwise, NO.
 */
- (BOOL)isUseOriginalSoundOn;

/**
 * @brief Sets to output original sound of mic in meeting.
 * @param enable YES if using original sound, No disabling, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)enableUseOriginalSound:(BOOL)enable;

/**
 * @brief Determines if the meeting supports user's original sound.
 * @return YES if supported. Otherwise, NO.
 */
- (BOOL)isSupportUseOriginalSound;

/**
 * @brief Swap to show sharing screen or video.
 * @param share YES to swap to sharing screen, NO to swap to video.
 * @note Only available for Zoom native ui mode.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)swapToShowShareViewOrVideo:(BOOL)share;

/**
 * @brief Determines if the user can swap between show sharing screen or video now.
 * @return YES if can. Otherwise, NO.
 */
- (BOOL)canSwapBetweenShareViewOrVideo;

/**
 * @brief Determines if the meeting is displaying the sharing screen now.
 * @param isShowingShareView YES if showing share screen, NO if showing video.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)isDisplayingShareViewOrVideo:(BOOL*)isShowingShareView;

/**
 * @brief Sets the meeting topic on meeting info.
 * @param topic The meeting topic.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @deprecated This method is no longer used.
 */
- (ZoomSDKError)setMeetingTopicOnMeetingInfo:(NSString *)topic DEPRECATED_MSG_ATTRIBUTE("No longer used");

/**
 * @brief Determines if the current user can change the meeting topic.
 * @return YES if can. Otherwise, NO.
 */
- (BOOL)canSetMeetingTopic;

/**
 * @brief Change the meeting topic.
 * @param topic The new meeting topic.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)setMeetingTopic:(NSString *)topic;

/**
 * @brief Checks if the host or cohost can enable mute on entry.
 * @return YES if the host or cohost can enable mute on entry. Otherwise, NO..
 */
- (BOOL)canEnableMuteOnEntry;

/**
 * @brief Determines if mute on entry is enabled.
 * @return YES if mute on entry is enabled. Otherwise, NO.
 */
- (BOOL)isMuteOnEntryEnabled;

/**
 * @brief Determines if the share screen is allowed.
 * @return YES if share screen is allowed. Otherwise, NO.
 */
- (BOOL)isParticipantsShareAllowed;

/**
 * @brief Allow participants to share screen.
 * @param allow YES to allow participants to share screen, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowParticipantsToShare:(BOOL)allow;

/**
 * @brief Determines if the chat is allowed.
 * @return YES if chat is allowed. Otherwise, NO.
 */
- (BOOL)isParticipantsChatAllowed;

/**
 * @brief Allow participants to chat.
 * @param allow YES to allow participants to chat, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowParticipantsToChat:(BOOL)allow;

/**
 * @brief Determines if the participant rename is disabled.
 * @return YES if participant rename is allowed. Otherwise, NO.
 */
- (BOOL)isParticipantsRenameAllowed;

/**
 * @brief Allow participants to rename.
 * @param allow YES to allow participants to rename, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowParticipantsToRename:(BOOL)allow;

/**
 * @brief Determines if user can spotlight someone.
 * @param userID The user's user ID you want to spotlight.
 * @param result A point to enum ZoomSDKSpotlightResult, if the function call successfully, the value of 'result' means whether the user can be spotlighted.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)canSpotlight:(unsigned int)userID result:(ZoomSDKSpotlightResult*)result;

/**
 * @brief Determines if user can unspotlight someone.
 * @param userID The user's user ID you want to unspotlight.
 * @param result A point to enum ZoomSDKSpotlightResult, if the function call successfully, the value of 'result' means whether the user can be unspotlighted.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)canUnSpotlight:(unsigned int)userID result:(ZoomSDKSpotlightResult*)result;

/**
 * @brief Spotlight someone's video.
 * @param userID The user's user ID you want to spotlight.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)spotlightVideo:(unsigned int)userID;

/**
 * @brief Unspotlight someone's video.
 * @param userID The user's user ID you want to unspotlight.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unSpotlightVideo:(unsigned int)userID;

/**
 * @brief Unspotlight all videos.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unSpotlightAllVideos;

/**
 * @brief Gets all users that has been spotlighted.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function fails and returns nil.
 */
- (NSArray*_Nullable)getSpotlightedUserList;

/**
 * @brief Determines if user can pin someone to first view.
 * @param userID The user's user ID you want to pin.
 * @param result A point to enum ZoomSDKPinResult, if the function call successfully, the value of 'result' means whether the user can be pined.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)canPinToFirstView:(unsigned int)userID result:(ZoomSDKPinResult*)result;

/**
 * @brief Pin user's video to first view.
 * @param userID The user's user ID you want to pin.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)pinVideoToFirstView:(unsigned int)userID;

/**
 * @brief Unpin user's video to first view.
 * @param userID The user's user ID you want to unpin.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unPinVideoFromFirstView:(unsigned int)userID;

/**
 * @brief Unpin all videos to first view.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unPinAllVideosFromFirstView;

/**
 * @brief Gets all users that has been pined in first view.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function fails and returns nil.
 */
- (NSArray*_Nullable)getPinnedUserListFromFirstView;

/**
 * @brief Determines if user can pin someone to second view.
 * @param userID The user's user ID you want to pin.
 * @param result A point to enum ZoomSDKPinResult, if the function call successfully, the value of 'result' means whether the user can be pined.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)canPinToSecondView:(unsigned int)userID result:(ZoomSDKPinResult*)result;

/**
 * @brief Pin user's video to second view.
 * @param userID The user's user ID you want to pin.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)pinVideoToSecondView:(unsigned int)userID;

/**
 * @brief Unpin user's video to second view.
 * @param userID The user's user ID you want to unpin.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unPinVideoFromSecondView:(unsigned int)userID;

/**
 * @brief Gets all users that has been pined in second view.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function fails and returns nil.
 */
- (NSArray*_Nullable)getPinnedUserListFromSecondView;

/**
 * @brief Determines if participants can unmute themselves.
 * @return YES if can unmute themselves. Otherwise, NO.
 */
- (BOOL)isParticipantsUnmuteSelfAllowed;

/**
 * @brief Determines whether the legal notice for chat is available.
 * @return YES if the legal notice for chat is available. Otherwise, NO.
 */
- (BOOL)isMeetingChatLegalNoticeAvailable;

/**
 * @brief Gets the chat legal notices prompt.
 * @return If the function succeeds, it returns the chat legal notices prompt. Otherwise, this function fails and returns nil.
 */
- (NSString *)getChatLegalNoticesPrompt;

/**
 * @brief Gets the chat legal notices explained.
 * @return If the function succeeds, it returns the chat legal notices explained. Otherwise, this function fails and returns nil.
 */
- (NSString *)getChatLegalNoticesExplained;

/**
 * @brief Enables of disable following host's video order.
 * @param enable YES to enable, NO otherwise.
 */
- (ZoomSDKError)enableFollowHostVideoOrder:(BOOL)enable;

/**
 * @brief Sets the video order.
 * @param orderList The array contains the listed user ID.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)setVideoOrder:(NSArray<NSNumber*>*)orderList;

/**
 * @brief Determines whether the option of following host's video order is on of off.
 * @return Yes means the option of following host's video order is on. Otherwise, NO.
 */
- (BOOL)isFollowHostVideoOrderOn;

/**
 * @brief Determines whether this meeting support following host's video orde.
 * @return Yes means supporting. Otherwise, NO.
 */
- (BOOL)isSupportFollowHostVideoOrder;

/**
 * @brief Gets the video orde list.
 * @return If the function succeeds, it returns the video orde list. Otherwise, this function fails and returns nil.
 */
- (NSArray<NSNumber*>*_Nullable)getVideoOrderList;

/**
 * @brief Lower all hands raised.
 * @param forWebinarAttendees YES to lower all hands for webinar attendee, NO otherwise.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note When forWebinarAttendees is YES, the SDK sends the lower all hands command only to
 webinar attendees. When forWebinarAttendees is false, the SDK sends the lower all hands command to anyone who is not a webinar attendee, such as the webinar host, cohost, panelist, or everyone in a regular meeting.
 */
- (ZoomSDKError)lowerAllHands:(BOOL)forWebinarAttendees;

/**
 * @brief Determines whether the message can be deleted.
 * @param msgID The message ID.
 * @return Yes means can be deleted. Otherwise, NO.
 */
- (BOOL)isChatMessageCanBeDeleted:(NSString*)msgID;

/**
 * @brief Deletes chat message by message ID.
 * @param msgID The message ID.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)deleteChatMessage:(NSString*)msgID;

/**
 * @brief Gets all chat message ID.
 * @return If the function succeeds, it returns an array with all message ID. Otherwise, this function fails and returns nil.
 */
- (NSArray<NSString*>*_Nullable)getAllChatMessageID;

/**
 * @brief Sets participants chat privilege.
 * @param privilege The chat privilege type to assign to participants.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)setParticipantsChatPrivilege:(ZoomSDKChatPrivilegeType)privilege;

/**
 * @brief Gets the current's chat status user.
 * @return If the function succeeds, the chat's return value is the info status. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKChatStatus *_Nullable)getChatStatus;

/**
 * @brief Determines whether the legal notice for sharing in meeting chat is available.
 * @return YES if the legal notice for chat is available. Otherwise, NO.
 */
- (BOOL)isShareMeetingChatLegalNoticeAvailable;

/**
 * @brief Gets the sharing in meeting chat started legal notices content.
 * @return If the function succeeds, it returns the the sharing in meeting chat started legal notices content. Otherwise, this function fails and returns nil.
 */
- (NSString*)getShareMeetingChatStartedLegalNoticeContent;

/**
 * @brief Gets the sharing in meeting chat stopped legal notices content.
 * @return If the function succeeds, it returns the the sharing in meeting chat stopped legal notices content. Otherwise, this function fails and returns nil.
 */
- (NSString*)getShareMeetingChatStoppedLegalNoticeContent;

/**
 * @brief Stops the incoming audio.
 * @param stop YES to stop, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)stopIncomingAudio:(BOOL)stop;

/**
 * @brief Determines if the incoming audio is stopped.
 * @return Yes indicates that incoming audio is stopped. Otherwise, NO.
 */
- (BOOL)isIncomingAudioStopped;

/**
 * @brief Stops in coming video.
 * @param stop YES to stop in coming video, NO is start coming video.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)stopIncomingVideo:(BOOL)stop;

/**
 * @brief Determines whether the coming video is stopedd.
 * @return YES if the coming video is stopedd. Otherwise, NO.
 */
- (BOOL)isIncomingVideoStopped;

/**
 * @brief Determines if show the last used avatar in the meeting.
 * @param show YES indicates to show the last used avatar, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)showAvatar:(BOOL)show;

/**
 * @brief Determines if the meeting is showing the avatar.
 * @return YES if the meeting is showing the avatar. Otherwise, NO.
 */
- (BOOL)isShowAvatar;

/**
 * @brief Allowing the regular attendees to start video, it can only be used in regular meeetings(no bo).
 * @param bAllow YES indicates Allowing the regular attendees to start video, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowParticipantsToStartVideo:(BOOL)bAllow;

/**
 * @brief Checks whether the current meeting allows participants to start video, it can only be used in regular meeetings(no bo).
 * @return YES if allows participants to start video. Otherwise, NO.
 */
- (BOOL)isParticipantsStartVideoAllowed;

/**
 * @brief Allowing the regular attendees to share whiteboard, it can only be used in regular meeetings(no bo).
 * @param bAllow YES indicates Allowing the regular attendees to share whiteboard, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowParticipantsToShareWhiteBoard:(BOOL)bAllow;

/**
 * @brief Checks whether the current meeting allows participants to share whiteboard, it can only be used in regular meeetings(no bo).
 * @return YES if allows participants to share whiteboard. Otherwise, NO.
 */
- (BOOL)isParticipantsShareWhiteBoardAllowed;

/**
 * @brief Checks whether the current meeting allows participants to send local recording privilege requests. It can only be used in regular meetings, not in webinar or breakout room.
 * @return If allows participants to share whiteboard, it returns YES. Otherwise, NO.
 */
- (BOOL)isParticipantRequestLocalRecordingAllowed;

/**
 * @brief Allow participant to request local recording. It can only be used in regular meetings, not in webinar or breakout room.
 * @param bAllow YES indicates Allowing the regular attendees to send local recording privilege request, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowParticipantsToRequestLocalRecording:(BOOL)bAllow;

/**
 * @brief Checks whether the current meeting auto-grants participants’ local recording privilege requests. It can only be used in regular meetings (not webinar or breakout room).
 * @return If auto grant participants local recording privilege request, it returns YES. Otherwise, NO.
 */
- (BOOL)isAutoAllowLocalRecordingRequest;

/**
 * @brief Allow participants to request local recording. It can only be used in regular meetings (not webinar or breakout room).
 * @param bAllow YES indicates Auto grant or deny the regular attendee's local recording privilege request, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)autoAllowLocalRecordingRequest:(BOOL)bAllow;

/**
 * @brief Determines whether suspend all participants activities.
 * @return YES if can suspend participants activities. Otherwise, NO.
 */
- (BOOL)canSuspendParticipantsActivities;

/**
 * @brief Suspend all participants activities.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)suspendParticipantsActivities;

/**
 * @brief Query if the current user can hide participant profile pictures.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note: This feature is influenced by focus mode change.
 */
- (ZoomSDKError)canHideParticipantProfilePictures;

/**
 * @brief Query if the current meeting hides participant pictures.
 * @return YES to hide participant pictures, NO means show participant pictures. Otherwise, NO.
 */
- (BOOL)isParticipantProfilePicturesHidden;

/**
 * @brief Hides or show participant profile pictures.
 * @param hide YES to hide participant profile pictures, NO to show participant pictures.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)hideParticipantProfilePictures:(BOOL)hide;

/**
 * @brief Determines if alpha channel mode can be enabled.
 * @return YES if it can be enabled. Otherwise, NO.
 */
- (BOOL)canEnableAlphaChannelMode;

/**
 * @brief Enables or disable alpha channel mode.
 * @param enabled YES indicates to enable alpha channel mode, disable it, NO to disable.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)enableAlphaChannelMode:(BOOL)enabled;

/**
 * @brief Determines if alpha channel mode is enabled.
 * @return YES if in alpha channel mode. Otherwise, NO.
 */
- (BOOL)isAlphaChannelModeEnabled;

/**
 * @brief Gets the focus mode enabled or not by web portal.
 * @return YES if focus mode enabled. Otherwise, NO.
 */
- (BOOL)isFocusModeEnabled;

/**
 * @brief Turn focus mode on or off. Focus mode on means Participants onlies be able to see hosts' videos and shared content, and videos of spotlighted participants.
 * @param on Yes means to turn on. No means to turn off.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)turnFocusModeOn:(BOOL)on;

/**
 * @brief Gets the focus mode on or off.
 * @return YES if focus mode on. Otherwise, NO.
 */
- (BOOL)isFocusModeOn;

/**
 * @brief Gets share focus mode  type indicating who can see the shared content which is controlled by host or co-host.
 * @return The current share focus mode type.
 */
- (ZoomSDKFocusModeShareType)getFocusModeShareType;

/**
 * @brief Sets the focus mode type indicating who can see the shared content which is controlled by host or co-host.
 * @param shareType The type of focus mode share type.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)setFocusModeShareType:(ZoomSDKFocusModeShareType)shareType;

/**
 * @brief Sets to enable or disable meeting QA.
 * @param enable YES if enabled, NO if disabled.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)enableMeetingQAFeature:(BOOL)enable;

/**
 * @brief Query if meeting QA is enabled in current meeting.
 * @return YES if enabled. Otherwise, NO.
 */
- (BOOL)isMeetingQAFeatureOn;
/**
 * @brief Determines if the current user can enable participant request clould recording.
 * @return YES if the current user can enable participant request clould recording. Otherwise, NO.
 */
- (BOOL)canEnableParticipantRequestCloudRecording;

/**
 * @brief Checks whether the current meeting allows participants to send cloud recording privilege request, it can be used in regular meeetings and webinar (no bo).
 * @return YES if allows participants to send request. Otherwise, NO.
 */
- (BOOL)isParticipantRequestCloudRecordingAllowed;

/**
 * @brief Toggle whether attendees can send requests for the host to start a cloud recording. This  can only be used in regular meeetings, not breakout rooms.
 * @param allow Yes indicates that  participants are allowed to send cloud recording privilege requests.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowParticipantsToRequestCloudRecording:(BOOL)allow;

/**
 * @brief Determines if the meeting has third party telephony audio enabled.
 * @return YES if enabled. Otherwise, NO.
 */
- (BOOL)is3rdPartyTelephonyAudioOn;

/**
 * @brief Gets the information about the bot's authorized user.
 * @param userId Specify the user ID for which to get the information.
 * @return If the function succeeds, it returns a ZoomSDKUserInfo object. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKUserInfo * _Nullable)getBotAuthorizedUserInfoByUserID:(unsigned int)userId;

/**
 * @brief Gets the authorizer's bot list.
 * @param userId Specify the user ID for which to get the information.
 * @return If the function succeeds, it returns the authorizer's bot list in the meeting. Otherwise, this function fails and returns nil.
 */
- (NSArray<NSNumber*>* _Nullable)getAuthorizedBotListByUserID:(unsigned int)userId;

/**
 * @brief Determines if there is support for the virtual name tag feature.
 * @return YES if supports the virtual name tag feature. Otherwise, NO.
 */
- (BOOL)isSupportVirtualNameTag;

/**
 * @brief Enables the virtual name tag feature for the account.
 * @param bEnabled YES if the feature is enabled, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)enableVirtualNameTag:(BOOL)bEnabled;

/**
 * @brief Updates the virtual name tag roster infomation for the account.
 * @param userRoster The virtual name tag roster info list for the specified user.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note The maximum size of userRoster should less 20. User should specify the tagName and tagID of each ZoomSDKVirtualNameTag object. The range of tagID is 0-1024.
 */
- (ZoomSDKError)updateVirtualNameTagRosterInfo:(NSArray<ZoomSDKVirtualNameTag*>*)userRoster;

/**
 * @brief Determines if play meeting audio is enabled or not.
 * @return YES if enabled. Otherwise, NO.
 */
- (BOOL)isPlayMeetingAudioEnabled;

/**
 * @brief Enables or disable SDK to play meeting audio.
 * @param bEnabled YES if SDK will play meeting audio, NO if SDK will not play meeting audio.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)enablePlayMeetingAudio:(BOOL)bEnabled;

/**
 * @brief Determines if contrast enhancement effect for speaker video is enabled.
 * @return YES if contrast enhancement effect is enabled. Otherwise, NO.
 */
- (BOOL)isSpeakerContrastEnhanceEnabled;

/**
 * @brief Enables or disable contrast enhancement effect for speaker video.
 * @param bEnabled YES indicates to enable contrast enhancement effect, disable it, NO to disable.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)enableSpeakerContrastEnhance:(BOOL)bEnabled;

/**
 * @brief Gets the information about the parent user.
 * @param userid Specify the user ID for which to get the information.
 * @return If the function succeeds, it returns a pointer to the ZoomSDKUserInfo. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKUserInfo* _Nullable)getCompanionParentUser:(unsigned int)userid;

/**
 * @brief Gets the child list.
 * @param userid Specify the user ID for which to get the information.
 * @return If the function succeeds, it returns the sub-user list of user companion mode. Otherwise, this function fails and returns nil.
 */
- (NSArray<NSNumber*>* _Nullable)getCompanionChildList:(unsigned int)userid;

/**
 * @brief Assigns a user as co-host and grants privileges to manage assets after the meeting.
 * @param userid The user's user ID to be assigned as co-host.
 * @param infoList An array of  \link ZoomSDKGrantCoOwnerAssetsInfo \endlink objects representing the assets and privileges to grant.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 *
 * @note Before calling this method, you must obtain the assets to be granted via  \link getGrantCoOwnerAssetsInfo \endlink.
 */
- (ZoomSDKError)assignCoHostWithAssetsPrivilege:(unsigned int)userid infoList:(NSArray<ZoomSDKGrantCoOwnerAssetsInfo *>*)infoList;

/**
 * @brief Assigns a user as host and grants privileges to manage assets after the meeting.
 * @param userid The user's user ID to be assigned as host.
 * @param infoList An array of  \link ZoomSDKGrantCoOwnerAssetsInfo \endlink objects representing the assets and privileges to grant.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 *
 * @note Before calling this method, you must obtain tThe assets to be granted via  \link getGrantCoOwnerAssetsInfo \endlink.
 */
- (ZoomSDKError)makeHostWithAssetsPrivilege:(unsigned int)userid infoList:(NSArray<ZoomSDKGrantCoOwnerAssetsInfo *>*)infoList;
@end
NS_ASSUME_NONNULL_END
