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
			
			class AudioRecorderImpl : public AudioRecorder
			{
			public:
				SLObjectItf m_engineObject;
				SLEngineItf m_engineInterface;

				SLObjectItf m_recorderObject;
				SLRecordItf m_recordInterface;
				SLAndroidSimpleBufferQueueItf m_bufferQueue;

				sl_int16* m_bufFrame;
				sl_uint32 m_indexBuffer;
				sl_uint32 m_nSamplesFrame;
				
			public:
				AudioRecorderImpl()
				{
					m_indexBuffer = 0;
				}

				~AudioRecorderImpl()
				{
					release();
				}

			public:
				static void logError(String text)
				{
					LogError("OpenSL_ES", text);
				}

				static Ref<AudioRecorderImpl> create(const AudioRecorderParam& param)
				{
					Ref<AudioRecorderImpl> ret;
					
					if (param.channelsCount != 1 && param.channelsCount != 2) {
						return ret;
					}
					
					sl_bool flagInitializeEngine = sl_false;

					SLObjectItf engineObject = sl_null;
					SLEngineItf engineInterface = sl_null;

					SLEngineOption options[] = {
						{SL_ENGINEOPTION_THREADSAFE, 1},
					};
					if (slCreateEngine(&engineObject, 1, options, 0, sl_null, sl_null) == SL_RESULT_SUCCESS) {
						if ((*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE) == SL_RESULT_SUCCESS) {
							if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,  &engineInterface) == SL_RESULT_SUCCESS) {
								flagInitializeEngine = sl_true;
							} else {
								logError("Failed to get engine interface");
							}
						} else {
							logError("Failed to realize engine");
						}
						if (!flagInitializeEngine) {
							(*engineObject)->Destroy(engineObject);
						}
					} else {
						logError("Failed to create engine");
					}

					if (flagInitializeEngine) {
						SLDataLocator_IODevice androidMicDevice = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, sl_null };
						SLDataSource slDataSource = { &androidMicDevice, sl_null };
						SLDataLocator_AndroidSimpleBufferQueue androidSBQ = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
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
									if ((*recorderObject)->GetInterface( recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bufferQueue) == SL_RESULT_SUCCESS) {
										ret = new AudioRecorderImpl();
										if (ret.isNotNull()) {
											ret->m_engineInterface = engineInterface;
											ret->m_engineObject = engineObject;
											ret->m_recorderObject = recorderObject;
											ret->m_recordInterface = recordInterface;
											ret->m_bufferQueue = bufferQueue;
											ret->m_nSamplesFrame = param.samplesPerSecond * param.frameLengthInMilliseconds / 1000 * param.channelsCount;
											ret->m_bufFrame = new sl_int16[ret->m_nSamplesFrame * 2];
											
											ret->_init(param);
											
											if (ret->m_bufFrame) {
												Base::zeroMemory(ret->m_bufFrame, sizeof(sl_int16) * ret->m_nSamplesFrame * 2);
												if ((*bufferQueue)->RegisterCallback(bufferQueue, AudioRecorderImpl::callback, ret.get()) == SL_RESULT_SUCCESS) {
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
									logError("Failed to get recorder interface");
								}
							} else {
								logError("Failed to realize recorder object");
							}
							(*recorderObject)->Destroy(recorderObject);
						} else {
							logError("Failed to create recorder object");
						}
						(*engineObject)->Destroy(engineObject);
					}
					return ret;
				}

				void _release() override
				{
					(*m_recorderObject)->Destroy(m_recorderObject);
					if (m_bufFrame) {
						delete[] m_bufFrame;
					}
					(*m_engineObject)->Destroy(m_engineObject);
				}

				sl_bool _start() override
				{
					if (onFrame()) {
						if ((*m_recordInterface)->SetRecordState(m_recordInterface, SL_RECORDSTATE_RECORDING) != SL_RESULT_SUCCESS) {
							logError("Failed to record buffer");
							return sl_false;
						}
						logError("start audio recorder.");
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
						logError("Failed to enqueue buffer");
					}
					return sl_false;
				}

				static void	callback(SLAndroidSimpleBufferQueueItf bufferQueue, void* p_context)
				{
					AudioRecorderImpl* object = (AudioRecorderImpl*)p_context;
					object->onFrame();
				}
			};

		}
	}

	Ref<AudioRecorder> OpenSL_ES::createRecorder(const AudioRecorderParam& param)
	{
		return priv::opensl_es::AudioRecorderImpl::create(param);
	}

}

#else

namespace slib
{

	Ref<AudioRecorder> OpenSL_ES::createRecorder(const AudioRecorderParam& param)
	{
		return sl_null;
	}

}

#endif
