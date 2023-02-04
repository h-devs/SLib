/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_MEDIA_AUDIO_PLAYER
#define CHECKHEADER_SLIB_MEDIA_AUDIO_PLAYER

#include "audio_data.h"

#include "../core/object.h"
#include "../core/string.h"
#include "../core/event.h"
#include "../core/memory_queue.h"
#include "../core/function.h"

namespace slib
{

	class Event;

	class SLIB_EXPORT AudioPlayerDeviceInfo
	{
	public:
		String id;
		String name;
		String description;

	public:
		AudioPlayerDeviceInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AudioPlayerDeviceInfo)

	};

	class SLIB_EXPORT AudioPlayerDeviceParam
	{
	public:
		String deviceId;

	public:
		AudioPlayerDeviceParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AudioPlayerDeviceParam)

	};

	class AudioPlayer;

	class SLIB_EXPORT AudioPlayerParam : public AudioPlayerDeviceParam
	{
	public:
		AudioStreamType streamType;

		sl_uint32 samplesPerSecond;
		sl_uint32 channelCount;
		sl_uint32 frameLengthInMilliseconds;
		sl_uint32 maxBufferLengthInMilliseconds;

		sl_bool flagAutoStart;

		// called before playing a frame
		Function<void(AudioPlayer*, sl_uint32 sampleCount)> onPlayAudio;
		Ref<Event> event;

	public:
		AudioPlayerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AudioPlayerParam)

	};

	class SLIB_EXPORT AudioPlayer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		AudioPlayer();

		~AudioPlayer();

	public:
		static Ref<AudioPlayer> create(const AudioPlayerParam& param);

		static List<AudioPlayerDeviceInfo> getDevices();

	public:
		void release();

		sl_bool isOpened();

		sl_bool start();

		void stop();

		sl_bool isRunning();

		float getVolume();

		void setVolume(float volume);

		sl_bool isMute();

		void setMute(sl_bool flag);

		const AudioPlayerParam& getParam();

		void write(const AudioData& audioPlay);

		void flush();

		sl_size getSampleCountInQueue();

	protected:
		virtual void _release() = 0;

		virtual sl_bool _start() = 0;

		virtual void _stop() = 0;

	protected:
		void _init(const AudioPlayerParam& param);

		Array<sl_int16> _getProcessData(sl_uint32 count);

		void _processFrame(sl_int16* s, sl_uint32 count);

	protected:
		AudioPlayerParam m_param;

		sl_bool m_flagRunning;
		sl_bool m_flagOpened;
		sl_uint32 m_volume;
		sl_bool m_flagMute;

		MemoryQueue m_buffer;
		sl_size m_lenBufferMax;

		sl_int16 m_lastSample;
		AtomicArray<sl_int16> m_processData;
	};

	class SLIB_EXPORT AudioPlayerDevice : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		AudioPlayerDevice();

		~AudioPlayerDevice();

	public:
		static Ref<AudioPlayerDevice> create(const AudioPlayerDeviceParam& param);

		static Ref<AudioPlayerDevice> create();

		static List<AudioPlayerDeviceInfo> getDevices();

	public:
		virtual Ref<AudioPlayer> createPlayer(const AudioPlayerParam& param) = 0;

	};
}

#endif
