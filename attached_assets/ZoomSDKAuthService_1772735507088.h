/**
 * @file ZoomSDKAuthService.h
 * @brief Interface for Zoom SDK authorization and login services.
 */

#import <Cocoa/Cocoa.h>
#import <ZoomSDK/ZoomSDKErrors.h>
#import <ZoomSDK/ZoomSDKNotificationServiceController.h>

NS_ASSUME_NONNULL_BEGIN
/**
 * @class ZoomSDKAuthContext
 * @brief Context object containing authentication token information.
 */
@interface ZoomSDKAuthContext : NSObject

/**
 * @brief JWT token used for SDK authentication.
 */
@property(nonatomic, copy, nullable) NSString *jwtToken;

/**
 * @brief Public app key used for SDK authentication. Alternative to JWT token.
 */
@property(nonatomic, copy, nullable) NSString *publicAppKey;
@end


/**
 * @class ZoomSDKWebinarRegistrationExplainInfo
 * @brief Holds explanatory information related to webinar registration.
 */
@interface ZoomSDKWebinarRegistrationExplainInfo : NSObject
/**
 * @brief Content of the webinar Registration Explain Info
 */
@property(nonatomic, copy, readonly, nullable) NSString *content;
/**
 * @brief AccountOwnerLink of the webinar Registration Explain Info
 */
@property(nonatomic, copy, readonly, nullable) NSString *accountOwnerLink;
/**
 * @brief TermLink of the webinar Registration Explain Info
 */
@property(nonatomic, copy, readonly, nullable) NSString *termLink;
/**
 * @brief PolicyLink of the webinar Registration Explain Info
 */
@property(nonatomic, copy, readonly, nullable) NSString *policyLink;

@end


/**
 * @class ZoomSDKAccountInfo
 * @brief Provides information about the currently logged-in user account.
 */
@interface ZoomSDKAccountInfo : NSObject
/**
 * @brief Gets screen name of the user.
 * @return If the function succeeds, it returns screen name of the user. Otherwise, this function fails and returns nil.
 */
- (NSString*_Nullable) getDisplayName;
/**
 * @brief Gets the type of user.
 * @return The user type.
 */
- (ZoomSDKUserType) getSDKUserType;
@end



@protocol ZoomSDKAuthDelegate;
/**
 * @class ZoomSDKAuthService
 * @brief Provides APIs to authorize the Zoom SDK and manage login or logout.
 */
@interface ZoomSDKAuthService : NSObject
{
    id<ZoomSDKAuthDelegate> _delegate;
}
/**
 * @brief Delegate to receive auth and login or logout events.
 */
@property (assign, nonatomic, nullable) id<ZoomSDKAuthDelegate> delegate;

/**
 * @brief New authenticate SDK.
 * @param authContext A ZoomSDKAuthContext object containing auth information.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 * @note If the jwttoken expired, will return \link ZoomSDKAuthDelegate::onZoomAuthIdentityExpired \endlink callback.
 */
- (ZoomSDKError)sdkAuth:(ZoomSDKAuthContext*)authContext;

/**
 * @brief Determines if SDK is authorized.
 * @return YES if it is authorized. Otherwise, NO.
 */
- (BOOL)isAuthorized;

/**
* @brief Generate SSO login web url.
* @param prefixOfVanityUrl The prefix of vanity url.
* @return If the function succeeds, it returns url of can launch app. Otherwise, this function fails and returns nil.
*/
- (NSString*_Nullable)generateSSOLoginWebURL:(NSString*)prefixOfVanityUrl;

/**
 * @brief Login ZOOM with uri protocol.
 * @param uriProtocol For the parameter to be used for sso account login.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)SSOLoginWithWebUriProtocol:(NSString*)uriProtocol;
/**
 * @brief Logout ZOOM.
 * @return If the function succeeds, it returns ZoomSDKError_Success, meanwhile it will call asynchronously onZoomSDKLogout. Otherwise, this function returns an error.
 */
- (ZoomSDKError)logout;

/**
 * @brief Gets account information of the user. 
 * @return When user logged in, it returns ZoomSDKAccountInfo object if the function calls successfully. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKAccountInfo *_Nullable)getAccountInfo;

/**
 * @brief Gets SDK identity.
 * @return If the function succeeds, it returns the SDK identity. Otherwise, this function fails and returns nil.
 */
- (NSString*_Nullable)getSDKIdentity;

/**
 * @brief Enables or disable auto register notification service. This is enabled by default.
 * @param enable YES if enabled, NO otherwise.
 */
- (void)enableAutoRegisterNotificationServiceForLogin:(BOOL)enable;

/**
 * @brief Register notification service.
 * @param accessToken Initialize parameter of notification service.
 * @return If the function succeeds, it returns @c ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)registerNotificationService:(NSString*)accessToken;

/**
 * @brief Unregister notification service.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unregisterNotificationService;

/**
 * @brief Gets notification service controller interface.
 * @return If the function succeeds, it returns ZoomSDKZpnsServiceController object. Otherwise, this function fails and returns nil.
 */
- (ZoomSDKNotificationServiceController *)getNotificationServiceController;
@end


/**
 * @protocol ZoomSDKAuthDelegate
 * @brief Delegate protocol to receive SDK authorization and login or logout events.
 */
@protocol ZoomSDKAuthDelegate <NSObject>

@required
/**
 * @brief Specify to get the response of ZOOM SDK authorization. 
 * @param returnValue Notify user that the authentication status changes.
 *
 */
- (void)onZoomSDKAuthReturn:(ZoomSDKAuthError)returnValue;

/**
 * @brief Specify to get the response of ZOOM SDK authorization identity expired.
 */
 - (void)onZoomAuthIdentityExpired;

@optional
/**
 * @brief Specify to get the response of ZOOM SDK Login.
 * @param loginStatus Notify user of login status.
 * @param reason Notify user that the failed reason.
 */
- (void)onZoomSDKLoginResult:(ZoomSDKLoginStatus)loginStatus failReason:(ZoomSDKLoginFailReason)reason;
/**
 * @brief Specify to get the response of ZOOM SDK logout.
 */
- (void)onZoomSDKLogout;

/**
 * @brief Specify to get the response if ZOOM identity is expired.
 * @note User will be forced to logout once ZOOM SDK identity expired.
 */
- (void)onZoomIdentityExpired;

/**
 * @brief Notification service status changed callback.
 * @param status The value of transfer meeting service.
 * @param error Connection Notification service fail error code.
 */
- (void)onNotificationServiceStatus:(ZoomSDKNotificationServiceStatus)status error:(ZoomSDKNotificationServiceError)error;
@end
NS_ASSUME_NONNULL_END
