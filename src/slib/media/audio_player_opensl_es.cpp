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

#include "slib/media/opensl_es.h"

#ifdef SLIB_AUDIO_SUPPORT_OPENSL_ES

#include "slib/core/log.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#endif

namespace slib
{

	namespace priv
	{
		namespace opensl_es
		{

			class AudioPlayerImpl : public AudioPlayer
			{
			public:
				SLObjectItf m_engineObject;
				SLEngineItf m_engineInterface;
				SLObjectItf m_mixerObject;

			public:
				AudioPlayerImpl()
				{
				}

				~AudioPlayerImpl()
				{
					(*m_mixerObject)->Destroy(m_mixerObject);
					(*m_engineObject)->Destroy(m_engineObject);
				}

			public:
				static void logError(String text)
				{
					LogError("OpenSL_ES", text);
				}

				static Ref<AudioPlayerImpl> create(const AudioPlayerParam& param)
				{
					Ref<AudioPlayerImpl> ret;

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
										ret = new AudioPlayerImpl();
										if (ret.isNotNull()) {
											ret->m_engineObject = engineObject;
											ret->m_engineInterface = engineInterface;
											ret->m_mixerObject = mixerObject;
											return ret;
										}
									} else {
										logError("Failed to realize output mixer");
									}
									(*mixerObject)->Destroy(mixerObject);
								} else {
									logError("Failed to create output mixer");
								}
							} else {
								logError("Failed to get engine interface");
							}
						} else {
							logError("Failed to realize engine");
						}
						(*engineObject)->Destroy(engineObject);
					} else {
						logError("Failed to create engine");
					}
					return ret;
				}

				Ref<AudioPlayerBuffer> createBuffer(const AudioPlayerBufferParam& param);

			};

			class AudioPlayerBufferImpl : public AudioPlayerBuffer
			{
			public:
				Ref<AudioPlayerImpl> m_engine;
				SLObjectItf m_playerObject;
				SLPlayItf m_playerInterface;
				SLAndroidSimpleBufferQueueItf m_bufferQueue;

				sl_int16* m_bufFrame;
				sl_uint32 m_indexBuffer;
				sl_uint32 m_nSamplesFrame;

			public:
				AudioPlayerBufferImpl()
				{
					m_indexBuffer = 0;
				}
				
				~AudioPlayerBufferImpl()
				{
					release();
				}

			public:
				static void logError(String text)
				{
					LogError("OpenSL_ES_Buffer", text);
				}

				static Ref<AudioPlayerBufferImpl> create(Ref<AudioPlayerImpl> engine, const AudioPlayerBufferParam& param)
				{
					Ref<AudioPlayerBufferImpl> ret;
					
					if (param.channelsCount != 1 && param.channelsCount != 2) {
						return ret;
					}
					
					SLEngineItf engineInterface = engine->m_engineInterface;

					SLDataLocator_AndroidSimpleBufferQueue androidSBQ = {
						SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
					};
					SLDataFormat_PCM configuration;
					{
						configuration.formatType = SL_DATAFORMAT_PCM;
						configuration.numChannels = param.channelsCount;
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
									ret = new AudioPlayerBufferImpl();
									if (ret.isNotNull()) {
										ret->m_engine = engine;
										ret->m_playerObject = playerObject;
										ret->m_playerInterface = playerInterface;
										ret->m_bufferQueue = bufferQueue;
										ret->m_nSamplesFrame = param.samplesPerSecond * param.frameLengthInMilliseconds / 1000 * param.channelsCount;
										ret->m_bufFrame = new sl_int16[ret->m_nSamplesFrame * 2];
										
										ret->_init(param);
										
										if (ret->m_bufFrame) {
											if ((*bufferQueue)->RegisterCallback(bufferQueue, AudioPlayerBufferImpl::callback, ret.get()) == SL_RESULT_SUCCESS) {
												if (param.flagAutoStart) {
													ret->start();
												}
												return ret;
											} else {
												logError("Failed to register callback");
											}
										}
										return sl_null;
									}
								} else {
									logError("Failed to get buffer queue");
								}
							} else {
								logError("Failed to get player interface");
							}
						} else {
							logError("Failed to realize player object");
						}
						(*playerObject)->Destroy(playerObject);
					} else {
						logError("Failed to create player object");
					}
					return ret;
				}

				void _release() override
				{
					(*m_playerObject)->Destroy(m_playerObject);
					if (m_bufFrame) {
						delete[] m_bufFrame;
					}
				}

				sl_bool _start() override
				{
					if (enqueue()) {
						if ((*m_playerInterface)->SetPlayState(m_playerInterface, SL_PLAYSTATE_PLAYING) != SL_RESULT_SUCCESS) {
							logError("Failed to play buffer");
							return sl_false;
						}
						return sl_true;
					}
					return sl_false;
				}

				void _stop() override
				{
					if ((*m_bufferQueue)->Clear(m_bufferQueue) != SL_RESULT_SUCCESS) {
						logError("Failed to clear buffer queue");
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
						logError("Failed to enqueue buffer");
					}
					return sl_false;
				}

				void onFrame()
				{
					enqueue();
				}

				static void	callback(SLAndroidSimpleBufferQueueItf bufferQueue, void* p_context)
				{
					AudioPlayerBufferImpl* object = (AudioPlayerBufferImpl*)p_context;
					object->onFrame();
				}
			};

			Ref<AudioPlayerBuffer> AudioPlayerImpl::createBuffer(const AudioPlayerBufferParam& param)
			{
				return AudioPlayerBufferImpl::create(this, param);
			}

		}
	}

	Ref<AudioPlayer> OpenSL_ES::createPlayer(const AudioPlayerParam& param)
	{
		return priv::opensl_es::AudioPlayerImpl::create(param);
	}

}

#endif
