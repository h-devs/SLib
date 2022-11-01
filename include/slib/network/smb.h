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
#include "../core/file.h"

namespace slib
{

	class SLIB_EXPORT SmbFileInfo
	{
	public:
		FileAttributes attributes;
		sl_uint64 size;
		Time createdAt;
		Time modifiedAt;

	public:
		SmbFileInfo() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SmbFileInfo)

	};

	class SmbServerFileContext : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		SmbServerFileContext(const String16& path);

		SmbServerFileContext(String16&& path);

		~SmbServerFileContext();

	public:
		const String16& getPath();

		SmbFileInfo& getInfo();

		void setInfo(const SmbFileInfo& info);

	protected:
		String16 m_path;
		SmbFileInfo m_info;

		// internal members
		sl_bool m_flagReturnedList;

		friend class SmbServer;

	};

	class SLIB_EXPORT SmbCreateFileParam
	{
	public:
		StringView16 path;

	public:
		SmbCreateFileParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SmbCreateFileParam)

	};

	class SLIB_EXPORT SmbServerShare : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		SmbServerShare();

		~SmbServerShare();

	public:
		virtual Ref<SmbServerFileContext> createFile(const SmbCreateFileParam& param) = 0;

		virtual sl_int32 readFile(SmbServerFileContext* file, sl_uint64 offset, void* buf, sl_uint32 size) = 0;

		virtual sl_bool getFileInfo(SmbServerFileContext* file, SmbFileInfo& _out) = 0;

		virtual HashMap<String16, SmbFileInfo> getFiles(SmbServerFileContext* file) = 0;

		virtual sl_uint64 getFileUniqueId(const String16& path);

	public:
		String getComment();

		void setComment(const String& comment);

	protected:
		String m_comment;
		CHashMap< String16, sl_uint64, HashIgnoreCase<String16>, CompareIgnoreCase<String16> > m_fileUniqueIds;
		sl_uint64 m_lastFileUniqueId;

	};

	class SLIB_EXPORT SmbServerFileShare : public SmbServerShare
	{
	public:
		SmbServerFileShare(const String& rootPath);

		SmbServerFileShare(const String& rootPath, const String& comment);

		~SmbServerFileShare();

	public:
		class FileContext : public SmbServerFileContext
		{
		public:
			File file;
			String absolutePath;

		public:
			FileContext(String16&& path, String&& absolutePath, File&& file);

			FileContext(String16&& path, String&& absolutePath);

			~FileContext();

		};

	public:
		Ref<SmbServerFileContext> createFile(const SmbCreateFileParam& param) override;

		sl_int32 readFile(SmbServerFileContext* context, sl_uint64 offset, void* buf, sl_uint32 size) override;

		sl_bool getFileInfo(SmbServerFileContext* context, SmbFileInfo& _out) override;

		HashMap<String16, SmbFileInfo> getFiles(SmbServerFileContext* context) override;

	public:
		String getAbsolutePath(const StringView16& path) noexcept;

	protected:
		String m_rootPath;

	};

	class SLIB_EXPORT SmbServerParam
	{
	public:
		IPAddress bindAddress;
		sl_uint16 port;

		String targetName;
		String domainName;
		String targetDescription;
		String computerName_NetBIOS;
		String domainName_NetBIOS;
		String computerName_DNS;
		String domainName_DNS;

		HashMap< String16, Ref<SmbServerShare>, HashIgnoreCase<String16>, CompareIgnoreCase<String16> > shares;

		sl_uint32 maxThreadCount;
		sl_bool flagStopWindowsService;

		sl_bool flagAutoStart;

	public:
		SmbServerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SmbServerParam)

	public:
		void initNames();

		void addShare(const String& name, const Ref<SmbServerShare>& share);

		void addFileShare(const String& name, const String& rootPath);

		void addFileShare(const String& name, const String& rootPath, const String& comment);

	};

	class SmbHeader;
	class Smb2Header;
	class SmbServerSession;

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

		class IOParam : public Connection
		{
		public:
			sl_uint8* data;
			sl_uint32 size;
			SmbServerSession* session;
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
			void* chain;
			sl_uint64 lastCreatedFileId;
		};

	protected:
		sl_bool _onProcessMessage(IOParam& param);

		sl_bool _onProcessSMB(SmbParam& param);

		sl_bool _onProcessSMB2(Smb2Param& param);

		sl_bool _onProcessNegotiate(Smb2Param& param);

		sl_bool _onProcessSessionSetup(Smb2Param& param);

		sl_bool _onProcessTreeConnect(Smb2Param& param);

		sl_bool _onProcessTreeDisconnect(Smb2Param& param);

		sl_bool _onProcessCreate(Smb2Param& param);

		sl_bool _onProcessClose(Smb2Param& param);

		sl_bool _onProcessRead(Smb2Param& param);

		sl_bool _onProcessWrite(Smb2Param& param);

		sl_bool _onProcessIoctl(Smb2Param& param);

		sl_bool _onProcessFind(Smb2Param& param);

		sl_bool _onProcessNotify(Smb2Param& param);

		sl_bool _onProcessGetInfo(Smb2Param& param);

		Memory _processRpc(sl_uint64 fileId, sl_uint8* packet, sl_uint32 size);

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
		volatile sl_int64 m_lastSessionId;

		volatile sl_int32 m_lastTreeId;
		volatile sl_int64 m_lastFileId;
		Time m_timeStarted;

		friend class SmbServerSession;

	};

	class SmbServerSession
	{
	public:
		SmbServer * server;
		CHashMap< sl_uint32, Ref<SmbServerShare> > trees;
		CHashMap< String16, sl_uint32, HashIgnoreCase<String16>, CompareIgnoreCase<String16> > treeIds;
		CHashMap< sl_uint64, Ref<SmbServerFileContext> > files;

	public:
		SmbServerSession();

		~SmbServerSession();

	public:
		sl_uint32 connectTree(const String16& name) noexcept;

		SmbServerShare* getTree(sl_uint32 treeId) noexcept;

		sl_uint64 registerFile(SmbServerFileContext* context) noexcept;

		void unregisterFile(sl_uint64 fileId) noexcept;

		Ref<SmbServerFileContext> getFile(sl_uint64 fileId) noexcept;

	};

}

#endif
