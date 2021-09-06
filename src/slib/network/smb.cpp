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

#include "slib/network/smb.h"

#include "slib/network/event.h"
#include "slib/network/netbios.h"
#include "slib/core/process.h"
#include "slib/core/service_manager.h"

#define SERVER_TAG "SMB SERVER"

namespace slib
{

	namespace priv
	{
		namespace smb
		{


		}
	}

	using namespace priv::smb;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(SmbServerParam)

	SmbServerParam::SmbServerParam()
	{
		port = 445;

		maxThreadsCount = 16;
		flagStopWindowsService = sl_true;

		flagAutoStart = sl_true;
	}


	SLIB_DEFINE_OBJECT(SmbServer, Object)

	SmbServer::SmbServer()
	{
		m_flagReleased = sl_false;
		m_flagRunning = sl_false;
	}

	SmbServer::~SmbServer()
	{
		release();
	}

	Ref<SmbServer> SmbServer::create(const SmbServerParam& param)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		sl_bool flagStopSystemService = sl_false;
		if (param.port == 445) {
			if (Process::isCurrentProcessAdmin()) {
				if (ServiceManager::getState("LanmanServer") == ServiceState::Running) {
					flagStopSystemService = sl_true;
					ServiceManager::stop("LanmanServer");
				}
				if (ServiceManager::getState("srv2") == ServiceState::Running) {
					ServiceManager::stop("srv2");
				}
				if (ServiceManager::getState("srvnet") == ServiceState::Running) {
					ServiceManager::stop("srvnet");
				}
			}
		}
#endif
		Socket socket = Socket::openTcp(SocketAddress(param.bindAddress, param.port));
#ifdef SLIB_PLATFORM_IS_WIN32
		if (flagStopSystemService) {
			ServiceManager::start("LanmanServer");
		}
#endif
		if (socket.isOpened()) {
			Ref<SmbServer> server = new SmbServer;
			if (server.isNotNull()) {
				server->m_socketListen = Move(socket);
				server->m_param = param;
				if (param.flagAutoStart) {
					server->start();
				}
				return server;
			}
		}
		return sl_null;
	}

	sl_bool SmbServer::start()
	{
		ObjectLocker lock(this);

		if (m_flagReleased) {
			return sl_false;
		}
		if (m_flagRunning) {
			return sl_true;
		}

		Ref<ThreadPool> threadPool = ThreadPool::create(0, m_param.maxThreadsCount);
		if (threadPool.isNull()) {
			return sl_false;
		}
		Ref<Thread> threadListen = Thread::start(SLIB_FUNCTION_MEMBER(SmbServer, _onRunListen, this));
		if (threadListen.isNull()) {
			return sl_false;
		}

		m_threadListen = Move(threadListen);
		m_threadPool = Move(threadPool);
		m_flagRunning = sl_true;

		return sl_true;
	}

	void SmbServer::release()
	{
		ObjectLocker lock(this);

		if (m_flagReleased) {
			return;
		}
		m_flagReleased = sl_true;
		m_flagRunning = sl_false;

		Ref<Thread> threadListen = m_threadListen;
		if (threadListen.isNotNull()) {
			threadListen->finishAndWait();
			threadListen.setNull();
		}

		Ref<ThreadPool> threadPool = m_threadPool;
		if (threadPool.isNotNull()) {
			threadPool->release();
			m_threadPool.setNull();
		}

		m_socketListen.close();

	}

	sl_bool SmbServer::isReleased()
	{
		return m_flagReleased;
	}

	sl_bool SmbServer::isRunning()
	{
		return m_flagRunning;
	}

	const SmbServerParam& SmbServer::getParam()
	{
		return m_param;
	}

	void SmbServer::_onRunListen()
	{
		Thread* thread = Thread::getCurrent();
		if (!thread) {
			return;
		}
		Ref<ThreadPool> threadPool = m_threadPool;
		if (threadPool.isNull()) {
			return;
		}
		Socket& socket = m_socketListen;
		socket.setNonBlockingMode();
		socket.listen();
		Ref<SocketEvent> ev = SocketEvent::createRead(socket);
		if (ev.isNull()) {
			return;
		}
		while (thread->isNotStopping()) {
			SocketAddress address;
			MoveT<Socket> client = socket.accept(address);
			if (client.isNotNone()) {
				threadPool->addTask([client, this]() {
					_onRunClient(client);
				});
			} else {
				ev->wait();
			}
		}
	}

	void SmbServer::_onRunClient(const Socket& socket)
	{
		Thread* thread = Thread::getCurrent();
		if (!thread) {
			return;
		}
		socket.setNonBlockingMode();
		Ref<SocketEvent> ev = SocketEvent::createRead(socket);
		if (ev.isNull()) {
			return;
		}
		NetBIOS_SessionMessage msg;
		while (thread->isNotStopping()) {
			sl_int32 n = msg.read(socket);
			if (n == SLIB_IO_ENDED) {
				if (!(_onProcessMessage(socket, ev.get(), msg.message, msg.sizeMessage))) {
					break;
				}
				msg.reset();
			} else if (n < 0) {
				if (n == SLIB_IO_WOULD_BLOCK) {
					ev->wait();
				} else {
					break;
				}
			}
		}
	}

	sl_bool SmbServer::_onProcessMessage(const Socket& socket, SocketEvent* ev, sl_uint8* msg, sl_uint32 size)
	{
		if (size < 4) {
			return sl_false;
		}
		if (msg[1] == 'S' && msg[2] == 'M' && msg[3] == 'B') {
			if (msg[0] == 0xff) {
				if (size >= sizeof(SmbHeader)) {
					return _onProcessSMB(socket, ev, (SmbHeader*)msg, size);
				}
			}
		}
		return sl_false;
	}

	sl_bool SmbServer::_onProcessSMB(const Socket& socket, SocketEvent* ev, SmbHeader* header, sl_uint32 size)
	{
		if (header->getCommand() == SmbCommand::Negotiate) {
			
			return sl_true;
		}
		return sl_false;
	}

}
