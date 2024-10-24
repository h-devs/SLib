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

#ifndef CHECKHEADER_SLIB_MEDIA_AUDIO_RECORDER
#define CHECKHEADER_SLIB_MEDIA_AUDIO_RECORDER

#include "audio_data.h"

#include "../core/object.h"
#include "../core/event.h"
#include "../core/function.h"
#include "../data/loop_queue.h"

#include "priv/audio_device.h"

namespace slib
{

	typedef AudioDeviceInfo AudioRecorderDeviceInfo;

	class AudioRecorder;

	class SLIB_EXPORT AudioRecorderParam : public AudioDeviceParam
	{
	public:
		AudioRecordingPreset recordingPreset;
		sl_bool flagLoopback; // In loopback mode, `deviceId` means player device

		sl_uint32 samplesPerSecond; // per channel
		sl_uint32 channelCount;
		sl_uint32 framesPerPacket; // frames per packet
		sl_uint32 packetLengthInMilliseconds; // required when `framesPerPacket` is not set
		sl_uint32 bufferLengthInMilliseconds;
		sl_uint32 framesPerCallback; // samples per callback (per channel)

		sl_bool flagAutoStart;

		Function<void(AudioRecorder*, AudioData&)> onRecordAudio;
		Ref<Event> event;

	public:
		AudioRecorderParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AudioRecorderParam)

	public:
		sl_uint32 getFramesPerPacket() const;

		sl_uint32 getPacketLengthInMilliseconds() const;

	};

	class SLIB_EXPORT AudioRecorder : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		AudioRecorder();

		~AudioRecorder();

	public:
		static Ref<AudioRecorder> create(const AudioRecorderParam& param);

		static List<AudioRecorderDeviceInfo> getDevices();

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

		const AudioRecorderParam& getParam();

		sl_bool read(const AudioData& audio);

	protected:
		virtual void _release() = 0;

		virtual sl_bool _start() = 0;

		virtual void _stop() = 0;

	protected:
		void _init(const AudioRecorderParam& param);

		Array<sl_int16> _getProcessData(sl_uint32 count);

		void _processFrame(sl_int16* s, sl_uint32 count);

	protected:
		AudioRecorderParam m_param;

		sl_bool m_flagRunning;
		sl_bool m_flagOpened;
		sl_uint32 m_volume;
		sl_bool m_flagMute;

		LoopQueue<sl_int16> m_queue;
		AtomicArray<sl_int16> m_processData;
		AtomicArray<sl_int16> m_bufCallback;
		sl_uint32 m_nSamplesInCallbackBuffer;

	};
}

#endif
