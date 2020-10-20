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

#include "slib/storage/dokany.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#define _EXPORTING
#include "dokany/dokan.h"

#include "slib/core/service_manager.h"
#include "slib/core/dynamic_library.h"
#include "slib/core/platform_windows.h"

#define DOKAN_DRIVER_SERVICE L"Dokan1"

namespace slib
{

	namespace priv
	{
		namespace dokany
		{

			void* g_libDll;

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanMain, int, DOKANAPI,
				PDOKAN_OPTIONS DokanOptions,
				PDOKAN_OPERATIONS DokanOperations)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanUnmount, BOOL, DOKANAPI,
				WCHAR DriveLetter)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanServiceInstall, BOOL, DOKANAPI,
				LPCWSTR ServiceName,
				DWORD ServiceType,
				LPCWSTR ServiceFullPath)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanServiceDelete, BOOL, DOKANAPI,
				LPCWSTR ServiceName)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanVersion, ULONG, DOKANAPI)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanDriverVersion, ULONG, DOKANAPI)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanResetTimeout, BOOL, DOKANAPI,
				ULONG Timeout,
				PDOKAN_FILE_INFO FileInfo)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanOpenRequestorToken, HANDLE, DOKANAPI,
				PDOKAN_FILE_INFO DokanFileInfo)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanRemoveMountPoint, BOOL, DOKANAPI,
				LPCWSTR MountPoint)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanMapKernelToUserCreateFileFlags, void, DOKANAPI,
				ACCESS_MASK DesiredAccess,
				ULONG FileAttributes,
				ULONG CreateOptions,
				ULONG CreateDisposition,
				ACCESS_MASK* outDesiredAccess,
				DWORD *outFileAttributesAndFlags,
				DWORD *outCreationDisposition)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanGetMountPointList, BOOL, DOKANAPI,
				PDOKAN_CONTROL list,
				ULONG length,
				BOOL uncOnly,
				PULONG nbRead)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanNtStatusFromWin32, NTSTATUS, DOKANAPI,
				DWORD Error)

		}
	}

	using namespace priv::dokany;

	sl_bool Dokany::initialize(const StringParam& pathDll)
	{
		if (g_libDll) {
			return sl_true;
		}
		g_libDll = DynamicLibrary::loadLibrary(pathDll);
		return g_libDll != sl_null;
	}

	sl_bool Dokany::initialize()
	{
		return initialize("dokan1.dll");
	}

	ServiceState Dokany::getDriverState()
	{
		return ServiceManager::getState(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::startDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Running) {
			return sl_true;
		}
		return ServiceManager::start(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::stopDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Stopped) {
			return sl_true;
		}
		return ServiceManager::stop(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::registerDriver(const StringParam& pathSys)
	{
		ServiceCreateParam param;
		param.type = ServiceType::FileSystem;
		param.name = DOKAN_DRIVER_SERVICE;
		if (pathSys.isNotEmpty()) {
			param.path = pathSys;
		} else {
			param.path = Windows::getSystemDirectory() + "\\drivers\\dokan1.sys";
		}
		return ServiceManager::create(param);
	}

	sl_bool Dokany::registerDriver()
	{
		return registerDriver(sl_null);
	}

	sl_bool Dokany::registerAndStartDriver(const StringParam& pathSys)
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::Running) {
			return sl_true;
		}
		if (state != ServiceState::None) {
			return startDriver();
		}
		if (registerDriver(pathSys)) {
			return startDriver();
		}
		return sl_false;
	}

	sl_bool Dokany::registerAndStartDriver()
	{
		return registerAndStartDriver(sl_null);
	}

	sl_bool Dokany::unregisterDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_true;
		}
		if (state != ServiceState::Stopped) {
			if (!(stopDriver())) {
				return sl_false;
			}
		}
		return ServiceManager::remove(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::unmount(const StringParam& _mountPoint)
	{
		auto func = getApi_DokanRemoveMountPoint();
		if (func) {
			StringCstr16 mountPoint(_mountPoint);
			return func((LPCWSTR)(mountPoint.getData()));
		}
		return sl_false;
	}

}

#include "slib/storage/dokan_host.h"

#ifndef SLIB_DOKAN_IS_DOKANY
# define DOKAN_RETERROR(error)	(-(SLIB_DOKAN_RET)(error))
# define FILESYSTEM_EXCEPTION_GUARD(...)\
    try { __VA_ARGS__ } catch (FileSystemError error) { return DOKAN_RETERROR(error); }
# define FILESYSTEM_EXCEPTION_GUARD_VOID(...)\
    try { __VA_ARGS__ } catch (FileSystemError error) { return DOKAN_RETERROR(error); } return 0;
#else
# define DOKAN_RETERROR(error)	getApi_DokanNtStatusFromWin32()((DWORD)error)
# define FILESYSTEM_EXCEPTION_GUARD(...)\
    try { __VA_ARGS__ } \
	catch (FileSystemError error) { \
		if (error == FileSystemError::NotImplemented) \
			return STATUS_NOT_IMPLEMENTED; \
		return DOKAN_RETERROR(error); \
	} \
	catch (NTSTATUS status) { \
		return status; \
	} \
	catch (...) { \
		return DOKAN_RETERROR(FileSystemError::GeneralError); \
	}
# define FILESYSTEM_EXCEPTION_GUARD_VOID(...)\
    try { __VA_ARGS__ } catch (...) { return; }
#endif

#define StringToWChar(STR, OUT)			OUT[STR.getUtf16((sl_char16 *)(OUT), sizeof(OUT))] = L'\0'
#define WCharToString(WCSTR)			String::fromUtf16((const sl_char16 *)(WCSTR))
#define FileTimeToTime(ft, t)			{ \
	SYSTEMTIME st = { 0 }; \
	FileTimeToSystemTime(&(ft), &st); \
	t.setUTC(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds); \
}
#define TimeToFileTime(t, ft)			{ \
	TimeComponents time; \
	t.getUTC(time); \
	SYSTEMTIME st = { (WORD)time.year, (WORD)time.month, (WORD)time.dayOfWeek, (WORD)time.day, \
		(WORD)time.hour, (WORD)time.minute, (WORD)time.second, (WORD)time.milliseconds }; \
	SystemTimeToFileTime(&st, &(ft)); \
}

namespace slib
{
	BOOL DokanHost::g_hasSeSecurityPrivilege = FALSE;

	DokanHost::DokanHost(Ref<FileSystemBase> base, sl_uint32 options) :
		FileSystemHost(base),
#ifdef SLIB_DOKAN_IS_DOKANY
		m_uncName(L""),
#endif
		m_mountPoint(L""),
		m_flagStarted(FALSE)
	{
		ZeroMemory(&m_dokanOptions, sizeof(DOKAN_OPTIONS));
		m_dokanOptions.Version = DOKAN_VERSION;
		m_dokanOptions.ThreadCount = 0; // use default
#ifdef SLIB_DOKAN_IS_DOKANY
		m_dokanOptions.UNCName = m_uncName;
#else
		m_dokanOptions.Options |= DOKAN_OPTION_KEEP_ALIVE;	// 0.6.0, use auto unmount
#endif
		m_dokanOptions.Options |= options;
		m_dokanOptions.GlobalContext = (ULONG64)(PVOID)m_base;
		m_dokanOptions.MountPoint = m_mountPoint;
	}

	DokanHost::~DokanHost()
	{
		if (m_flagStarted)
			fsStop();
	}

	void DokanHost::setVersion(sl_uint16 version)
	{
		m_dokanOptions.Version = version;
	}

	void DokanHost::setThreadCount(sl_uint16 threadCount)
	{
		m_dokanOptions.ThreadCount = threadCount;
	}

	void DokanHost::setMountPoint(const StringParam& mountPoint)
	{
		StringToWChar(mountPoint.toString(), m_mountPoint);
	}

#ifdef SLIB_DOKAN_IS_DOKANY
	void DokanHost::setUNCName(const StringParam& uncName)
	{
		StringToWChar(uncName.toString(), m_uncName);
	}

	void DokanHost::setTimeout(sl_uint32 timeout)
	{
		m_dokanOptions.Timeout = timeout;
	}
#endif

	void DokanHost::setDebugMode(sl_bool flagUseStdErr)
	{
		m_dokanOptions.Options |= DOKAN_OPTION_DEBUG;
		if (flagUseStdErr)
			m_dokanOptions.Options |= DOKAN_OPTION_STDERR;
		else
			m_dokanOptions.Options &= ~DOKAN_OPTION_STDERR;
	}

	int DokanHost::fsRun() {
#ifdef SLIB_DOKAN_IS_DOKANY
		if (wcscmp(m_uncName, L"") != 0 &&
			!(m_dokanOptions.Options & DOKAN_OPTION_NETWORK)) {
			fwprintf(
				stderr,
				L"  Warning: UNC provider name should be set on network drive only.\n");
		}

		if (m_dokanOptions.Options & DOKAN_OPTION_NETWORK &&
			m_dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) {
			fwprintf(stderr, L"Mount manager cannot be used on network drive.\n");
			return EXIT_FAILURE;
		}

		if (!(m_dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) &&
			wcscmp(m_mountPoint, L"") == 0) {
			fwprintf(stderr, L"Mount Point required.\n");
			return EXIT_FAILURE;
		}

		if ((m_dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) &&
			(m_dokanOptions.Options & DOKAN_OPTION_CURRENT_SESSION)) {
			fwprintf(stderr,
				L"Mount Manager always mount the drive for all user sessions.\n");
			return EXIT_FAILURE;
		}

		try {
			VolumeInfo volumeInfo = m_base->fsGetVolumeInfo();
			if (volumeInfo.sectorSize) {
				m_dokanOptions.SectorSize = volumeInfo.sectorSize;
				if (volumeInfo.sectorsPerAllocationUnit)
					m_dokanOptions.AllocationUnitSize = volumeInfo.sectorSize * volumeInfo.sectorsPerAllocationUnit;
			}
			if (volumeInfo.fileSystemFlags & FILE_READ_ONLY_VOLUME)
				m_dokanOptions.Options |= DOKAN_OPTION_WRITE_PROTECT;
		} catch (...) {}

		try {
			m_base->fsFindStreams(new FileContext("\\", sl_true));
			m_dokanOptions.Options |= DOKAN_OPTION_ALT_STREAM;
		} catch (FileSystemError error) {
			if (error != FileSystemError::NotImplemented)
				m_dokanOptions.Options |= DOKAN_OPTION_ALT_STREAM;
		} catch (...) {}
#endif // SLIB_DOKAN_IS_DOKANY

		auto func = getApi_DokanMain();
		if (!func) {
			fwprintf(stderr, L"Cannot get DokanMain function address.\n");
			return EXIT_FAILURE;
		}

		m_flagStarted = TRUE;
		int status = getApi_DokanMain()(&m_dokanOptions, DokanHost::Interface());
		m_flagStarted = FALSE;

		switch (status) {
		case DOKAN_SUCCESS:
			fprintf(stderr, "Success\n");
			break;
		case DOKAN_ERROR:
			fprintf(stderr, "Error\n");
			break;
		case DOKAN_DRIVE_LETTER_ERROR:
			fprintf(stderr, "Bad Drive letter\n");
			break;
		case DOKAN_DRIVER_INSTALL_ERROR:
			fprintf(stderr, "Can't install driver\n");
			break;
		case DOKAN_START_ERROR:
			fprintf(stderr, "Driver something wrong\n");
			break;
		case DOKAN_MOUNT_ERROR:
			fprintf(stderr, "Can't assign a drive letter\n");
			break;
		case DOKAN_MOUNT_POINT_ERROR:
			fprintf(stderr, "Mount point error\n");
			break;
#ifdef SLIB_DOKAN_IS_DOKANY
		case DOKAN_VERSION_ERROR:
			fprintf(stderr, "Version error\n");
			break;
#endif // SLIB_DOKAN_IS_DOKANY
		default:
			fprintf(stderr, "Unknown error: %d\n", status);
			break;
		}

		return status;
	}

	int DokanHost::fsStop() 
	{
		if (m_flagStarted)
			return ! Dokany::unmount(m_mountPoint);
		return -1;
	}

	int DokanHost::isRunning() 
	{
		return m_flagStarted;
	}

	BOOL DokanHost::addSeSecurityNamePrivilege() 
	{
		DokanHost::g_hasSeSecurityPrivilege = FALSE;

		HANDLE token = 0;
		DWORD err;
		LUID luid;
		if (!LookupPrivilegeValue(0, SE_SECURITY_NAME, &luid)) {
			err = GetLastError();
			if (err != ERROR_SUCCESS) {
				return FALSE;
			}
		}

		LUID_AND_ATTRIBUTES attr;
		attr.Attributes = SE_PRIVILEGE_ENABLED;
		attr.Luid = luid;

		TOKEN_PRIVILEGES priv;
		priv.PrivilegeCount = 1;
		priv.Privileges[0] = attr;

		if (!OpenProcessToken(GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
			err = GetLastError();
			if (err != ERROR_SUCCESS) {
				return FALSE;
			}
		}

		TOKEN_PRIVILEGES oldPriv;
		DWORD retSize;
		AdjustTokenPrivileges(token, FALSE, &priv, sizeof(TOKEN_PRIVILEGES), &oldPriv,
			&retSize);
		err = GetLastError();
		if (err != ERROR_SUCCESS) {
			CloseHandle(token);
			return FALSE;
		}

		BOOL privAlreadyPresent = FALSE;
		for (unsigned int i = 0; i < oldPriv.PrivilegeCount; i++) {
			if (oldPriv.Privileges[i].Luid.HighPart == luid.HighPart &&
				oldPriv.Privileges[i].Luid.LowPart == luid.LowPart) {
				privAlreadyPresent = TRUE;
				break;
			}
		}
		if (token)
			CloseHandle(token);

		DokanHost::g_hasSeSecurityPrivilege = TRUE;
		return TRUE;
	}

	inline Ref<FileContext> GetFileContext(PDOKAN_FILE_INFO DokanFileInfo, LPCWSTR FileName)
	{
		if (DokanFileInfo->Context == 0)
			return new FileContext(WCharToString(FileName), DokanFileInfo->IsDirectory);
		else
			return (FileContext*)(DokanFileInfo->Context);
	}

#ifdef SLIB_DOKAN_IS_DOKANY
	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::ZwCreateFile(
			LPCWSTR					FileName,
			PDOKAN_IO_SECURITY_CONTEXT SecurityContext,
			ACCESS_MASK				DesiredAccess,
			ULONG					FileAttributes,
			ULONG					ShareAccess,
			ULONG					CreateDisposition,
			ULONG					CreateOptions,
			PDOKAN_FILE_INFO		DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		DWORD creationDisposition;
		DWORD fileAttributesAndFlags;
		ACCESS_MASK genericDesiredAccess;

		getApi_DokanMapKernelToUserCreateFileFlags()(
			DesiredAccess, FileAttributes, CreateOptions, CreateDisposition,
			&genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);

		// When filePath is a directory, needs to change the flag so that the file can
		// be opened.
		DWORD fileAttr = 0;
		try {
			fileAttr = base->fsGetFileInfo(GetFileContext(DokanFileInfo, FileName)).fileAttributes;
		} catch (...) {}
		if (!fileAttr) fileAttr = INVALID_FILE_ATTRIBUTES;
		if (fileAttr != INVALID_FILE_ATTRIBUTES
			&& fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
			if (!(CreateOptions & FILE_NON_DIRECTORY_FILE)) {
				DokanFileInfo->IsDirectory = TRUE;
				// Needed by FindFirstFile to list files in it
				// TODO: use ReOpenFile in MirrorFindFiles to set share read temporary
				ShareAccess |= FILE_SHARE_READ;
			} else { // FILE_NON_DIRECTORY_FILE - Cannot open a dir as a file
				return STATUS_FILE_IS_A_DIRECTORY;
			}
		}

		if (DokanFileInfo->IsDirectory) {
			// Check first if we're trying to open a file as a directory.
			if (fileAttr != INVALID_FILE_ATTRIBUTES &&
				!(fileAttr & FILE_ATTRIBUTE_DIRECTORY) &&
				(CreateOptions & FILE_DIRECTORY_FILE)) {
				return STATUS_NOT_A_DIRECTORY;
			}
			fileAttributesAndFlags |= FILE_ATTRIBUTE_DIRECTORY;
		} else {
			// Cannot overwrite a hidden or system file if flag not set
			if (fileAttr != INVALID_FILE_ATTRIBUTES &&
				((!(fileAttributesAndFlags & FILE_ATTRIBUTE_HIDDEN) &&
				(fileAttr & FILE_ATTRIBUTE_HIDDEN)) ||
					(!(fileAttributesAndFlags & FILE_ATTRIBUTE_SYSTEM) &&
					(fileAttr & FILE_ATTRIBUTE_SYSTEM))) &&
						(creationDisposition == TRUNCATE_EXISTING ||
							creationDisposition == CREATE_ALWAYS))
				return STATUS_ACCESS_DENIED;

			// Cannot delete a read only file
			if ((fileAttr != INVALID_FILE_ATTRIBUTES &&
				(fileAttr & FILE_ATTRIBUTE_READONLY) ||
				(fileAttributesAndFlags & FILE_ATTRIBUTE_READONLY)) &&
				(fileAttributesAndFlags & FILE_FLAG_DELETE_ON_CLOSE))
				return STATUS_CANNOT_DELETE;

			// Truncate should always be used with write access
			if (creationDisposition == TRUNCATE_EXISTING)
				genericDesiredAccess |= GENERIC_WRITE;
		}

		// Pass security context to CreateFile()
		DokanFileInfo->Context = (ULONG64)(PVOID)SecurityContext;
		DWORD ret = DokanHost::CreateFile(FileName,
			genericDesiredAccess,
			ShareAccess,
			creationDisposition,
			fileAttributesAndFlags,
			DokanFileInfo);

		if (ret == STATUS_SUCCESS &&
			!DokanFileInfo->IsDirectory) {
			// Need to update FileAttributes with previous when Overwrite file
			if (fileAttr != INVALID_FILE_ATTRIBUTES &&
				creationDisposition == TRUNCATE_EXISTING) {
				DokanHost::SetFileAttributes(FileName,
					fileAttributesAndFlags | fileAttr,
					DokanFileInfo);
			}
		}

		return ret;
	}
#endif // SLIB_DOKAN_IS_DOKANY

	SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
		DokanHost::CreateFile(
			LPCWSTR					FileName,
			DWORD					AccessMode,
			DWORD					ShareMode,
			DWORD					CreationDisposition,
			DWORD					FlagsAndAttributes,
			PDOKAN_FILE_INFO		DokanFileInfo)
	{
		PSECURITY_DESCRIPTOR securityDescriptor = 0;
#ifdef SLIB_DOKAN_IS_DOKANY
		if (DokanFileInfo->Context) {
			PDOKAN_IO_SECURITY_CONTEXT SecurityContext = (PDOKAN_IO_SECURITY_CONTEXT)DokanFileInfo->Context;
			securityDescriptor = SecurityContext->AccessState.SecurityDescriptor;
			DokanFileInfo->Context = 0;
		}
#endif
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		int ret = 0;
		FILESYSTEM_EXCEPTION_GUARD(
			FileCreationParams params;
			params.accessMode = AccessMode;
			params.shareMode = ShareMode;
			params.flagsAndAttributes = FlagsAndAttributes;
			params.securityDescriptor = securityDescriptor;

			switch (CreationDisposition) {
			case CREATE_NEW:		// create only
				base->fsCreate(context, params);
				break;
			case CREATE_ALWAYS:		// create or overwrite existing			// modeWrite
				params.createAlways = sl_true;
				params.openTruncate = sl_true;
				try {
					base->fsCreate(context, params);
					if (params.createAlways == sl_false &&
						context->status == FileSystemError::FileExist) 	// fsCreate() supports createAlways, and file exists
						ret = ERROR_ALREADY_EXISTS;
					if (params.openTruncate)	// fsCreate() *may* not supports openTruncate, so truncate manually
						base->fsSetFileInfo(context, FileInfo(), FileInfoFlags::SizeInfo);
				} catch (FileSystemError error) {
					if (error == FileSystemError::FileExist) {	// fsCreate() not supports createAlways, and file exists
						params.createAlways = sl_false;
						params.openTruncate = sl_true;
						base->fsOpen(context, params);
						if (params.openTruncate)	// fsOpen() *may* not support openTruncate, so truncate manually
							base->fsSetFileInfo(context, FileInfo(), FileInfoFlags::SizeInfo);
						ret = ERROR_ALREADY_EXISTS;
					} else throw error;
				}
				break;
			case OPEN_ALWAYS:		// open or create
				params.createAlways = sl_true;
				try {
					base->fsOpen(context, params);
					if (params.createAlways) 	// fsOpen() not supports createAlways, and file exists
						ret = ERROR_ALREADY_EXISTS;
					else if (context->status == FileSystemError::FileExist) 	// fsOpen() supports createAlways, and file exists
						ret = ERROR_ALREADY_EXISTS;
				} catch (FileSystemError error) {
					if (error == FileSystemError::NotFound) {	// fsOpen() not supports createAlways, and file NOT exists, so create manually
						params.createAlways = sl_false;
						base->fsCreate(context, params);
					} else throw error;
				}
				break;
			case OPEN_EXISTING:		// open only							// modeRead
				base->fsOpen(context, params);
				break;
			case TRUNCATE_EXISTING:	// open only, and truncate
				params.openTruncate = sl_true;
				base->fsOpen(context, params);
				if (params.openTruncate)	// fsOpen() *may* not support openTruncate, so truncate manually
					base->fsSetFileInfo(context, FileInfo(), FileInfoFlags::SizeInfo);
				break;
			}
		)

		DokanFileInfo->IsDirectory = context->isDirectory;
		DokanFileInfo->Context = (ULONG64)(context.ptr);
		context->increaseReference();
		base->increaseHandleCount(context->path);
#ifndef SLIB_DOKAN_IS_DOKANY
		ret = -ret;		// if ret has value, must return positive value
#endif
		return DOKAN_RETERROR(ret);
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
		DokanHost::CreateDirectory(
			LPCWSTR					FileName,
			PDOKAN_FILE_INFO		DokanFileInfo)
	{
		return DokanHost::CreateFile(FileName,
			0,
			0,
			CREATE_NEW,
			FILE_ATTRIBUTE_DIRECTORY,
			DokanFileInfo);
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
		DokanHost::OpenDirectory(
			LPCWSTR					FileName,
			PDOKAN_FILE_INFO		DokanFileInfo)
	{
		return DokanHost::CreateFile(FileName,
			0,
			0,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_DIRECTORY,
			DokanFileInfo);
	}

	SLIB_DOKAN_RET_VOID DOKAN_CALLBACK
		DokanHost::Cleanup(
			LPCWSTR					FileName,
			PDOKAN_FILE_INFO		DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD_VOID(
			if (DokanFileInfo->DeleteOnClose) {
				base->fsClose(context);
				// Should already be deleted by CloseHandle
				// if open with FILE_FLAG_DELETE_ON_CLOSE
				base->fsDelete(context, FALSE);
			}
		) // DO NOT ADD CODE AFTER FILESYSTEM_EXCEPTION_GUARD_VOID
	}

	SLIB_DOKAN_RET_VOID DOKAN_CALLBACK
		DokanHost::CloseFile(
			LPCWSTR					FileName,
			PDOKAN_FILE_INFO		DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD_VOID(
			base->fsClose(context);
			base->decreaseHandleCount(context->path);
			context->decreaseReference();
			DokanFileInfo->Context = 0;
		) // DO NOT ADD CODE AFTER FILESYSTEM_EXCEPTION_GUARD_VOID
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::ReadFile(
			LPCWSTR				FileName,
			LPVOID				Buffer,
			DWORD				BufferLength,
			LPDWORD				ReadLength,
			LONGLONG			Offset,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			MemoryData buf;
			*ReadLength = (DWORD)base->fsRead(context,
				Memory::createStatic(Buffer, BufferLength), Offset);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::WriteFile(
			LPCWSTR		FileName,
			LPCVOID		Buffer,
			DWORD		NumberOfBytesToWrite,
			LPDWORD		NumberOfBytesWritten,
			LONGLONG			Offset,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			// Paging IO cannot write after allocate file size.
			if (DokanFileInfo->PagingIo) {
				UINT64 fileSize = base->fsGetFileInfo(context).size;
				if ((UINT64)Offset >= fileSize) {
					*NumberOfBytesWritten = 0;
					return 0;
				}
				if (((UINT64)Offset + NumberOfBytesToWrite) > fileSize) {
					NumberOfBytesToWrite = (DWORD)((fileSize - Offset) & 0xFFFFFFFFUL);
				}
			}

			*NumberOfBytesWritten = (DWORD)base->fsWrite(context,
				Memory::createStatic(Buffer, NumberOfBytesToWrite), Offset,
				DokanFileInfo->WriteToEndOfFile);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::FlushFileBuffers(
			LPCWSTR		FileName,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			base->fsFlush(context);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::GetFileInformation(
			LPCWSTR							FileName,
			LPBY_HANDLE_FILE_INFORMATION	HandleFileInformation,
			PDOKAN_FILE_INFO				DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			FileInfo fileInfo = base->fsGetFileInfo(context);
			HandleFileInformation->dwFileAttributes = fileInfo.fileAttributes;
			HandleFileInformation->nFileSizeHigh = (UINT32)(fileInfo.size >> 32);
			HandleFileInformation->nFileSizeLow = (UINT32)(fileInfo.size & 0xFFFFFFFF);
			TimeToFileTime(fileInfo.createdAt, HandleFileInformation->ftCreationTime);
			TimeToFileTime(fileInfo.lastAccessedAt, HandleFileInformation->ftLastAccessTime);
			TimeToFileTime(fileInfo.modifiedAt, HandleFileInformation->ftLastWriteTime);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::FindFiles(
			LPCWSTR				PathName,
			PFillFindData		FillFindData,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		return FindFilesWithPattern(PathName, L"*", FillFindData, DokanFileInfo);
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::FindFilesWithPattern(
			LPCWSTR				PathName,
			LPCWSTR				SearchPattern,
			PFillFindData		FillFindData,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, PathName);
		FILESYSTEM_EXCEPTION_GUARD(
			WIN32_FIND_DATA findData;
			auto files = base->fsFindFiles(context, WCharToString(SearchPattern));

			for (auto& file : files) {
				FileInfo fileInfo = file.value;
				memset(&findData, 0, sizeof(findData));
				findData.dwFileAttributes = fileInfo.fileAttributes;
				findData.nFileSizeHigh = (UINT32)(fileInfo.size >> 32);
				findData.nFileSizeLow = (UINT32)(fileInfo.size & 0xFFFFFFFF);
				TimeToFileTime(fileInfo.createdAt, findData.ftCreationTime);
				TimeToFileTime(fileInfo.lastAccessedAt, findData.ftLastAccessTime);
				TimeToFileTime(fileInfo.modifiedAt, findData.ftLastWriteTime);
				StringToWChar(file.key, findData.cFileName);
				FillFindData(&findData, DokanFileInfo);
			}
		)
		return 0;
	}
#ifdef SLIB_DOKAN_IS_DOKANY
	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::FindStreams(
			LPCWSTR				FileName,
			PFillFindStreamData	FillFindStreamData,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			WIN32_FIND_STREAM_DATA findData;
			auto streams = base->fsFindStreams(context);

			for (auto& stream : streams) {
				StreamInfo streamInfo = stream.value;
				memset(&findData, 0, sizeof(findData));
				findData.StreamSize.QuadPart = streamInfo.size;
				StringToWChar(stream.key, findData.cStreamName);
				FillFindStreamData(&findData, DokanFileInfo);
			}
		)
		return 0;
	}
#endif // SLIB_DOKAN_IS_DOKANY
	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::DeleteFile(
			LPCWSTR				FileName,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			base->fsDelete(context, TRUE);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::DeleteDirectory(
			LPCWSTR				FileName,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		// Dokan notify that the file is requested not to be deleted.
		if (!DokanFileInfo->DeleteOnClose)
			return 0;
		return DeleteFile(FileName, DokanFileInfo);
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::MoveFile(
			LPCWSTR				FileName, // existing file name
			LPCWSTR				NewFileName,
			BOOL				ReplaceIfExisting,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			base->fsRename(context, WCharToString(NewFileName), ReplaceIfExisting);
			context->path = WCharToString(NewFileName);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::SetEndOfFile(
			LPCWSTR				FileName,
			LONGLONG			ByteOffset,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			FileInfo fileInfo;
			fileInfo.size = ByteOffset;
			base->fsSetFileInfo(context, fileInfo, FileInfoFlags::SizeInfo);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::SetAllocationSize(
			LPCWSTR				FileName,
			LONGLONG			AllocSize,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			FileInfo fileInfo;
			fileInfo.allocationSize = AllocSize;
			base->fsSetFileInfo(context, fileInfo, FileInfoFlags::AllocSizeInfo);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::SetFileAttributes(
			LPCWSTR				FileName,
			DWORD				FileAttributes,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			FileInfo fileInfo;
			fileInfo.fileAttributes = FileAttributes;
			base->fsSetFileInfo(context, fileInfo, FileInfoFlags::AttrInfo);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::SetFileTime(
			LPCWSTR				FileName,
			CONST FILETIME*		CreationTime,
			CONST FILETIME*		LastAccessTime,
			CONST FILETIME*		LastWriteTime,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			FileInfo fileInfo;
			if (CreationTime)
				FileTimeToTime(*CreationTime, fileInfo.createdAt);
			if (LastAccessTime)
				FileTimeToTime(*LastAccessTime, fileInfo.lastAccessedAt);
			if (LastWriteTime)
				FileTimeToTime(*LastWriteTime, fileInfo.modifiedAt);
			base->fsSetFileInfo(context, fileInfo, FileInfoFlags::TimeInfo);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::GetFileSecurity(
			LPCWSTR					FileName,
			PSECURITY_INFORMATION	SecurityInformation,
			PSECURITY_DESCRIPTOR	SecurityDescriptor,
			ULONG				BufferLength,
			PULONG				LengthNeeded,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);

		if (!DokanHost::g_hasSeSecurityPrivilege) {
			*SecurityInformation &= ~0x00000008L/*SACL_SECURITY_INFORMATION*/;
			*SecurityInformation &= ~0x00010000L/*BACKUP_SECURITY_INFORMATION*/;
			// TODO ReOpen with READ_CONTROL|ACCESS_SYSTEM_SECURITY access
		}
		// else TODO ReOpen with READ_CONTROL access

		try {
			Memory fileSecurity = base->fsGetSecurity(context, *SecurityInformation);
			*LengthNeeded = (ULONG)fileSecurity.getSize();
			if (BufferLength < *LengthNeeded) {
#ifdef SLIB_DOKAN_IS_DOKANY
				return STATUS_BUFFER_OVERFLOW;
#else
				return DOKAN_RETERROR(ERROR_INSUFFICIENT_BUFFER);
#endif
			} else
				memcpy(SecurityDescriptor, fileSecurity.getData(), *LengthNeeded);
		} catch (FileSystemError error) {
#ifdef SLIB_DOKAN_IS_DOKANY
			if (error == FileSystemError::NotImplemented) {
				return STATUS_NOT_IMPLEMENTED;	// to let dokan library build a sddl of the current process user with authenticate user rights for context menu.
			}
#endif
			return DOKAN_RETERROR(error);
		}
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::SetFileSecurity(
			LPCWSTR					FileName,
			PSECURITY_INFORMATION	SecurityInformation,
			PSECURITY_DESCRIPTOR	SecurityDescriptor,
			ULONG				SecurityDescriptorLength,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			base->fsSetSecurity(context, *SecurityInformation, Memory::create(SecurityDescriptor, SecurityDescriptorLength));
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::LockFile(
			LPCWSTR				FileName,
			LONGLONG			ByteOffset,
			LONGLONG			Length,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			base->fsLock(context, ByteOffset, Length);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::UnlockFile(
			LPCWSTR				FileName,
			LONGLONG			ByteOffset,
			LONGLONG			Length,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
		FILESYSTEM_EXCEPTION_GUARD(
			base->fsUnlock(context, ByteOffset, Length);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::GetDiskFreeSpace(
			PULONGLONG			FreeBytesAvailable,
			PULONGLONG			TotalNumberOfBytes,
			PULONGLONG			TotalNumberOfFreeBytes,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		FILESYSTEM_EXCEPTION_GUARD(
			VolumeInfo volumeInfo = base->fsGetVolumeInfo(VolumeInfoFlags::SizeInfo);
			*FreeBytesAvailable = ((PLARGE_INTEGER)&volumeInfo.freeSize)->QuadPart;
			*TotalNumberOfBytes = ((PLARGE_INTEGER)&volumeInfo.totalSize)->QuadPart;
			*TotalNumberOfFreeBytes = ((PLARGE_INTEGER)&volumeInfo.freeSize)->QuadPart;
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::GetVolumeInformation(
			LPWSTR		VolumeNameBuffer,
			DWORD		VolumeNameSize,
			LPDWORD		VolumeSerialNumber,
			LPDWORD		MaximumComponentLength,
			LPDWORD		FileSystemFlags,
			LPWSTR		FileSystemNameBuffer,
			DWORD		FileSystemNameSize,
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		FileSystemBase *base = (FileSystemBase *)DokanFileInfo->DokanOptions->GlobalContext;
		FILESYSTEM_EXCEPTION_GUARD(
			VolumeInfo volumeInfo = base->fsGetVolumeInfo();
			WCHAR Buffer[MAX_PATH] = L"";

			*VolumeSerialNumber = volumeInfo.serialNumber;
			*MaximumComponentLength = volumeInfo.maxComponentLength;
			*FileSystemFlags = volumeInfo.fileSystemFlags;

			StringToWChar(volumeInfo.volumeName, Buffer);
			wcscpy_s(VolumeNameBuffer, VolumeNameSize / sizeof(WCHAR), Buffer);

			StringToWChar(volumeInfo.fileSystemName, Buffer);
			wcscpy_s(FileSystemNameBuffer, FileSystemNameSize / sizeof(WCHAR), Buffer);
		)
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK	// SLIB_DOKAN_IS_DOKANY
		DokanHost::Mounted(
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		return 0;
	}

	SLIB_DOKAN_RET DOKAN_CALLBACK
		DokanHost::Unmounted(
			PDOKAN_FILE_INFO	DokanFileInfo)
	{
		return 0;
	}

}

#endif
