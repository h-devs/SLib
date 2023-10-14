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

// Core Audio
#pragma warning(disable: 4091)
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audioclient.h>
#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

#define TAG "Audio"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)

#define NUM_PLAY_NOTIFICATIONS  2

#pragma comment(lib, "dsound.lib")

namespace slib
{

	namespace
	{

		static void InitCOM()
		{
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		}

		struct DSoundDeviceProperty
		{
			GUID guid;
			String szGuid;
			String name;
		};

		static BOOL CALLBACK DSoundDeviceEnumProc(LPGUID lpGUID, LPCWSTR lpszDesc, LPCWSTR lpszDrvName, LPVOID lpContext)
		{
			List<DSoundDeviceProperty>& list = *((List<DSoundDeviceProperty>*)lpContext);
			DSoundDeviceProperty prop;
			if (lpGUID) {
				prop.guid = *lpGUID;
				prop.szGuid = Win32::getStringFromGUID(*lpGUID);
				prop.name = String::create(lpszDesc);
				list.add_NoLock(prop);
			}
			return TRUE;
		}

		class DSoundRecorderImpl : public AudioRecorder
		{
		public:
			IDirectSoundCapture8* m_device = NULL;
			IDirectSoundCaptureBuffer8* m_buffer = NULL;
			sl_uint32 m_nSamplesPerFrame = 0;
			HANDLE m_events[3] = {0};
			Ref<Thread> m_thread;

		public:
			DSoundRecorderImpl()
			{
				m_events[2] = CreateEventW(NULL, FALSE, FALSE, NULL);
			}

			~DSoundRecorderImpl()
			{
				release();
			}

		public:
			static Ref<DSoundRecorderImpl> create(const AudioRecorderParam& param)
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
					ListLocker<DSoundDeviceProperty> props(queryDeviceInfos());
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

				IDirectSoundCapture8* device = NULL;
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

				IDirectSoundCaptureBuffer* _buffer = NULL;
				IDirectSoundCaptureBuffer8* buffer = NULL;
				hr = device->CreateCaptureBuffer(&desc, &_buffer, NULL);
				if (SUCCEEDED(hr)) {
					_buffer->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&buffer);
					_buffer->Release();
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
								Ref<DSoundRecorderImpl> ret = new DSoundRecorderImpl();
								if (ret.isNotNull()) {
									ret->m_device = device;
									ret->m_buffer = buffer;
									ret->m_events[0] = hEvent0;
									ret->m_events[1] = hEvent1;
									ret->m_nSamplesPerFrame = samplesPerFrame;
									ret->_init(param);
									if (param.flagAutoStart) {
										ret->start();
									}
									return ret;
								}
							} else {
								LOG_ERROR("Failed to set DirectSound notify postions");
							}
							notify->Release();
						} else {
							LOG_ERROR("Failed to get IDirectSoundNotify8");
						}
						buffer->Release();
					} else {
						LOG_ERROR("Failed to get IDirectSoundCaptureBuffer8");
					}
				} else {
					LOG_ERROR("Failed to create IDirectSoundCaptureBuffer");
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

			static List<DSoundDeviceProperty> queryDeviceInfos()
			{
				InitCOM();
				List<DSoundDeviceProperty> list;
				HRESULT hr = DirectSoundCaptureEnumerateW(DSoundDeviceEnumProc, (VOID*)&list);
				if (FAILED(hr)) {
					LOG_ERROR("Can not query device info");
				}
				return list;
			}

			void onFrame(int no)
			{
				sl_uint32 offset = 0;
				sl_uint32 size = m_nSamplesPerFrame * 2;
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

		class DSoundPlayerDeviceImpl : public AudioPlayerDevice
		{
		public:
			LPDIRECTSOUND m_device = NULL;
			GUID m_deviceID;

		public:
			DSoundPlayerDeviceImpl()
			{
			}

			~DSoundPlayerDeviceImpl()
			{
				release();
			}

		public:
			static Ref<DSoundPlayerDeviceImpl> create(const AudioPlayerDeviceParam& param)
			{
				GUID gid;
				String deviceID = param.deviceId;
				if (deviceID.isEmpty()) {
					gid = DSDEVID_DefaultPlayback;
				} else {
					ListLocker<DSoundDeviceProperty> props(queryDeviceInfos());
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

				InitCOM();

				IDirectSound* device;
				HRESULT hr = DirectSoundCreate(&gid, &device, NULL);
				if (SUCCEEDED(hr)) {
					hr = device->SetCooperativeLevel(GetDesktopWindow(), DSSCL_NORMAL);
					if (SUCCEEDED(hr)) {
						Ref<DSoundPlayerDeviceImpl> ret = new DSoundPlayerDeviceImpl();
						if (ret.isNotNull()) {
							ret->m_device = device;
							ret->m_deviceID = gid;
							return ret;
						}
					} else {
						LOG_ERROR("Failed to call DirectSound::SetCooperativeLevel");
					}
					device->Release();
				} else {
					if (hr == DSERR_ALLOCATED) {
						LOG_ERROR("DirectSound playback device is already used");
					} else {
						LOG_ERROR("Can not create DirectSound playback device");
					}
				}
				return sl_null;
			}

			void release()
			{
				ObjectLocker lock(this);
				if (m_device) {
					m_device->Release();
					m_device = NULL;
				}
			}

			Ref<AudioPlayer> createPlayer(const AudioPlayerParam& param) override;

			static List<DSoundDeviceProperty> queryDeviceInfos()
			{
				List<DSoundDeviceProperty> list;
				HRESULT hr = DirectSoundEnumerateW(DSoundDeviceEnumProc, (void*)&list);
				if (FAILED(hr)) {
					LOG_ERROR("Can not query player device info");
				}
				return list;
			}

		};

		class DSoundPlayerImpl : public AudioPlayer
		{
		public:
			Ref<DSoundPlayerDeviceImpl> m_device;
			IDirectSoundBuffer* m_buffer = NULL;
			IDirectSoundNotify* m_notify = NULL;
			HANDLE m_events[2] = {0};

			sl_uint32 m_nBufferSize = 0;
			sl_uint32 m_nOffsetNextWrite = 0;
			sl_uint32 m_nNotifySize = 0;

			Ref<Thread> m_thread;

		public:
			DSoundPlayerImpl()
			{
			}

			~DSoundPlayerImpl()
			{
				release();
			}

		public:
			static Ref<DSoundPlayerImpl> create(const Ref<DSoundPlayerDeviceImpl>& device, const AudioPlayerParam& param)
			{
				if (param.channelCount != 1 && param.channelCount != 2) {
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

				sl_uint32 samplesPerFrame = wf.nSamplesPerSec * param.frameLengthInMilliseconds / 1000;
				sl_uint32 sizeBuffer = samplesPerFrame * wf.nBlockAlign * 3;
				sl_int32 notifySize = sizeBuffer / NUM_PLAY_NOTIFICATIONS;

				DSBUFFERDESC desc;
				ZeroMemory(&desc, sizeof(DSBUFFERDESC));
				desc.dwSize = sizeof(DSBUFFERDESC);
				desc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
				desc.dwBufferBytes = sizeBuffer;
				desc.lpwfxFormat = &wf;

				IDirectSoundBuffer* buffer = NULL;
				HANDLE events[2];
				events[0] = CreateEventW(NULL, FALSE, FALSE, NULL);
				events[1] = CreateEventW(NULL, FALSE, FALSE, NULL);

				HRESULT hr = device->m_device->CreateSoundBuffer(&desc, &buffer, NULL);
				if (SUCCEEDED(hr)) {
					IDirectSoundNotify* notify = NULL;
					hr = buffer->QueryInterface(IID_IDirectSoundNotify, (VOID**)&notify);
					if (SUCCEEDED(hr)) {
						DSBPOSITIONNOTIFY pos[NUM_PLAY_NOTIFICATIONS + 1];
						sl_int32 notifyIndex = 0;
						for (notifyIndex = 0; notifyIndex < NUM_PLAY_NOTIFICATIONS; notifyIndex++) {
							pos[notifyIndex].dwOffset = (notifySize * notifyIndex) + notifySize - 1;
							pos[notifyIndex].hEventNotify = events[0];
						}
						pos[notifyIndex].dwOffset = DSBPN_OFFSETSTOP;
						pos[notifyIndex].hEventNotify = events[1];
						hr = notify->SetNotificationPositions(NUM_PLAY_NOTIFICATIONS + 1, pos);
						if (SUCCEEDED(hr)) {
							Ref<DSoundPlayerImpl> ret = new DSoundPlayerImpl();
							if (ret.isNotNull()) {
								ret->m_device = device;
								ret->m_buffer = buffer;
								ret->m_notify = notify;
								ret->m_nNotifySize = notifySize;
								ret->m_events[0] = events[0];
								ret->m_events[1] = events[1];
								ret->m_nBufferSize = sizeBuffer;
								ret->_init(param);
								if (param.flagAutoStart) {
									ret->start();
								}
								return ret;
							}
						} else {
							LOG_ERROR("Failed to set DirectSound notify positions");
						}
						notify->Release();
					} else {
						LOG_ERROR("Failed to get IDirectSoundNotify");
					}
					buffer->Release();
				} else {
					LOG_ERROR("Failed to create IDirectSoundBuffer");
				}

				CloseHandle(events[0]);
				CloseHandle(events[1]);

				return sl_null;
			}

			void _release() override
			{
				m_buffer->Release();
				m_buffer = NULL;

				m_notify->Release();
				m_notify = NULL;

				m_device.setNull();

				for (int i = 0; i < 2; i++) {
					CloseHandle(m_events[i]);
				}
			}

			sl_bool _start() override
			{
				HRESULT hr = m_buffer->Play(0, 0, DSBPLAY_LOOPING);
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
				SetEvent(m_events[1]);
				m_thread->finishAndWait();
				m_thread.setNull();

				m_buffer->Stop();
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				InitCOM();
				while (thread->isNotStopping()) {
					DWORD dwWait = WaitForMultipleObjects(2, m_events, FALSE, INFINITE);
					if (dwWait == WAIT_OBJECT_0) {
						onFrame();
					} else {
						m_buffer->Stop();
					}
				}
			}

			void onFrame()
			{
				VOID* pbBuffer = NULL;
				DWORD bufferSize;

				HRESULT hr = m_buffer->Lock(m_nOffsetNextWrite, m_nNotifySize, &pbBuffer, &bufferSize, NULL, NULL, 0L);
				if (SUCCEEDED(hr)) {
					sl_uint32 nSamples = bufferSize / 2;
					sl_int16* s = (sl_int16*)pbBuffer;

					_processFrame(s, nSamples);

					m_nOffsetNextWrite += bufferSize;
					m_nOffsetNextWrite %= m_nBufferSize;
					m_buffer->Unlock(pbBuffer, bufferSize, NULL, NULL);
				}
			}

		};

		Ref<AudioPlayer> DSoundPlayerDeviceImpl::createPlayer(const AudioPlayerParam& param)
		{
			return DSoundPlayerImpl::create(this, param);
		}

		// WASAPI: Windows Audio Session API
		static sl_bool IsUsingCoreAudio()
		{
			return Win32::isWindows7OrGreater();
		}

		static ERole GetCoreAudioDeviceRole(sl_bool flagInput, AudioDeviceRole role)
		{
			switch (role) {
				case AudioDeviceRole::Console:
					return eConsole;
				case AudioDeviceRole::Multimedia:
					return eMultimedia;
				case AudioDeviceRole::Communications:
					return eCommunications;
				default:
					break;
			}
			if (flagInput) {
				return eConsole;
			} else {
				return eMultimedia;
			}
		}

		template <class INFO>
		static void GetCoreAudioDeviceInfos(List<INFO>& ret, sl_bool flagInput)
		{
			InitCOM();
			IMMDeviceEnumerator* enumerator = NULL;
			HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
			if (SUCCEEDED(hr)) {
				IMMDeviceCollection* collection = NULL;
				hr = enumerator->EnumAudioEndpoints(flagInput ? eCapture : eRender, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &collection);
				if (SUCCEEDED(hr)) {
					UINT n = 0;
					hr = collection->GetCount(&n);
					if (SUCCEEDED(hr)) {
						for (UINT i = 0; i < n; i++) {
							IMMDevice* device = NULL;
							hr = collection->Item(i, &device);
							if (SUCCEEDED(hr)) {
								LPWSTR strId = NULL;
								hr = device->GetId(&strId);
								if (strId) {
									IPropertyStore* props = NULL;
									hr = device->OpenPropertyStore(STGM_READ, &props);
									if (SUCCEEDED(hr)) {
										PROPVARIANT varName;
										PropVariantInit(&varName);
										hr = props->GetValue(PKEY_Device_FriendlyName, &varName);
										if (SUCCEEDED(hr)) {
											AudioPlayerDeviceInfo info;
											info.id = String::from(strId);
											info.name = String::from(varName.bstrVal);
											ret.add_NoLock(Move(info));
											PropVariantClear(&varName);
										}
										props->Release();
									}
									CoTaskMemFree(strId);
								}
								device->Release();
							}
						}
					}
					collection->Release();
				}
				enumerator->Release();
			}
		}

		static IMMDevice* GetCoreAudioDevice(sl_bool flagInput, const AudioDeviceParam& param)
		{
			InitCOM();
			IMMDevice* device = NULL;
			IMMDeviceEnumerator* enumerator = NULL;
			HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
			if (SUCCEEDED(hr)) {
				if (param.deviceId.isNotEmpty()) {
					StringCstr16 deviceId(param.deviceId);
					hr = enumerator->GetDevice((LPCWSTR)(deviceId.getData()), &device);
				} else {
					hr = enumerator->GetDefaultAudioEndpoint(flagInput ? eCapture : eRender, GetCoreAudioDeviceRole(flagInput, param.role), &device);
				}
				if (FAILED(hr)) {
					if (hr == E_NOTFOUND) {
						LOG_ERROR("WASAPI device is not found");
					} else {
						LOG_ERROR("Can not create WASAPI device");
					}
				}
				enumerator->Release();
			}
			return device;
		}

		class WASRecorderImpl : public AudioRecorder
		{
		public:
			IMMDevice* m_device = NULL;
			IAudioClient* m_client = NULL;
			IAudioCaptureClient* m_capture = NULL;
			sl_uint32 m_bufferSize = 0;
			sl_uint32 m_nBlocksPerSecond = 0;
			Ref<Thread> m_thread;

		public:
			WASRecorderImpl()
			{
			}

			~WASRecorderImpl()
			{
				release();
			}

		public:
			static Ref<WASRecorderImpl> create(const AudioRecorderParam& param)
			{
				IMMDevice* device = GetCoreAudioDevice(!(param.flagLoopback), param);
				if (device) {
					IAudioClient* client = NULL;
					HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&client);
					if (SUCCEEDED(hr)) {
						WAVEFORMATEX wf;
						wf.wFormatTag = WAVE_FORMAT_PCM;
						wf.nChannels = param.channelCount;
						wf.wBitsPerSample = 16;
						wf.nSamplesPerSec = param.samplesPerSecond;
						wf.nBlockAlign = (wf.wBitsPerSample * wf.nChannels) >> 3;
						wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
						wf.cbSize = 0;
						DWORD flags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
						if (param.flagLoopback) {
							flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
						}
						hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, param.frameLengthInMilliseconds * 10000, 0, &wf, NULL);
						if (SUCCEEDED(hr)) {
							IAudioCaptureClient* capture = NULL;
							hr = client->GetService(IID_PPV_ARGS(&capture));
							if (SUCCEEDED(hr)) {
								UINT32 sizeBuf = 0;
								HRESULT hr = client->GetBufferSize(&sizeBuf);
								if (SUCCEEDED(hr) && sizeBuf) {
									Ref<WASRecorderImpl> ret = new WASRecorderImpl();
									if (ret.isNotNull()) {
										ret->m_device = device;
										ret->m_client = client;
										ret->m_capture = capture;
										ret->m_bufferSize = (sl_uint32)sizeBuf;
										ret->m_nBlocksPerSecond = (sl_uint32)(wf.nSamplesPerSec * wf.nChannels);
										ret->_init(param);
										if (param.flagAutoStart) {
											ret->start();
										}
										return ret;
									}
								} else {
									LOG_ERROR("Failed to get client buffer size");
								}
								capture->Release();
							} else {
								LOG_ERROR("Failed to get IAudioCaptureClient");
							}
							client->Release();
						} else {
							LOG_ERROR("Failed to initialize IAudioClient");
						}
					} else {
						LOG_ERROR("Failed to activate IAudioClient");
						return sl_null;
					}
					device->Release();
				}
				return sl_null;
			}

			void _release() override
			{
				m_capture->Release();
				m_capture = NULL;
				m_client->Release();
				m_client = NULL;
				m_device->Release();
				m_device = NULL;
			}

			sl_bool _start() override
			{
				HRESULT hr = m_client->Start();
				if (SUCCEEDED(hr)) {
					m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
					if (m_thread.isNotNull()) {
						return sl_true;
					}
					m_client->Stop();
				} else {
					LOG_ERROR("Failed to start");
				}
				return sl_false;
			}

			void _stop() override
			{
				m_thread->finishAndWait();
				m_thread.setNull();
				m_client->Stop();
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				InitCOM();
				UINT32 bufferSize = (UINT32)m_bufferSize;
				sl_uint32 iterDuration = bufferSize * 500 / m_nBlocksPerSecond;
				sl_uint32 nChannels = m_param.channelCount;
				while (thread->isNotStopping()) {
					UINT32 n = 0;
					HRESULT hr = m_capture->GetNextPacketSize(&n);
					if (FAILED(hr)) {
						return;
					}
					if (n) {
						BYTE* pData = NULL;
						DWORD flags = 0;
						n = 0;
						hr = m_capture->GetBuffer(&pData, &n, &flags, NULL, NULL);
						if (FAILED(hr)) {
							return;
						}
						if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
							_processSilent(n * nChannels);
						} else {
							_processFrame((sl_int16*)pData, n * nChannels);
						}
						m_capture->ReleaseBuffer(n);
					} else {
						thread->sleep(iterDuration);
					}
				}
			}

		};

		class WASPlayerDeviceImpl : public AudioPlayerDevice
		{
		public:
			IMMDevice* m_device = NULL;

		public:
			WASPlayerDeviceImpl()
			{
			}

			~WASPlayerDeviceImpl()
			{
				release();
			}

		public:
			static Ref<WASPlayerDeviceImpl> create(const AudioPlayerDeviceParam& param)
			{
				IMMDevice* device = GetCoreAudioDevice(sl_false, param);
				if (device) {
					Ref<WASPlayerDeviceImpl> ret = new WASPlayerDeviceImpl();
					if (ret.isNotNull()) {
						ret->m_device = device;
						return ret;
					}
				}
				return sl_null;
			}

			void release()
			{
				ObjectLocker lock(this);
				if (m_device) {
					m_device->Release();
					m_device = NULL;
				}
			}

			Ref<AudioPlayer> createPlayer(const AudioPlayerParam& param) override;

		};

		class WASPlayerImpl : public AudioPlayer
		{
		public:
			Ref<WASPlayerDeviceImpl> m_device;
			IAudioClient* m_client = NULL;
			IAudioRenderClient* m_renderer = NULL;
			sl_uint32 m_bufferSize = 0;
			sl_uint32 m_nBlocksPerSecond = 0;
			Ref<Thread> m_thread;

		public:
			WASPlayerImpl()
			{
			}

			~WASPlayerImpl()
			{
				release();
			}

		public:
			static Ref<WASPlayerImpl> create(const Ref<WASPlayerDeviceImpl>& device, const AudioPlayerParam& param)
			{
				IAudioClient* client = NULL;
				HRESULT hr = device->m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&client);
				if (FAILED(hr)) {
					LOG_ERROR("Failed to activate IAudioClient");
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
				hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, param.frameLengthInMilliseconds * 10000, 0, &wf, NULL);
				if (SUCCEEDED(hr)) {
					IAudioRenderClient* renderer = NULL;
					hr = client->GetService(IID_PPV_ARGS(&renderer));
					if (SUCCEEDED(hr)) {
						UINT32 sizeBuf = 0;
						HRESULT hr = client->GetBufferSize(&sizeBuf);
						if (SUCCEEDED(hr) && sizeBuf) {
							Ref<WASPlayerImpl> ret = new WASPlayerImpl();
							if (ret.isNotNull()) {
								ret->m_device = device;
								ret->m_client = client;
								ret->m_renderer = renderer;
								ret->m_bufferSize = (sl_uint32)sizeBuf;
								ret->m_nBlocksPerSecond = (sl_uint32)(wf.nSamplesPerSec * wf.nChannels);
								ret->_init(param);
								if (param.flagAutoStart) {
									ret->start();
								}
								return ret;
							}
						} else {
							LOG_ERROR("Failed to get client buffer size");
						}
						renderer->Release();
					} else {
						LOG_ERROR("Failed to get IAudioRenderClient");
					}
					client->Release();
				} else {
					LOG_ERROR("Failed to initialize IAudioClient");
				}
				return sl_null;
			}

			void _release() override
			{
				m_renderer->Release();
				m_renderer = NULL;

				m_client->Release();
				m_client = NULL;

				m_device.setNull();
			}

			sl_bool _start() override
			{
				m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
				if (m_thread.isNotNull()) {
					return sl_true;
				}
				return sl_false;
			}

			void _stop() override
			{
				m_thread->finishAndWait();
				m_thread.setNull();
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				InitCOM();
				UINT32 bufferSize = (UINT32)m_bufferSize;
				sl_uint32 iterDuration = bufferSize * 500 / m_nBlocksPerSecond;
				sl_uint32 nChannels = m_param.channelCount;
				sl_bool flagStarted = sl_false;
				while (thread->isNotStopping()) {
					UINT32 nPadding = 0;
					HRESULT hr = m_client->GetCurrentPadding(&nPadding);
					if (FAILED(hr)) {
						return;
					}
					if (nPadding < bufferSize) {
						UINT32 n = bufferSize - nPadding;
						BYTE* pData = NULL;
						HRESULT hr = m_renderer->GetBuffer((UINT32)n, &pData);
						if (FAILED(hr)) {
							return;
						}
						_processFrame((sl_int16*)pData, (sl_uint32)(n * nChannels));
						m_renderer->ReleaseBuffer((UINT32)n, 0);
						if (!flagStarted) {
							m_client->Start();
							flagStarted = sl_true;
						}
					} else {
						thread->sleep(iterDuration);
					}
				}
				if (flagStarted) {
					m_client->Stop();
				}
			}

		};

		Ref<AudioPlayer> WASPlayerDeviceImpl::createPlayer(const AudioPlayerParam& param)
		{
			return WASPlayerImpl::create(this, param);
		}

	}

	Ref<AudioRecorder> AudioRecorder::create(const AudioRecorderParam& param)
	{
		if (IsUsingCoreAudio()) {
			return WASRecorderImpl::create(param);
		} else {
			return DSoundRecorderImpl::create(param);
		}
	}

	List<AudioRecorderDeviceInfo> AudioRecorder::getDevices()
	{
		List<AudioPlayerDeviceInfo> ret;
		if (IsUsingCoreAudio()) {
			GetCoreAudioDeviceInfos(ret, sl_true);
		} else {
			ListElements<DSoundDeviceProperty> props(DSoundRecorderImpl::queryDeviceInfos());
			for (sl_size i = 0; i < props.count; i++) {
				DSoundDeviceProperty& prop = props[i];
				AudioRecorderDeviceInfo info;
				info.id = prop.szGuid;
				info.name = prop.name;
				ret.add_NoLock(Move(info));
			}
		}
		return ret;
	}

	Ref<AudioPlayerDevice> AudioPlayerDevice::create(const AudioPlayerDeviceParam& param)
	{
		if (IsUsingCoreAudio()) {
			return WASPlayerDeviceImpl::create(param);
		} else {
			return DSoundPlayerDeviceImpl::create(param);
		}
	}

	List<AudioPlayerDeviceInfo> AudioPlayerDevice::getDevices()
	{
		List<AudioPlayerDeviceInfo> ret;
		if (IsUsingCoreAudio()) {
			GetCoreAudioDeviceInfos(ret, sl_false);
		} else {
			ListElements<DSoundDeviceProperty> props(DSoundPlayerDeviceImpl::queryDeviceInfos());
			for (sl_size i = 0; i < props.count; i++) {
				DSoundDeviceProperty& prop = props[i];
				AudioPlayerDeviceInfo info;
				info.id = prop.szGuid;
				info.name = prop.name;
				ret.add_NoLock(info);
			}
		}
		return ret;
	}

}

#endif
