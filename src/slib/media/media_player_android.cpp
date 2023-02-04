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

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/media/media_player.h"

#include "slib/render/opengl.h"
#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"
#include "slib/platform.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace slib
{

	namespace {

		class MediaPlayerImpl;

		typedef CHashMap<jlong, WeakRef<MediaPlayerImpl> > MediaPlayerMap;
		SLIB_SAFE_STATIC_GETTER(MediaPlayerMap, GetMediaPlayerMap)

		static void OnCompleted(JNIEnv* env, jobject _this, jlong instance);
		static void OnPrepared(JNIEnv* env, jobject _this, jlong instance);

		SLIB_JNI_BEGIN_CLASS(JMediaPlayer, "slib/android/media/SMediaPlayer")
			SLIB_JNI_STATIC_METHOD(openUrl, "openUrl", "(Ljava/lang/String;)Lslib/android/media/SMediaPlayer;");
			SLIB_JNI_STATIC_METHOD(openAsset, "openAsset", "(Landroid/content/Context;Ljava/lang/String;)Lslib/android/media/SMediaPlayer;");
			SLIB_JNI_METHOD(setInstance, "setInstance", "(J)V");
			SLIB_JNI_METHOD(start, "start", "()V");
			SLIB_JNI_METHOD(pause, "pause", "()V");
			SLIB_JNI_METHOD(stop, "stop", "()V");
			SLIB_JNI_METHOD(isPlaying, "isPlaying", "()Z");
			SLIB_JNI_METHOD(getVolume, "getVolume", "()F");
			SLIB_JNI_METHOD(setVolume, "setVolume", "(F)V");
			SLIB_JNI_METHOD(getDuration, "getDuration", "()D");
			SLIB_JNI_METHOD(getCurrentTime, "getCurrentTime", "()D");
			SLIB_JNI_METHOD(seekTo, "seekTo", "(D)V");
			SLIB_JNI_METHOD(setLooping, "setLooping", "(Z)V");
			SLIB_JNI_METHOD(getVideoWidth, "getVideoWidth", "()I");
			SLIB_JNI_METHOD(getVideoHeight, "getVideoHeight", "()I");
			SLIB_JNI_METHOD(renderVideo, "renderVideo", "(IZ)Z");
			SLIB_JNI_NATIVE(onCompleted, "nativeOnCompleted", "(J)V", OnCompleted);
			SLIB_JNI_NATIVE(onPrepared, "nativeOnPrepared", "(J)V", OnPrepared);
			SLIB_JNI_OBJECT_FIELD(mTextureMatrix, "[F");
		SLIB_JNI_END_CLASS

		class MediaPlayerImpl : public MediaPlayer
		{
		public:
			JniGlobal<jobject> m_player;
			sl_bool m_flagInited;
			sl_bool m_flagPlaying;
			sl_bool m_flagPrepared;
			sl_bool m_flagVideo;

		public:
			MediaPlayerImpl()
			{
				m_flagInited = sl_false;
				m_flagPlaying = sl_false;
				m_flagPrepared = sl_false;
				m_flagVideo = sl_false;
			}

			~MediaPlayerImpl()
			{
				release();
				MediaPlayerMap* map = GetMediaPlayerMap();
				if (map) {
					map->remove((jlong) this);
				}
			}

		public:
			static Ref<MediaPlayerImpl> create(const MediaPlayerParam& param)
			{
				JniGlobal<jobject> player;
				if (param.url.isNotEmpty()) {
					JniLocal<jstring> url = Jni::getJniString(param.url);
					player = JMediaPlayer::openUrl.callObject(sl_null, url.get());
				} else if (param.filePath.isNotEmpty()) {
					JniLocal<jstring> filePath = Jni::getJniString(param.filePath);
					player = JMediaPlayer::openUrl.callObject(sl_null, filePath.get());
				} else if (param.assetFileName.isNotEmpty()) {
					JniLocal<jstring> assetFileName = Jni::getJniString(param.assetFileName);
					jobject context = Android::getCurrentContext();
					if (context) {
						player = JMediaPlayer::openAsset.callObject(sl_null, context, assetFileName.get());
					}
				}
				if (player.isNull()) {
					return sl_null;
				}

				Ref<MediaPlayerImpl> ret = new MediaPlayerImpl;
				if (ret.isNotNull()) {
					ret->_init(param);
					MediaPlayerMap* map = GetMediaPlayerMap();
					if (map) {
						jlong instance = (jlong)(ret.get());
						map->put(instance, ret);
						JMediaPlayer::setInstance.call(player, instance);
						JMediaPlayer::setLooping.call(player, param.flagAutoRepeat);
						ret->m_flagVideo = param.flagVideo;
						ret->m_flagInited = sl_true;
						ret->m_player = Move(player);
						return ret;
					}
				}
				return sl_null;
			}

			void release() override
			{
				ObjectLocker lock(this);
				if (!m_flagInited) {
					return;
				}
				m_flagInited = sl_false;
				if (m_flagPrepared) {
					JMediaPlayer::stop.call(m_player);
				}
				_removeFromMap();
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
				if (m_flagPrepared) {
					JMediaPlayer::start.call(m_player);
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
				if (m_flagPrepared) {
					JMediaPlayer::pause.call(m_player);
				}
				m_flagPlaying = sl_false;
				_removeFromMap();
			}

			sl_bool isPlaying() override
			{
				ObjectLocker lock(this);
				if (!m_flagInited) {
					return sl_false;
				}
				return m_flagPlaying;
			}

			sl_real getVolume() override
			{
				return (sl_real) JMediaPlayer::getVolume.callFloat(m_player);
			}

			void setVolume(sl_real volume) override
			{
				ObjectLocker lock(this);
				if (!m_flagInited) {
					return;
				}
				JMediaPlayer::setVolume.call(m_player, volume);
			}

			double getDuration() override
			{
				ObjectLocker lock(this);
				if (m_flagInited) {
					return JMediaPlayer::getDuration.callDouble(m_player);
				}
				return 0;
			}

			double getCurrentTime() override
			{
				ObjectLocker lock(this);
				if (m_flagInited) {
					return JMediaPlayer::getCurrentTime.callDouble(m_player);
				}
				return 0;
			}

			void seekTo(double seconds) override
			{
				ObjectLocker lock(this);
				if (m_flagInited) {
					JMediaPlayer::seekTo.call(m_player, seconds);
				}
			}

			void setAutoRepeat(sl_bool flagRepeat) override
			{
				ObjectLocker lock(this);
				if (!m_flagInited) {
					return;
				}
				JMediaPlayer::setLooping.call(m_player, flagRepeat);
			}

			void renderVideo(MediaPlayerRenderVideoParam& param) override
			{
				ObjectLocker lock(this);
				if (!m_flagInited) {
					return;
				}
				if (!m_flagVideo) {
					return;
				}
				if (param.glEngine.isNull()) {
					return;
				}
				sl_uint64 engineId = param.glEngine->getUniqueId();
				if (param._glEngineIdLast != engineId) {
					param.glTextureOES.setNull();
				}
				param._glEngineIdLast = engineId;
				GLuint textureName = 0;
				sl_bool flagResetTexture = sl_false;
				if (param.glTextureOES.isNull()) {
					glGenTextures(1, &textureName);
					if (textureName == 0) {
						return;
					}
					param.glTextureOES = param.glEngine->createTextureFromName(GL_TEXTURE_EXTERNAL_OES, textureName);
					if (param.glTextureOES.isNull()) {
						return;
					}
					param._glTextureNameOES = textureName;
					flagResetTexture = sl_true;
				} else {
					textureName = param._glTextureNameOES;
					if (textureName == 0) {
						return;
					}
				}

				EngineTexture* engineTexture = (EngineTexture*)(param.glTextureOES.get());
				engineTexture->setWidth(JMediaPlayer::getVideoWidth.callInt(m_player));
				engineTexture->setHeight(JMediaPlayer::getVideoHeight.callInt(m_player));

				param.flagUpdated = JMediaPlayer::renderVideo.callBoolean(m_player, textureName, flagResetTexture) != 0;
				if (param.flagUpdated) {
					JniLocal<jfloatArray> arr = JMediaPlayer::mTextureMatrix.get(m_player);
					if (arr.isNotNull()) {
						float t[16];
						Jni::getFloatArrayRegion(arr.get(), 0, 16, t);
						Matrix3 mat;
						mat.m00 = t[0];
						mat.m01 = t[1];
						mat.m02 = 0;
						mat.m10 = -t[4];
						mat.m11 = -t[5];
						mat.m12 = 0;
						mat.m20 = t[12];
						mat.m21 = 1-t[13];
						mat.m22 = 1;
						param.glTextureTransformOES = mat;
					}
				}
			}

			void _onPrepared()
			{
				ObjectLocker lock(this);
				if (!m_flagInited) {
					return;
				}
				m_flagPrepared = sl_true;
				if (m_flagPlaying) {
					JMediaPlayer::start.call(m_player);
				}
				lock.unlock();
				_onReadyToPlay();
			}

			void _onReachEnd()
			{
				_onComplete();
				_removeFromMap();
			}

		};

		void OnCompleted(JNIEnv* env, jobject _this, jlong instance)
		{
			MediaPlayerMap* map = GetMediaPlayerMap();
			if (map) {
				WeakRef<MediaPlayerImpl> _player;
				map->get(instance, &_player);
				Ref<MediaPlayerImpl> player = _player;
				if (player.isNotNull()) {
					player->_onReachEnd();
				}
			}
		}

		void OnPrepared(JNIEnv* env, jobject _this, jlong instance)
		{
			MediaPlayerMap* map = GetMediaPlayerMap();
			if (map) {
				WeakRef<MediaPlayerImpl> _player;
				map->get(instance, &_player);
				Ref<MediaPlayerImpl> player = _player;
				if (player.isNotNull()) {
					player->_onPrepared();
				}
			}
		}

	}

	Ref<MediaPlayer> MediaPlayer::_createNative(const MediaPlayerParam& param)
	{
		return MediaPlayerImpl::create(param);
	}

}

#endif
