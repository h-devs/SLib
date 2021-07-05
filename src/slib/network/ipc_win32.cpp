/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/network/ipc.h"

#include "slib/core/thread.h"
#include "slib/core/process.h"
#include "slib/core/event.h"
#include "slib/core/memory_output.h"
#include "slib/core/time_counter.h"
#include "slib/core/win32/platform.h"

namespace slib
{

	namespace priv
	{
		namespace ipc
		{

			class NamedPipeIPC : public IPC
			{
			public:
				CList< Ref<Thread> > m_threads;

			public:
				NamedPipeIPC()
				{
				}

				~NamedPipeIPC()
				{
					List< Ref<Thread> > threads = m_threads.duplicate();
					for (auto& item : threads) {
						item->finish();
					}
					for (auto& item : threads) {
						item->finishAndWait();
					}
				}

			public:
				static Ref<NamedPipeIPC> create(const IPCParam& param)
				{
					Ref<NamedPipeIPC> ret = new NamedPipeIPC;
					if (ret.isNotNull()) {
						ret->_init(param);
						return ret;
					}
					return sl_null;
				}

			public:
				void sendMessage(const StringParam& _targetName, const Memory& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse) override
				{
					if (data.isNotNull()) {
						if (_targetName.isNotEmpty()) {
							String16 targetName = String16::join(L"\\\\.\\pipe\\", _targetName);
							if (m_threads.getCount() < m_maxThreadsCount) {
								HANDLE hPipe = CreateFileW(
									(LPCWSTR)(targetName.getData()),
									GENERIC_READ | GENERIC_WRITE,
									0, // No sharing 
									NULL, // Default security attributes
									OPEN_EXISTING,
									FILE_FLAG_OVERLAPPED,
									NULL // No template file
								);
								if (hPipe != INVALID_HANDLE_VALUE) {
									DWORD dwMode = PIPE_READMODE_MESSAGE;
									if (SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL)) {
										auto thiz = ToWeakRef(this);
										Ref<Thread> thread = Thread::create([thiz, this, hPipe, data, callbackResponse]() {
											auto ref = ToRef(thiz);
											if (ref.isNull()) {
												callbackResponse(sl_null, 0);
												return;
											}
											processSending(hPipe, data, callbackResponse);
											CloseHandle(hPipe);
										});
										if (thread.isNotNull()) {
											m_threads.add(thread);
											thread->start();
											return;
										}
									}
									CloseHandle(hPipe);
								}
							}
						}
					}
					callbackResponse(sl_null, 0);
				}

				void processSending(HANDLE hPipe, const Memory& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse)
				{
					Thread* thread = Thread::getCurrent();
					if (thread) {
						if (writeMessage(thread, hPipe, data.getData(), (sl_uint32)(data.getSize()))) {
							if (thread->isNotStoppingCurrent()) {
								Memory mem = readMessage(thread, hPipe);
								callbackResponse((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()));
								m_threads.remove(thread);
								return;
							}
						}
					}
					callbackResponse(sl_null, 0);
					m_threads.remove(thread);
				}

				Memory readMessage(Thread* thread, HANDLE handle)
				{
					Ref<Event> event = Event::create();
					if (event.isNull()) {
						return sl_null;
					}
					HANDLE hEvent = Win32::getEventHandle(event.get());

					TimeCounter tc;

					MemoryBuffer bufRead;
					char buf[1024];

					while (thread->isNotStopping()) {

						OVERLAPPED overlapped;
						Base::zeroMemory(&overlapped, sizeof(overlapped));
						overlapped.hEvent = hEvent;

						DWORD dwRead = 0;
						BOOL bRet = ReadFile(handle, buf, sizeof(buf), &dwRead, &overlapped);
						if (bRet) {
							if (dwRead) {
								bufRead.add(Memory::create(buf, dwRead));
							}
							return bufRead.merge();
						} else {
							DWORD err = GetLastError();
							if (err == ERROR_MORE_DATA) {
								if (dwRead) {
									bufRead.add(Memory::create(buf, dwRead));
								}
							} else {
								if (err == ERROR_IO_PENDING) {
									event->wait(10);
									if (GetOverlappedResult(handle, &overlapped, &dwRead, FALSE)) {
										if (dwRead) {
											bufRead.add(Memory::create(buf, dwRead));
										}
										return bufRead.merge();
									}
									if (tc.getElapsedMilliseconds() > m_timeout) {
										return sl_null;
									}
									err = GetLastError();
									if (err == ERROR_MORE_DATA) {
										if (dwRead) {
											bufRead.add(Memory::create(buf, dwRead));
										}
									} else {
										return sl_null;
									}
								} else {
									return sl_null;
								}
							}
						}
					}
					return sl_null;
				}

				sl_bool writeMessage(Thread* thread, HANDLE handle, const void* _data, sl_uint32 size)
				{
					sl_uint8* data = (sl_uint8*)_data;
					Ref<Event> event = Event::create();
					if (event.isNull()) {
						return sl_null;
					}
					HANDLE hEvent = Win32::getEventHandle(event.get());

					TimeCounter tc;

					OVERLAPPED overlapped;
					Base::zeroMemory(&overlapped, sizeof(overlapped));
					overlapped.hEvent = hEvent;

					DWORD dwWritten = 0;
					BOOL bRet = WriteFile(handle, data, (DWORD)size, &dwWritten, &overlapped);
					if (bRet) {
						return dwWritten == (DWORD)size;
					} else {
						DWORD err = GetLastError();
						if (err == ERROR_IO_PENDING) {
							event->wait(10);
							if (GetOverlappedResult(handle, &overlapped, &dwWritten, FALSE)) {
								return dwWritten == (DWORD)size;
							}
							if (tc.getElapsedMilliseconds() > m_timeout) {
								return sl_null;
							}
						}
						return sl_false;
					}
				}

			};

			class NamedPipeServer : public NamedPipeIPC
			{
			public:
				Ref<Thread> m_threadListen;
				String16 m_name;

			public:
				NamedPipeServer()
				{
				}

				~NamedPipeServer()
				{
					if (m_threadListen.isNotNull()) {
						m_threadListen->finishAndWait();
					}
				}

			public:
				static Ref<NamedPipeServer> create(const IPCParam& param)
				{
					Ref<NamedPipeServer> ret = new NamedPipeServer;
					if (ret.isNotNull()) {
						ret->_init(param);
						Ref<Thread> thread = Thread::start(SLIB_FUNCTION_MEMBER(NamedPipeServer, runListen, ret.get()));
						if (thread.isNotNull()) {
							ret->m_name = String16::join(L"\\\\.\\pipe\\", param.name);
							ret->m_threadListen = Move(thread);
							return ret;
						}
					}
					return sl_null;
				}

				void runListen()
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}

					Ref<Event> ev = Event::create();
					if (ev.isNull()) {
						return;
					}
					HANDLE hEvent = Win32::getEventHandle(ev);

					SECURITY_ATTRIBUTES* pSA = NULL;
					SECURITY_ATTRIBUTES sa;
					SECURITY_DESCRIPTOR sd;
					if (m_flagAcceptOtherUsers) {
						if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
							if (SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE)) {
								sa.nLength = sizeof(SECURITY_ATTRIBUTES);
								sa.lpSecurityDescriptor = &sd;
								sa.bInheritHandle = FALSE;
								pSA = &sa;
							}
						}
					}

					while (thread->isNotStopping()) {

						if (m_threads.getCount() < m_maxThreadsCount) {

							HANDLE hPipe = CreateNamedPipeW(
								(LPCWSTR)(m_name.getData()),
								PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
								PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
								PIPE_UNLIMITED_INSTANCES,
								64 << 10, // output buffer size 
								64 << 10, // input buffer size 
								0, // client time-out 
								pSA);

							if (hPipe != INVALID_HANDLE_VALUE) {

								if (pSA) {
									ImpersonateNamedPipeClient(hPipe);
								}

								OVERLAPPED overlapped;
								Base::zeroMemory(&overlapped, sizeof(overlapped));
								overlapped.hEvent = hEvent;
								
								while (thread->isNotStopping()) {
									sl_bool flagConnected = sl_false;
									if (ConnectNamedPipe(hPipe, &overlapped)) {
										flagConnected = sl_true;
									} else {
										DWORD err = GetLastError();
										if (err == ERROR_PIPE_CONNECTED) {
											flagConnected = sl_true;
										} else if (err == ERROR_IO_PENDING) {
											ev->wait();
											if (HasOverlappedIoCompleted(&overlapped)) {
												flagConnected = sl_true;
											}
										} else {
											CloseHandle(hPipe);
											thread->wait(10);
											break;
										}
									}
									if (flagConnected) {
										auto thiz = ToWeakRef(this);
										Ref<Thread> threadNew = Thread::create([thiz, this, hPipe]() {
											auto ref = ToRef(thiz);
											if (ref.isNull()) {
												return;
											}
											processReceiving(hPipe);
											CloseHandle(hPipe);
										});
										if (threadNew.isNotNull()) {
											m_threads.add(threadNew);
											threadNew->start();
										} else {
											CloseHandle(hPipe);
										}
										break;
									}
								}
								
							} else {
								break;
							}
						} else {
							thread->wait(10);
						}
					}
				}

				void processReceiving(HANDLE hPipe)
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}
					Memory mem = readMessage(thread, hPipe);
					if (mem.isNotNull() && thread->isNotStoppingCurrent()) {
						MemoryOutput response;
						m_onReceiveMessage((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()), &response);
						Memory output = response.getData();
						writeMessage(thread, hPipe, output.getData(), (sl_uint32)(output.getSize()));
					}
					m_threads.remove(thread);
				}

			};

		}
	}

	using namespace priv::ipc;

	Ref<IPC> IPC::create(const IPCParam& param)
	{
		if (param.name.isEmpty()) {
			return Ref<IPC>::from(NamedPipeIPC::create(param));
		} else {
			return Ref<IPC>::from(NamedPipeServer::create(param));
		}
	}

}

#endif