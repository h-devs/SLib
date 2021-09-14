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

#ifndef CHECKHEADER_SLIB_NETWORK_SMB
#define CHECKHEADER_SLIB_NETWORK_SMB

#include "socket.h"
#include "smb_constant.h"

#include "../core/thread_pool.h"
#include "../core/hash_map.h"
#include "../core/shared.h"

namespace slib
{

	class SLIB_EXPORT SmbServerParam
	{
	public:
		IPAddress bindAddress;
		sl_uint16 port;

		String targetName;
		String computerName_NetBIOS;
		String domainName_NetBIOS;
		String computerName_DNS;
		String domainName_DNS;

		sl_uint32 maxThreadsCount;
		sl_bool flagStopWindowsService;

		sl_bool flagAutoStart;

	public:
		SmbServerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SmbServerParam)

	};

	class SmbHeader;
	class Smb2Header;

	class SLIB_EXPORT SmbServer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		SmbServer();

		~SmbServer();

	public:
		static Ref<SmbServer> create(const SmbServerParam& param);

	public:
		sl_bool start();

		void release();

		sl_bool isReleased();

		sl_bool isRunning();

		const SmbServerParam& getParam();

	protected:
		void _onRunListen();

		void _onRunClient(const Socket& socket);
		
	public:
		class Connection
		{
		public:
			const Socket* socket;
			SocketEvent* event;
		};

	protected:
		class IOParam : public Connection
		{
		public:
			sl_uint8* data;
			sl_uint32 size;
		};

		class SmbParam : public IOParam
		{
		public:
			SmbHeader* smb;
		};

		class Smb2Param : public IOParam
		{
		public:
			Smb2Header* smb;
		};

		sl_bool _onProcessMessage(IOParam& param);

		sl_bool _onProcessSMB(SmbParam& param);

		sl_bool _onProcessSMB2(Smb2Param& param);

		sl_bool _onProcessNegotiate(Smb2Param& param);

		sl_bool _onProcessSessionSetup(Smb2Param& param);

		sl_bool _onProcessTreeConnect(Smb2Param& param);

		sl_bool _onProcessCreate(Smb2Param& param);

		sl_bool _onProcessClose(Smb2Param& param);

		sl_bool _onProcessRead(Smb2Param& param);

		sl_bool _onProcessWrite(Smb2Param& param);

		sl_bool _onProcessIoctl(Smb2Param& param);

		sl_bool _onProcessGetInfo(Smb2Param& param);

	protected:
		class FileContext
		{
		public:
			sl_uint64 fileId;
			String16 tree;
			String16 fileName;

		public:
			FileContext();
			~FileContext();
		};

		sl_uint32 _registerTree(const String16& path) noexcept;

		String16 _getTree(sl_uint32 treeId) noexcept;

		Shared<FileContext> _createFile(const String16& tree, const String16& fileName) noexcept;

		Shared<FileContext> _getFile(sl_uint8* guid) noexcept;

	protected:
		sl_bool m_flagReleased;
		sl_bool m_flagRunning;

		Socket m_socketListen;
		AtomicRef<Thread> m_threadListen;
		AtomicRef<ThreadPool> m_threadPool;
		
		SmbServerParam m_param;

		sl_uint8 m_serverGuid[16];
		sl_uint8 m_serverChallenge[8];
		sl_uint8 m_hashSalt[32];
		sl_int64 m_lastSessionId;

		CHashMap<sl_uint32, String16> m_trees;
		CHashMap<String16, sl_uint32> m_treeIds;
		sl_uint32 m_lastTreeId;
		Mutex m_mutexTrees;

		CHashMap< sl_uint64, Shared<FileContext> > m_files;
		CHashMap< String16, Shared<FileContext> > m_filesByPath;
		sl_uint64 m_lastFileId;
		Mutex m_mutexFiles;

	};

}

#endif
