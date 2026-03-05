/**
 * @file ZoomSDKRawDataController.h
 * @brief Interface definitions for managing Zoom SDK raw data.
 *
 * This file provides interfaces for accessing and controlling audio, video,
 * and share raw data streams, including subscription control and rendering integration.
 */


#import <Foundation/Foundation.h>
#import <ZoomSDK/ZoomSDKErrors.h>
#import <ZoomSDK/ZoomSDKRenderer.h>

@class ZoomSDKRawDataVideoSourceController;
@class ZoomSDKRawDataShareSourceController;
@class ZoomSDKRawDataAudioSourceController;

NS_ASSUME_NONNULL_BEGIN
/**
 * @class ZoomSDKAudioRawData
 * @brief Represents audio raw data received from the SDK.
 */
@interface ZoomSDKAudioRawData : NSObject
/**
 * @brief Gets if this object can add ref.
 * @return If can add ref, it returns YES. Otherwise, NO.
 */
- (BOOL)canAddRef;
/**
 * @brief Adds reference for this object, if you doesn't add ref, this object is released when the callback response ends.
 * @return If the function succeeds, it returns YES. Otherwise, NO.
 */
- (BOOL)addRef;
/**
 * @brief Release the object, if you has add ref, remeber to call this api to release when you wantn't to use this object.
 * @return If the function succeeds, it returns reference count of this object. Otherwise, returns -1.
 */
- (int)releaseData;
/**
 * @brief Gets the buffer data.
 * @return If the function succeeds, it returns the buffer data. Otherwise, this function fails and returns nil.
 */
- (char*_Nullable)getBuffer;
/**
 * @brief Gets the buffer length of this data.
 * @return If the function succeeds, it returns the buffer length of this data. Otherwise, returns 0.
 */
- (unsigned int)getBufferLen;
/**
 * @brief Gets the sample rate of this data.
 * @return If the function succeeds, it returns the sample rate of this data. Otherwise, returns 0.
 */
- (unsigned int)getSampleRate;
/**
 * @brief Gets the channel number of this data.
 * @return If the function succeeds, it returns the channel number of this data. Otherwise, returns 0.
 */
- (unsigned int)getChannelNum;
/**
 * @brief Gets the raw's timestamp data.
 * @return If the function succeeds, it returns the millisecond timestamp. Otherwise, returns 0.
 */
- (long long)getTimeStamp;
@end

/**
 * @protocol ZoomSDKAudioRawDataDelegate
 * @brief Delegate to handle raw audio data.
 */
@protocol ZoomSDKAudioRawDataDelegate <NSObject>
/**
 * @brief Notify to receive the mixed audio raw data.
 * @param data The received audio raw data.
 */
- (void)onMixedAudioRawDataReceived:(ZoomSDKAudioRawData*_Nullable)data;
/**
 * @brief Notify to receive the one way audio raw data.
 * @param data The received audio raw data.
 * @param nodeID The user ID of received user's data.
 * @deprecated Use \link ZoomSDKAudioRawDataDelegate::onOneWayAudioRawDataReceived:userID: \endlink instead.
 */
- (void)onOneWayAudioRawDataReceived:(ZoomSDKAudioRawData*_Nullable)data nodeID:(unsigned int)nodeID DEPRECATED_MSG_ATTRIBUTE("Use -onOneWayAudioRawDataReceived: userID: instead");
/**
 * @brief Notify to receive the one way audio raw data.
 * @param data The received audio raw data.
 * @param userID The user ID of received user's data.
 */
- (void)onOneWayAudioRawDataReceived:(ZoomSDKAudioRawData*_Nullable)data userID:(unsigned int)userID;
/**
 * @brief Notify to receive the share audio raw data.
 * @param data The received audio raw data.
 */
- (void)onShareAudioRawDataReceived:(ZoomSDKAudioRawData*_Nullable)data DEPRECATED_MSG_ATTRIBUTE("Use -onShareAudioRawDataReceived: userID: instead");
/**
 * @brief Notify to receive the share audio raw data.
 * @param data The received audio raw data.
 * @param userID The user ID of received user's data.
 */
- (void)onShareAudioRawDataReceived:(ZoomSDKAudioRawData*_Nullable)data userID:(unsigned int)userID;
/**
 * @brief Invoked when individual interpreter's raw audio data received.
 * @param data Raw audio data.
 * @param languageName The interpreter language name of the audio raw data.
 */
- (void)onOneWayInterpreterAudioRawDataReceived:(ZoomSDKAudioRawData*)data strLanguageName:(NSString*)languageName;
@end


/**
 * @class ZoomSDKAudioRawDataHelper
 * @brief Helper to subscribe or unsubscribe audio raw data.
 */
@interface ZoomSDKAudioRawDataHelper : NSObject
{
    id<ZoomSDKAudioRawDataDelegate> _delegate;
}
/**
 * @brief Delegate for receiving audio raw data events.
 */
@property(nonatomic, assign, nullable)id<ZoomSDKAudioRawDataDelegate> delegate;
/**
 * @brief If audioWithInterpreters is YES, it means that you want to get the audio data of interpreters. Otherwise, NO.
 * @note if audioWithInterpreters is YES, it will cause your local interpreter related functions to be unavailable.
 */
@property(nonatomic, assign)BOOL audioWithInterpreters;
/**
 * @brief Starts the audio raw data process.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)subscribe;
/**
 * @brief Stops the audio raw data process.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unSubscribe;
@end

/**
 * @class ZoomSDKRawDataController
 * @brief Controller for managing raw data helpers and sources.
 */
@interface ZoomSDKRawDataController : NSObject
{
    ZoomSDKAudioRawDataHelper*              _audioRawDataHelper;
    ZoomSDKRawDataVideoSourceController*    _rawDataVideoSourceHelper;
    ZoomSDKRawDataShareSourceController*    _rawDataShareSourceHelper;
    ZoomSDKRawDataAudioSourceController*    _rawDataAudioSourceHelper;
}
/**
 * @brief Query if the user has raw data license.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise not ZoomSDKError_NoPermission.
 */
- (ZoomSDKError)hasRawDataLicense;
/**
 * @brief Gets the object of ZoomSDKAudioRawDataHelper.
 * @param audioRawDataHelper The point to the object of ZoomSDKAudioRawDataHelper.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)getAudioRawDataHelper:(ZoomSDKAudioRawDataHelper*_Nullable* _Nonnull)audioRawDataHelper;

/**
 * @brief Gets the object of ZoomSDKRawDataVideoSourceController.
 * @param videoRawDataSendHelper The point to the object of ZoomSDKRawDataVideoSourceController.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)getRawDataVideoSourceHelper:(ZoomSDKRawDataVideoSourceController*_Nullable*_Nonnull)videoRawDataSendHelper;

/**
 * @brief Gets the object of ZoomSDKRawDatShareSourceController.
 * @param shareRawDataSendHelper The point to the object of ZoomSDKRawDatShareSourceController.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)getRawDataShareSourceHelper:(ZoomSDKRawDataShareSourceController*_Nullable*_Nonnull)shareRawDataSendHelper;

/**
 * @brief Gets the object of ZoomSDKRawDataAudioSourceController.
 * @param audioRawDataSendHelper The point to the object of ZoomSDKRawDataAudioSourceController.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)getRawDataAudioSourceHelper:(ZoomSDKRawDataAudioSourceController*_Nullable*_Nonnull)audioRawDataSendHelper;

/**
 * @brief Creat the object of ZoomSDKRenderer.
 * @param render The point to the object of ZoomSDKRenderer.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)createRender:(ZoomSDKRenderer*_Nullable*_Nonnull)render;

/**
 * @brief Destory the object of ZoomSDKRenderer.
 * @param render The point to the object of ZoomSDKRenderer.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)destroyRender:(ZoomSDKRenderer*_Nullable)render;
@end
NS_ASSUME_NONNULL_END
