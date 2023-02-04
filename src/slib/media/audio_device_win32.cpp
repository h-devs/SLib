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

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/media/audio_device.h"

#include "slib/core/thread.h"
#include "slib/core/log.h"
#include "slib/platform.h"

#include <initguid.h>
#include <mmsystem.h>
#include <dsound.h>

#define TAG "Audio"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)

#define NUM_PLAY_NOTIFICATIONS  2

#pragma comment(lib, "dsound.lib")

namespace slib
{

	namespace {
		static void InitCOM()
		{
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		}

		class AudioRecorderImpl : public AudioRecorder
		{
		public:
			IDirectSoundCapture8* m_device;
			IDirectSoundCaptureBuffer8* m_buffer;
			sl_uint32 m_nSamplesFrame;
			HANDLE m_events[3];
			Ref<Thread> m_thread;

		public:
			AudioRecorderImpl()
			{
				m_events[2] = CreateEventW(NULL, FALSE, FALSE, NULL);
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

				InitCOM();

				String deviceID = param.deviceId;
				GUID guid;
				if (deviceID.isEmpty()) {
					guid = DSDEVID_DefaultCapture;
				} else {
					ListLocker<DeviceProperty> props(queryDeviceInfos());
					sl_size i = 0;
					for (; i < props.count; i++) {
						if (deviceID == props[i].szGuid) {
							guid = props[i].guid;
							break;
						}
					}
					if (i == props.count) {
						LOG_ERROR("Failed to find capture device: %s", deviceID);
						return sl_null;
					}
				}

				LPDIRECTSOUNDCAPTURE8 device = NULL;
				HRESULT hr = DirectSoundCaptureCreate8(&guid, &device, NULL);
				if (FAILED(hr)) {
					if (hr == DSERR_ALLOCATED) {
						LOG_ERROR("Dsound capture device is already used");
					} else {
						LOG_ERROR("Can not create dsound capture device");
					}
					return sl_null;
				}

				WAVEFORMATEX wf;
				wf.wFormatTag = WAVE_FORMAT_PCM;
				wf.nChannels = param.channelCount;
				wf.wBitsPerSample = 16;
				wf.nSamplesPerSec = param.samplesPerSecond;
				wf.nBlockAlign = (wf.wBitsPerSample * wf.nChannels) >> 3;
				wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
				wf.cbSize = 0;

				sl_uint32 samplesPerFrame = param.getSamplesPerFrame();
				sl_uint32 sizeBuffer = samplesPerFrame * wf.nBlockAlign * 2;
				samplesPerFrame *= param.channelCount;
				DSCBUFFERDESC desc;
				desc.dwSize = sizeof(desc);
				desc.dwBufferBytes = sizeBuffer;
				desc.dwReserved = 0;
				desc.lpwfxFormat = &wf;
				DSCEFFECTDESC effects[2];
				if (0) { // use voice mode
					desc.dwFlags = DSCBCAPS_CTRLFX;
					desc.dwFXCount = 2;
					desc.lpDSCFXDesc = effects;
					effects[0].dwSize = sizeof(DSCEFFECTDESC);
					effects[0].dwFlags = DSCFX_LOCSOFTWARE;
					effects[0].guidDSCFXClass = GUID_DSCFX_CLASS_AEC;
					effects[0].guidDSCFXInstance = GUID_DSCFX_SYSTEM_AEC;
					effects[0].dwReserved1 = 0;
					effects[0].dwReserved2 = 0;
					effects[1].dwSize = sizeof(DSCEFFECTDESC);
					effects[1].dwFlags = DSCFX_LOCSOFTWARE;
					effects[1].guidDSCFXClass = GUID_DSCFX_CLASS_NS;
					effects[1].guidDSCFXInstance = GUID_DSCFX_SYSTEM_NS;
					effects[1].dwReserved1 = 0;
					effects[1].dwReserved2 = 0;
				} else {
					desc.dwFlags = 0;
					desc.dwFXCount = 0;
					desc.lpDSCFXDesc = NULL;
				}

				HANDLE hEvent0 = CreateEventW(NULL, FALSE, FALSE, NULL);
				HANDLE hEvent1 = CreateEventW(NULL, FALSE, FALSE, NULL);

				IDirectSoundCaptureBuffer* dsbuf;
				IDirectSoundCaptureBuffer8* buffer = sl_null;
				hr = device->CreateCaptureBuffer(&desc, &dsbuf, NULL);

				if (SUCCEEDED(hr)) {

					dsbuf->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&buffer);
					dsbuf->Release();

					if (buffer) {

						IDirectSoundNotify8* notify;
						hr = buffer->QueryInterface(IID_IDirectSoundNotify8, (LPVOID*)&notify);

						if (SUCCEEDED(hr)) {

							DSBPOSITIONNOTIFY pos[2];
							pos[0].dwOffset = (sizeBuffer / 2) - 1;
							pos[0].hEventNotify = hEvent0;
							pos[1].dwOffset = sizeBuffer - 1;
							pos[1].hEventNotify = hEvent1;

							hr = notify->SetNotificationPositions(2, pos);

							if (SUCCEEDED(hr)) {

								Ref<AudioRecorderImpl> ret = new AudioRecorderImpl();
								if (ret.isNotNull()) {
									ret->m_device = device;
									ret->m_buffer = buffer;
									ret->m_events[0] = hEvent0;
									ret->m_events[1] = hEvent1;
									ret->m_nSamplesFrame = samplesPerFrame;

									ret->_init(param);

									if (param.flagAutoStart) {
										ret->start();
									}

									return ret;
								}

							} else {
								LOG_ERROR("Failed to set dsound notify postions");
							}
							notify->Release();
						} else {
							LOG_ERROR("Failed to get dsound notify");
						}
						buffer->Release();
					} else {
						LOG_ERROR("Failed to get dsound buffer 8");
					}
				} else {
					LOG_ERROR("Failed to create dsound buffer");
				}

				device->Release();
				CloseHandle(hEvent0);
				CloseHandle(hEvent1);

				return sl_null;
			}

			void _release() override
			{
				m_buffer->Release();
				m_buffer = sl_null;
				m_device->Release();
				m_device = sl_null;
				for (int i = 0; i < 3; i++) {
					CloseHandle(m_events[i]);
				}
			}

			sl_bool _start() override
			{
				HRESULT hr = m_buffer->Start(DSCBSTART_LOOPING);
				if (SUCCEEDED(hr)) {
					m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
					if (m_thread.isNotNull()) {
						return sl_true;
					}
					m_buffer->Stop();
				} else {
					LOG_ERROR("Failed to start");
				}
				return sl_false;
			}

			void _stop() override
			{
				m_thread->finish();
				SetEvent(m_events[2]);
				m_thread->finishAndWait();
				m_thread.setNull();

				m_buffer->Stop();
			}

			struct DeviceProperty {
				GUID guid;
				String szGuid;
				String name;
				String description;
			};

			static List<DeviceProperty> queryDeviceInfos()
			{
				List<DeviceProperty> list;
				HRESULT hr = DirectSoundCaptureEnumerateW(DeviceEnumProc, (VOID*)&list);
				if (FAILED(hr)) {
					LOG_ERROR("Can not query device info");
				}
				return list;
			}

			static BOOL CALLBACK DeviceEnumProc(LPGUID lpGUID, LPCWSTR lpszDesc, LPCWSTR lpszDrvName, LPVOID lpContext)
			{
				List<DeviceProperty>& list = *((List<DeviceProperty>*)lpContext);
				DeviceProperty prop;
				if (lpGUID) {
					prop.guid = *lpGUID;
					prop.szGuid = Win32::getStringFromGUID(*lpGUID);
					prop.name = String::create(lpszDrvName);
					prop.description = String::create(lpszDesc);
					list.add_NoLock(prop);
				}
				return TRUE;
			}

			void onFrame(int no)
			{
				sl_uint32 offset = 0;
				sl_uint32 size = m_nSamplesFrame * 2;
				if (no) {
					offset = size;
				}
				LPVOID buf = NULL;
				DWORD dwSize = 0;
				m_buffer->Lock(offset, size, &buf, &dwSize, NULL, NULL, 0);
				if (buf && dwSize) {
					sl_uint32 n;
					if (dwSize > size) {
						n = size / 2;
					} else {
						n = dwSize / 2;
					}
					_processFrame((sl_int16*)buf, n);
					m_buffer->Unlock(buf, dwSize, NULL, NULL);
				}
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				InitCOM();
				while (thread->isNotStopping()) {
					DWORD dwWait = WaitForMultipleObjects(3, m_events, FALSE, INFINITE);
					if (dwWait >= WAIT_OBJECT_0 && dwWait < WAIT_OBJECT_0 + 2) {
						onFrame(dwWait - WAIT_OBJECT_0);
					}
				}
			}

		};
	}

	Ref<AudioRecorder> AudioRecorder::create(const AudioRecorderParam& param)
	{
		return AudioRecorderImpl::create(param);
	}

	List<AudioRecorderDeviceInfo> AudioRecorder::getDevices()
	{
		List<AudioRecorderDeviceInfo> ret;
		ListElements<AudioRecorderImpl::DeviceProperty> props(AudioRecorderImpl::queryDeviceInfos());
		for (sl_size i = 0; i < props.count; i++) {
			AudioRecorderImpl::DeviceProperty& prop = props[i];
			AudioRecorderDeviceInfo info;
			info.id = prop.szGuid;
			info.name = prop.description;
			info.description = prop.description;
			ret.add_NoLock(info);
		}
		return ret;
	}

	namespace {
		class AudioPlayerDeviceImpl : public AudioPlayerDevice
		{
		public:
			LPDIRECTSOUND m_ds;
			GUID m_deviceID;

		public:
			AudioPlayerDeviceImpl()
			{
				m_ds = NULL;
			}

			~AudioPlayerDeviceImpl()
			{
				release();
			}

		public:
			static Ref<AudioPlayerDeviceImpl> create(const AudioPlayerDeviceParam& param)
			{
				InitCOM();

				GUID gid;
				String deviceID = param.deviceId;
				if (deviceID.isEmpty()) {
					gid = DSDEVID_DefaultPlayback;
				} else {
					ListLocker<DeviceProperty> props(queryDeviceInfos());
					sl_size i = 0;
					for (; i < props.count; i++) {
						if (deviceID == props[i].szGuid) {
							gid = props[i].guid;
							break;
						}
					}
					if (i == props.count) {
						LOG_ERROR("Failed to find player device: %s", deviceID);
						return sl_null;
					}
				}

				LPDIRECTSOUND ds;
				HRESULT hr = DirectSoundCreate(&gid, &ds, NULL);
				if (SUCCEEDED(hr)) {
					hr = ds->SetCooperativeLevel(GetDesktopWindow(), DSSCL_NORMAL);
					if (SUCCEEDED(hr)) {
						Ref<AudioPlayerDeviceImpl> ret = new AudioPlayerDeviceImpl();
						if (ret.isNotNull()) {
							ret->m_deviceID = gid;
							ret->m_ds = ds;
							return ret;
						}
					} else {
						LOG_ERROR("Direct sound set CooperativeLevel failed.");
					}
					ds->Release();
				} else {
					if (hr == DSERR_ALLOCATED) {
						LOG_ERROR("Direct sound playback device is already used");
					} else {
						LOG_ERROR("Can not create direct sound playback device");
					}
				}
				return sl_null;
			}

			void release()
			{
				ObjectLocker lock(this);
				if (m_ds) {
					m_ds->Release();
					m_ds = NULL;
				}
			}

			Ref<AudioPlayer> createPlayer(const AudioPlayerParam& param) override;

			struct DeviceProperty {
				GUID guid;
				String szGuid;
				String name;
				String description;
			};

			static List<DeviceProperty> queryDeviceInfos()
			{
				List<DeviceProperty> list;
				HRESULT hr = DirectSoundEnumerateW(DeviceEnumProc, (VOID*)&list);
				if (FAILED(hr)) {
					LOG_ERROR("Can not query player device info");
				}
				return list;
			}

			static BOOL CALLBACK DeviceEnumProc(LPGUID lpGUID, LPCWSTR lpszDesc, LPCWSTR lpszDrvName, LPVOID lpContext)
			{
				List<DeviceProperty>& list = *((List<DeviceProperty>*)lpContext);
				DeviceProperty prop;
				if (lpGUID) {
					prop.guid = *lpGUID;
					prop.szGuid = Win32::getStringFromGUID(*lpGUID);
					prop.name = String::create(lpszDrvName);
					prop.description = String::create(lpszDesc);
					list.add_NoLock(prop);
				}
				return TRUE;
			}
		};

		class AudioPlayerImpl : public AudioPlayer
		{
		public:
			Ref<AudioPlayerDeviceImpl> m_device;
			LPDIRECTSOUNDBUFFER m_dsBuffer;
			LPDIRECTSOUNDNOTIFY m_dsNotify;
			HANDLE m_hNotificationEvents[2];

			sl_uint32 m_nBufferSize;
			sl_uint32 m_nOffsetNextWrite;
			sl_uint32 m_nNotifySize;

			Ref<Thread> m_thread;

		public:
			AudioPlayerImpl()
			{
				m_nOffsetNextWrite = m_nNotifySize = 0;
				m_dsBuffer = NULL;
				m_dsNotify = NULL;
				m_flagOpened = sl_true;
				m_flagRunning = sl_false;
				m_thread.setNull();
			}

			~AudioPlayerImpl()
			{
				release();
			}

		public:
			static Ref<AudioPlayerImpl> create(const Ref<AudioPlayerDeviceImpl>& device, const AudioPlayerParam& param)
			{
				if (param.channelCount != 1 && param.channelCount != 2) {
					return sl_null;
				}

				WAVEFORMATEX wf;
				wf.wFormatTag = WAVE_FORMAT_PCM;
				wf.nChannels = param.channelCount;
				wf.wBitsPerSample = 16;
				wf.nSamplesPerSec = param.samplesPerSecond;
				wf.nBlockAlign = wf.wBitsPerSample * wf.nChannels / 8;
				wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
				wf.cbSize = 0;

				sl_uint32 samplesPerFrame = wf.nSamplesPerSec * param.frameLengthInMilliseconds / 1000;
				sl_uint32 sizeBuffer = samplesPerFrame * wf.nBlockAlign * 3;
				sl_int32 notifySize = sizeBuffer / NUM_PLAY_NOTIFICATIONS;

				DSBUFFERDESC desc;
				ZeroMemory(&desc, sizeof(DSBUFFERDESC));
				desc.dwSize = sizeof(DSBUFFERDESC);
				desc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
				desc.dwBufferBytes = sizeBuffer;
				desc.lpwfxFormat = &wf;

				LPDIRECTSOUNDBUFFER dsBuffer;
				HANDLE hNotificationEvents[2];
				hNotificationEvents[0] = CreateEventW(NULL, FALSE, FALSE, NULL);
				hNotificationEvents[1] = CreateEventW(NULL, FALSE, FALSE, NULL);

				HRESULT hr = device->m_ds->CreateSoundBuffer(&desc, &dsBuffer, NULL);

				if (SUCCEEDED(hr)) {

					LPDIRECTSOUNDNOTIFY dsNotify;
					hr = dsBuffer->QueryInterface(IID_IDirectSoundNotify, (VOID**)&dsNotify);

					if (SUCCEEDED(hr)) {

						DSBPOSITIONNOTIFY   dsPosNotify[NUM_PLAY_NOTIFICATIONS + 1];
						sl_int32 notifyIndex = 0;

						for (notifyIndex = 0; notifyIndex < NUM_PLAY_NOTIFICATIONS; notifyIndex++) {
							dsPosNotify[notifyIndex].dwOffset = (notifySize * notifyIndex) + notifySize - 1;
							dsPosNotify[notifyIndex].hEventNotify = hNotificationEvents[0];
						}
						dsPosNotify[notifyIndex].dwOffset = DSBPN_OFFSETSTOP;
						dsPosNotify[notifyIndex].hEventNotify = hNotificationEvents[1];

						hr = dsNotify->SetNotificationPositions(NUM_PLAY_NOTIFICATIONS + 1, dsPosNotify);

						if (SUCCEEDED(hr)) {

							Ref<AudioPlayerImpl> ret = new AudioPlayerImpl();

							if (ret.isNotNull()) {
								ret->m_device = device;
								ret->m_dsBuffer = dsBuffer;
								ret->m_dsNotify = dsNotify;
								ret->m_nNotifySize = notifySize;
								ret->m_hNotificationEvents[0] = hNotificationEvents[0];
								ret->m_hNotificationEvents[1] = hNotificationEvents[1];
								ret->m_nBufferSize = sizeBuffer;

								ret->_init(param);

								if (param.flagAutoStart) {
									ret->start();
								}

								return ret;
							}

						} else {
							LOG_ERROR("Failed to set dsound notify positions");
						}
					} else {
						LOG_ERROR("Failed to get dsound notify");
					}
					dsBuffer->Release();
				} else {
					LOG_ERROR("Failed to create dsound buffer 8");
				}
				CloseHandle(hNotificationEvents[0]);
				CloseHandle(hNotificationEvents[1]);

				return sl_null;
			}

			void _release() override
			{
				m_dsBuffer->Release();
				m_dsBuffer = NULL;

				m_dsNotify->Release();
				m_dsNotify = NULL;

				m_device.setNull();

				for (int i = 0; i < 2; i++) {
					CloseHandle(m_hNotificationEvents[i]);
				}
			}

			sl_bool _start() override
			{
				HRESULT hr = m_dsBuffer->Play(0, 0, DSBPLAY_LOOPING);
				if (SUCCEEDED(hr)) {
					m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
					if (m_thread.isNotNull()) {
						return sl_true;
					}
				} else {
					LOG_ERROR("Failed to play");
				}
				return sl_false;
			}

			void _stop() override
			{
				m_thread->finish();
				SetEvent(m_hNotificationEvents[1]);
				m_thread->finishAndWait();
				m_thread.setNull();

				m_dsBuffer->Stop();
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				InitCOM();
				while (thread->isNotStopping()) {
					DWORD dwWait = WaitForMultipleObjects(2, m_hNotificationEvents, FALSE, INFINITE);
					if (dwWait == WAIT_OBJECT_0) {
						onFrame();
					} else {
						m_dsBuffer->Stop();
					}
				}
			}

			void onFrame()
			{
				VOID* pbBuffer = NULL;
				DWORD bufferSize;

				HRESULT hr = m_dsBuffer->Lock(m_nOffsetNextWrite, m_nNotifySize, &pbBuffer, &bufferSize, NULL, NULL, 0L);
				if (SUCCEEDED(hr)) {
					sl_uint32 nSamples = bufferSize / 2;
					sl_int16* s = (sl_int16*)pbBuffer;

					_processFrame(s, nSamples);

					m_nOffsetNextWrite += bufferSize;
					m_nOffsetNextWrite %= m_nBufferSize;
					m_dsBuffer->Unlock(pbBuffer, bufferSize, NULL, NULL);
				}
			}

		};

		Ref<AudioPlayer> AudioPlayerDeviceImpl::createPlayer(const AudioPlayerParam& param)
		{
			return AudioPlayerImpl::create(this, param);
		}
	}

	Ref<AudioPlayerDevice> AudioPlayerDevice::create(const AudioPlayerDeviceParam& param)
	{
		return AudioPlayerDeviceImpl::create(param);
	}

	List<AudioPlayerDeviceInfo> AudioPlayerDevice::getDevices()
	{
		List<AudioPlayerDeviceInfo> ret;
		ListElements<AudioPlayerDeviceImpl::DeviceProperty> props(AudioPlayerDeviceImpl::queryDeviceInfos());
		for (sl_size i = 0; i < props.count; i++) {
			AudioPlayerDeviceImpl::DeviceProperty& prop = props[i];
			AudioPlayerDeviceInfo info;
			info.id = prop.szGuid;
			info.name = prop.description;
			info.description = prop.description;
			ret.add_NoLock(info);
		}
		return ret;
	}

}

#endif
