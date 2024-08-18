/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/screen_capture.h"

#include "slib/system/system.h"
#include "slib/media/audio_data.h"
#include "slib/graphics/util.h"
#include "slib/graphics/platform.h"
#include "slib/core/event.h"
#include "slib/core/shared.h"
#include "slib/core/safe_static.h"
#include "slib/core/scoped_buffer.h"

#include <AppKit/AppKit.h>
#ifdef __MAC_13_0
#	include <ScreenCaptureKit/ScreenCaptureKit.h>
#	define USE_SCREEN_CAPTURE_KIT
#endif
#ifdef __MAC_14_0
#	define USE_SCREENSHOT_MANAGER
#endif

#ifdef USE_SCREEN_CAPTURE_KIT
API_AVAILABLE(macos(13.0))
@interface SLIBScreenCapture : NSObject<SCStreamDelegate, SCStreamOutput>
{
	@public slib::WeakRef<slib::CRef> m_capture;
}
@end

API_AVAILABLE(macos(13.0))
@interface SLIBSCStream : SCStream
{
	@public sl_uint32 m_index;
}
@end
#endif

namespace slib
{

	namespace
	{
		class Helper
		{
		public:
			Ref<Image> getImage(CGImageRef cgImage, sl_uint32 dstWidth, sl_uint32 dstHeight)
			{
				Ref<Bitmap>& bitmap = m_bitmapCache;
				Ref<Canvas>& canvas = m_canvasCache;
				CGContextRef& context = m_contextCache;
				if (bitmap.isNull() || canvas.isNull() || bitmap->getWidth() < dstWidth || bitmap->getHeight() < dstHeight) {
					bitmap = Bitmap::create(dstWidth, dstHeight);
					if (bitmap.isNull()) {
						return sl_null;
					}
					canvas = bitmap->getCanvas();
					if (canvas.isNull()) {
						return sl_null;
					}
					context = GraphicsPlatform::getCanvasHandle(canvas.get());
					CGContextTranslateCTM(context, 0, (CGFloat)dstHeight);
					CGContextScaleCTM(context, 1, -1);
				}
				if (context) {
					CGContextDrawImage(context, CGRectMake(0, 0, (CGFloat)dstWidth, (CGFloat)dstHeight), cgImage);
					return Image::createCopyBitmap(bitmap, 0, bitmap->getHeight() - dstHeight, dstWidth, dstHeight);
				}
				return sl_null;
			}

		private:
			Ref<Bitmap> m_bitmapCache;
			Ref<Canvas> m_canvasCache;
			CGContextRef m_contextCache;

		public:
			Mutex m_lock;
#ifdef USE_SCREEN_CAPTURE_KIT
			Mutex m_lockShareableContent;
			id m_shareableContent;
#endif
		};

		SLIB_SAFE_STATIC_GETTER(Helper, GetHelper)

#ifdef USE_SCREEN_CAPTURE_KIT
		static void ProcessCapturedScreen(CMSampleBufferRef sampleBuffer, const Function<void(BitmapData&)>& callback)
		{
			CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
			if (!imageBuffer) {
				return;
			}
			OSType pixelFormat = CVPixelBufferGetPixelFormatType(imageBuffer);
			if (pixelFormat != kCVPixelFormatType_32BGRA) {
				return;
			}
			sl_uint32 width = (sl_uint32)(CVPixelBufferGetWidth(imageBuffer));
			if (!width) {
				return;
			}
			sl_uint32 height = (sl_uint32)(CVPixelBufferGetHeight(imageBuffer));
			if (!width || !height) {
				return;
			}
			if (CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess) {
				BitmapData bd;
				bd.format = BitmapFormat::BGRA;
				bd.width = width;
				bd.height = height;
				bd.data = CVPixelBufferGetBaseAddress(imageBuffer);
				bd.pitch = (sl_uint32)(CVPixelBufferGetBytesPerRow(imageBuffer));
				callback(bd);
				CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
			}
		}
#endif

#ifdef USE_SCREENSHOT_MANAGER
		API_AVAILABLE(macos(14.0))
		static Ref<Image> TakeScreenshot(Helper* helper, SCDisplay* display, sl_uint32 width, sl_uint32 height)
		{
			SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];
			SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
			[config setWidth:width];
			[config setHeight:height];
			[config setPixelFormat:kCVPixelFormatType_32BGRA];
			Ref<Event> ev = Event::create();
			if (ev.isNull()) {
				return nil;
			}
			Shared< AtomicRef<Image> > ret = Shared< AtomicRef<Image> >::create();
			if (ret.isNull()) {
				return nil;
			}
			[SCScreenshotManager captureSampleBufferWithFilter:filter configuration:config completionHandler:^(CMSampleBufferRef sampleBuffer, NSError* error) {
				if (sampleBuffer) {
					ProcessCapturedScreen(sampleBuffer, [&ret](BitmapData& bd) {
						*ret = Image::create(bd);
					});
					CFRelease(sampleBuffer);
				}
				ev->set();
			}];
			ev->wait(5000);
			return ret->release();
		}
#endif

#ifdef USE_SCREEN_CAPTURE_KIT
		API_AVAILABLE(macos(13.0))
		static SCShareableContent* GetShareableContent(Helper* helper)
		{
			Ref<Event> ev = Event::create();
			if (ev.isNull()) {
				return nil;
			}
			[SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content, NSError* err) {
				{
					MutexLocker lock(&(helper->m_lockShareableContent));
					helper->m_shareableContent = content;
				}
				ev->set();
			}];
			ev->wait(3000);
			MutexLocker lock(&(helper->m_lockShareableContent));
			return helper->m_shareableContent;
		}
#endif
#ifdef USE_SCREENSHOT_MANAGER
		API_AVAILABLE(macos(13.0))
		static SCDisplay* GetShareableDisplay(Helper* helper, CGDirectDisplayID displayId)
		{
			SCShareableContent* content = GetShareableContent(helper);
			if (content == nil) {
				return nil;
			}
			for (SCDisplay* display in [content displays]) {
				if ([display displayID] == displayId) {
					return display;
				}
			}
			return nil;
		}
#endif

		static sl_bool GetDisplayID(NSScreen* screen, CGDirectDisplayID& ret)
		{
			NSDictionary* desc = [screen deviceDescription];
			if (desc == nil) {
				return sl_false;
			}
			NSNumber* num = [desc objectForKey:@"NSScreenNumber"];
			if (num == nil) {
				return sl_false;
			}
			ret = (CGDirectDisplayID)([num unsignedIntValue]);
			return sl_true;
		}

		static void GetScreenInfo(NSScreen* screen, CaptureScreenInfo& info)
		{
			NSSize size = screen.frame.size;
			info.screenWidth = size.width;
			info.screenHeight = size.height;
			info.scaleFactor = (float)([screen backingScaleFactor]);
		}

		static sl_bool TakeScreenshot(Screenshot& _out, Helper* helper, NSScreen* screen, sl_uint32 maxWidth, sl_uint32 maxHeight)
		{
			CGDirectDisplayID displayID;
			if (!(GetDisplayID(screen, displayID))) {
				return sl_false;
			}
			GetScreenInfo(screen, _out);
			sl_uint32 width = (sl_uint32)(_out.screenWidth * _out.scaleFactor);
			sl_uint32 height = (sl_uint32)(_out.screenHeight * _out.scaleFactor);
			GraphicsUtil::toSmallSize(width, height, maxWidth, maxHeight);
#ifdef USE_SCREENSHOT_MANAGER
			if (@available(macos 14.0, *)) {
				SCDisplay* display = GetShareableDisplay(helper, displayID);
				if (display != nil) {
					_out.image = TakeScreenshot(helper, display, width, height);
					return _out.image.isNotNull();
				}
			} else
#endif
			{
				CGImageRef cgImage = CGDisplayCreateImage(displayID);
				if (cgImage) {
					_out.image = helper->getImage(cgImage, width, height);
					CGImageRelease(cgImage);
					return _out.image.isNotNull();
				}
			}
			return sl_false;
		}

#ifdef USE_SCREEN_CAPTURE_KIT
		static sl_bool IsMutedFloatData(const void* data, sl_size n)
		{
			sl_uint32 m = (sl_uint32)(n >> 2);
			float* f = (float*)data;
			for (sl_uint32 i = 0; i < m; i++) {
				if (f[i] != 0.0f) {
					return sl_false;
				}
			}
			return sl_true;
		}

		class API_AVAILABLE(macos(13.0)) ScreenCaptureImpl : public ScreenCapture
		{
		public:
			SLIBScreenCapture* m_object;
			NSArray<SLIBSCStream*>* m_streams;
			List<CaptureScreenInfo> m_screenInfos;

			dispatch_queue_t m_queueScreen;
			dispatch_queue_t m_queueAudio;

		public:
			ScreenCaptureImpl()
			{
			}

			~ScreenCaptureImpl()
			{
				release();
			}

		public:
			static Ref<ScreenCaptureImpl> create(const ScreenCaptureParam& param)
			{
				if (!(param.flagCaptureScreen || param.flagCaptureAudio)) {
					return sl_null;
				}
				Helper* helper = GetHelper();
				if (!helper) {
					return sl_null;
				}
				SLIBScreenCapture* object = [[SLIBScreenCapture alloc] init];
				if (object == nil) {
					return sl_null;
				}
				dispatch_queue_t queueScreen = dispatch_queue_create(sl_null, DISPATCH_QUEUE_SERIAL);
				if (queueScreen == nil) {
					return sl_null;
				}
				dispatch_queue_t queueAudio = nil;
				if (param.flagCaptureAudio) {
					queueAudio = dispatch_queue_create(sl_null, DISPATCH_QUEUE_SERIAL);
					if (queueAudio == nil) {
						return sl_null;
					}
				}
				SCShareableContent* content = GetShareableContent(helper);
				if (content == nil) {
					return sl_null;
				}
				NSArray* displays = [content displays];
				if (![[content displays] count]) {
					return sl_null;
				}
				NSMutableArray* streams = [[NSMutableArray alloc] init];
				List<CaptureScreenInfo> screenInfos;
				sl_bool flagFirst = sl_true;
				for (NSScreen* screen in [NSScreen screens]) {
					CGDirectDisplayID displayID;
					if (!(GetDisplayID(screen, displayID))) {
						continue;
					}
					SCDisplay* display = nil;
					{
						for (SCDisplay* item in displays) {
							if ([item displayID] == displayID) {
								display = item;
							}
						}
					}
					if (display == nil) {
						continue;
					}
					SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];
					if (filter == nil) {
						return sl_null;
					}
					SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
					if (config == nil) {
						return sl_null;
					}
					if (param.flagCaptureScreen) {
						CaptureScreenInfo info;
						GetScreenInfo(screen, info);
						sl_uint32 width = (sl_uint32)(info.screenWidth * info.scaleFactor);
						sl_uint32 height = (sl_uint32)(info.screenHeight * info.scaleFactor);
						GraphicsUtil::toSmallSize(width, height, param.maxWidth, param.maxHeight);
						[config setWidth:width];
						[config setHeight:height];
						[config setPixelFormat:kCVPixelFormatType_32BGRA];
						[config setShowsCursor:(param.flagShowCursor ? YES : NO)];
						if (param.screenInterval) {
							[config setMinimumFrameInterval:CMTimeMake(param.screenInterval, 1000)];
						}
						screenInfos.add_NoLock(Move(info));
					} else {
						[config setMinimumFrameInterval:CMTimeMake(3600, 1)];
					}
					if (flagFirst) {
						if (param.flagCaptureAudio) {
							[config setCapturesAudio:YES];
							[config setSampleRate:(NSInteger)(param.audioSamplesPerSecond)];
							[config setChannelCount:(NSInteger)(param.audioChannelCount)];
							[config setExcludesCurrentProcessAudio:(param.flagExcludeCurrentProcessAudio ? YES : NO)];
						}
					}
					SLIBSCStream* stream = [[SLIBSCStream alloc] initWithFilter:filter configuration:config delegate:object];
					if (stream == nil) {
						return sl_null;
					}
					stream->m_index = (sl_uint32)([streams count]);
					NSError* error = nil;
					if (!([stream addStreamOutput:object type:SCStreamOutputTypeScreen sampleHandlerQueue:queueScreen error:&error])) {
						return sl_null;
					}
					if (flagFirst) {
						flagFirst = sl_false;
						if (param.flagCaptureAudio) {
							if (!([stream addStreamOutput:object type:SCStreamOutputTypeAudio sampleHandlerQueue:queueAudio error:&error])) {
								return sl_null;
							}
						}
					}
					[streams addObject:stream];
					if (!(param.flagCaptureScreen)) {
						break;
					}
				}
				if (flagFirst) {
					return sl_null;
				}
				Ref<ScreenCaptureImpl> ret = new ScreenCaptureImpl;
				if (ret.isNull()) {
					return sl_null;
				}
				ret->m_object = object;
				ret->m_streams = streams;
				ret->m_screenInfos = Move(screenInfos);
				ret->m_queueScreen = queueScreen;
				ret->m_queueAudio = queueAudio;
				ret->_init(param);
				object->m_capture = ret;
				ret->start();
				return ret;
			}

			void release() override
			{
				ObjectLocker lock(this);
				sl_bool flagFirst = sl_true;
				for (SCStream* stream in m_streams) {
					sl_bool flagCaptureAudio = sl_false;
					if (flagFirst) {
						flagFirst = sl_false;
						if (m_flagCaptureAudio) {
							flagCaptureAudio = sl_true;
						}
					}
					[stream stopCaptureWithCompletionHandler:^(NSError* error) {
						NSError* _error = nil;
						[stream removeStreamOutput:m_object type:SCStreamOutputTypeScreen error:&_error];
						if (flagCaptureAudio) {
							[stream removeStreamOutput:m_object type:SCStreamOutputTypeAudio error:&_error];
						}
					}];
				}
			}

			void start()
			{
				ObjectLocker lock(this);
				for (SCStream* stream in m_streams) {
					[stream startCaptureWithCompletionHandler:nil];
				}
			}

			void stop()
			{
				ObjectLocker lock(this);
				for (SCStream* stream in m_streams) {
					[stream startCaptureWithCompletionHandler:nil];
				}
			}

			static int GetScreenBufferStatus(CMSampleBufferRef sampleBuffer)
			{
				NSArray* infos = (__bridge NSArray*)(CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, NO));
				if (infos != nil) {
					if (infos.count) {
						NSDictionary* info = [infos firstObject];
						if (info != nil) {
							NSNumber* status = info[SCStreamFrameInfoStatus];
							if (status != nil) {
								return [status intValue];
							}
						}
					}
				}
				return SCFrameStatusSuspended;
			}

			void onCaptureScreen(SLIBSCStream* stream, CMSampleBufferRef sampleBuffer, CaptureScreenStatus status)
			{
				if (!m_flagCaptureScreen) {
					return;
				}
				CaptureScreenResult result;
				sl_uint32 index = (sl_uint32)(stream->m_index);
				if (!(m_screenInfos.getAt_NoLock(index, &result))) {
					return;
				}
				result.screenIndex = index;
				if (status != CaptureScreenStatus::None) {
					result.status = status;
				} else {
					int iStatus = GetScreenBufferStatus(sampleBuffer);
					if (iStatus == SCFrameStatusComplete || iStatus == SCFrameStatusStarted) {
						ProcessCapturedScreen(sampleBuffer, [this, &result](BitmapData& data) {
							result.data = Move(data);
							m_onCaptureScreen(this, result);
						});
						return;
					}
					if (iStatus == SCFrameStatusBlank) {
						return;
					}
					if (iStatus == SCFrameStatusIdle) {
						result.status = CaptureScreenStatus::Idle;
					} else if (iStatus == SCFrameStatusStopped) {
						result.status = CaptureScreenStatus::Stopped;
					} else {
						result.status = CaptureScreenStatus::Error;
					}
				}
				m_onCaptureScreen(this, result);
			}

			void onCaptureAudio(CMSampleBufferRef sampleBuffer)
			{
				CMFormatDescriptionRef fmt = CMSampleBufferGetFormatDescription(sampleBuffer);
				if (!fmt) {
					return;
				}
				const AudioStreamBasicDescription* desc = CMAudioFormatDescriptionGetStreamBasicDescription(fmt);
				if (!desc) {
					return;
				}
				if (desc->mChannelsPerFrame != 2 && desc->mChannelsPerFrame != 1) {
					return;
				}
				if (desc->mBytesPerFrame != 4 || !(desc->mFormatFlags & kAudioFormatFlagIsFloat)) {
					return;
				}
				size_t sizeAudioList = 0;
				OSStatus result = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(sampleBuffer, &sizeAudioList, NULL, 0, NULL, NULL, 0, NULL);
				if (result != noErr && result != kCMSampleBufferError_ArrayTooSmall) {
					return;
				}
				SLIB_SCOPED_BUFFER(sl_uint8, 128, bufAudioList, sizeAudioList);
				AudioBufferList& audioList = *((AudioBufferList*)bufAudioList);
				CMBlockBufferRef blockBuffer = NULL;
				result = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(sampleBuffer, NULL, &audioList, sizeAudioList, NULL, NULL, 0, &blockBuffer);
				if (result != noErr) {
					return;
				}
				if (!blockBuffer) {
					return;
				}
				AudioData ad;
				if (desc->mChannelsPerFrame == 2) {
					if (audioList.mNumberBuffers == 2) {
						AudioBuffer& audioBuffer1 = *(audioList.mBuffers);
						AudioBuffer& audioBuffer2 = *(audioList.mBuffers + 1);
						if (audioBuffer1.mDataByteSize == audioBuffer2.mDataByteSize) {
							sl_bool flagMute = IsMutedFloatData(audioBuffer1.mData, audioBuffer1.mDataByteSize) && IsMutedFloatData(audioBuffer2.mData, audioBuffer2.mDataByteSize);
							ad.format = AudioFormat::Float_Stereo_NonInterleaved;
							ad.data = (float*)(audioBuffer1.mData);
							ad.data1 = (float*)(audioBuffer2.mData);
							ad.count = (sl_uint32)(audioBuffer1.mDataByteSize >> 2);
							_processAudioFrame(ad, flagMute);
						}
					}
				} else {
					if (audioList.mNumberBuffers == 1) {
						AudioBuffer& audioBuffer = *(audioList.mBuffers);
						sl_bool flagMute = IsMutedFloatData(audioBuffer.mData, audioBuffer.mDataByteSize);
						ad.format = AudioFormat::Float_Mono;
						ad.data = (float*)(audioBuffer.mData);
						ad.count = (sl_uint32)(audioBuffer.mDataByteSize >> 2);
						_processAudioFrame(ad, flagMute);
					}
				}
				CFRelease(blockBuffer);
			}
		};
#endif
	}

#if defined(USE_SCREEN_CAPTURE_KIT)
	Ref<ScreenCapture> ScreenCapture::create(const ScreenCaptureParam& param)
	{
		if (@available(macos 13.0, *)) {
			return Ref<ScreenCapture>::cast(ScreenCaptureImpl::create(param));
		} else {
			return sl_null;
		}
	}
#else
	Ref<ScreenCapture> ScreenCapture::create(const ScreenCaptureParam& param)
	{
		return sl_null;
	}
#endif

	sl_bool ScreenCapture::takeScreenshot(Screenshot& _out, sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		Helper* helper = GetHelper();
		if (!helper) {
			return sl_false;
		}
		MutexLocker lock(&(helper->m_lock));
		return TakeScreenshot(_out, helper, [[NSScreen screens] objectAtIndex:0], maxWidth, maxHeight);
	}

	sl_bool ScreenCapture::takeScreenshotFromCurrentMonitor(Screenshot& _out, sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		Helper* helper = GetHelper();
		if (!helper) {
			return sl_false;
		}
		MutexLocker lock(&(helper->m_lock));
		return TakeScreenshot(_out, helper, [NSScreen mainScreen], maxWidth, maxHeight);
	}

	List<Screenshot> ScreenCapture::takeScreenshotsFromAllMonitors(sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		Helper* helper = GetHelper();
		if (!helper) {
			return sl_null;
		}
		MutexLocker lock(&(helper->m_lock));
		List<Screenshot> ret;
#ifdef USE_SCREENSHOT_MANAGER
		if (@available(macos 14.0, *)) {
			SCShareableContent* content = GetShareableContent(helper);
			NSArray* displays = [content displays];
			for (NSScreen* screen in [NSScreen screens]) {
				CGDirectDisplayID displayID;
				if (!(GetDisplayID(screen, displayID))) {
					continue;
				}
				SCDisplay* display = nil;
				{
					for (SCDisplay* item in displays) {
						if ([item displayID] == displayID) {
							display = item;
						}
					}
				}
				if (display != nil) {
					Screenshot screenshot;
					GetScreenInfo(screen, screenshot);
					sl_uint32 width = (sl_uint32)(screenshot.screenWidth * screenshot.scaleFactor);
					sl_uint32 height = (sl_uint32)(screenshot.screenHeight * screenshot.scaleFactor);
					GraphicsUtil::toSmallSize(width, height, maxWidth, maxHeight);
					screenshot.image = TakeScreenshot(helper, display, width, height);
					if (screenshot.image.isNotNull()) {
						ret.add_NoLock(Move(screenshot));
					}
				}
			}
		} else
#endif
		{
			for (NSScreen* screen in [NSScreen screens]) {
				Screenshot screenshot;
				if (TakeScreenshot(screenshot, helper, screen, maxWidth, maxHeight)) {
					ret.add_NoLock(Move(screenshot));
				}
			}
		}
		return ret;
	}

	sl_uint32 ScreenCapture::getScreenCount()
	{
		sl_uint32 count = (sl_uint32)([[NSScreen screens] count]);
		return count;
	}

	sl_bool ScreenCapture::isEnabled()
	{
		if (@available(macOS 10.15, *)) {
			return CGPreflightScreenCaptureAccess();
		} else {
			return sl_true;
		}
	}

	void ScreenCapture::openSystemPreferences()
	{
		if (@available(macos 10.15, *)) {
			[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
		}
	}

	void ScreenCapture::requestAccess()
	{
#ifdef USE_SCREEN_CAPTURE_KIT
		if (@available(macos 13.0, *)) {
			[SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent*, NSError*) {}];
		} else
#endif
		if (@available(macos 10.15, *)) {
			ScreenCapture::takeScreenshot();
		}
	}

	void ScreenCapture::resetAccess(const StringParam& appBundleId)
	{
		if (@available(macos 10.15, *)) {
			System::execute(String::concat(StringView::literal("tccutil reset ScreenCapture "), appBundleId));
		}
	}

}

#if defined(USE_SCREEN_CAPTURE_KIT)
API_AVAILABLE(macos(13.0))
@implementation SLIBScreenCapture
- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error
{
	slib::Ref<slib::ScreenCaptureImpl> capture = slib::WeakRef<slib::ScreenCaptureImpl>::cast(m_capture);
	if (capture.isNotNull()) {
		capture->onCaptureScreen((SLIBSCStream*)stream, NULL, slib::CaptureScreenStatus::Stopped);
	}
}

- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
{
	if (!sampleBuffer) {
		return;
	}
	if (!(CMSampleBufferIsValid(sampleBuffer))) {
		return;
	}
	slib::Ref<slib::ScreenCaptureImpl> capture = slib::WeakRef<slib::ScreenCaptureImpl>::cast(m_capture);
	if (capture.isNotNull()) {
		if (type == SCStreamOutputTypeScreen) {
			capture->onCaptureScreen((SLIBSCStream*)stream, sampleBuffer, slib::CaptureScreenStatus::None);
		} else if (type == SCStreamOutputTypeAudio) {
			capture->onCaptureAudio(sampleBuffer);
		}
	}
}
@end

API_AVAILABLE(macos(13.0))
@implementation SLIBSCStream
@end
#endif

#endif
