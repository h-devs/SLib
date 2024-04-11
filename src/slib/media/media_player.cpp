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

#include "slib/media/media_player.h"
#include "slib/media/wave_player.h"

#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MediaPlayerParam)

	MediaPlayerParam::MediaPlayerParam()
	{
		flagVideo = sl_false;

		flagAutoStart = sl_true;
		flagAutoRepeat = sl_false;
		flagAutoRelease = sl_true;

		flagSelfAlive = sl_false;
	}

	void MediaPlayerParam::applyFlags(const MediaPlayerFlags& flags)
	{
		flagAutoStart = (flags & MediaPlayerFlags::NotStart) == 0;
		flagAutoRepeat = (flags & MediaPlayerFlags::Repeat) != 0;
		flagVideo = (flags & MediaPlayerFlags::Video) != 0;
		flagAutoRelease = (flags & MediaPlayerFlags::NotAutoRelease) == 0;
		flagSelfAlive = (flags & MediaPlayerFlags::NotSelfAlive) == 0;
	}


	SLIB_DEFINE_OBJECT(MediaPlayer, Object)

	MediaPlayer::MediaPlayer()
	{
		m_flagSelfAlive = sl_false;
		m_flagAutoRepeat = sl_false;
		m_flagAutoRelease = sl_true;
	}

	MediaPlayer::~MediaPlayer()
	{
	}

	Ref<MediaPlayer> MediaPlayer::create(const MediaPlayerParam& param)
	{
		if (param.url.isEmpty() && param.filePath.isEmpty() && param.assetFileName.isEmpty()) {
			return sl_null;
		}
		Ref<MediaPlayer> player = _createNative(param);
		if (player.isNotNull()) {
			if (param.flagAutoStart) {
				player->resume();
			}
			return player;
		}
		return sl_null;
	}

	Ref<MediaPlayer> MediaPlayer::openUrl(const String& url, const MediaPlayerFlags& flags)
	{
		MediaPlayerParam param;
		param.url = url;
		param.applyFlags(flags);
		return create(param);
	}

	Ref<MediaPlayer> MediaPlayer::openFile(const String& filePath, const MediaPlayerFlags& flags)
	{
		MediaPlayerParam param;
		param.filePath = filePath;
		param.applyFlags(flags);
		return create(param);
	}

	Ref<MediaPlayer> MediaPlayer::openAsset(const String& fileName, const MediaPlayerFlags& flags)
	{
		MediaPlayerParam param;
		param.assetFileName = fileName;
		param.applyFlags(flags);
		return create(param);
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(MediaPlayer, RenderVideoParam)

	MediaPlayer::RenderVideoParam::RenderVideoParam()
	{
		flagUpdated = sl_false;

		glTextureTransformOES = Matrix3::identity();

		_glEngineIdLast = 0;
		_glTextureNameOES = 0;
	}

	sl_bool MediaPlayer::isAutoRepeat()
	{
		return m_flagAutoRepeat;
	}

	void MediaPlayer::setAutoRepeat(sl_bool flagRepeat)
	{
		m_flagAutoRepeat = flagRepeat;
	}

	sl_bool MediaPlayer::isAutoRelease()
	{
		return m_flagAutoRelease;
	}

	void MediaPlayer::setAutoRelease(sl_bool flagRelease)
	{
		m_flagAutoRelease = flagRelease;
	}

	void MediaPlayer::_onReadyToPlay()
	{
		getOnReadyToPlay()(this);
	}

	void MediaPlayer::_onComplete()
	{
		getOnComplete()(this);
	}

	void MediaPlayer::_init(const MediaPlayerParam& param)
	{
		m_flagAutoRepeat = param.flagAutoRepeat;
		m_flagAutoRelease = param.flagAutoRelease;
		m_flagSelfAlive = param.flagSelfAlive;
		setOnReadyToPlay(param.onReadyToPlay);
		setOnComplete(param.onComplete);
	}

	namespace {
		typedef CHashMap< MediaPlayer*, Ref<MediaPlayer> > MediaPlayersMap;
		SLIB_SAFE_STATIC_GETTER(MediaPlayersMap, GetMediaPlayersMap)
	}

	void MediaPlayer::_addToMap()
	{
		if (m_flagSelfAlive) {
			MediaPlayersMap* map = GetMediaPlayersMap();
			if (map) {
				map->put(this, this);
			}
		}
	}

	void MediaPlayer::_removeFromMap()
	{
		if (m_flagSelfAlive) {
			MediaPlayersMap* map = GetMediaPlayersMap();
			if (map) {
				map->remove(this);
			}
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(WavePlayerParam)

	WavePlayerParam::WavePlayerParam()
	{
		flagSynchronous = sl_false;
		flagLoop = sl_false;
	}

#ifndef SLIB_PLATFORM_IS_WIN32
	sl_bool WavePlayer::play(const WavePlayerParam& param)
	{
		return sl_false;
	}
#endif

	sl_bool WavePlayer::play(const Memory& wave)
	{
		WavePlayerParam param;
		param.content = wave;
		return play(param);
	}

	sl_bool WavePlayer::playSynchronous(const Memory& wave)
	{
		WavePlayerParam param;
		param.content = wave;
		param.flagSynchronous = sl_true;
		return play(param);
	}

	sl_bool WavePlayer::playFile(const StringParam& path)
	{
		WavePlayerParam param;
		param.filePath = path;
		return play(param);
	}

	sl_bool WavePlayer::playFileSynchronous(const StringParam& path)
	{
		WavePlayerParam param;
		param.filePath = path;
		param.flagSynchronous = sl_true;
		return play(param);
	}

	sl_bool WavePlayer::playAsset(const StringParam& path)
	{
		WavePlayerParam param;
		param.assetFileName = path;
		return play(param);
	}

	sl_bool WavePlayer::playAssetSynchronous(const StringParam& path)
	{
		WavePlayerParam param;
		param.assetFileName = path;
		param.flagSynchronous = sl_true;
		return play(param);
	}

	sl_bool WavePlayer::playResource(const StringParam& resourceName)
	{
		WavePlayerParam param;
		param.resourceName = resourceName;
		return play(param);
	}

	sl_bool WavePlayer::playResourceSynchronous(const StringParam& resourceName)
	{
		WavePlayerParam param;
		param.resourceName = resourceName;
		param.flagSynchronous = sl_true;
		return play(param);
	}

}
