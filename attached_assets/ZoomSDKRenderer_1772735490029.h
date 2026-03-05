/**
 * @file ZoomSDKRenderer.h
 * @brief Interface for rendering and receiving raw video data in Zoom SDK.
 */

#import <Foundation/Foundation.h>
#import <ZoomSDK/ZoomSDKErrors.h>

NS_ASSUME_NONNULL_BEGIN
@class ZoomSDKYUVRawDataI420;

/**
 * @class ZoomSDKYUVRawDataI420
 * @brief Represents raw YUV I420 format data received from subscribed video stream.
 */
@interface ZoomSDKYUVRawDataI420 : NSObject
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
 * @brief Gets the Y data.
 * @return If the function succeeds, it returns the Y data. Otherwise, this function fails and returns nil.
 */
- (char*_Nullable)getYBuffer;
/**
 * @brief Gets the U data.
 * @return If the function succeeds, it returns the U data. Otherwise, this function fails and returns nil.
 */
- (char*_Nullable)getUBuffer;
/**
 * @brief Gets the V data.
 * @return If the function succeeds, it returns the V data. Otherwise, this function fails and returns nil.
 */
- (char*_Nullable)getVBuffer;
/**
 * @brief Gets the buffer data.
 * @return If the function succeeds, it returns the buffer data. Otherwise, this function fails and returns nil.
 */
- (char*_Nullable)getBuffer;
/**
 * @brief Gets video alpha mask data buffer.
 * @return Buffer address if alpha data exists. Otherwise, this function fails and returns nil.
 */
- (char*_Nullable)getAlphaBuffer;
/**
 * @brief Gets the buffer length of this data.
 * @return If the function succeeds, it returns the buffer length of this data. Otherwise, returns 0.
 */
- (unsigned int)getBufferLen;
/**
 * @brief Gets the alpha buffer length.
  * @return The length of alpha data.
 */
- (unsigned int)getAlphaBufferLen;
/**
 * @brief Gets if this data is limited I420 format.
 * @return If is limited I420 format, it returns YES. Otherwise, NO.
 */
- (BOOL)isLimitedI420;
/**
 * @brief Gets the stream width of this data.
 * @return If the function succeeds, it returns the stream width of this data. Otherwise, returns 0.
 */
- (unsigned int)getStreamWidth;
/**
 * @brief Gets the stream height of this data.
 * @return If the function succeeds, it returns the stream height of this data. Otherwise, returns 0.
 */
- (unsigned int)getStreamHeight;
/**
 * @brief Gets the rotation of this data.
 * @return If the function succeeds, it returns the rotation of this data. Otherwise, returns 0.
 */
- (unsigned int)getRotation;
/**
 * @brief Gets the source ID of this data.
 * @return If the function succeeds, it returns the source ID of this data. Otherwise, returns 0.
 */
- (unsigned int)getSourceID;
/**
 * @brief Gets the raw data's timestamp.
 * @return If the function succeeds, it returns the millisecond timestamp. Otherwise, returns 0.
 */
- (long long)getTimeStamp;
@end


/**
 * @protocol ZoomSDKRendererDelegate
 * @brief Callback interface for receiving video render events.
 */
@protocol ZoomSDKRendererDelegate <NSObject>
/**
 * @brief Notify if subscribed user's video data becomes available.
 */
- (void)onSubscribedUserDataOn;
/**
 * @brief Notify if subscribed user's video data becomes unavailable.
 */
- (void)onSubscribedUserDataOff;
/**
 * @brief Notify if subscribed user has left.
 * @deprecated This method is no longer used.
 */
- (void)onSubscribedUserLeft DEPRECATED_MSG_ATTRIBUTE("No longer used");
/**
 * @brief Notify if raw data is received for rendering.
 * @param data The rawData status.
 */
- (void)onRawDataReceived:(ZoomSDKYUVRawDataI420*_Nullable)data;
/**
 * @brief Notify if the renderer is being destroyed.
 */
- (void)onRendererBeDestroyed;
@end


/**
 * @class ZoomSDKRenderer
 * @brief Subscribe to raw video or share data and handle rendering.
 */
@interface ZoomSDKRenderer : NSObject
{
    unsigned int                  _subscribeID;
    ZoomSDKRawDataType            _rawDataType;
    ZoomSDKResolution             _resolution;
    id<ZoomSDKRendererDelegate>   _delegate;
}
/**
 * @brief The delegate to receive rendering events.
 */
@property(nonatomic, assign, nullable)id<ZoomSDKRendererDelegate> delegate;

/**
 * @brief Subscribe to receive raw data.
 * @param subscribeID The subscribe ID of the raw data user want to receive. If subscribe video, the subscribeID is user ID. If subscribe the sharing, the subscribeID is share source ID.
 * @param rawDataType The type of raw data user want to receive.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)subscribe:(unsigned int)subscribeID rawDataType:(ZoomSDKRawDataType)rawDataType;

/**
 * @brief Unsubscribe to receive raw data.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)unSubscribe;

/**
 * @brief Gets the type of raw data.
 * @return The type of raw data.
 */
- (ZoomSDKRawDataType)getRawDataType;

/**
 * @brief Gets the subscribe ID of raw data user is subscribing to.
 * @return If the function succeeds, it returns the subscribe ID. Otherwise, returns -1.
 */
- (unsigned int)getSubscribeID;

/**
 * @brief Gets the resolution of raw data.
 * @return The resolution.
 */
- (ZoomSDKResolution)getResolution;

/**
 * @brief Sets the resolution of raw data.
 * @param resolution The resolution of raw data user want to receive.
 * @return If the function succeeds, it returns ZoomSDKError_Success. Otherwise, this function returns an error.
 */
- (ZoomSDKError)setResolution:(ZoomSDKResolution)resolution;
@end

NS_ASSUME_NONNULL_END
