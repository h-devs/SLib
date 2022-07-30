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

#include "slib/media/definition.h"

#ifdef SLIB_PLATFORM_IS_ANDROID

#include "slib/media/audio_device.h"

#include "slib/core/log.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#define TAG "Audio"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)

namespace slib
{

	namespace priv
	{
		namespace opensl_es
		{
			
			class AudioRecorderImpl : public AudioRecorder
			{
			public:
				sl_bool m_flagInitialized;

				SLObjectItf m_engineObject;
				SLEngineItf m_engineInterface;

				SLObjectItf m_recorderObject;
				SLRecordItf m_recordInterface;
				SLAndroidSimpleBufferQueueItf m_bufferQueue;

				Memory m_memFrame;
				sl_int16* m_bufFrame;
				sl_uint32 m_indexBuffer;
				sl_uint32 m_nSamplesFrame;
				
			public:
				AudioRecorderImpl()
				{
					m_flagInitialized = sl_false;
					m_indexBuffer = 0;
				}

				~AudioRecorderImpl()
				{
					if (m_flagInitialized) {
						release();
						m_flagInitialized = sl_false;
					}
				}

			public:
				static Ref<AudioRecorderImpl> create(const AudioRecorderParam& param)
				{
					if (param.channelCount != 1 && param.channelCount != 2) {
						return sl_null;
					}
					
					SLObjectItf engineObject = sl_null;
					SLEngineItf engineInterface = sl_null;

					SLEngineOption options[] = {
						{SL_ENGINEOPTION_THREADSAFE, 1},
					};
					if (slCreateEngine(&engineObject, 1, options, 0, sl_null, sl_null) == SL_RESULT_SUCCESS) {
						sl_bool flagSuccess = sl_false;
						if ((*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE) == SL_RESULT_SUCCESS) {
							if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,  &engineInterface) == SL_RESULT_SUCCESS) {
								flagSuccess = sl_true;
							} else {
								LOG_ERROR("Failed to get engine interface");
							}
						} else {
							LOG_ERROR("Failed to realize engine");
						}
						if (!flagSuccess) {
							(*engineObject)->Destroy(engineObject);
							return sl_null;
						}
					} else {
						LOG_ERROR("Failed to create engine");
						return sl_null;
					}

					SLDataLocator_IODevice androidMicDevice = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, sl_null };
					SLDataSource slDataSource = { &androidMicDevice, sl_null };
					SLDataLocator_AndroidSimpleBufferQueue androidSBQ = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
					SLDataFormat_PCM configuration;
					{
						configuration.formatType = SL_DATAFORMAT_PCM;
						configuration.numChannels = param.channelCount;
						configuration.samplesPerSec = param.samplesPerSecond * 1000;
						configuration.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
						configuration.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
						if (configuration.numChannels == 2) {
							configuration.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
						} else {
							configuration.channelMask = SL_SPEAKER_FRONT_CENTER;
						}
						configuration.endianness = SL_BYTEORDER_LITTLEENDIAN;
					}
					SLDataSink slDataSink = { &androidSBQ, &configuration };

					const SLInterfaceID id[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
					const SLboolean req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

					SLObjectItf recorderObject;
					SLRecordItf recordInterface;
					SLAndroidSimpleBufferQueueItf bufferQueue;

					if ((*engineInterface)->CreateAudioRecorder(engineInterface, &recorderObject, &slDataSource, &slDataSink, 2, id, req) == SL_RESULT_SUCCESS) {
						
						if (param.recordingPreset != AudioRecordingPreset::None) {
							SLAndroidConfigurationItf confAndroid;
							if ((*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION, &confAndroid) == SL_RESULT_SUCCESS) {
								SLuint32 presetValue = (SLuint32)(param.recordingPreset);
								(*confAndroid)->SetConfiguration(confAndroid, SL_ANDROID_KEY_RECORDING_PRESET, &presetValue, sizeof(SLuint32));
							}
						}

						if ((*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE) == SL_RESULT_SUCCESS) {

							if ((*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recordInterface) == SL_RESULT_SUCCESS) {
								
								if ((*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bufferQueue) == SL_RESULT_SUCCESS) {
									
									sl_uint32 nSamplesPerFrame = param.getSamplesPerFrame() * param.channelCount;
									Memory memFrame = Memory::create(nSamplesPerFrame << 2);

									if (memFrame.isNotNull()) {

										Ref<AudioRecorderImpl> ret = new AudioRecorderImpl();

										if (ret.isNotNull()) {

											if ((*bufferQueue)->RegisterCallback(bufferQueue, AudioRecorderImpl::callback, ret.get()) == SL_RESULT_SUCCESS) {
												
												ret->m_engineInterface = engineInterface;
												ret->m_engineObject = engineObject;
												ret->m_recorderObject = recorderObject;
												ret->m_recordInterface = recordInterface;
												ret->m_bufferQueue = bufferQueue;
												ret->m_nSamplesFrame = nSamplesPerFrame;
												ret->m_memFrame = memFrame;
												ret->m_bufFrame = (sl_int16*)(memFrame.getData());
												Base::zeroMemory(memFrame.getData(), memFrame.getSize());

												ret->_init(param);
												
												ret->m_flagInitialized = sl_true;

												if (param.flagAutoStart) {
													ret->start();
												}

												return ret;

											} else {
												LOG_ERROR("Failed to register callback");
											}
											
										}
									}
								} else {
									LOG_ERROR("Failed to get buffer queue");
								}
							} else {
								LOG_ERROR("Failed to get recorder interface");
							}
						} else {
							LOG_ERROR("Failed to realize recorder object");
						}
						(*recorderObject)->Destroy(recorderObject);
					} else {
						LOG_ERROR("Failed to create recorder object");
					}

					(*engineObject)->Destroy(engineObject);
					
					return sl_null;
				}

				void _release() override
				{
					(*m_recorderObject)->Destroy(m_recorderObject);
					(*m_engineObject)->Destroy(m_engineObject);
				}

				sl_bool _start() override
				{
					if (onFrame()) {
						if ((*m_recordInterface)->SetRecordState(m_recordInterface, SL_RECORDSTATE_RECORDING) != SL_RESULT_SUCCESS) {
							LOG_ERROR("Failed to record buffer");
							return sl_false;
						}
						return sl_true;
					}
					return sl_false;
				}

				void _stop() override
				{
					if ((*m_bufferQueue)->Clear(m_bufferQueue) != SL_RESULT_SUCCESS) {
						LOG_ERROR("Failed to clear buffer queue");
						return;
					}
					(*m_recordInterface)->SetRecordState(m_recordInterface, SL_RECORDSTATE_STOPPED);
				}

				sl_bool onFrame()
				{
					m_indexBuffer = (m_indexBuffer + 1) % 2;
					if ((*m_bufferQueue)->Enqueue(m_bufferQueue, m_bufFrame + m_indexBuffer * m_nSamplesFrame, m_nSamplesFrame * sizeof(sl_int16)) == SL_RESULT_SUCCESS) {
						sl_int16* s = m_bufFrame + m_indexBuffer * m_nSamplesFrame;
						_processFrame(s, m_nSamplesFrame);
						return sl_true;
					} else {
						LOG_ERROR("Failed to enqueue buffer");
					}
					return sl_false;
				}

				static void	callback(SLAndroidSimpleBufferQueueItf bufferQueue, void* p_context)
				{
					AudioRecorderImpl* object = (AudioRecorderImpl*)p_context;
					if (object->m_flagInitialized) {
						object->onFrame();
					}
				}
			};


			class AudioPlayerDeviceImpl : public AudioPlayerDevice
			{
			public:
				SLObjectItf m_engineObject;
				SLEngineItf m_engineInterface;
				SLObjectItf m_mixerObject;

			public:
				AudioPlayerDeviceImpl()
				{
				}

				~AudioPlayerDeviceImpl()
				{
					(*m_mixerObject)->Destroy(m_mixerObject);
					(*m_engineObject)->Destroy(m_engineObject);
				}

			public:
				static Ref<AudioPlayerDeviceImpl> create(const AudioPlayerDeviceParam& param)
				{
					SLObjectItf engineObject;
					SLEngineItf engineInterface;
					SLObjectItf mixerObject;

					SLEngineOption options[] = {
						{SL_ENGINEOPTION_THREADSAFE, 1},
					};
					if (slCreateEngine(&engineObject, 1, options, 0, sl_null, sl_null) == SL_RESULT_SUCCESS) {
						if ((*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE) == SL_RESULT_SUCCESS) {
							if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,  &engineInterface) == SL_RESULT_SUCCESS) {
								if ((*engineInterface)->CreateOutputMix(engineInterface, &mixerObject, 0, sl_null, sl_null) == SL_RESULT_SUCCESS) {
									if ((*mixerObject)->Realize(mixerObject, SL_BOOLEAN_FALSE) == SL_RESULT_SUCCESS) {
										Ref<AudioPlayerDeviceImpl> ret = new AudioPlayerDeviceImpl();
										if (ret.isNotNull()) {
											ret->m_engineObject = engineObject;
											ret->m_engineInterface = engineInterface;
											ret->m_mixerObject = mixerObject;
											return ret;
										}
									} else {
										LOG_ERROR("Failed to realize output mixer");
									}
									(*mixerObject)->Destroy(mixerObject);
								} else {
									LOG_ERROR("Failed to create output mixer");
								}
							} else {
								LOG_ERROR("Failed to get engine interface");
							}
						} else {
							LOG_ERROR("Failed to realize engine");
						}
						(*engineObject)->Destroy(engineObject);
					} else {
						LOG_ERROR("Failed to create engine");
					}
					return sl_null;
				}

				Ref<AudioPlayer> createPlayer(const AudioPlayerParam& param) override;

			};

			class AudioPlayerImpl : public AudioPlayer
			{
			public:
				sl_bool m_flagInitialized;
			
				Ref<AudioPlayerDeviceImpl> m_engine;
				SLObjectItf m_playerObject;
				SLPlayItf m_playerInterface;
				SLAndroidSimpleBufferQueueItf m_bufferQueue;

				Memory m_memFrame;
				sl_int16* m_bufFrame;
				sl_uint32 m_indexBuffer;
				sl_uint32 m_nSamplesFrame;

			public:
				AudioPlayerImpl()
				{
					m_flagInitialized = sl_false;
					m_indexBuffer = 0;
				}
				
				~AudioPlayerImpl()
				{
					if (m_flagInitialized) {
						release();
						m_flagInitialized = sl_false;
					}
				}

			public:
				static Ref<AudioPlayerImpl> create(Ref<AudioPlayerDeviceImpl> engine, const AudioPlayerParam& param)
				{					
					if (param.channelCount != 1 && param.channelCount != 2) {
						return sl_null;
					}
					
					SLEngineItf engineInterface = engine->m_engineInterface;

					SLDataLocator_AndroidSimpleBufferQueue androidSBQ = {
						SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
					};
					SLDataFormat_PCM configuration;
					{
						configuration.formatType = SL_DATAFORMAT_PCM;
						configuration.numChannels = param.channelCount;
						configuration.samplesPerSec = param.samplesPerSecond * 1000;
						configuration.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
						configuration.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
						if (configuration.numChannels == 2) {
							configuration.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
						} else {
							configuration.channelMask = SL_SPEAKER_FRONT_CENTER;
						}
						configuration.endianness = SL_BYTEORDER_LITTLEENDIAN;
					}
					SLDataSource slDataSource = { &androidSBQ, &configuration };

					SLDataLocator_OutputMix outputMix;
					outputMix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
					outputMix.outputMix = engine->m_mixerObject;
					SLDataSink slDataSink = { &outputMix, sl_null };

					SLObjectItf playerObject;
					SLPlayItf playerInterface;
					SLAndroidSimpleBufferQueueItf bufferQueue;

					SLInterfaceID ids[3] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_ANDROIDCONFIGURATION };
					SLboolean req[3] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

					if ((*engineInterface)->CreateAudioPlayer(engineInterface, &playerObject, &slDataSource, &slDataSink, 3, ids, req) == SL_RESULT_SUCCESS) {
						
						if (param.streamType != AudioStreamType::Default) {
							SLAndroidConfigurationItf confAndroid;
							if ((*playerObject)->GetInterface(playerObject, SL_IID_ANDROIDCONFIGURATION, &confAndroid) == SL_RESULT_SUCCESS) {
								SLuint32 presetValue = (SLuint32)(param.streamType);
								(*confAndroid)->SetConfiguration(confAndroid, SL_ANDROID_KEY_STREAM_TYPE, &presetValue, sizeof(SLuint32));
							}
						}

						if ((*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE) == SL_RESULT_SUCCESS) {
							
							if ((*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerInterface) == SL_RESULT_SUCCESS) {
								
								if ((*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &bufferQueue) == SL_RESULT_SUCCESS) {
									
									sl_uint32 nSamplesPerFrame = param.samplesPerSecond * param.frameLengthInMilliseconds / 1000 * param.channelCount;
									Memory memFrame = Memory::create(nSamplesPerFrame << 2);

									if (memFrame.isNotNull()) {

										Ref<AudioPlayerImpl> ret = new AudioPlayerImpl();
										
										if (ret.isNotNull()) {
											
											if ((*bufferQueue)->RegisterCallback(bufferQueue, AudioPlayerImpl::callback, ret.get()) == SL_RESULT_SUCCESS) {

												ret->m_engine = engine;
												ret->m_playerObject = playerObject;
												ret->m_playerInterface = playerInterface;
												ret->m_bufferQueue = bufferQueue;
												ret->m_nSamplesFrame = nSamplesPerFrame;
												ret->m_bufFrame = (sl_int16*)(memFrame.getData());
												Base::zeroMemory(memFrame.getData(), memFrame.getSize());

												ret->_init(param);
											
												ret->m_flagInitialized = sl_true;

												if (param.flagAutoStart) {
													ret->start();
												}

												return ret;

											} else {
												LOG_ERROR("Failed to register callback");
											}
										}
									}
								} else {
									LOG_ERROR("Failed to get buffer queue");
								}
							} else {
								LOG_ERROR("Failed to get player interface");
							}
						} else {
							LOG_ERROR("Failed to realize player object");
						}
						(*playerObject)->Destroy(playerObject);
					} else {
						LOG_ERROR("Failed to create player object");
					}

					return sl_null;
				}

				void _release() override
				{
					(*m_playerObject)->Destroy(m_playerObject);
				}

				sl_bool _start() override
				{
					if (enqueue()) {
						if ((*m_playerInterface)->SetPlayState(m_playerInterface, SL_PLAYSTATE_PLAYING) != SL_RESULT_SUCCESS) {
							LOG_ERROR("Failed to play buffer");
							return sl_false;
						}
						return sl_true;
					}
					return sl_false;
				}

				void _stop() override
				{
					if ((*m_bufferQueue)->Clear(m_bufferQueue) != SL_RESULT_SUCCESS) {
						LOG_ERROR("Failed to clear buffer queue");
						return;
					}
					(*m_playerInterface)->SetPlayState(m_playerInterface, SL_PLAYSTATE_STOPPED);
				}

				sl_bool enqueue()
				{
					m_indexBuffer = (m_indexBuffer + 1) % 2;
					sl_int16* s = m_bufFrame + m_indexBuffer * m_nSamplesFrame;
					_processFrame(s, m_nSamplesFrame);
					if ((*m_bufferQueue)->Enqueue(m_bufferQueue, s, m_nSamplesFrame * 2) == SL_RESULT_SUCCESS) {
						return sl_true;
					} else {
						LOG_ERROR("Failed to enqueue buffer");
					}
					return sl_false;
				}

				void onFrame()
				{
					enqueue();
				}

				static void	callback(SLAndroidSimpleBufferQueueItf bufferQueue, void* p_context)
				{
					AudioPlayerImpl* object = (AudioPlayerImpl*)p_context;
					if (object->m_flagInitialized) {
						object->onFrame();
					}
				}
			};

			Ref<AudioPlayer> AudioPlayerDeviceImpl::createPlayer(const AudioPlayerParam& param)
			{
				return AudioPlayerImpl::create(this, param);
			}

		}
	}

	using namespace priv::opensl_es;

	Ref<AudioRecorder> AudioRecorder::create(const AudioRecorderParam& param)
	{
		return AudioRecorderImpl::create(param);
	}

	List<AudioRecorderInfo> AudioRecorder::getRecordersList()
	{
		AudioRecorderInfo ret;
		SLIB_STATIC_STRING(s, "Internal Microphone");
		ret.name = s;
		return List<AudioRecorderInfo>::createFromElement(ret);
	}

	Ref<AudioPlayerDevice> AudioPlayerDevice::create(const AudioPlayerDeviceParam& param)
	{
		return AudioPlayerDeviceImpl::create(param);
	}

	List<AudioPlayerInfo> AudioPlayerDevice::getPlayersList()
	{
		AudioPlayerInfo ret;
		SLIB_STATIC_STRING(s, "Internal Speaker");
		ret.name = s;
		return List<AudioPlayerInfo>::createFromElement(ret);
	}

}

#endif
