/**
 * @file ZoomSDKMeetingRecordController.h
 * @brief Interfaces for controlling meeting recording features in Zoom SDK.
 */

#import <ZoomSDK/ZoomSDKErrors.h>
NS_ASSUME_NONNULL_BEGIN

/**
 * @brief Enumeration of recording layout mode.
 */
typedef enum
{
	/** For initialization. */
    RecordingLayoutMode_None = 0,
    /** Only record active speaker's video. */
    RecordingLayoutMode_ActiveVideoOnly = 1,
    /** Video wall mode. */
    RecordingLayoutMode_VideoWall = (1<<1),
    /** Record shared content with participants' video. */
    RecordingLayoutMode_VideoShare = (1<<2),
    /** Record only the audio. */
    RecordingLayoutMode_OnlyAudio = (1<<3),
    /** Record only the shared content. */
    RecordingLayoutMode_OnlyShare = (1<<4),
}RecordingLayoutMode;



/**
 * @class CustomizedRecordingLayoutHelper
 * @brief Helper class to customize recording layout during meetings.
 */
@interface CustomizedRecordingLayoutHelper : NSObject
/**
 * @brief Gets the layout mode supported by the current meeting.
 * @return If the function succeeds, it returns the layout mode. The value is the 'bitwise OR' of each supported layout mode. Otherwise, returns 0.
 */
- (int)getSupportLayoutMode;

/**
 * @brief Gets the list of users whose video source is available.
 * @return The list of users. ZERO(0) indicates that there is no available video source of users. Otherwise, this function fails and returns nil.
 */
- (NSArray*)getValidVideoSource;

/**
 * @brief Gets available shared source received by users. 
 * @return If the function succeeds, it returns a NSArray including all valid shared source received by users. Otherwise, this function fails and returns nil.
 */
- (NSArray*)getValidReceivedShareSource;

/**
 * @brief Query if sending shared source is available. 
 * @return YES if available. Otherwise, NO.
 */
- (BOOL)isSendingShareSourceAvailable;

/**
 * @brief Determines if there exists the active video source.
 * @return YES if existing. Otherwise, NO.
 */
- (BOOL)haveActiveVideoSource;

/**
 * @brief Select layout mode for recording.
 * @param mode The layout mode for recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)selectRecordingLayoutMode:(RecordingLayoutMode)mode;

/**
 * @brief Adds the video source of specified user to the list of recorded videos.
 * @param userid The ID of specified user.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)addVideoSourceToResArray:(unsigned int)userid;

/**
 * @brief Adds active video source to the array.
 * @return If the function succeeds, it returns the @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note It works only when RecordingLayoutMode is  or RecordingLayoutMode_VideoShare.
 */
- (ZoomSDKError)selectActiveVideoSource;

/**
 * @brief Select the specified's shared source user.
 * @param shareSourceID The share source ID of specified user.
 * @return If the function succeeds, it returns the @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note It works only when RecordingLayoutMode is RecordingLayoutMode_OnlyShare.
 */
- (ZoomSDKError)selectShareSource:(unsigned int)shareSourceID;
/**
 * @brief Select shared source of the current user.
 * @return If the function succeeds, it returns the @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note It works only when RecordingLayoutMode is RecordingLayoutMode_OnlyShare.
 */
- (ZoomSDKError)selectSendShareSource;
@end


/**
 * @class ZoomSDKRequestLocalRecordingPrivilegeHandler
 * @brief Handler for managing requests for local recording privileges.
 */
@interface ZoomSDKRequestLocalRecordingPrivilegeHandler : NSObject
/**
 * @brief Gets the request ID.
 */
@property(nonatomic, copy, readonly, nullable)NSString* requestId;
/**
 * @brief Gets the user ID who requested privilege.
 */
@property(nonatomic, assign, readonly)int requesterId;
/**
 * @brief Gets the user name who requested privileges.
 */
@property(nonatomic, copy, readonly, nullable)NSString* requesterName;
/**
 * @brief Allows the user to start local recording.
 * @return If the function succeeds, it returns the @c ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)grantLocalRecordingPrivilege;
/**
 * @brief Denies the user permission to start local recording.
 * @return If the function succeeds, it returns the @c ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)denyLocalRecordingPrivilege;
@end


/**
 * @class ZoomSDKRequestEnableAndStartSmartRecordingHandler
 * @brief Handler for managing a user's request to enable and start smart recording.
 */
@interface ZoomSDKRequestEnableAndStartSmartRecordingHandler : NSObject

/**
 * @brief The user who requests to enable and start smart cloud recording.
 */
@property(nonatomic, assign, readonly)unsigned int requestUserID;

/**
 * @brief Gets legal tip that you should agree to handle the user request.
 */
@property(nonatomic, copy, readonly, nullable)NSString* tipString;

/**
 * @brief Starts normal cloud recording without enabling smart recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)startCloudRecordingWithoutEnableSmartRecording;

/**
 * @brief Agree to the legal notice to enable and start smart cloud recording.
 * @param allMeetings NO to only enable smart recording for the current meeting. YES to enable smart recording for all future meetings including the current meeting,
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)agreeToEnableAndStart:(BOOL)allMeetings;

/**
 * @brief Decline the request to start cloud recording.
 * @param denyAll YES indicates to deny all attendees' requests for the host to start cloud recording. Participants can't send these types of requests again until the host changes the setting, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)deny:(BOOL)denyAll;
@end

/**
 * @class ZoomSDKSmartRecordingEnableActionHandler
 * @brief Object to handle enable and start the smart recording.
 */
@interface ZoomSDKSmartRecordingEnableActionHandler : NSObject
/**
 * @brief Gets the legal tip to enable smart recording.
 */
@property(nonatomic, copy, readonly)NSString* tipString;
/**
 * @brief Confirms enabling and starting the smart recording.
 * @param allMeetings NO to only enable smart recording for the current meeting. YES to enable smart recording for all future meetings including the current meeting,
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)actionConfirm:(BOOL)allMeetings;

/**
 * @brief Cancels enabling and starting the smart recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)actionCancel;
@end

/**
 * @class ZoomSDKRequestStartCloudRecordingHandler
 * @brief Object that handles a user's request to start cloud recording. If the current user can control web setting for smart recording, they get ZoomSDKRequestEnableAndStartSmartRecordingHandler when an attendee requests to start cloud recording or start cloud recording by self.
 */
@interface ZoomSDKRequestStartCloudRecordingHandler : NSObject

/**
 * @brief Gets the user ID who requested that the host start cloud recording.
 */
@property(nonatomic, assign, readonly)unsigned int requesterId;

/**
 * @brief Gets the user name who requested that the host start cloud recording.
 */
@property(nonatomic, copy, readonly, nullable)NSString* requesterName;

/**
 * @brief Allows to start cloud recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)start;

/**
 * @brief Deny the request to start cloud recording.
 * @param denyAll  YES indicates to deny all attendees' requests for the host to start cloud recording. Participants can't send these types of requests again until the host changes the setting, NO otherwise.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)deny:(BOOL)denyAll;
@end



/**
 * @protocol ZoomSDKMeetingRecordDelegate
 * @brief Delegate protocol to receive recording-related events.
 */
@protocol ZoomSDKMeetingRecordDelegate <NSObject>

/**
 * @brief Callback event of ending the conversion to MP4 format.
 * @param success YES if converting successfully, NO otherwise. 
 * @param recordPath The path of saving the recording file.
 */
- (void)onRecord2MP4Done:(BOOL)success Path:(NSString*)recordPath;

/**
 * @brief Callback event of the process to convert the recording file to MP4 format. 
 * @param percentage Percentage of conversion process. Range from ZERO(0) to ONE HUNDREAD(100).
 */
- (void)onRecord2MP4Progressing:(int)percentage;

/**
 * @brief Callback event that the status of Cloud recording changes.
 * @param status Value of recording status.
 */
- (void)onCloudRecordingStatus:(ZoomSDKRecordingStatus)status;

/**
 * @brief Callback event that the recording authority changes.
 * @param canRec YES if it is able to record, NO otherwise.
 */
- (void)onRecordPrivilegeChange:(BOOL)canRec;

/**
 * @brief Callback event that the local recording source changes in the custom UI mode.
 * @param helper A CustomizedRecordingLayoutHelper pointer.
 */
- (void)onCustomizedRecordingSourceReceived:(CustomizedRecordingLayoutHelper*)helper;

/**
 * @brief Callback event that the specified user's local recording status changes.
 * @param status Value of recording status.
 * @param userID The specified user's ID.
 */
- (void)onLocalRecordStatus:(ZoomSDKRecordingStatus)status userID:(unsigned int)userID;

/**
 * @brief Callback event that the status of request local recording privilege.
 * @param status Value of request local recording privilege status.
 */
- (void)onLocalRecordingPrivilegeRequestStatus:(ZoomSDKRequestLocalRecordingStatus)status;

/**
 * @brief Callback event when a user requests local recording privilege.
 * @param handler A pointer to the ZoomSDKRequestLocalRecordingPrivilegeHandler.
 */
- (void)onLocalRecordingPrivilegeRequested:(ZoomSDKRequestLocalRecordingPrivilegeHandler *_Nullable)handler;

/**
 * @brief An event sink that the cloud recording storage is full.
 * @param gracePeriodDate A point in time in time, in milliseconds, in UTC.  You can use the cloud recording storage until the gracePeriodDate.
 */
- (void)onCloudRecordingStorageFull:(time_t)gracePeriodDate;

/**
 * @brief Callback event for when the host responds to a cloud recording permission request.
 * @param status Value of request host to start cloud recording response status.
 */
- (void)onRequestCloudRecordingResponse:(ZoomSDKRequestStartCloudRecordingStatus)status;

/**
 * @brief Callback event received only by the host when a user requests to start cloud recording.
 * @param handler A pointer to the ZoomSDKRequestStartCloudRecordingHandler.
 */
- (void)onStartCloudRecordingRequested:(ZoomSDKRequestStartCloudRecordingHandler *_Nullable)handler;

/**
 * @brief Callback event received only by the host when a user requests to enable and start smart cloud recording.
 * @param handler The handler to agree or decline the request.
 */
- (void)onEnableAndStartSmartRecordingRequested:(ZoomSDKRequestEnableAndStartSmartRecordingHandler *_Nullable)handler;

/**
 * @brief Callback event received when you call enableSmartRecording.
 * @param handler The handler to confirm or cancel enabling and starting the smart recording.
 */
- (void)onSmartRecordingEnableActionCallback:(ZoomSDKSmartRecordingEnableActionHandler *_Nullable)handler;
@end


/**
 * @class ZoomSDKMeetingRecordController
 * @brief Controller to manage meeting recording features.
 */
@interface ZoomSDKMeetingRecordController : NSObject
{
    id<ZoomSDKMeetingRecordDelegate> _delegate;
}
/**
 * @brief Delegate to receive recording events.
 */
@property(nonatomic, assign, nullable)id<ZoomSDKMeetingRecordDelegate> delegate;
/**
 * @brief Determines if the current user is enabled to start recording.
 * @param isCloud YES to determine whether to enable the cloud recording, NO local recording.
 * @return If the value of cloud_recording is set to YES and the cloud recording is enabled, it returns ZoomSDKError_Success. If the value of cloud_recording is set to NO and the local recording is enabled, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)canStartRecording:(BOOL)isCloud;
/**
 * @brief Determines if the current user owns the authority to change the recording permission of the others.
 * @return If the user own the authority, it returns ZoomSDKError_Success. Otherwise, this function returns an error. 
 */
- (ZoomSDKError)canAllowDisallowRecording;

/**
 * @brief Starts cloud recording.
 * @param start Set it to YES to start cloud recording, NO to stop recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error. 
 */
- (ZoomSDKError)startCloudRecording:(BOOL)start;

/**
 * @brief Starts recording on the local computer.
 * @param startTimestamp The timestamps when start recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.  
 */
- (ZoomSDKError)startRecording:(time_t*)startTimestamp;

/**
 * @brief Stops recording on the local computer.
 * @param stopTimestamp The timestamps when stop recording.
 * @return If the function succeeds, it returns SDKErr_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)stopRecording:(time_t*)stopTimestamp;

/**
 * @brief Determines if the user owns the authority to enable the local recording.
 * @param userid Specify the user ID.
 * @return If the specified user is enabled to start local recording, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)isSupportLocalRecording:(unsigned int)userid;

/**
 * @brief Give the specified user authority for local recording.
 * @param allow YES if allowing user to record on the local computer, NO otherwise.
 * @param userid Specify the user ID.
 * @return If the specified user is enabled to start local recording, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)allowLocalRecording:(BOOL)allow User:(unsigned int)userid;

/**
 * @brief Sets whether to enable custom local recording notification.
 * @param request YES to receive callback of onCustomizedRecordingSourceReceived, NO otherwise.	
 * @return If the specified user is enabled to start local recording, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)requestCustomizedLocalRecordingNotification:(BOOL)request;

/**
 * @brief Pause cloud recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)pauseCloudRecording;
/**
 * @brief Resume cloud recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)resumeCloudRecording;
/**
 * @brief Pause local recording
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)pauseLocalRecording;
/**
 * @brief Resume local recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)resumeLocalRecording;

/**
 * @brief Starts rawdata recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)startRawRecording;

/**
 * @brief Stops rawdata recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)stopRawRecording;

/**
 * @brief Determines if the specified user is enabled to start raw recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)canStartRawRecording;

/**
 * @brief Gets current cloud recording status.
 * @return The recording status.
 */
- (ZoomSDKRecordingStatus)getCloudRecordingStatus;

/**
 * @brief Determines if the user owns the authority to enable the local recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)isSupportRequestLocalRecordingPrivilege;

/**
 * @brief Sends a request to ask the host to start cloud recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)requestStartCloudRecording;

/**
 * @brief Sends a request to enable the SDK to start local recording.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)requestLocalRecordingPrivilege;

/**
 * @brief Determines if the smart recording feature is enabled in the meeting.
 * @return YES if the feature enabled. Otherwise, NO.
 */
- (BOOL)isSmartRecordingEnabled;

/**
 * @brief Whether the current user can enable the smart recording feature.
 * @return YES if the current user can enable the smart recording feature. Otherwise, NO.
 */
- (BOOL)canEnableSmartRecordingFeature;

/**
 * @brief Enables the smart recording feature.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)enableSmartRecording;
@end
NS_ASSUME_NONNULL_END
