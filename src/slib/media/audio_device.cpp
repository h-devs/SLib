/*
 *   Copyright (c) 2008-2025 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/media/audio_device.h"

#include "slib/media/audio_format.h"
#include "slib/core/event.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioDeviceInfo)

	AudioDeviceInfo::AudioDeviceInfo()
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioDeviceParam)

	AudioDeviceParam::AudioDeviceParam()
	{
		role = AudioDeviceRole::Default;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioRecorderParam)

	AudioRecorderParam::AudioRecorderParam()
	{
		recordingPreset = AudioRecordingPreset::None;
		flagLoopback = sl_false;
		samplesPerSecond = 16000;
		channelCount = 1;
		framesPerPacket = 0;
		packetLengthInMilliseconds = 50;
		bufferLengthInMilliseconds = 0;
		framesPerCallback = 0;
		flagAutoStart = sl_true;
	}

	sl_uint32 AudioRecorderParam::getFramesPerPacket() const
	{
		if (framesPerPacket) {
			return framesPerPacket;
		} else {
			return samplesPerSecond * packetLengthInMilliseconds / 1000;
		}
	}

	sl_uint32 AudioRecorderParam::getPacketLengthInMilliseconds() const
	{
		if (framesPerPacket) {
			return framesPerPacket * 1000 / samplesPerSecond;
		} else {
			return packetLengthInMilliseconds;
		}
	}


	SLIB_DEFINE_OBJECT(AudioRecorder, Object)

	AudioRecorder::AudioRecorder()
	{
		m_flagOpened = sl_true;
		m_flagRunning = sl_false;
		m_volume = 256;
		m_flagMute = sl_false;
		m_nSamplesInCallbackBuffer = 0;
	}

	AudioRecorder::~AudioRecorder()
	{
	}

	void AudioRecorder::release()
	{
		ObjectLocker lock(this);
		if (!m_flagOpened) {
			return;
		}
		stop();
		m_flagOpened = sl_false;
		_release();
	}

	sl_bool AudioRecorder::isOpened()
	{
		return m_flagOpened;
	}

	sl_bool AudioRecorder::start()
	{
		ObjectLocker lock(this);
		if (!m_flagOpened) {
			return sl_false;
		}
		if (m_flagRunning) {
			return sl_true;
		}
		if (_start()) {
			m_flagRunning = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	void AudioRecorder::stop()
	{
		ObjectLocker lock(this);
		if (!m_flagOpened) {
			return;
		}
		if (!m_flagRunning) {
			return;
		}
		m_flagRunning = sl_false;
		_stop();
	}

	sl_bool AudioRecorder::isRunning()
	{
		return m_flagRunning;
	}

	float AudioRecorder::getVolume()
	{
		if (m_volume >= 256) {
			return 1;
		}
		if (!m_volume) {
			return 0;
		}
		return (float)(m_volume) / 256.0f;
	}

	void AudioRecorder::setVolume(float volume)
	{
		sl_int32 v = (sl_int32)(volume * 256);
		if (v >= 256) {
			v = 256;
		}
		if (v <= 0) {
			v = 0;
		}
		m_volume = v;
	}

	sl_bool AudioRecorder::isMute()
	{
		return m_flagMute;
	}

	void AudioRecorder::setMute(sl_bool flag)
	{
		m_flagMute = flag;
	}

	const AudioRecorderParam& AudioRecorder::getParam()
	{
		return m_param;
	}

	sl_bool AudioRecorder::read(const AudioData& audioOut)
	{
		AudioFormat format;
		sl_uint32 nChannels = m_param.channelCount;
		if (nChannels == 1) {
			format = AudioFormat::Int16_Mono;
		} else {
			format = AudioFormat::Int16_Stereo;
		}
		if (audioOut.format == format && (((sl_size)(audioOut.data)) & 1) == 0) {
			return m_queue.pop((sl_int16*)(audioOut.data), nChannels * audioOut.count);
		} else {
			sl_int16 samples[2048];
			AudioData temp;
			temp.format = format;
			temp.data = samples;
			temp.count = 1024;
			sl_size n = audioOut.count;
			ObjectLocker lock(&m_queue);
			if (n <= m_queue.getCount()) {
				while (n > 0) {
					sl_size m = n;
					if (m > 1024) {
						m = 1024;
					}
					n -= m;
					m_queue.pop(samples, nChannels*m);
					audioOut.copySamplesFrom(temp, m);
				}
				return sl_true;
			}
			return sl_false;
		}
	}

	void AudioRecorder::_init(const AudioRecorderParam& param)
	{
		m_param = param;
		m_queue.setQueueSize(param.samplesPerSecond * param.bufferLengthInMilliseconds / 1000 * param.channelCount);
	}

	Array<sl_int16> AudioRecorder::_getProcessData(sl_uint32 count)
	{
		Array<sl_int16> data = m_processData;
		if (data.getCount() >= count) {
			return data;
		} else {
			data = Array<sl_int16>::create(count);
			m_processData = data;
			return data;
		}
	}

	void AudioRecorder::_processFrame(sl_int16* s, sl_uint32 nSamples)
	{
		if (m_flagMute) {
			return;
		}
		if (!nSamples) {
			return;
		}

		sl_int32 volume = m_volume;
		if (volume < 256) {
			for (sl_uint32 i = 0; i < nSamples; i++) {
				s[i] = (sl_int16)((((sl_int32)(s[i])) * volume) >> 8);
			}
		}

		if (m_param.onRecordAudio.isNotNull()) {

			sl_uint32 nChannels = m_param.channelCount;
			sl_uint32 nFrames = nSamples / nChannels;
			sl_uint32 nFramesPerCallback = m_param.framesPerCallback;
			sl_uint32 nSamplesInBuf = m_nSamplesInCallbackBuffer;

			AudioData audio;
			if (nChannels == 1) {
				audio.format = AudioFormat::Int16_Mono;
			} else {
				audio.format = AudioFormat::Int16_Stereo;
			}

			if (!nFramesPerCallback || (!nSamplesInBuf && nFramesPerCallback == nFrames)) {
				audio.count = nFrames;
				audio.data = s;
				m_param.onRecordAudio(this, audio);
			} else {
				sl_uint32 nSamplesPerCallback = nFramesPerCallback * nChannels;
				if (nSamplesInBuf >= nSamplesPerCallback) {
					nSamplesInBuf = 0;
				}
				Array<sl_int16> buf = m_bufCallback;
				if (buf.getCount() != nSamplesPerCallback) {
					buf = Array<sl_int16>::create(nSamplesPerCallback);
					if (buf.isNull()) {
						return;
					}
					m_bufCallback = buf;
				}
				sl_int16* pBufData = buf.getData();
				if (nSamplesInBuf) {
					if (nSamplesInBuf + nSamples < nSamplesPerCallback) {
						Base::copyMemory(pBufData + nSamplesInBuf, s, nSamples << 1);
						m_nSamplesInCallbackBuffer = nSamplesInBuf + nSamples;
						return;
					} else {
						sl_uint32 nRemain = nSamplesPerCallback - nSamplesInBuf;
						Base::copyMemory(pBufData + nSamplesInBuf, s, nRemain << 1);
						audio.count = nFramesPerCallback;
						audio.data = pBufData;
						m_param.onRecordAudio(this, audio);
						s += nRemain;
						nSamples -= nRemain;
					}
				}
				sl_uint32 nSegments = nSamples / nSamplesPerCallback;
				for (sl_uint32 i = 0; i < nSegments; i++) {
					audio.count = nFramesPerCallback;
					audio.data = s;
					m_param.onRecordAudio(this, audio);
					s += nSamplesPerCallback;
					nSamples -= nSamplesPerCallback;
				}
				if (nSamples) {
					Base::copyMemory(pBufData, s, nSamples << 1);
				}
				m_nSamplesInCallbackBuffer = nSamples;
			}
		}

		m_queue.pushAll(s, nSamples);

		if (m_param.event.isNotNull()) {
			m_param.event->set();
		}
	}

#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_MACOS)
	String AudioRecorder::getDefaultDeviceId()
	{
		return sl_null;
	}
#endif


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioPlayerParam)

	AudioPlayerParam::AudioPlayerParam()
	{
		streamType = AudioStreamType::Default;
		samplesPerSecond = 16000;
		channelCount = 1;
		packetLengthInMilliseconds = 50;
		maxBufferLengthInMilliseconds = 0;
		flagAutoStart = sl_false;
	}


	SLIB_DEFINE_OBJECT(AudioPlayer, Object)

	AudioPlayer::AudioPlayer()
	{
		m_flagOpened = sl_true;
		m_flagRunning = sl_false;
		m_volume = 256;
		m_flagMute = sl_false;

		m_lenBufferMax = 0;
		m_lastSample = 0;
	}

	AudioPlayer::~AudioPlayer()
	{
	}

	void AudioPlayer::release()
	{
		ObjectLocker lock(this);
		if (!m_flagOpened) {
			return;
		}
		stop();
		m_flagOpened = sl_false;
		_release();
	}

	sl_bool AudioPlayer::isOpened()
	{
		return m_flagOpened;
	}

	sl_bool AudioPlayer::start()
	{
		ObjectLocker lock(this);
		if (!m_flagOpened) {
			return sl_false;
		}
		if (m_flagRunning) {
			return sl_true;
		}
		if (_start()) {
			m_flagRunning = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	void AudioPlayer::stop()
	{
		ObjectLocker lock(this);
		if (!m_flagOpened) {
			return;
		}
		if (!m_flagRunning) {
			return;
		}
		m_flagRunning = sl_false;
		_stop();
	}

	sl_bool AudioPlayer::isRunning()
	{
		return m_flagRunning;
	}

	float AudioPlayer::getVolume()
	{
		if (m_volume >= 256) {
			return 1;
		}
		if (!m_volume) {
			return 0;
		}
		return (float)(m_volume) / 256.0f;
	}

	void AudioPlayer::setVolume(float volume)
	{
		sl_int32 v = (sl_int32)(volume * 256);
		if (v >= 256) {
			v = 256;
		}
		if (v <= 0) {
			v = 0;
		}
		m_volume = v;
	}

	sl_bool AudioPlayer::isMute()
	{
		return m_flagMute;
	}

	void AudioPlayer::setMute(sl_bool flag)
	{
		m_flagMute = flag;
	}

	const AudioPlayerParam& AudioPlayer::getParam()
	{
		return m_param;
	}

	void AudioPlayer::write(const AudioData& audioIn)
	{
		AudioFormat format;
		sl_uint32 nChannels = m_param.channelCount;
		if (nChannels == 1) {
			format = AudioFormat::Int16_Mono;
		} else {
			format = AudioFormat::Int16_Stereo;
		}
		sl_size nFrames = audioIn.count;
		sl_size nSamples = nChannels * nFrames;
		sl_size nOriginal = m_buffer.getSize() >> 1;
		if (m_lenBufferMax) {
			if (nOriginal >= m_lenBufferMax) {
				return;
			}
			if (nOriginal + nSamples > m_lenBufferMax) {
				nSamples = m_lenBufferMax - nOriginal;
			}
		}
		sl_size sizeTotal = nSamples << 1;
		if (audioIn.format == format && (((sl_size)(audioIn.data)) & 1) == 0) {
			if (audioIn.ref.isNotNull()) {
				MemoryData m;
				m.data = audioIn.data;
				m.size = sizeTotal;
				m.ref = audioIn.ref;
				m_buffer.add(m);
			} else {
				m_buffer.addNew(audioIn.data, sizeTotal);
			}
		} else {
			Memory mem = Memory::create(sizeTotal);
			if (mem.isNotNull()) {
				AudioData temp;
				temp.format = format;
				temp.data = mem.getData();
				temp.count = nFrames;
				temp.copySamplesFrom(audioIn, nFrames);
				m_buffer.add(mem);
			}
		}
	}

	void AudioPlayer::flush()
	{
		m_buffer.clear();
	}

	sl_size AudioPlayer::getSampleCountInQueue()
	{
		return m_buffer.getSize() >> 1;
	}

	void AudioPlayer::_init(const AudioPlayerParam& param)
	{
		m_param = param;
		m_lenBufferMax = param.samplesPerSecond * param.maxBufferLengthInMilliseconds / 1000 * param.channelCount;
	}

	Array<sl_int16> AudioPlayer::_getProcessData(sl_uint32 count)
	{
		Array<sl_int16> data = m_processData;
		if (data.getCount() >= count) {
			return data;
		} else {
			data = Array<sl_int16>::create(count);
			m_processData = data;
			return data;
		}
	}

	void AudioPlayer::_processFrame(sl_int16* s, sl_uint32 nSamples)
	{
		if (m_param.event.isNotNull()) {
			m_param.event->set();
		}
		m_param.onPlayAudio(this, nSamples / m_param.channelCount);
		sl_uint32 nRead = ((sl_uint32)(m_buffer.pop(s, nSamples << 1))) >> 1;
		if (nRead != nSamples) {
			if (nRead) {
				m_lastSample = s[nRead - 1];
			}
			for (sl_uint32 i = nRead; i < nSamples; i++) {
				s[i] = m_lastSample;
			}
		}
		m_lastSample = s[nSamples - 1];

		if (m_flagMute) {
			for (sl_uint32 i = 0; i < nSamples; i++) {
				s[i] = 0;
			}
		} else {
			sl_int32 volume = m_volume;
			if (volume < 256) {
				for (sl_uint32 i = 0; i < nSamples; i++) {
					s[i] = (sl_int16)((((sl_int32)(s[i])) * volume) >> 8);
				}
			}
		}
	}


	SLIB_DEFINE_OBJECT(AudioPlayerDevice, Object)

	AudioPlayerDevice::AudioPlayerDevice()
	{
	}

	AudioPlayerDevice::~AudioPlayerDevice()
	{
	}

#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_MACOS)
	String AudioPlayerDevice::getDefaultDeviceId()
	{
		return sl_null;
	}
#endif

#ifndef SLIB_PLATFORM_IS_APPLE
	Ref<AudioPlayer> AudioPlayer::create(const AudioPlayerParam& param)
	{
		Ref<AudioPlayerDevice> player = AudioPlayerDevice::create(param);
		if (player.isNotNull()) {
			return player->createPlayer(param);
		}
		return sl_null;
	}

	List<AudioPlayerDeviceInfo> AudioPlayer::getDevices()
	{
		return AudioPlayerDevice::getDevices();
	}

	String AudioPlayer::getDefaultDeviceId()
	{
		return AudioPlayerDevice::getDefaultDeviceId();
	}
#endif

}
