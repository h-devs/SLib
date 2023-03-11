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
#include "../io/file.h"

namespace slib
{

	class SmbHeader;
	class Smb2Header;

	class SLIB_EXPORT SmbServer : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		class FileInfo
		{
		public:
			FileAttributes attributes;
			sl_uint64 size;
			Time createdAt;
			Time modifiedAt;

		public:
			FileInfo() noexcept;
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileInfo)
		};

		class FileContext : public CRef
		{
		public:
			FileContext(const String16& path);
			FileContext(String16&& path);
			~FileContext();

		public:
			const String16& getPath();
			FileInfo& getInfo();
			void setInfo(const FileInfo& info);

		protected:
			String16 m_path;
			FileInfo m_info;
			// internal members
			sl_bool m_flagReturnedList;

			friend class SmbServer;
		};

		class CreateFileParam
		{
		public:
			StringView16 path;

		public:
			CreateFileParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreateFileParam)
		};

		class Share : public Object
		{
		public:
			Share();
			~Share();

		public:
			typedef SmbServer::CreateFileParam CreateFileParam;
			virtual Ref<FileContext> createFile(const CreateFileParam& param) = 0;
			virtual sl_int32 readFile(FileContext* file, sl_uint64 offset, void* buf, sl_uint32 size) = 0;
			virtual sl_bool getFileInfo(FileContext* file, FileInfo& _out) = 0;
			virtual HashMap<String16, FileInfo> getFiles(FileContext* file) = 0;
			virtual sl_uint64 getFileUniqueId(const String16& path);

		public:
			String getComment();
			void setComment(const String& comment);

		protected:
			String m_comment;
			CHashMap< String16, sl_uint64, Hash_IgnoreCase<String16>, Compare_IgnoreCase<String16> > m_fileUniqueIds;
			sl_uint64 m_lastFileUniqueId;
		};

		class FileShare : public Share
		{
		public:
			FileShare(const String& rootPath);
			FileShare(const String& rootPath, const String& comment);
			~FileShare();

		public:
			class Context : public FileContext
			{
			public:
				File file;
				String absolutePath;

			public:
				Context(String16&& path, String&& absolutePath, File&& file);
				Context(String16&& path, String&& absolutePath);
				~Context();
			};

		public:
			Ref<FileContext> createFile(const CreateFileParam& param) override;
			sl_int32 readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size) override;
			sl_bool getFileInfo(FileContext* context, FileInfo& _out) override;
			HashMap<String16, FileInfo> getFiles(FileContext* context) override;

		public:
			String getAbsolutePath(const StringView16& path) noexcept;

		protected:
			String m_rootPath;
		};

		class Param
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

			HashMap< String16, Ref<Share>, Hash_IgnoreCase<String16>, Compare_IgnoreCase<String16> > shares;

			sl_uint32 maxThreadCount;
			sl_bool flagStopWindowsService;

			sl_bool flagAutoStart;

		public:
			Param();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Param)

		public:
			void initNames();
			void addShare(const String& name, const Ref<Share>& share);
			void addFileShare(const String& name, const String& rootPath);
			void addFileShare(const String& name, const String& rootPath, const String& comment);
		};

	protected:
		SmbServer();

		~SmbServer();

	public:
		static Ref<SmbServer> create(const Param& param);

	public:
		sl_bool start();

		void release();

		sl_bool isReleased();

		sl_bool isRunning();

		const Param& getParam();

	protected:
		void _onRunListen();

		void _onRunClient(const Socket& socket);

	public:
		class Session
		{
		public:
			SmbServer* server;
			CHashMap< sl_uint32, Ref<Share> > trees;
			CHashMap< String16, sl_uint32, Hash_IgnoreCase<String16>, Compare_IgnoreCase<String16> > treeIds;
			CHashMap< sl_uint64, Ref<FileContext> > files;

		public:
			Session();
			~Session();

		public:
			sl_uint32 connectTree(const String16& name) noexcept;
			Share* getTree(sl_uint32 treeId) noexcept;
			sl_uint64 registerFile(FileContext* context) noexcept;
			void unregisterFile(sl_uint64 fileId) noexcept;
			Ref<FileContext> getFile(sl_uint64 fileId) noexcept;
		};

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
			Session* session;
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

		Param m_param;

		sl_uint8 m_serverGuid[16];
		sl_uint8 m_serverChallenge[8];
		sl_uint8 m_hashSalt[32];
		volatile sl_int64 m_lastSessionId;

		volatile sl_int32 m_lastTreeId;
		volatile sl_int64 m_lastFileId;
		Time m_timeStarted;

	};

	typedef SmbServer::Param SmbServerParam;

}

#endif
