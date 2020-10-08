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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "slib/media/audio_device.h"

#include "slib/core/thread.h"
#include "slib/core/time.h"
#include "slib/core/log.h"
#include "slib/core/endian.h"
#include "slib/core/scoped.h"

#include "slib/media/dl_linux_alsa.h"

#define TAG "Audio"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)

#define DEFAULT_PERIODS_COUNT 4

namespace slib
{

	namespace priv
	{
		namespace alsa
		{

			struct AudioDeviceInfo
			{
				String id;
				String name;
				String description;
			};
			
			static List<AudioDeviceInfo> GetAllDevices(sl_bool flagInput)
			{
				List<AudioDeviceInfo> ret;
				auto func_snd_device_name_hint = slib::alsa::getApi_snd_device_name_hint();
				auto func_snd_device_name_get_hint = slib::alsa::getApi_snd_device_name_get_hint();
				auto func_snd_device_name_free_hint = slib::alsa::getApi_snd_device_name_free_hint();
				if (func_snd_device_name_hint && func_snd_device_name_get_hint && func_snd_device_name_free_hint) {
					// version 1.0.14 or later
					void** hints;
					if (func_snd_device_name_hint(-1, "pcm", &hints) < 0) {
						return sl_null;
					}
					void** p = hints;
					const char* filter;
					if(flagInput) {
						filter = "Input";
					} else {
						filter = "Output";
					}
					while (*p) {
						char* name = func_snd_device_name_get_hint(*p, "NAME");
						if (name && !(Base::equalsString(name, "null"))) {
							char* description = func_snd_device_name_get_hint(*p, "DESC");
							char* io = func_snd_device_name_get_hint(*p, "IOID");
							if (description && (!io || Base::equalsString(io, filter))) {
								AudioDeviceInfo info;
								info.name = name;
								info.description = description;
								info.id = info.name;
								ret.add_NoLock(info);
							}
							free(description);
							free(io);
						}
						free(name);
						p++;
					}
					func_snd_device_name_free_hint(hints);
				} else {
					int index = 0;
					for (;;) {
						char* name = sl_null;
						char* description = sl_null;
						if (snd_card_get_name(index, &name) != 0) {
							break;
						}
						if (name) {
							snd_card_get_longname(index, &description);
							AudioDeviceInfo info;
							info.id = String::format("hw:%d,0", index);
							info.name = name;
							if (description) {
								info.description = description;
							} else {
								info.description = name;
							}
							ret.add_NoLock(info);
						}
						index++;
					}
				}
				return ret;
			}

			static sl_bool GetDefaultDevice(AudioDeviceInfo& outInfo, sl_bool flagInput)
			{
				ListElements<AudioDeviceInfo> list = GetAllDevices(flagInput);
				for (sl_size i = 0; i < list.count; i++) {
					if (list[i].name == "default") {
						outInfo = list[i];
						return sl_true;
					}
				}
				if (list.count > 0) {
					outInfo = list[0];
					return sl_true;
				}
				return sl_false;
			}

			static sl_bool SetHardwareParameters(snd_pcm_t* handle, snd_pcm_hw_params_t* hwparams, sl_uint32 nChannels, sl_uint32 sampleRate, sl_uint32 frameTime)
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

				if (Endian::isLE()) {
					err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_LE);
				} else {
					err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_BE);
				}
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
				
				unsigned int buffer_time = (unsigned int)(frameTime * DEFAULT_PERIODS_COUNT);
				err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
				if (err < 0) {
					LOG_ERROR("Failed on snd_pcm_hw_params_set_buffer_time_near");
					return sl_false;
				}

				unsigned int period_time = (unsigned int)(frameTime);
				dir = 0;
				err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
				if (err < 0) {
					LOG_ERROR("Failed on snd_pcm_hw_params_set_period_time_near");
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
			
			class AudioRecorderImpl: public AudioRecorder
			{
			public:
				snd_pcm_t* m_handle;

				Ref<Thread> m_thread;
				
			public:
				AudioRecorderImpl()
				{
					m_handle = sl_null;
				}
				
				~AudioRecorderImpl()
				{
					release();
				}
				
			public:
				static Ref<AudioRecorderImpl> create(const AudioRecorderParam& param)
				{
					if (param.channelsCount != 1 && param.channelsCount != 2) {
						return sl_null;
					}

					String deviceId = param.deviceId;
					if (deviceId.isEmpty()) {
						AudioDeviceInfo info;
						if (GetDefaultDevice(info, sl_true)) {
							deviceId = info.id;
						} else {
							return sl_null;
						}
					}

					snd_pcm_t* handle;
					if (snd_pcm_open(&handle, deviceId.getData(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK) >= 0) {

						snd_pcm_hw_params_t* hwparams;
						snd_pcm_hw_params_alloca(&hwparams);

						if (SetHardwareParameters(handle, hwparams, param.channelsCount, param.samplesPerSecond, param.frameLengthInMilliseconds)) {
							if (snd_pcm_hw_params(handle, hwparams) >= 0) {
								
								snd_pcm_sw_params_t* swparams;
								snd_pcm_sw_params_alloca(&swparams);
								snd_pcm_sw_params_current(handle, swparams);
								snd_pcm_sw_params_set_start_threshold(handle, swparams, 1);
								snd_pcm_sw_params_set_stop_threshold(handle, swparams, DEFAULT_PERIODS_COUNT);
								snd_pcm_sw_params_set_avail_min(handle, swparams, 1);
								snd_pcm_sw_params(handle, swparams);

								Ref<AudioRecorderImpl> ret = new AudioRecorderImpl;

								if (ret.isNotNull()) {

									ret->m_handle = handle;
									
									ret->_init(param);

									if (param.flagAutoStart) {
										ret->start();
									}

									return ret;
								}

							} else {
								LOG_ERROR("Failed on snd_pcm_hw_params");
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
					if (snd_pcm_prepare(m_handle) >= 0) {
						if (snd_pcm_start(m_handle) >= 0) {
							m_thread = Thread::start(SLIB_FUNCTION_WEAKREF(AudioRecorderImpl, run, this));
							if (m_thread.isNotNull()) {
								return sl_true;
							}
						} else {
							LOG_ERROR("Failed to start");
						}
					} else {
						LOG_ERROR("Failed to prepare");
					}
					return sl_false;
				}
				
				void _stop() override
				{
					if (m_thread.isNotNull()) {
						m_thread->finishAndWait();
						m_thread.setNull();
					}
					if (snd_pcm_drain(m_handle) < 0) {
						LOG_ERROR("Failed to stop");
					}
				}

				void run()
				{
					Ref<Thread> thread = Thread::getCurrent();
					if (thread.isNull()) {
						return;
					}

					snd_pcm_t* handle = m_handle;
					sl_uint32 nBytesPerFrame = (sl_uint32)(snd_pcm_frames_to_bytes(handle, 1));
					
					SLIB_SCOPED_BUFFER(sl_int16, 4096, buf, nBytesPerFrame)
					
					TimeCounter t;
					sl_uint32 st = m_param.frameLengthInMilliseconds / 2;
					
					while (thread->isNotStopping()) {
						int nFrames = (int)(snd_pcm_readi(handle, buf, 1));
						if (nFrames == 1) {
							_processFrame(buf, nBytesPerFrame);
						} else {
							sl_uint32 dt = (sl_uint32)(t.getElapsedMilliseconds());
							if (dt < st) {
								Thread::sleep(st - dt);
							}
							t.reset();
						}
					}
				}
				
			};
			
			class AudioPlayerImpl: public AudioPlayer
			{
			public:
				snd_pcm_t* m_handle;

				Ref<Thread> m_thread;
				
			public:
				AudioPlayerImpl()
				{
					m_handle = sl_null;
				}
				
				~AudioPlayerImpl()
				{
					release();
				}
				
			public:
				static Ref<AudioPlayerImpl> create(const String& _deviceId, const AudioPlayerParam& param)
				{
					if (param.channelsCount != 1 && param.channelsCount != 2) {
						return sl_null;
					}

					String deviceId = _deviceId;
					if (deviceId.isEmpty()) {
						AudioDeviceInfo info;
						if (GetDefaultDevice(info, sl_false)) {
							deviceId = info.id;
						} else {
							return sl_null;
						}
					}

					snd_pcm_t* handle;
					if (snd_pcm_open(&handle, deviceId.getData(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) >= 0) {

						snd_pcm_hw_params_t* hwparams;
						snd_pcm_hw_params_alloca(&hwparams);

						if (SetHardwareParameters(handle, hwparams, param.channelsCount, param.samplesPerSecond, param.frameLengthInMilliseconds)) {
							if (snd_pcm_hw_params(handle, hwparams) >= 0) {
								
								snd_pcm_sw_params_t* swparams;
								snd_pcm_sw_params_alloca(&swparams);
								snd_pcm_sw_params_current(handle, swparams);
								snd_pcm_sw_params_set_start_threshold(handle, swparams, 1);
								snd_pcm_sw_params_set_stop_threshold(handle, swparams, DEFAULT_PERIODS_COUNT);
								snd_pcm_sw_params_set_avail_min(handle, swparams, 1);
								snd_pcm_sw_params(handle, swparams);

								Ref<AudioPlayerImpl> ret = new AudioPlayerImpl;

								if (ret.isNotNull()) {

									ret->m_handle = handle;
									
									ret->_init(param);

									if (param.flagAutoStart) {
										ret->start();
									}

									return ret;
								}
								
							} else {
								LOG_ERROR("Failed on snd_pcm_hw_params");
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
					if (snd_pcm_prepare(m_handle) >= 0) {
						if (snd_pcm_start(m_handle) >= 0) {
							m_thread = Thread::start(SLIB_FUNCTION_WEAKREF(AudioPlayerImpl, run, this));
							if (m_thread.isNotNull()) {
								return sl_true;
							}
						} else {
							LOG_ERROR("Failed to start");
						}
					} else {
						LOG_ERROR("Failed to prepare");
					}
					return sl_false;
				}
				
				void _stop() override
				{
					if (m_thread.isNotNull()) {
						m_thread->finishAndWait();
						m_thread.setNull();
					}
					if (snd_pcm_drain(m_handle) < 0) {
						LOG_ERROR("Failed to stop");
					}
				}

				void run()
				{
					Ref<Thread> thread = Thread::getCurrent();
					if (thread.isNull()) {
						return;
					}

					snd_pcm_t* handle = m_handle;
					sl_uint32 nBytesPerFrame = (sl_uint32)(snd_pcm_frames_to_bytes(handle, 1));
					
					SLIB_SCOPED_BUFFER(sl_int16, 4096, buf, nBytesPerFrame)
					
					TimeCounter t;
					sl_uint32 st = m_param.frameLengthInMilliseconds / 2;
					
					while (thread->isNotStopping()) {
						sl_bool flagWritten = sl_false;
						int nAvail = (int)(snd_pcm_avail_update(handle));
						if (nAvail > 0) {
							_processFrame(buf, nBytesPerFrame);
							int nFrames = (int)(snd_pcm_writei(handle, buf, 1));
							if (nFrames == 1) {
								flagWritten = sl_true;
							}
						}
						if (!flagWritten) {
							sl_uint32 dt = (sl_uint32)(t.getElapsedMilliseconds());
							if (dt < st) {
								Thread::sleep(st - dt);
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
				AudioPlayerDeviceImpl()
				{
				}

				~AudioPlayerDeviceImpl()
				{
				}
				
			public:
				static Ref<AudioPlayerDeviceImpl> create(const AudioPlayerDeviceParam& param)
				{
					String deviceId = param.deviceId;
					if (deviceId.isEmpty()) {
						AudioDeviceInfo info;
						if (GetDefaultDevice(info, sl_false)) {
							deviceId = info.id;
						} else {
							return sl_null;
						}
					} else {
						sl_bool flagFound = sl_false;
						ListElements<AudioDeviceInfo> list(GetAllDevices(sl_false));
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
	}

	using namespace priv::alsa;

	Ref<AudioRecorder> AudioRecorder::create(const AudioRecorderParam& param)
	{
		return AudioRecorderImpl::create(param);
	}

	List<AudioRecorderInfo> AudioRecorder::getRecordersList()
	{
		ListElements<AudioDeviceInfo> list(GetAllDevices(sl_true));
		List<AudioRecorderInfo> ret;
		for (sl_size i = 0; i < list.count; i++) {
			AudioRecorderInfo info;
			info.id = list[i].id;
			info.name = list[i].name;
			info.description = list[i].description;
			ret.add_NoLock(info);
		}
		return ret;
	}

	Ref<AudioPlayerDevice> AudioPlayerDevice::create(const AudioPlayerDeviceParam& param)
	{
		return AudioPlayerDeviceImpl::create(param);
	}

	List<AudioPlayerInfo> AudioPlayerDevice::getPlayersList()
	{
		ListElements<AudioDeviceInfo> list(GetAllDevices(sl_false));
		List<AudioPlayerInfo> ret;
		for (sl_size i = 0; i < list.count; i++) {
			AudioPlayerInfo info;
			info.id = list[i].id;
			info.name = list[i].name;
			info.description = list[i].description;
			ret.add_NoLock(info);
		}
		return ret;
	}

}

#endif
