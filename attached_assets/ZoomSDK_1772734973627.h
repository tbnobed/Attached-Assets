/**
 * @file ZoomSDK.h
 * @brief Main interface header for the Zoom SDK on macOS.
 *
 * This header aggregates all major components and services provided by the Zoom SDK, including authentication,
 * meeting management, settings, network handling, and raw data access.
 * Use this header to initialize and interact with the Zoom SDK.
 */

#import <ZoomSDK/ZoomSDKErrors.h>
#import <ZoomSDK/ZoomSDKAuthService.h>
#import <ZoomSDK/ZoomSDKMeetingService.h>
#import <ZoomSDK/ZoomSDKSettingService.h>
#import <ZoomSDK/ZoomSDKPremeetingService.h>
#import <ZoomSDK/ZoomSDKNetworkService.h>
#import <ZoomSDK/ZoomSDKVideoContainer.h>
#import <ZoomSDK/ZoomSDKShareContainer.h>
#import <ZoomSDK/ZoomSDKRawDataVideoSourceController.h>
#import <ZoomSDK/ZoomSDKRawDataController.h>
#import <ZoomSDK/ZoomSDKRawDataShareSourceController.h>
#import <ZoomSDK/ZoomSDKRawDataAudioSourceController.h>
#import <ZoomSDK/ZoomSDKReminderController.h>
#import <ZoomSDK/ZoomSDKMeetingSmartSummaryController.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * @brief Enumerates of supported SDK locales.
 */
typedef enum
{
    /** Default locale. */
    ZoomSDKLocale_Def = 0,
    /** Chinese locale. */
    ZoomSDKLocale_CN  = 1,
}ZoomSDKLocale;

/**
 * @class ZoomSDKInitParams
 * @brief The configuration object used to initialize the Zoom SDK.
 */
@interface ZoomSDKInitParams : NSObject
{
    BOOL                        _needCustomizedUI;
    BOOL                        _enableLog;
    int                         _logFileSize;
    ZoomSDKLocale               _appLocale;
    NSString*                   _preferedLanguage;
    NSString*                   _customLocalizationFileName;
    NSString*                   _customLocalizationFilePath;
    NSString*                   _zoomDomain;
}
/**
 * @brief Whether to use customized UI mode instead of the default Zoom UI.
 */
@property (assign, nonatomic) BOOL needCustomizedUI;
/**
 * @brief Whether to enable SDK internal logging (default max 5MB).
 */
@property (assign, nonatomic) BOOL enableLog;
/**
 * @brief Maximum size of log files in MB. Range: 1~50 MB.
 */
@property (assign, nonatomic) int logFileSize;
/**
 * @brief Locale setting for the SDK.
 */
@property (assign, nonatomic) ZoomSDKLocale appLocale;
/**
 * @brief Preferred language for the SDK.
 * @note if user does not specify the language, it will follow up the systematical language.
 */
@property (retain, nonatomic, nullable) NSString *preferedLanguage;
/**
 * @brief Name of the custom localization file (e.g., Localizable.strings).
 */
@property (retain, nonatomic, nullable) NSString *customLocalizationFileName;
/**
 * @brief Path to the custom localization file. The default is under ZoomSDK.framework/Resources.
 */
@property (retain, nonatomic, nullable) NSString *customLocalizationFilePath;
/**
 * @brief SDK wrapper type (reserved for internal use).
 */
@property (assign, nonatomic) int wrapperType;
/**
 * @brief Zoom domain for SDK communication (e.g., zoom.us).
 */
@property (retain, nonatomic, nullable) NSString *zoomDomain;
/**
 * @brief Gets the list of supported language codes.
 * @return An array of supported language codes. Otherwise, this function fails and returns nil.
 */
- (NSArray*)getLanguageArray;
@end


/**
 * @class ZoomSDK
 * @brief Singleton class to access and manage all major Zoom SDK services.
 */
@interface ZoomSDK : NSObject
{
    ZoomSDKMeetingService  *_meetingService;
    ZoomSDKAuthService     *_authService;
    ZoomSDKSettingService  *_settingService;
    ZoomSDKPremeetingService *_premeetingService;
    ZoomSDKNetworkService    *_networkService;
    ZoomSDKRawDataMemoryMode _videoRawDataMode;
    ZoomSDKRawDataMemoryMode _shareRawDataMode;
    ZoomSDKRawDataMemoryMode _audioRawDataMode;
    ZoomSDKReminderController *_reminderController;
}
/**
 * @brief Whether to enable intermediate mode for raw data.
 */
@property (assign, nonatomic) BOOL enableRawdataIntermediateMode;
/**
 * @brief Memory mode for handling raw video data.
 */
@property (assign, nonatomic) ZoomSDKRawDataMemoryMode videoRawDataMode;
/**
 * @brief Memory mode for handling raw share data.
 */
@property (assign, nonatomic) ZoomSDKRawDataMemoryMode shareRawDataMode;
/**
 * @brief Memory mode for handling raw audio data.
 */
@property (assign, nonatomic) ZoomSDKRawDataMemoryMode audioRawDataMode;
/**
 * @brief Gets the shared SDK singleton instance.
 * @return The ZoomSDK singleton instance. Otherwise, this function fails and returns nil.
 * @note The shared SDK is instantiated only once over the application's lifespan.
 */
+ (ZoomSDK*)sharedSDK;

/**
 * @brief Initialize the Zoom SDK with specified parameters.
 * @param initParams Specify the init params.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)initSDKWithParams:(ZoomSDKInitParams*)initParams;

/**
 * @brief Uninitialize and clean up the Zoom SDK.
 */
- (void)unInitSDK;

/**
 * @brief Gets the authentication service.
 * @return An instance of ZoomSDKAuthService for SDK authentication. Otherwise, this function fails and returns nil.
 * @note The Zoom SDK cannot be called unless the authentication service is called successfully.
 */
- (ZoomSDKAuthService*)getAuthService;

/**
 * @brief Gets the meeting service.
 * @return An instance of ZoomSDKMeetingService. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKMeetingService*_Nullable)getMeetingService;

/**
 * @brief Gets the setting service.
 * @return An instance of ZoomSDKSettingService. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKSettingService*_Nullable)getSettingService;

/**
 * @brief Gets the pre-meeting service.
 * @return An instance of ZoomSDKPremeetingService. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKPremeetingService*_Nullable)getPremeetingService;

/**
 * @brief Gets the network service.
 * @return An instance of ZoomSDKNetworkService. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKNetworkService*_Nullable)getNetworkService;

/**
 * @brief Gets the raw data controller.
 * @return An instance of ZoomSDKRawDataController. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKRawDataController*_Nullable)getRawDataController;

/**
 * @brief Gets the reminder controller instance.
 * @return An instance of ZoomSDKReminderController. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKReminderController*_Nullable)getReminderHelper;


/**
 * @brief Gets the smart summary controller.
 * @return An instance of ZoomSDKMeetingSmartSummaryController. Otherwise, this function fails and returns nil.
 * @deprecated This method is no longer used.
 */
- (ZoomSDKMeetingSmartSummaryController*_Nullable)getMeetingSmartSummaryController DEPRECATED_MSG_ATTRIBUTE("No longer used");

/**
 * @brief Gets the SDK version number.
 * @return A string representing the SDK version number. Otherwise, this function fails and returns nil.
 */
- (NSString*_Nullable)getSDKVersionNumber;

/**
 * @brief Switch to a new domain.
 * @param newDomain The new domain.
 * @param force Whether to force switch.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)switchDomain:(NSString*)newDomain force:(BOOL)force;
@end
NS_ASSUME_NONNULL_END
