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

#include "slib/media/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "slib/media/audio_device.h"

#include "slib/core/time_counter.h"
#include "slib/core/thread.h"
#include "slib/core/log.h"
#include "slib/core/endian.h"
#include "slib/core/scoped_buffer.h"

#include "slib/dl/linux/alsa.h"

#define TAG "Audio"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)

#define DEFAULT_PERIODS_COUNT 2

namespace slib
{

	namespace
	{
		SLIB_STATIC_STRING(g_strDefaultDeviceId, "default")

		static sl_bool SetHardwareParameters(snd_pcm_t* handle, snd_pcm_hw_params_t* hwparams, sl_uint32 nChannels, sl_uint32 sampleRate, sl_uint32 nFramesPerPeriod)
		{
			int err = snd_pcm_hw_params_any(handle, hwparams);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_any");
				return sl_false;
			}

			err = snd_pcm_hw_params_set_rate_resample(handle, hwparams, 1);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_rate_resample");
				return sl_false;
			}

			err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_access");
				return sl_false;
			}

			err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_format");
				return sl_false;
			}

			err = snd_pcm_hw_params_set_channels(handle, hwparams, (unsigned int)nChannels);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_channels");
				return sl_false;
			}

			int dir = 0;
			unsigned int _sampleRate = (unsigned int)sampleRate;
			err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &_sampleRate, &dir);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_rate");
				return sl_false;
			}

			snd_pcm_uframes_t period_size = (unsigned int)(nFramesPerPeriod);
			dir = 0;
			err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &period_size, &dir);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_period_time_near");
				return sl_false;
			}

			snd_pcm_uframes_t buffer_size = (unsigned int)(nFramesPerPeriod * DEFAULT_PERIODS_COUNT);
			err = snd_pcm_hw_params_set_buffer_size_near(handle, hwparams, &buffer_size);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_buffer_time_near");
				return sl_false;
			}

			unsigned int chunks = DEFAULT_PERIODS_COUNT;
			dir = 0;
			err = snd_pcm_hw_params_set_periods_near(handle, hwparams, &chunks, &dir);
			if (err < 0) {
				LOG_ERROR("Failed on snd_pcm_hw_params_set_periods_near");
				return sl_false;
			}

			return sl_true;
		}

		static sl_bool SetHardwareParameters(snd_pcm_t* handle, sl_uint32 nChannels, sl_uint32 sampleRate, sl_uint32 nFramesPerPeriod)
		{
			snd_pcm_hw_params_t* hwparams;
			snd_pcm_hw_params_alloca(&hwparams);
			sl_bool bRet = sl_false;
			if (SetHardwareParameters(handle, hwparams, nChannels, sampleRate, nFramesPerPeriod)) {
				if (snd_pcm_hw_params(handle, hwparams) >= 0) {
					bRet = sl_true;
				} else {
					LOG_ERROR("Failed on snd_pcm_hw_params");
				}
			}
			return bRet;
		}

		static sl_bool SetSoftwareParameters(snd_pcm_t* handle, sl_uint32 nFramesPerPeriod)
		{
			snd_pcm_sw_params_t* swparams;
			snd_pcm_sw_params_alloca(&swparams);
			snd_pcm_sw_params_current(handle, swparams);
			snd_pcm_sw_params_set_start_threshold(handle, swparams, nFramesPerPeriod);
			snd_pcm_sw_params_set_stop_threshold(handle, swparams, DEFAULT_PERIODS_COUNT * nFramesPerPeriod);
			snd_pcm_sw_params_set_avail_min(handle, swparams, nFramesPerPeriod);
			if (snd_pcm_sw_params(handle, swparams) >= 0) {
				return sl_true;
			} else {
				LOG_ERROR("Failed on snd_pcm_sw_params");
			}
			return sl_false;
		}

		static sl_bool SetParameters(snd_pcm_t* handle, sl_uint32 nChannels, sl_uint32 sampleRate, sl_uint32 nFramesPerPeriod)
		{
			if (SetHardwareParameters(handle, nChannels, sampleRate, nFramesPerPeriod)) {
				if (SetSoftwareParameters(handle, nFramesPerPeriod)) {
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class INFO>
		static void GetAllDevices(List<INFO>& ret, sl_bool flagInput)
		{
			auto func_snd_device_name_hint = slib::alsa::getApi_snd_device_name_hint();
			auto func_snd_device_name_get_hint = slib::alsa::getApi_snd_device_name_get_hint();
			auto func_snd_device_name_free_hint = slib::alsa::getApi_snd_device_name_free_hint();
			if (func_snd_device_name_hint && func_snd_device_name_get_hint && func_snd_device_name_free_hint) {
				// version 1.0.14 or later
				void** hints;
				if (func_snd_device_name_hint(-1, "pcm", &hints) < 0) {
					return;
				}
				void** p = hints;
				const char* filter = flagInput ? "Input" : "Output";
				while (*p) {
					char* name = func_snd_device_name_get_hint(*p, "NAME");
					if (name) {
						if (!(Base::equalsString(name, "null"))) {
							char* io = func_snd_device_name_get_hint(*p, "IOID");
							if (!io || Base::equalsString(io, filter)) {
								AudioDeviceInfo info;
								info.id = name;
								char* desc = func_snd_device_name_get_hint(*p, "DESC");
								if (desc) {
									info.name = desc;
									info.name = info.name.replaceAll('\n', ' ');
									free(desc);
								} else {
									info.name = info.id;
								}
								ret.add_NoLock(Move(info));
							}
							if (io) {
								free(io);
							}
						}
						free(name);
					}
					p++;
				}
				func_snd_device_name_free_hint(hints);
			} else {
				int index = 0;
				for (;;) {
					char* name = sl_null;
					if (snd_card_get_name(index, &name) != 0) {
						break;
					}
					if (name) {
						AudioDeviceInfo info;
						info.id = String::format("hw:%d,0", index);
						info.name = name;
						ret.add_NoLock(info);
					}
					index++;
				}
			}
		}
	}

	List<AudioRecorderDeviceInfo> AudioRecorder::getDevices()
	{
		List<AudioRecorderDeviceInfo> ret;
		GetAllDevices(ret, sl_true);
		return ret;
	}

	List<AudioPlayerDeviceInfo> AudioPlayerDevice::getDevices()
	{
		List<AudioRecorderDeviceInfo> ret;
		GetAllDevices(ret, sl_false);
		return ret;
	}

	namespace
	{
		class AudioRecorderImpl: public AudioRecorder
		{
		public:
			snd_pcm_t* m_handle;

			sl_uint32 m_nFramesPerPeriod;
			sl_bool m_flagRunning;

			Ref<Thread> m_thread;

		public:
			AudioRecorderImpl()
			{
				m_handle = sl_null;
				m_flagRunning = sl_false;
			}

			~AudioRecorderImpl()
			{
				release();
			}

		public:
			static Ref<AudioRecorderImpl> create(const AudioRecorderParam& param)
			{
				if (param.channelCount != 1 && param.channelCount != 2) {
					return sl_null;
				}

				StringCstr deviceId = param.deviceId;
				if (deviceId.isEmpty()) {
					deviceId = g_strDefaultDeviceId;
				}

				snd_pcm_t* handle;
				if (snd_pcm_open(&handle, deviceId.getData(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK) >= 0) {
					sl_uint32 nFramesPerPeriod = param.getFramesPerPacket();
					if (SetParameters(handle, param.channelCount, param.samplesPerSecond, nFramesPerPeriod)) {
						Ref<AudioRecorderImpl> ret = new AudioRecorderImpl;
						if (ret.isNotNull()) {
							ret->m_handle = handle;
							ret->m_nFramesPerPeriod = nFramesPerPeriod;
							ret->_init(param);
							if (param.flagAutoStart) {
								ret->start();
							}
							return ret;
						}
					}
					snd_pcm_close(handle);
				} else {
					LOG_ERROR("Failed to open capture device: %s", deviceId);
				}
				return sl_null;
			}

			void _release() override
			{
				snd_pcm_close(m_handle);
			}

			sl_bool _start() override
			{
				if (m_flagRunning) {
					if (snd_pcm_pause(m_handle, 0) < 0) {
						LOG_ERROR("Failed to resume recorder");
						return sl_false;
					}
				} else {
					m_flagRunning = sl_true;
					if (snd_pcm_prepare(m_handle) < 0) {
						LOG_ERROR("Failed to prepare recorder");
						return sl_false;
					}
					if (snd_pcm_start(m_handle) < 0) {
						LOG_ERROR("Failed to prepare recorder");
						return sl_false;
					}
				}
				m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
				if (m_thread.isNotNull()) {
					return sl_true;
				}
				return sl_false;
			}

			void _stop() override
			{
				if (m_thread.isNotNull()) {
					m_thread->finishAndWait();
					m_thread.setNull();
				}
				if (snd_pcm_pause(m_handle, 1) < 0) {
					LOG_ERROR("Failed to stop recorder");
				}
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				snd_pcm_t* handle = m_handle;
				sl_uint32 nFramesPerPeriod = m_nFramesPerPeriod;
				sl_uint32 nBytesPerFrame = (sl_uint32)(snd_pcm_frames_to_bytes(handle, 1));
				sl_uint32 nSamplesPerFrame = nBytesPerFrame >> 1;
				sl_uint32 nSamplesPerPeriod = nFramesPerPeriod * nSamplesPerFrame;

				SLIB_SCOPED_BUFFER(sl_int16, 4096, buf, nSamplesPerPeriod)
				if (!buf) {
					return;
				}

				TimeCounter t;
				sl_uint32 st = m_param.getPacketLengthInMilliseconds() / 2;

				while (thread->isNotStopping()) {
					sl_bool flagRead = sl_false;
					int nAvail = (int)(snd_pcm_avail_update(handle));
					if (nAvail == -EPIPE) {
						snd_pcm_recover(handle, nAvail, 0);
						nAvail = (int)(snd_pcm_avail_update(handle));
					}
					if (nAvail > 0) {
						if (nAvail >= (int)nFramesPerPeriod) {
							int nFrames = (int)(snd_pcm_readi(handle, buf, nFramesPerPeriod));
							if (nFrames > 0) {
								_processFrame(buf, nFrames * nSamplesPerFrame);
								flagRead = sl_true;
							}
						}
					}
					if (!flagRead) {
						sl_uint32 dt = (sl_uint32)(t.getElapsedMilliseconds());
						if (dt < st) {
							thread->wait(st - dt);
						}
						t.reset();
					}
				}
			}
		};
	}

	Ref<AudioRecorder> AudioRecorder::create(const AudioRecorderParam& param)
	{
		return AudioRecorderImpl::create(param);
	}

	namespace
	{
		class AudioPlayerImpl: public AudioPlayer
		{
		public:
			snd_pcm_t* m_handle;

			sl_uint32 m_nFramesPerPeriod;
			sl_bool m_flagRunning;

			Ref<Thread> m_thread;

		public:
			AudioPlayerImpl()
			{
				m_handle = sl_null;
				m_flagRunning = sl_false;
			}

			~AudioPlayerImpl()
			{
				release();
			}

		public:
			static Ref<AudioPlayerImpl> create(const StringParam& _deviceId, const AudioPlayerParam& param)
			{
				if (param.channelCount != 1 && param.channelCount != 2) {
					return sl_null;
				}

				StringCstr deviceId = _deviceId;
				if (deviceId.isEmpty()) {
					deviceId = g_strDefaultDeviceId;
				}

				snd_pcm_t* handle;
				if (snd_pcm_open(&handle, deviceId.getData(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) >= 0) {
					sl_uint32 nFramesPerPeriod = param.samplesPerSecond * param.packetLengthInMilliseconds / 1000;
					if (SetParameters(handle, param.channelCount, param.samplesPerSecond, nFramesPerPeriod)) {
						Ref<AudioPlayerImpl> ret = new AudioPlayerImpl;
						if (ret.isNotNull()) {
							ret->m_handle = handle;
							ret->m_nFramesPerPeriod = nFramesPerPeriod;
							ret->_init(param);
							if (param.flagAutoStart) {
								ret->start();
							}
							return ret;
						}
					}
					snd_pcm_close(handle);
				} else {
					LOG_ERROR("Failed to open play device: %s", deviceId);
				}
				return sl_null;
			}

			void _release() override
			{
				snd_pcm_close(m_handle);
			}

			sl_bool _start() override
			{
				if (m_flagRunning) {
					if (snd_pcm_pause(m_handle, 0) < 0) {
						LOG_ERROR("Failed to resume player");
						return sl_false;
					}
				} else {
					m_flagRunning = sl_true;
					if (snd_pcm_prepare(m_handle) < 0) {
						LOG_ERROR("Failed to prepare player");
						return sl_false;
					}
					if (snd_pcm_start(m_handle) < 0) {
						LOG_ERROR("Failed to prepare player");
						return sl_false;
					}
				}
				m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
				if (m_thread.isNotNull()) {
					return sl_true;
				}
				return sl_false;
			}

			void _stop() override
			{
				if (m_thread.isNotNull()) {
					m_thread->finishAndWait();
					m_thread.setNull();
				}
				if (snd_pcm_pause(m_handle, 1) < 0) {
					LOG_ERROR("Failed to stop player");
				}
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				snd_pcm_t* handle = m_handle;
				sl_uint32 nFramesPerPeriod = m_nFramesPerPeriod;
				sl_uint32 nBytesPerFrame = (sl_uint32)(snd_pcm_frames_to_bytes(handle, 1));
				sl_uint32 nSamplesPerFrame = nBytesPerFrame >> 1;
				sl_uint32 nSamplesPerPeriod = nFramesPerPeriod * nSamplesPerFrame;

				SLIB_SCOPED_BUFFER(sl_int16, 4096, buf, nSamplesPerPeriod)
				if (!buf) {
					return;
				}

				TimeCounter t;
				sl_uint32 st = m_param.packetLengthInMilliseconds / 2;

				while (thread->isNotStopping()) {
					sl_bool flagWritten = sl_false;
					int nAvail = (int)(snd_pcm_avail_update(handle));
					if (nAvail == -EPIPE) {
						snd_pcm_recover(handle, nAvail, 0);
						nAvail = (int)(snd_pcm_avail_update(handle));
					}
					if (nAvail > 0) {
						_processFrame(buf, nAvail * nSamplesPerFrame);
						int nWriten = (int)(snd_pcm_writei(handle, buf, nAvail));
						if (nWriten > 0) {
							flagWritten = sl_true;
						}
					}
					if (!flagWritten) {
						sl_uint32 dt = (sl_uint32)(t.getElapsedMilliseconds());
						if (dt < st) {
							thread->wait(st - dt);
						}
						t.reset();
					}
				}
			}
		};

		class AudioPlayerDeviceImpl : public AudioPlayerDevice
		{
		public:
			String m_deviceId;

		public:
			static Ref<AudioPlayerDeviceImpl> create(const AudioPlayerDeviceParam& param)
			{
				String deviceId = param.deviceId;
				if (deviceId.isNotEmpty()) {
					sl_bool flagFound = sl_false;
					ListElements<AudioPlayerDeviceInfo> list(AudioPlayerDevice::getDevices());
					for (sl_size i = 0; i < list.count; i++) {
						if (list[i].id == deviceId) {
							flagFound = sl_true;
							break;
						}
					}
					if (!flagFound) {
						return sl_null;
					}
				}
				Ref<AudioPlayerDeviceImpl> ret = new AudioPlayerDeviceImpl();
				if (ret.isNotNull()) {
					ret->m_deviceId = deviceId;
					return ret;
				}
				return sl_null;
			}

			Ref<AudioPlayer> createPlayer(const AudioPlayerParam& param) override
			{
				return AudioPlayerImpl::create(m_deviceId, param);
			}
		};
	}

	Ref<AudioPlayerDevice> AudioPlayerDevice::create(const AudioPlayerDeviceParam& param)
	{
		return AudioPlayerDeviceImpl::create(param);
	}

}

#endif
