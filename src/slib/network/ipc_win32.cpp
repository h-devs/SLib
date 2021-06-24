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
#include "slib/core/event.h"
#include "slib/core/memory_output.h"
#include "slib/core/win32/platform.h"

namespace slib
{

	namespace priv
	{
		namespace ipc
		{

			class NamedPipeIPCImpl : public IPC
			{
			public:
				Ref<Thread> m_threadListen;
				CList< Ref<Thread> > m_threads;

				String16 m_name;
				sl_uint32 m_maxThreadsCount;
				sl_uint32 m_maxReceivingMessageSize;

				Function<void(sl_uint8* data, sl_uint32 size, MemoryOutput* output)> m_onReceiveMessage;

			public:
				NamedPipeIPCImpl()
				{
				}

				~NamedPipeIPCImpl()
				{
					if (m_threadListen.isNotNull()) {
						m_threadListen->finishAndWait();
					}
					List< Ref<Thread> > threads = m_threads.duplicate();
					for (auto& item : threads) {
						item->finish();
					}
					for (auto& item : threads) {
						item->finishAndWait();
					}
				}

			public:
				static Ref<NamedPipeIPCImpl> create(const IPCParam& param)
				{
					Ref<NamedPipeIPCImpl> ret = new NamedPipeIPCImpl;
					if (ret.isNotNull()) {
						ret->m_maxThreadsCount = param.maxThreadsCount;
						ret->m_maxReceivingMessageSize = param.maxReceivingMessageSize;
						Ref<Thread> thread = Thread::start(SLIB_FUNCTION_MEMBER(NamedPipeIPCImpl, runListen, ret.get()));
						if (thread.isNotNull()) {
							ret->m_name = String16::join(L"\\\\.\\pipe\\", param.name);
							ret->m_threadListen = Move(thread);
							ret->m_onReceiveMessage = param.onReceiveMessage;
							return ret;
						}
					}
					return sl_null;
				}

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
					HANDLE hEvent = Win32::getEventHandle(ev.get());

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
								NULL);

							if (hPipe != INVALID_HANDLE_VALUE) {

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
										Ref<Thread> thread = Thread::create([thiz, this, hPipe]() {
											auto ref = ToRef(thiz);
											if (ref.isNull()) {
												return;
											}
											processReceiving(hPipe);
											CloseHandle(hPipe);
										});
										if (thread.isNotNull()) {
											m_threads.add(thread);
											thread->start();
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

				Memory readMessage(Thread* thread, HANDLE handle)
				{
					Ref<Event> event = Event::create();
					if (event.isNull()) {
						return sl_null;
					}
					HANDLE hEvent = Win32::getEventHandle(event.get());
	
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
									event->wait();
									if (GetOverlappedResult(handle, &overlapped, &dwRead, FALSE)) {
										if (dwRead) {
											bufRead.add(Memory::create(buf, dwRead));
										}
										return bufRead.merge();
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
							event->wait();
							if (GetOverlappedResult(handle, &overlapped, &dwWritten, FALSE)) {
								return dwWritten == (DWORD)size;
							}
						}
						return sl_false;
					}
				}

			};

		}
	}

	using namespace priv::ipc;

	Ref<IPC> IPC::create(const IPCParam& param)
	{
		return Ref<IPC>::from(NamedPipeIPCImpl::create(param));
	}

}

#endif