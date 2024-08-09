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

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/media/audio_device.h"

#include "slib/core/log.h"
#include "slib/core/scoped_buffer.h"
#include "slib/platform.h"

#import <CoreAudio/CoreAudio.h>
#import <AudioToolbox/AudioConverter.h>

#define TAG "Audio"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)

namespace slib
{

	namespace
	{
		struct MacAudioDeviceInfo
		{
			AudioDeviceID id;
			String uid;
			String name;
		};

		static sl_bool GetDeviceInfo(MacAudioDeviceInfo& outInfo, AudioDeviceID deviceID, sl_bool flagInput)
		{
			AudioObjectPropertyAddress propDeviceConfig;
			propDeviceConfig.mScope = flagInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
			propDeviceConfig.mElement = kAudioObjectPropertyElementMaster;
			UInt32 sizeValue;

			// Check for input channels
			propDeviceConfig.mSelector = kAudioDevicePropertyStreamConfiguration;
			sizeValue = 0;
			if (AudioObjectGetPropertyDataSize(deviceID, &propDeviceConfig, 0, NULL, &sizeValue) == kAudioHardwareNoError) {
				SLIB_SCOPED_BUFFER(char, 1024, memBufferList, sizeValue);
				if (!memBufferList) {
					return sl_false;
				}
				AudioBufferList* bufferList = (AudioBufferList*)(memBufferList);
				if (AudioObjectGetPropertyData(deviceID, &propDeviceConfig, 0, NULL, &sizeValue, bufferList) == kAudioHardwareNoError) {
					if (bufferList->mNumberBuffers == 0) {
						return sl_false;
					}
				} else {
					LOG_ERROR("Failed to get stream configuration - Dev%d", deviceID);
					return sl_false;
				}
			} else {
				LOG_ERROR("Failed to get size of stream configuration - Dev%d", deviceID);
				return sl_false;
			}

			outInfo.id = deviceID;

			// Device UID
			CFStringRef deviceUID = NULL;
			propDeviceConfig.mSelector = kAudioDevicePropertyDeviceUID;
			sizeValue = sizeof(deviceUID);
			if (AudioObjectGetPropertyData(deviceID, &propDeviceConfig, 0, NULL, &sizeValue, &deviceUID) == kAudioHardwareNoError) {
				NSString* s = (__bridge NSString*)deviceUID;
				outInfo.uid = Apple::getStringFromNSString(s);
				CFRelease(deviceUID);
			}

			// Device Name
			CFStringRef deviceName;
			propDeviceConfig.mSelector = kAudioDevicePropertyDeviceNameCFString;
			sizeValue = sizeof(deviceName);
			if (AudioObjectGetPropertyData(deviceID, &propDeviceConfig, 0, NULL, &sizeValue, &deviceName) == kAudioHardwareNoError) {
				NSString* s = (__bridge NSString*)deviceName;
				outInfo.name = Apple::getStringFromNSString(s);
				CFRelease(deviceName);
			}

			return sl_true;
		}

		static sl_bool GetDefaultDeviceInfo(MacAudioDeviceInfo& outInfo, sl_bool flagInput)
		{
			AudioObjectPropertyAddress propDeviceConfig;
			propDeviceConfig.mSelector = flagInput ? kAudioHardwarePropertyDefaultInputDevice : kAudioHardwarePropertyDefaultOutputDevice;
			propDeviceConfig.mScope = kAudioObjectPropertyScopeGlobal;
			propDeviceConfig.mElement = kAudioObjectPropertyElementMaster;

			AudioDeviceID dev;
			UInt32 size = sizeof(dev);
			if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propDeviceConfig, 0, NULL, &size, &dev) != kAudioHardwareNoError) {
				LOG_ERROR("Failed to get default device");
				return sl_false;
			}
			if (dev == kAudioDeviceUnknown) {
				LOG_ERROR("Unknown default device");
				return sl_false;
			}
			return GetDeviceInfo(outInfo, dev, flagInput);
		}

		static List<MacAudioDeviceInfo> GetAllDevices(sl_bool flagInput)
		{
			List<MacAudioDeviceInfo> ret;

			AudioObjectPropertyAddress propDeviceListing;
			propDeviceListing.mSelector = kAudioHardwarePropertyDevices;
			propDeviceListing.mScope = kAudioObjectPropertyScopeGlobal;
			propDeviceListing.mElement = kAudioObjectPropertyElementMaster;
			UInt32 nSizeCountDevices = 0;
			if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propDeviceListing, 0, NULL, &nSizeCountDevices) != kAudioHardwareNoError) {
				LOG_ERROR("Failed to get count of audio devices");
				return ret;
			}
			if (nSizeCountDevices % sizeof(AudioDeviceID) != 0) {
				LOG_ERROR("Invalid size of audio device count");
				return ret;
			}
			sl_uint32 nCountDevices = nSizeCountDevices / sizeof(AudioDeviceID);
			if (nCountDevices == 0) {
				LOG_ERROR("No devices");
				return ret;
			}

			SLIB_SCOPED_BUFFER(AudioDeviceID, 16, deviceIds, nCountDevices);
			if (!deviceIds) {
				return ret;
			}
			if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propDeviceListing, 0, NULL, &nSizeCountDevices, deviceIds) != kAudioHardwareNoError){
				LOG_ERROR("Failed to list audio device ids");
				return ret;
			}

			for (sl_uint32 i = 0; i < nCountDevices; i++) {
				MacAudioDeviceInfo info;
				if (GetDeviceInfo(info, deviceIds[i], flagInput)) {
					ret.add_NoLock(info);
				}
			}
			return ret;
		}

		static sl_bool SelectDevice(MacAudioDeviceInfo& outInfo, sl_bool flagInput, String uid)
		{
			if (uid.isEmpty()) {
				return GetDefaultDeviceInfo(outInfo, flagInput);
			} else {
				ListElements<MacAudioDeviceInfo> list(GetAllDevices(flagInput));
				for (sl_size i = 0; i < list.count; i++) {
					MacAudioDeviceInfo& element = list[i];
					if (element.uid == uid) {
						outInfo = element;
						return sl_true;
					}
				}
				return sl_false;
			}
		}
	}

	List<AudioRecorderDeviceInfo> AudioRecorder::getDevices()
	{
		ListElements<MacAudioDeviceInfo> list(GetAllDevices(sl_true));
		List<AudioRecorderDeviceInfo> ret;
		for (sl_size i = 0; i < list.count; i++) {
			AudioRecorderDeviceInfo info;
			info.id = list[i].uid;
			info.name = list[i].name;
			ret.add_NoLock(info);
		}
		return ret;
	}

	List<AudioPlayerDeviceInfo> AudioPlayerDevice::getDevices()
	{
		ListElements<MacAudioDeviceInfo> list(GetAllDevices(sl_false));
		List<AudioPlayerDeviceInfo> ret;
		for (sl_size i = 0; i < list.count; i++) {
			AudioPlayerDeviceInfo info;
			info.id = list[i].uid;
			info.name = list[i].name;
			ret.add_NoLock(info);
		}
		return ret;
	}

	namespace
	{
		class AudioRecorderImpl : public AudioRecorder
		{
		public:
			sl_bool m_flagInitialized;

			AudioDeviceID m_deviceID;
			AudioConverterRef m_converter;
			AudioDeviceIOProcID m_callback;

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
				if (param.flagLoopback) {
					// Not support loopback recording
					return sl_null;
				}
				if (param.channelCount != 1 && param.channelCount != 2) {
					return sl_null;
				}

				MacAudioDeviceInfo deviceInfo;
				if (!(SelectDevice(deviceInfo, sl_true, param.deviceId))) {
					LOG_ERROR("Failed to find audio input device: %s", param.deviceId);
					return sl_null;
				}

				AudioDeviceID deviceID = deviceInfo.id;

				// Get current stream description
				AudioObjectPropertyAddress prop;
				prop.mSelector = kAudioDevicePropertyStreamFormat;
				prop.mScope = kAudioDevicePropertyScopeInput;
				prop.mElement = kAudioObjectPropertyElementMaster;

				UInt32 sizeValue;
				AudioStreamBasicDescription formatSrc;
				Base::zeroMemory(&formatSrc, sizeof(formatSrc));
				sizeValue = sizeof(formatSrc);
				if (AudioObjectGetPropertyData(deviceID, &prop, 0, NULL, &sizeValue, &formatSrc) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to get source input format");
					return sl_null;
				}

				prop.mSelector = kAudioDevicePropertyBufferSizeRange;
				AudioValueRange rangeBufferSize;
				sizeValue = sizeof(rangeBufferSize);
				if (AudioObjectGetPropertyData(deviceID, &prop, 0, NULL, &sizeValue, &rangeBufferSize) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to get buffer size range");
					return sl_null;
				}

				UInt32 frameSize = param.getFrameLengthInMilliseconds() * formatSrc.mSampleRate * formatSrc.mBytesPerFrame / 1000;
				if (frameSize < rangeBufferSize.mMinimum) {
					frameSize = rangeBufferSize.mMinimum;
				} else if (frameSize > rangeBufferSize.mMaximum) {
					frameSize = rangeBufferSize.mMaximum;
				}

				// Create Audio Converter
				AudioStreamBasicDescription formatDst;
				formatDst.mFormatID = kAudioFormatLinearPCM;
				formatDst.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
				formatDst.mSampleRate = param.samplesPerSecond;
				formatDst.mBitsPerChannel = 16;
				formatDst.mChannelsPerFrame = param.channelCount;
				formatDst.mBytesPerFrame = formatDst.mChannelsPerFrame * formatDst.mBitsPerChannel / 8;
				formatDst.mFramesPerPacket = 1;
				formatDst.mBytesPerPacket = formatDst.mBytesPerFrame * formatDst.mFramesPerPacket;

				AudioConverterRef converter;
				if (AudioConverterNew(&formatSrc, &formatDst, &converter) == kAudioHardwareNoError) {

					prop.mSelector = kAudioDevicePropertyBufferSize;
					sizeValue = sizeof(frameSize);

					if (AudioObjectSetPropertyData(deviceID, &prop, 0, NULL, sizeValue, &frameSize) == kAudioHardwareNoError) {

						Ref<AudioRecorderImpl> ret = new AudioRecorderImpl();

						if (ret.isNotNull()) {

							AudioDeviceIOProcID callback;

							if (AudioDeviceCreateIOProcID(deviceID, DeviceIOProc, ret.get(), &callback) == kAudioHardwareNoError) {

								ret->m_deviceID = deviceID;
								ret->m_converter = converter;
								ret->m_formatSrc = formatSrc;
								ret->m_formatDst = formatDst;
								ret->m_callback = callback;

								ret->_init(param);

								ret->m_flagInitialized = sl_true;

								if (param.flagAutoStart) {
									ret->start();
								}

								return ret;

							} else {
								LOG_ERROR("Failed to create io proc");
							}
						}

					} else {
						LOG_ERROR("Failed to get set buffer size");
					}

					AudioConverterDispose(converter);

				} else {
					LOG_ERROR("Failed to create audio converter");
				}

				return sl_null;
			}

			void _release() override
			{
				AudioDeviceDestroyIOProcID(m_deviceID, m_callback);
				AudioConverterDispose(m_converter);
			}

			sl_bool _start() override
			{
				if (AudioDeviceStart(m_deviceID, m_callback) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to start device");
					return sl_false;
				}
				return sl_true;
			}

			void _stop() override
			{
				if (AudioDeviceStop(m_deviceID, m_callback) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to stop play device");
				}
			}

			struct ConverterContext
			{
				sl_uint32 nBytesPerPacket;
				const AudioBufferList* data;
				sl_bool flagUsed;
			};

			static OSStatus ConverterProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription**  outDataPacketDescription, void* inUserData)
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

			void onFrame(const AudioBufferList* data)
			{
				const AudioBuffer& buffer = data->mBuffers[0];
				sl_uint32 nFrames = buffer.mDataByteSize / m_formatSrc.mBytesPerPacket * m_formatDst.mSampleRate / m_formatSrc.mSampleRate * 2; // double buffer to be enough to convert all source packets

				sl_uint32 nChannels = m_param.channelCount;
				sl_uint32 nSamples = nFrames * nChannels;

				Array<sl_int16> arrData = _getProcessData(nSamples);
				if (arrData.isNull()) {
					return;
				}
				sl_int16* output = arrData.getData();

				AudioBufferList bufferOutput;
				bufferOutput.mNumberBuffers = 1;
				bufferOutput.mBuffers[0].mData = output;
				bufferOutput.mBuffers[0].mDataByteSize = (UInt32)nSamples * 2;
				bufferOutput.mBuffers[0].mNumberChannels = nChannels;

				UInt32 sizeFrame = (UInt32)nFrames;
				ConverterContext context;
				context.flagUsed = sl_false;
				context.data = data;
				context.nBytesPerPacket = m_formatSrc.mBytesPerPacket;
				OSStatus result = AudioConverterFillComplexBuffer(m_converter, ConverterProc, (void*)&context, &sizeFrame, &bufferOutput, NULL);
				if (result == noErr || result == 500) {
					_processFrame(output, sizeFrame * nChannels);
				}
			}

			static OSStatus DeviceIOProc(AudioObjectID inDevice, const AudioTimeStamp* inNow, const AudioBufferList*  inInputData, const AudioTimeStamp*   inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData)
			{
				AudioRecorderImpl* object = (AudioRecorderImpl*)(inClientData);
				if (object->m_flagInitialized) {
					object->onFrame(inInputData);
				}
				return 0;
			}

		};
	}

	Ref<AudioRecorder> AudioRecorder::create(const AudioRecorderParam& param)
	{
		return AudioRecorderImpl::create(param);
	}

	namespace
	{
		class AudioPlayerImpl : public AudioPlayer
		{
		public:
			sl_bool m_flagInitialized;

			AudioDeviceID m_deviceID;
			AudioConverterRef m_converter;
			AudioDeviceIOProcID m_callback;

			AudioStreamBasicDescription m_formatSrc;
			AudioStreamBasicDescription m_formatDst;

		public:
			AudioPlayerImpl()
			{
				m_flagInitialized = sl_false;
			}

			~AudioPlayerImpl()
			{
				if (m_flagInitialized) {
					release();
					m_flagInitialized = sl_false;
				}
			}

		public:
			static Ref<AudioPlayerImpl> create(const AudioDeviceID deviceID, const AudioPlayerParam& param)
			{
				if (param.channelCount != 1 && param.channelCount != 2) {
					return sl_null;
				}

				AudioObjectPropertyAddress prop;
				prop.mSelector = kAudioDevicePropertyStreamFormat;
				prop.mScope = kAudioDevicePropertyScopeOutput;
				prop.mElement = kAudioObjectPropertyElementMaster;

				UInt32 sizeValue;
				AudioStreamBasicDescription formatDst;
				Base::zeroMemory(&formatDst, sizeof(formatDst));
				sizeValue = sizeof(formatDst);
				if (AudioObjectGetPropertyData(deviceID, &prop, 0, NULL, &sizeValue, &formatDst) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to get source input format");
					return sl_null;
				}

				prop.mSelector = kAudioDevicePropertyBufferSizeRange;
				AudioValueRange rangeBufferSize;
				sizeValue = sizeof(rangeBufferSize);
				if (AudioObjectGetPropertyData(deviceID, &prop, 0, NULL, &sizeValue, &rangeBufferSize) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to get buffer size range");
					return sl_null;
				}

				UInt32 sizeFrame = param.frameLengthInMilliseconds * formatDst.mSampleRate * formatDst.mBytesPerFrame / 1000;
				if (sizeFrame < rangeBufferSize.mMinimum) {
					LOG_ERROR("Required frame size(%d) is smaller than minimum %d", sizeFrame, rangeBufferSize.mMinimum);
					return sl_null;
				}
				if (sizeFrame > rangeBufferSize.mMaximum) {
					LOG_ERROR("Required frame size(%d) is bigger than maximum %d", sizeFrame, rangeBufferSize.mMaximum);
					return sl_null;
				}

				// Create Audio Converter
				AudioStreamBasicDescription formatSrc;
				formatSrc.mFormatID = kAudioFormatLinearPCM;
				formatSrc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
				formatSrc.mSampleRate = param.samplesPerSecond;
				formatSrc.mBitsPerChannel = 16;
				formatSrc.mChannelsPerFrame = param.channelCount;
				formatSrc.mBytesPerFrame = formatSrc.mChannelsPerFrame * formatSrc.mBitsPerChannel / 8;
				formatSrc.mFramesPerPacket = 1;
				formatSrc.mBytesPerPacket = formatSrc.mBytesPerFrame * formatSrc.mFramesPerPacket;

				AudioConverterRef converter;
				if (AudioConverterNew(&formatSrc, &formatDst, &converter) == kAudioHardwareNoError) {

					prop.mSelector = kAudioDevicePropertyBufferSize;
					sizeValue = sizeof(sizeFrame);

					if (AudioObjectSetPropertyData(deviceID, &prop, 0, NULL, sizeValue, &sizeFrame) == kAudioHardwareNoError) {

						Ref<AudioPlayerImpl> ret = new AudioPlayerImpl();

						if (ret.isNotNull()) {

							AudioDeviceIOProcID callback;

							if (AudioDeviceCreateIOProcID(deviceID, DeviceIOProc, ret.get(), &callback) == kAudioHardwareNoError) {

								ret->m_deviceID = deviceID;
								ret->m_converter = converter;
								ret->m_formatSrc = formatSrc;
								ret->m_formatDst = formatDst;
								ret->m_callback = callback;

								ret->_init(param);

								ret->m_flagInitialized = sl_true;

								if (param.flagAutoStart) {
									ret->start();
								}

								return ret;

							} else {
								LOG_ERROR("Failed to create io proc");
							}
						}

					} else {
						LOG_ERROR("Failed to get set buffer size");
					}

					AudioConverterDispose(converter);

				} else {
					LOG_ERROR("Failed to create audio converter");
				}

				return sl_null;
			}

			void _release() override
			{
				AudioDeviceDestroyIOProcID(m_deviceID, m_callback);
				AudioConverterDispose(m_converter);
			}

			sl_bool _start() override
			{
				if (AudioDeviceStart(m_deviceID, m_callback) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to start device");
					return sl_false;
				}
				return sl_true;
			}

			void _stop() override
			{
				if (AudioDeviceStop(m_deviceID, m_callback) != kAudioHardwareNoError) {
					LOG_ERROR("Failed to stop play device");
				}
			}

			AtomicArray<sl_int16> m_dataConvert;
			void onConvert(sl_uint32 nFrames, AudioBufferList* data)
			{
				sl_uint32 nChannels = m_param.channelCount;
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

			static OSStatus ConverterProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription** outDataPacketDescription, void* inUserData)
			{
				AudioPlayerImpl* object = (AudioPlayerImpl*)inUserData;
				object->onConvert(*ioNumberDataPackets, ioData);
				return noErr;
			}

			OSStatus onFrame(AudioBufferList *outputData)
			{
				UInt32 size = outputData->mBuffers->mDataByteSize / m_formatDst.mBytesPerFrame;
				AudioConverterFillComplexBuffer(m_converter, ConverterProc, this, &size, outputData, NULL);
				return 0;
			}

			static OSStatus DeviceIOProc(AudioDeviceID, const AudioTimeStamp*, const AudioBufferList* inputData, const AudioTimeStamp* inputTime, AudioBufferList* outputData, const AudioTimeStamp* outputTime, void *clientData)
			{
				AudioPlayerImpl* object = (AudioPlayerImpl*)(clientData);
				if (object->m_flagInitialized) {
					return object->onFrame(outputData);
				}
				return 0;
			}
		};

		class AudioPlayerDeviceImpl : public AudioPlayerDevice
		{
		public:
			AudioDeviceID m_deviceID;

		public:
			AudioPlayerDeviceImpl()
			{
				m_deviceID = 0;
			}

		public:
			static Ref<AudioPlayerDeviceImpl> create(const AudioPlayerDeviceParam& param)
			{
				MacAudioDeviceInfo deviceInfo;
				if (!(SelectDevice(deviceInfo, sl_false, param.deviceId))) {
					LOG_ERROR("Failed to find audio ouptut device: %s", param.deviceId);
					return sl_null;
				}

				Ref<AudioPlayerDeviceImpl> ret = new AudioPlayerDeviceImpl();
				if (ret.isNotNull()) {
					ret->m_deviceID = deviceInfo.id;
					return ret;
				}
				return sl_null;
			}

			Ref<AudioPlayer> createPlayer(const AudioPlayerParam& param) override
			{
				return AudioPlayerImpl::create(m_deviceID, param);
			}
		};
	}

	Ref<AudioPlayerDevice> AudioPlayerDevice::create(const AudioPlayerDeviceParam& param)
	{
		return AudioPlayerDeviceImpl::create(param);
	}

}

#endif
