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

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/network/ipc.h"

#include "slib/io/chunk.h"
#include "slib/io/async_file_stream.h"
#include "slib/core/thread.h"
#include "slib/core/event.h"
#include "slib/platform/win32/platform.h"
#include "slib/platform/win32/async_handle.h"
#include "slib/platform/win32/scoped_handle.h"

namespace slib
{

	namespace
	{
		static String16 GetPipeName(const StringParam& _targetName, sl_bool flagGlobal)
		{
			if (flagGlobal) {
				return String16::concat(L"\\\\.\\pipe\\", _targetName);
			} else {
				return String16::concat(L"\\\\.\\pipe\\", System::getUserId(), "_", _targetName);
			}
		}

		static HANDLE CreatePipe(const StringParam& _targetName, sl_bool flagGlobal, sl_int32 timeout)
		{
			if (_targetName.isEmpty()) {
				return INVALID_HANDLE_VALUE;
			}
			for (;;) {
				String16 targetName = GetPipeName(_targetName, flagGlobal);
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
					return hPipe;
				}
				if (!timeout) {
					break;
				}
				if (GetLastError() == ERROR_PIPE_BUSY) {
					if (!(WaitNamedPipeW((LPCWSTR)(targetName.getData()), timeout > 0 ? (DWORD)timeout : NMPWAIT_WAIT_FOREVER))) {
						break;
					}
				} else {
					break;
				}
			}
			return INVALID_HANDLE_VALUE;
		}

		class PipeRequest : public IPCStreamRequest
		{
		public:
			static Ref<PipeRequest> create(const IPCRequestParam& param)
			{
				Ref<PipeRequest> request = new PipeRequest;
				if (request.isNotNull()) {
					sl_int64 tickEnd = GetTickFromTimeout(param.timeout);
					HANDLE hPipe = CreatePipe(param.targetName, param.flagGlobal, param.timeout);
					if (hPipe != INVALID_HANDLE_VALUE) {
						AsyncFileStreamParam streamParam;
						streamParam.handle = hPipe;
						streamParam.ioLoop = param.ioLoop;
						Ref<AsyncFileStream> stream = AsyncFileStream::create(streamParam);
						if (stream.isNotNull()) {
							if (request->initialize(Move(stream), param, tickEnd)) {
								request->sendRequest();
								return request;
							}
						}
					}
				}
				IPCResponseMessage errorMsg;
				param.onResponse(errorMsg);
				return sl_null;
			}
		};

		class PipeServer : public IPCStreamServer
		{
		public:
			String16 m_name;
			Ref<Thread> m_threadListen;

		public:
			PipeServer()
			{
			}

			~PipeServer()
			{
				if (m_threadListen.isNotNull()) {
					m_threadListen->finishAndWait();
				}
			}

		public:
			static Ref<PipeServer> create(const IPCServerParam& param)
			{
				Ref<PipeServer> ret = new PipeServer;
				if (ret.isNotNull()) {
					if (ret->initialize(param)) {
						Ref<Thread> thread = Thread::create(SLIB_FUNCTION_MEMBER(ret.get(), runListen));
						if (thread.isNotNull()) {
							ret->m_name = GetPipeName(param.name, param.flagGlobal);
							ret->m_threadListen = Move(thread);
							ret->m_ioLoop->start();
							ret->m_threadListen->start();
							return ret;
						}
					}
				}
				return sl_null;
			}

		public:
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
					HANDLE hPipe = CreateNamedPipeW(
						(LPCWSTR)(m_name.getData()),
						PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
						PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_REJECT_REMOTE_CLIENTS,
						PIPE_UNLIMITED_INSTANCES,
						64 << 10, // output buffer size 
						64 << 10, // input buffer size 
						0, // client time-out 
						pSA);
					if (hPipe == INVALID_HANDLE_VALUE) {
						DWORD dwErr = GetLastError();
						break;
					}
					if (pSA) {
						ImpersonateNamedPipeClient(hPipe);
					}
					OVERLAPPED overlapped;
					Base::zeroMemory(&overlapped, sizeof(overlapped));
					overlapped.hEvent = Win32::getEventHandle(ev);
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
								thread->wait(10);
								break;
							}
						}
						if (flagConnected) {
							onAccept(hPipe);
							hPipe = INVALID_HANDLE_VALUE;
							break;
						}
					}
					if (hPipe != INVALID_HANDLE_VALUE) {
						CloseHandle(hPipe);
					}
				}
			}

			void onAccept(HANDLE hPipe)
			{
				AsyncFileStreamParam streamParam;
				streamParam.handle = hPipe;
				streamParam.ioLoop = m_ioLoop;
				Ref<AsyncFileStream> stream = AsyncFileStream::create(streamParam);
				if (stream.isNotNull()) {
					startStream(stream.get());
				}
			}

			void prepareRequest(AsyncStream* stream, IPCRequestMessage& msg) override
			{
				HANDLE hPipe = (HANDLE)(stream->getHandle());
				if (hPipe != INVALID_HANDLE_VALUE) {
					ULONG processId = 0;
					if (GetNamedPipeClientProcessId(hPipe, &processId)) {
						msg.remoteProcessId = (sl_uint32)processId;
					}
				}
			}
		};
	}

	Ref<IPCRequest> IPC::sendMessage(const RequestParam& param)
	{
		return Ref<IPCRequest>::cast(PipeRequest::create(param));
	}

	sl_bool IPC::sendMessageSynchronous(const RequestParam& param, ResponseMessage& response)
	{
		sl_int64 tickEnd = GetTickFromTimeout(param.timeout);
		ScopedHandle hPipe = CreatePipe(param.targetName, param.flagGlobal, param.timeout);
		if (hPipe.isNone()) {
			return sl_false;
		}
		AsyncHandleIO io;
		io.handle = hPipe.get();
		if (!(ChunkIO::write(&io, MemoryView(param.message.data, param.message.size), GetTimeoutFromTick(tickEnd)))) {
			return sl_false;
		}
		CurrentThread thread;
		if (thread.isStopping()) {
			return sl_false;
		}
		Nullable<Memory> ret = ChunkIO::read(&io, param.maximumMessageSize, param.messageSegmentSize, GetTimeoutFromTick(tickEnd));
		if (ret.isNull()) {
			return sl_false;
		}
		response.setMemory(ret.value);
		return sl_true;
	}

	Ref<IPCServer> IPC::createServer(const ServerParam& param)
	{
		return Ref<IPCServer>::cast(PipeServer::create(param));
	}

}

#endif