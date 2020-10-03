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

#if defined(SLIB_PLATFORM_IS_IOS)

#include "slib/media/audio_device.h"

#include "slib/core/log.h"
#include "slib/core/platform_apple.h"

#import <Foundation/Foundation.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

#define TAG "Audio"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)

namespace slib
{
	
	namespace priv
	{
		namespace audio_device
		{
			
			class AudioRecorderImpl: public AudioRecorder
			{
			public:
				sl_bool m_flagInitialized;
				
				AudioComponentInstance m_audioUnitInput;
				AudioConverterRef m_converter;
				
				AudioStreamBasicDescription m_formatSrc;
				AudioStreamBasicDescription m_formatDst;
				
			public:
				AudioRecorderImpl()
				{
					m_flagInitialized = sl_false;
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
					if (param.channelsCount != 1 && param.channelsCount != 2) {
						return sl_null;
					}
					
					AudioComponentDescription desc;
					desc.componentType = kAudioUnitType_Output;
					desc.componentSubType = kAudioUnitSubType_RemoteIO;
					desc.componentManufacturer = kAudioUnitManufacturer_Apple;
					desc.componentFlags = 0;
					desc.componentFlagsMask = 0;
					
					AudioComponent comp = AudioComponentFindNext(NULL, &desc);
					if (! comp) {
						LOG_ERROR("Failed to find audio component");
						return sl_null;
					}
					
					OSStatus result;
					AudioComponentInstance audioUnitInput;
					result = AudioComponentInstanceNew(comp, &audioUnitInput);
					if (result == noErr) {
						UInt32 enableIO = 1;
						AudioUnitElement bus1 = 1;
						result = AudioUnitSetProperty(audioUnitInput,
													kAudioOutputUnitProperty_EnableIO,
													kAudioUnitScope_Input,
													bus1, // input bus
													&enableIO,
													sizeof(enableIO));
						if (result == noErr) {
							
							AudioStreamBasicDescription formatSrc;
							UInt32 size = sizeof(formatSrc);
							
							result = AudioUnitGetProperty(audioUnitInput, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, bus1, &formatSrc, &size);
							if (result == noErr) {
							
#if defined(SLIB_PLATFORM_IS_IOS_SIMULATOR)
								formatSrc.mSampleRate = 44100;
#else
								formatSrc.mSampleRate = [[AVAudioSession sharedInstance] sampleRate];
#endif
								
								AudioStreamBasicDescription formatDst;
								formatDst.mFormatID = kAudioFormatLinearPCM;
								formatDst.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
								formatDst.mSampleRate = param.samplesPerSecond;
								formatDst.mBitsPerChannel = 16;
								formatDst.mChannelsPerFrame = param.channelsCount;
								formatDst.mBytesPerFrame = formatDst.mChannelsPerFrame * formatDst.mBitsPerChannel / 8;
								formatDst.mFramesPerPacket = 1;
								formatDst.mBytesPerPacket = formatDst.mBytesPerFrame * formatDst.mFramesPerPacket;
								
								AudioConverterRef converter;
								if (AudioConverterNew(&formatSrc, &formatDst, &converter) == noErr) {
									
									Ref<AudioRecorderImpl> ret = new AudioRecorderImpl();
									
									if (ret.isNotNull()) {
										
										AURenderCallbackStruct cs;
										cs.inputProc = CallbackInput;
										cs.inputProcRefCon = ret.get();
										
										result = AudioUnitSetProperty(audioUnitInput,
																	kAudioOutputUnitProperty_SetInputCallback,
																	kAudioUnitScope_Input,
																	bus1,
																	&cs,
																	sizeof(cs));
										if (result == noErr) {
											
											result = AudioUnitInitialize(audioUnitInput);
											
											if (result == noErr) {
												
												ret->m_audioUnitInput = audioUnitInput;
												ret->m_converter = converter;
												ret->m_formatSrc = formatSrc;
												ret->m_formatDst = formatDst;
												
												ret->_init(param);
												
												ret->m_flagInitialized = sl_true;
												
												if (param.flagAutoStart) {
													ret->start();
												}
												
												return ret;
												
											} else {
												LOG_ERROR("Failed to initialize audio unit");
											}
											
										} else {
											LOG_ERROR("Failed to set callback");
										}
									}
									
									AudioConverterDispose(converter);
									
								} else {
									LOG_ERROR("Failed to create audio converter");
								}
								
							} else {
								LOG_ERROR("Failed to get stream format");
							}
						} else {
							LOG_ERROR("Failed to enable output");
						}
						
						AudioComponentInstanceDispose(audioUnitInput);
						
					} else {
						LOG_ERROR("Failed to create audio component instance");
					}
					
					return sl_null;
				}
				
				void _release() override
				{
					AudioUnitUninitialize(m_audioUnitInput);
					AudioComponentInstanceDispose(m_audioUnitInput);
					AudioConverterDispose(m_converter);
				}
				
				sl_bool _start() override
				{
					if (AudioOutputUnitStart(m_audioUnitInput) != noErr) {
						LOG_ERROR("Failed to start audio unit");
						return sl_false;
					}
					return sl_true;
				}
				
				void _stop() override
				{
					if (AudioOutputUnitStop(m_audioUnitInput) != noErr) {
						LOG_ERROR("Failed to stop audio unit");
					}
				}
				
				struct ConverterContext
				{
					sl_uint32 nBytesPerPacket;
					const AudioBufferList* data;
					sl_bool flagUsed;
				};
				
				static OSStatus ConverterInputProc(AudioConverterRef          inAudioConverter,
												UInt32*                         ioNumberDataPackets,
												AudioBufferList*                ioData,
												AudioStreamPacketDescription**  outDataPacketDescription,
												void*                           inUserData)
				{
					ConverterContext* context = (ConverterContext*)inUserData;
					if (context->flagUsed) {
						return 500;
					} else {
						if (ioData->mNumberBuffers != context->data->mNumberBuffers) {
							return 501;
						}
						sl_uint32 nBuffers = ioData->mNumberBuffers;
						for (sl_uint32 i = 0; i < nBuffers; i++) {
							ioData->mBuffers[i].mData = context->data->mBuffers[i].mData;
							ioData->mBuffers[i].mDataByteSize = context->data->mBuffers[i].mDataByteSize;
							ioData->mBuffers[i].mNumberChannels = context->data->mBuffers[i].mNumberChannels;
						}
						*ioNumberDataPackets = ioData->mBuffers[0].mDataByteSize / context->nBytesPerPacket;
						context->flagUsed = sl_true;
					}
					return 0;
				}
				
				void onFrame(AudioBufferList* data)
				{
					const AudioBuffer& buffer = data->mBuffers[0];
					sl_uint32 nFrames = buffer.mDataByteSize / m_formatSrc.mBytesPerPacket * m_formatDst.mSampleRate / m_formatSrc.mSampleRate * 2; // double buffer to be enough to convert all source packets
					
					sl_uint32 nChannels = m_param.channelsCount;
					sl_uint32 nSamples = nFrames * nChannels;
					
					Array<sl_int16> arrData = _getProcessData(nSamples);
					if (arrData.isNull()) {
						return;
					}
					sl_int16* output = arrData.getData();

					AudioBufferList bufferOutput;
					bufferOutput.mNumberBuffers = 1;
					bufferOutput.mBuffers[0].mData = output;
					bufferOutput.mBuffers[0].mDataByteSize = (UInt32)(nFrames * 2);
					bufferOutput.mBuffers[0].mNumberChannels = nChannels;
					
					UInt32 sizeFrame = (UInt32)nFrames;
					ConverterContext context;
					context.flagUsed = sl_false;
					context.data = data;
					context.nBytesPerPacket = m_formatSrc.mBytesPerPacket;
					OSStatus result = AudioConverterFillComplexBuffer(m_converter, ConverterInputProc, (void*)&context, &sizeFrame, &bufferOutput, NULL);
					if (result == noErr || result == 500) {
						_processFrame(output, sizeFrame * nChannels);
					}
				}
				
				Memory m_memInput;
				Memory getInputMemory(sl_size size)
				{
					Memory mem = m_memInput;
					if (mem.getSize() >= size) {
						return mem;
					}
					mem = Memory::create(size);
					m_memInput = mem;
					return mem;
				}
				
				static OSStatus CallbackInput(void *inRefCon,
											AudioUnitRenderActionFlags *ioActionFlags,
											const AudioTimeStamp *inTimeStamp,
											UInt32 inBusNumber,
											UInt32 inNumberFrames,
											AudioBufferList *ioData)
				{
					AudioRecorderImpl* object = (AudioRecorderImpl*)(inRefCon);
					if (!(object->m_flagInitialized)) {
						return 0;
					}

					AudioStreamBasicDescription& format = object->m_formatSrc;
					UInt32 nChannels = format.mChannelsPerFrame;
					UInt32 nSize = format.mBytesPerFrame * inNumberFrames;
					
					Memory mem = object->getInputMemory(nSize * nChannels);
					if (mem.isNull()) {
						return 0;
					}
					
					char* buf = (char*)(mem.getData());
					
					char _abList[sizeof(AudioBufferList) * 2];
					AudioBufferList& abList = *((AudioBufferList*)_abList);
					
					if (nChannels == 2 && (format.mFormatFlags & kAudioFormatFlagIsNonInterleaved)) {
						abList.mNumberBuffers = 2;
						abList.mBuffers[0].mData = buf;
						abList.mBuffers[0].mDataByteSize = nSize;
						abList.mBuffers[0].mNumberChannels = 1;
						abList.mBuffers[1].mData = buf + nSize;
						abList.mBuffers[1].mDataByteSize = nSize;
						abList.mBuffers[1].mNumberChannels = 1;
					} else {
						abList.mNumberBuffers = 1;
						abList.mBuffers[0].mData = buf;
						abList.mBuffers[0].mDataByteSize = nSize;
						abList.mBuffers[0].mNumberChannels = 1;
					}
					
					OSStatus res = AudioUnitRender(object->m_audioUnitInput,
												ioActionFlags, inTimeStamp,
												inBusNumber, inNumberFrames, &abList);
					
					if (res == 0) {
						object->onFrame(&abList);
					}
					
					return 0;
				}
			};
			
			
			class AudioPlayerBufferImpl : public AudioPlayerBuffer
			{
			public:
				sl_bool m_flagInitialized;
				
				AudioComponentInstance m_audioUnitOutput;
				AudioConverterRef m_converter;
				
				AudioStreamBasicDescription m_formatSrc;
				AudioStreamBasicDescription m_formatDst;
				
			public:
				AudioPlayerBufferImpl()
				{
					m_flagInitialized = sl_false;
				}
				
				~AudioPlayerBufferImpl()
				{
					if (m_flagInitialized) {
						release();
						m_flagInitialized = sl_false;
					}
				}
				
			public:
				static Ref<AudioPlayerBufferImpl> create(const AudioPlayerBufferParam& param)
				{
					if (param.channelsCount != 1 && param.channelsCount != 2) {
						return sl_null;
					}
					
					AudioComponentDescription desc;
					desc.componentType = kAudioUnitType_Output;
					desc.componentSubType = kAudioUnitSubType_RemoteIO;
					desc.componentManufacturer = kAudioUnitManufacturer_Apple;
					desc.componentFlags = 0;
					desc.componentFlagsMask = 0;
					
					AudioComponent comp = AudioComponentFindNext(NULL, &desc);
					if (!comp) {
						LOG_ERROR("Failed to find audio component");
						return sl_null;
					}
					
					OSStatus result;
					AudioComponentInstance audioUnitOutput;
					result = AudioComponentInstanceNew(comp, &audioUnitOutput);
					if (result == noErr) {
						UInt32 enableOutput = 1;
						AudioUnitElement bus0 = 0;
						result = AudioUnitSetProperty(audioUnitOutput,
													  kAudioOutputUnitProperty_EnableIO,
													  kAudioUnitScope_Output,
													  bus0, // output bus
													  &enableOutput,
													  sizeof(enableOutput));
						if (result == noErr) {
							
							AudioStreamBasicDescription formatDst;
							UInt32 size = sizeof(formatDst);
							
							result = AudioUnitGetProperty(audioUnitOutput, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, bus0, &formatDst, &size);
							if (result == noErr) {
								
#if defined(SLIB_PLATFORM_IS_IOS_SIMULATOR)
								formatDst.mSampleRate = 44100;
#else
								formatDst.mSampleRate = [[AVAudioSession sharedInstance] sampleRate];
#endif
								
								AudioStreamBasicDescription formatSrc;
								formatSrc.mFormatID = kAudioFormatLinearPCM;
								formatSrc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
								formatSrc.mSampleRate = param.samplesPerSecond;
								formatSrc.mBitsPerChannel = 16;
								formatSrc.mChannelsPerFrame = (UInt32)(param.channelsCount);
								formatSrc.mBytesPerFrame = formatSrc.mChannelsPerFrame * formatSrc.mBitsPerChannel / 8;
								formatSrc.mFramesPerPacket = 1;
								formatSrc.mBytesPerPacket = formatSrc.mBytesPerFrame * formatSrc.mFramesPerPacket;
								
								AudioConverterRef converter;
								if (AudioConverterNew(&formatSrc, &formatDst, &converter) == noErr) {
									
									Ref<AudioPlayerBufferImpl> ret = new AudioPlayerBufferImpl();
									
									if (ret.isNotNull()) {
										
										AURenderCallbackStruct cs;
										cs.inputProc = CallbackOutput;
										cs.inputProcRefCon = ret.get();
										
										result = AudioUnitSetProperty(audioUnitOutput,
																	  kAudioUnitProperty_SetRenderCallback,
																	  kAudioUnitScope_Input,
																	  bus0,
																	  &cs,
																	  sizeof(cs));
										if (result == noErr) {
											
											result = AudioUnitInitialize(audioUnitOutput);
											
											if (result == noErr) {
												
												ret->m_audioUnitOutput = audioUnitOutput;
												ret->m_converter = converter;
												ret->m_formatSrc = formatSrc;
												ret->m_formatDst = formatDst;
												
												ret->_init(param);
												
												ret->m_flagInitialized = sl_true;

												if (param.flagAutoStart) {
													ret->start();
												}
												
												return ret;
												
											} else {
												LOG_ERROR("Failed to initialize audio unit");
											}
											
										} else {
											LOG_ERROR("Failed to set callback");
										}
									}
									
									AudioConverterDispose(converter);
									
								} else {
									LOG_ERROR("Failed to create audio converter");
								}
							} else {
								LOG_ERROR("Failed to get stream format");
							}
						} else {
							LOG_ERROR("Failed to enable output");
						}
						
						AudioComponentInstanceDispose(audioUnitOutput);
						
					} else {
						LOG_ERROR("Failed to create audio component instance");
					}
					
					return sl_null;
				}
				
				void _release() override
				{
					AudioUnitUninitialize(m_audioUnitOutput);
					AudioComponentInstanceDispose(m_audioUnitOutput);
					AudioConverterDispose(m_converter);
				}
				
				sl_bool _start() override
				{
					if (AudioOutputUnitStart(m_audioUnitOutput) != noErr) {
						LOG_ERROR("Failed to start audio unit");
						return sl_false;
					}
					return sl_true;
				}
				
				void _stop() override
				{
					if (AudioOutputUnitStop(m_audioUnitOutput) != noErr) {
						LOG_ERROR("Failed to stop audio unit");
					}
				}
				
				AtomicArray<sl_int16> m_dataConvert;
				void onConvert(sl_uint32 nFrames, AudioBufferList* data)
				{
					sl_uint32 nChannels = m_param.channelsCount;
					sl_uint32 nSamples = nFrames * nChannels;
					
					Array<sl_int16> dataConvert = _getProcessData(nSamples);
					m_dataConvert = dataConvert;
					if (dataConvert.isNull()) {
						return;
					}
					sl_int16* s = dataConvert.getData();
					_processFrame(s, nSamples);
					
					data->mBuffers[0].mDataByteSize = (UInt32)nSamples * 2;
					data->mBuffers[0].mData = s;
					data->mBuffers[0].mNumberChannels = (UInt32)nChannels;
					
				}
				
				static OSStatus ConverterProc(AudioConverterRef               inAudioConverter,
											  UInt32*                         ioNumberDataPackets,
											  AudioBufferList*                ioData,
											  AudioStreamPacketDescription**  outDataPacketDescription,
											  void*                           inUserData)
				{
					AudioPlayerBufferImpl* object = (AudioPlayerBufferImpl*)inUserData;
					object->onConvert(*ioNumberDataPackets, ioData);
					return noErr;
				}
				
				OSStatus onFrame(UInt32 nFrames, AudioBufferList* data)
				{
					UInt32 sizeFrame = nFrames;
					OSStatus result = AudioConverterFillComplexBuffer(m_converter, ConverterProc, this, &sizeFrame, data, NULL);
					return result;
				}
				
				static OSStatus CallbackOutput(void                        *inRefCon,
											   AudioUnitRenderActionFlags  *ioActionFlags,
											   const AudioTimeStamp        *inTimeStamp,
											   UInt32                      inBusNumber,
											   UInt32                      inNumberFrames,
											   AudioBufferList             *ioData)
				{
					AudioPlayerBufferImpl* object = (AudioPlayerBufferImpl*)(inRefCon);
					if (object && object->m_flagInitialized) {
						return object->onFrame(inNumberFrames, ioData);
					} else {
						return 500;
					}
				}
				
			};
			
			class AudioPlayerImpl : public AudioPlayer
			{
			public:
				AudioPlayerImpl()
				{
				}
				
				~AudioPlayerImpl()
				{
				}
				
			public:
				static void logError(String text)
				{
					LogError("AudioPlayer", text);
				}
				
				static Ref<AudioPlayerImpl> create(const AudioPlayerParam& param)
				{
					Ref<AudioPlayerImpl> ret = new AudioPlayerImpl();
					return ret;
				}
				
				Ref<AudioPlayerBuffer> createBuffer(const AudioPlayerBufferParam& param)
				{
					return AudioPlayerBufferImpl::create(param);
				}
			};
			
		}
	}
	
	using namespace priv::audio_device;

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

	Ref<AudioPlayer> AudioPlayer::create(const AudioPlayerParam& param)
	{
		return AudioPlayerImpl::create(param);
	}
	
	List<AudioPlayerInfo> AudioPlayer::getPlayersList()
	{
		AudioPlayerInfo ret;
		SLIB_STATIC_STRING(s, "Internal Speaker");
		ret.name = s;
		return List<AudioPlayerInfo>::createFromElement(ret);
	}

}

#endif
