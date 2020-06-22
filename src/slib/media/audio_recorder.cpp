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

#include "slib/media/audio_recorder.h"

#include "slib/media/audio_format.h"

namespace slib
{
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioRecorderInfo)

	AudioRecorderInfo::AudioRecorderInfo()
	{
	}
	
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioRecorderParam)
	
	AudioRecorderParam::AudioRecorderParam()
	{
		recordingPreset = AudioRecordingPreset::None;
		
		samplesPerSecond = 16000;
		channelsCount = 1;
		frameLengthInMilliseconds = 50;
		bufferLengthInMilliseconds = 1000;
		flagAutoStart = sl_true;
	}


	SLIB_DEFINE_OBJECT(AudioRecorder, Object)

	AudioRecorder::AudioRecorder()
	{
		m_flagOpened = sl_true;
		m_flagRunning = sl_false;
		m_volume = 256;
		m_flagMute = sl_false;
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
		sl_uint32 nChannels = m_param.channelsCount;
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
			if (n > m_queue.getCount()) {
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
		m_queue.setQueueSize(param.samplesPerSecond * param.bufferLengthInMilliseconds / 1000 * param.channelsCount);
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

	void AudioRecorder::_processFrame(sl_int16* s, sl_uint32 count)
	{
		if (m_flagMute) {
			return;
		}
		sl_int32 volume = m_volume;
		if (volume < 256) {
			for (sl_uint32 i = 0; i < count; i++) {
				s[i] = (sl_int16)((((sl_int32)(s[i])) * volume) >> 8);
			}
		}
		if (m_param.onRecordAudio.isNotNull()) {
			AudioData audio;
			AudioFormat format;
			sl_uint32 nChannels = m_param.channelsCount;
			if (nChannels == 1) {
				format = AudioFormat::Int16_Mono;
			} else {
				format = AudioFormat::Int16_Stereo;
			}
			audio.format = format;
			audio.count = count / nChannels;
			audio.data = s;
			m_param.onRecordAudio(this, audio);
		}
		m_queue.push(s, count);
		if (m_param.event.isNotNull()) {
			m_param.event->set();
		}
	}

}
