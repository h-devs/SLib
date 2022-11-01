/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/media/definition.h"

#if defined(SLIB_PLATFORM_IS_APPLE)

#include "slib/media/media_player.h"

#include "slib/core/asset.h"
#include "slib/core/endian.h"
#include "slib/core/apple/platform.h"

#import <AVFoundation/AVFoundation.h>


namespace slib
{
	namespace priv
	{
		namespace media_player
		{
			class MediaPlayerImpl;
		}
	}
}

@interface SLIBMediaPlayerObserver : NSObject
{
	@public slib::WeakRef<slib::priv::media_player::MediaPlayerImpl> m_player;
}
- (void)playerItemDidReachEnd:(NSNotification *)notification;
@end

namespace slib
{

	namespace priv
	{
		namespace media_player
		{

			class MediaPlayerImpl : public MediaPlayer
			{
			public:
				AVPlayer* m_player;
				AVPlayerItem* m_playerItem;
				AVPlayerItemVideoOutput* m_videoOutput;
				SLIBMediaPlayerObserver* m_observer;

				AVPlayerStatus m_status;
				sl_bool m_flagInited;
				sl_bool m_flagPlaying;
				sl_real m_volume;

			public:
				MediaPlayerImpl()
				{
					m_status = AVPlayerStatusUnknown;
					m_flagInited = sl_false;
					m_flagPlaying = sl_false;
					m_volume = 1;
				}

				~MediaPlayerImpl()
				{
					_release(sl_true, sl_false);
				}

			public:
				static Ref<MediaPlayerImpl> create(const MediaPlayerParam& param)
				{
					AVPlayerItem* playerItem = nil;
					if (param.url.isNotEmpty()) {
						NSURL* url = [NSURL URLWithString: Apple::getNSStringFromString(param.url)];
						playerItem = [AVPlayerItem playerItemWithURL:url];
					} else if (param.filePath.isNotEmpty()) {
						NSURL* url = [NSURL fileURLWithPath: Apple::getNSStringFromString(param.filePath)];
						playerItem = [AVPlayerItem playerItemWithURL:url];
					} else if (param.assetFileName.isNotEmpty()) {
						NSURL* url = [NSURL fileURLWithPath: Apple::getNSStringFromString(Assets::getFilePath(param.assetFileName))];
						playerItem = [AVPlayerItem playerItemWithURL:url];
					}
					if (playerItem == nil) {
						return sl_null;
					}

					AVPlayer* player = [AVPlayer playerWithPlayerItem:playerItem];

					if (player != nil) {

						[player setAutomaticallyWaitsToMinimizeStalling:NO];
						[player setActionAtItemEnd:AVPlayerActionAtItemEndNone];

						AVPlayerItemVideoOutput* videoOutput = nil;
						if (param.flagVideo) {
							NSDictionary *pixBuffAttributes = @{(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)};
							videoOutput = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:pixBuffAttributes];
						}

						Ref<MediaPlayerImpl> ret = new MediaPlayerImpl;
						if (ret.isNotNull()) {
							SLIBMediaPlayerObserver* observer = [[SLIBMediaPlayerObserver alloc] init];
							if (observer != nil) {
								observer->m_player = ret;
								ret->m_flagInited = sl_true;
								ret->m_player = player;
								ret->m_playerItem = playerItem;
								ret->m_observer = observer;
								ret->m_videoOutput = videoOutput;
								ret->_init(param);
								[player addObserver:observer forKeyPath:@"status" options:0 context:nil];
								[[NSNotificationCenter defaultCenter] addObserver:observer
																		selector:@selector(playerItemDidReachEnd:)
																			name:AVPlayerItemDidPlayToEndTimeNotification
																		object:playerItem];
								if (videoOutput != nil) {
									[playerItem addOutput:videoOutput];
								}
								return ret;
							}
						}

					}
					return sl_null;
				}

			public:
				void release() override
				{
					_release(sl_false, sl_false);
				}

				void resume() override
				{
					ObjectLocker lock(this);
					if (!m_flagInited) {
						return;
					}
					if (m_flagPlaying) {
						return;
					}
					if (m_status == AVPlayerStatusFailed) {
						return;
					}
					if (m_status == AVPlayerStatusReadyToPlay) {
						[m_player play];
					}
					m_flagPlaying = sl_true;
					_addToMap();
				}

				void pause() override
				{
					ObjectLocker lock(this);
					if (!m_flagInited) {
						return;
					}
					if (!m_flagPlaying) {
						return;
					}
					if (m_status == AVPlayerStatusFailed) {
						return;
					}
					if (m_status == AVPlayerStatusReadyToPlay) {
						[m_player pause];
					}
					m_flagPlaying = sl_false;
					_removeFromMap();
				}

				sl_bool isPlaying() override
				{
					return m_flagPlaying;
				}

				sl_real getVolume() override
				{
					return m_volume;
				}

				void setVolume(sl_real volume) override
				{
					ObjectLocker lock(this);
					if (!m_flagInited) {
						return;
					}
					if (m_status == AVPlayerStatusReadyToPlay) {
						[m_player setVolume:volume];
					}
					m_volume = volume;
				}

				double convertCMTime(const CMTime& time)
				{
					if (CMTIME_IS_NUMERIC(time)) {
						return (double)(time.value) / (double)(time.timescale);
					}
					return -1;
				}

				double getDuration() override
				{
					ObjectLocker lock(this);
					if (m_flagInited) {
						return convertCMTime(m_player.currentItem.duration);
					}
					return 0;
				}

				double getCurrentTime() override
				{
					ObjectLocker lock(this);
					if (m_flagInited) {
						return convertCMTime(m_player.currentTime);
					}
					return 0;
				}

				void seekTo(double time) override
				{
					ObjectLocker lock(this);
					if (m_flagInited) {
						[m_player seekToTime:CMTimeMakeWithSeconds(time, 1000)];
					}
				}

				void renderVideo(MediaPlayerRenderVideoParam& param) override
				{
					param.flagUpdated = sl_false;
					if (param.onUpdateFrame.isNull()) {
						return;
					}

					ObjectLocker lock(this);

					if (m_flagInited) {

						if (m_status == AVPlayerStatusReadyToPlay) {

							CMTime time = [m_playerItem currentTime];

							if ([m_videoOutput hasNewPixelBufferForItemTime:time]) {

								CVPixelBufferRef pixelBuffer = [m_videoOutput copyPixelBufferForItemTime:time itemTimeForDisplay:nil];

								if (pixelBuffer != NULL) {

									lock.unlock();

									if (CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess) {

										sl_uint32 frameWidth = (sl_uint32)(CVPixelBufferGetWidth(pixelBuffer));
										sl_uint32 frameHeight = (sl_uint32)(CVPixelBufferGetHeight(pixelBuffer));
										sl_uint8* baseAddress = (sl_uint8*)(CVPixelBufferGetBaseAddress(pixelBuffer));

										if (baseAddress && (frameWidth & 1) == 0 && (frameHeight & 1) == 0) {
											VideoFrame frame;
											frame.image.width = frameWidth;
											frame.image.height = frameHeight;
											OSType type = CVPixelBufferGetPixelFormatType(pixelBuffer);
											if (type == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange) {
												CVPlanarPixelBufferInfo_YCbCrBiPlanar* p = reinterpret_cast<CVPlanarPixelBufferInfo_YCbCrBiPlanar*>(baseAddress);
												if (p->componentInfoY.offset == 0) {
													frame.image.format = BitmapFormat::YUV_NV12;
													frame.image.data = baseAddress + sizeof(CVPlanarPixelBufferInfo_YCbCrBiPlanar);
												} else {
													frame.image.format = BitmapFormat::YUV_NV12;
													frame.image.data = baseAddress + (sl_int32)(Endian::swap32LE(p->componentInfoY.offset));
													frame.image.pitch = Endian::swap32LE(p->componentInfoY.rowBytes);
													frame.image.data1 = baseAddress + (sl_int32)(Endian::swap32LE(p->componentInfoCbCr.offset));
													frame.image.pitch1 = Endian::swap32LE(p->componentInfoCbCr.rowBytes);
												}
												param.onUpdateFrame(frame);

												param.flagUpdated = sl_true;
											}
										}

										CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
									}

									CFRelease(pixelBuffer);
								}
							}
						}
					}
				}

				void _release(sl_bool flagFromDestructor, sl_bool flagFromObserver)
				{
					ObjectLocker lock(this);
					if (!m_flagInited) {
						return;
					}
					m_flagInited = sl_false;
					m_flagPlaying = sl_false;
					lock.unlock();
					[m_player removeObserver:m_observer forKeyPath:@"status"];
					[[NSNotificationCenter defaultCenter] removeObserver:m_observer name:AVPlayerItemDidPlayToEndTimeNotification object:m_playerItem];
					if (!flagFromObserver) {
						[m_playerItem cancelPendingSeeks];
						[m_playerItem.asset cancelLoading];
					}
					if (!flagFromDestructor) {
						_removeFromMap();
					}
					m_playerItem = nil;
					m_player = nil;
					m_videoOutput = nil;
					m_observer = nil;
				}

				void _onReachEnd()
				{
					_onComplete();
					if (m_flagAutoRepeat) {
						CMTime t;
						t.value = 0;
						t.timescale = 1;
						t.flags = kCMTimeFlags_Valid;
						t.epoch = 0;
#ifdef SLIB_PLATFORM_IS_IOS_CATALYST
						[m_playerItem seekToTime:t completionHandler:nil];
#else
						[m_playerItem seekToTime:t];
#endif
					} else {
						_release(sl_false, sl_true);
					}
				}

				void _onStatus()
				{
					m_status = m_player.status;
					if (m_status == AVPlayerStatusFailed) {
						_release(sl_false, sl_true);
					} else if (m_status == AVPlayerStatusReadyToPlay) {
						ObjectLocker lock(this);
						if (m_flagPlaying) {
							[m_player play];
						}
						[m_player setVolume:m_volume];
						lock.unlock();
						_onReadyToPlay();
					}
				}

			};

		}
	}

	Ref<MediaPlayer> MediaPlayer::_createNative(const MediaPlayerParam& param)
	{
		return priv::media_player::MediaPlayerImpl::create(param);
	}

}

using namespace slib;
using namespace slib::priv::media_player;

@implementation SLIBMediaPlayerObserver

- (void)playerItemDidReachEnd:(NSNotification *)notification {
	Ref<MediaPlayerImpl> player(m_player);
	if (player.isNotNull()) {
		player->_onReachEnd();
	}
}

-(void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)contex
{
	if ([keyPath isEqualToString:@"status"]) {
		Ref<MediaPlayerImpl> player(m_player);
		if (player.isNotNull()) {
			player->_onStatus();
		}
	}
}

@end

#endif
