/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "async_config.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/core/async_file.h"

#include "slib/core/string.h"
#include "slib/core/handle_ptr.h"
#include "slib/core/win32/windows.h"

namespace slib
{

	namespace priv
	{
		namespace async
		{

			class FileInstance : public AsyncFileInstance
			{
			public:
				Ref<AsyncStreamRequest> m_requestOperating;
				sl_uint64 m_offset;
				OVERLAPPED m_overlappedRead;
				OVERLAPPED m_overlappedWrite;

			public:
				FileInstance()
				{
					m_offset = 0;
				}

			public:
				static Ref<FileInstance> create(const AsyncFileParam& param)
				{
					if (param.handle != SLIB_FILE_INVALID_HANDLE) {
						Ref<FileInstance> ret = new FileInstance();
						if (ret.isNotNull()) {
							ret->setHandle(param.handle);
							ret->m_flagCloseOnRelease = param.flagCloseOnRelease;
							if (param.initialPosition >= 0) {
								ret->m_offset = param.initialPosition;
							} else {
								ret->m_offset = (HandlePtr<File>(param.handle))->getPosition();
							}
							return ret;
						} else {
							if (param.flagCloseOnRelease) {
								File::close(param.handle);
							}
						}
					}
					return sl_null;
				}
				
				void onOrder() override
				{
					sl_file handle = getHandle();
					if (handle == SLIB_FILE_INVALID_HANDLE) {
						return;
					}
					if (m_requestOperating.isNull()) {
						Ref<AsyncStreamRequest> req;
						if (popReadRequest(req)) {
							if (req.isNotNull()) {
								if (req->data && req->size) {
									if (req->position >= 0) {
										m_offset = req->position;
									}
									Base::zeroMemory(&m_overlappedRead, sizeof(m_overlappedRead));
									m_overlappedRead.Offset = (DWORD)m_offset;
									m_overlappedRead.OffsetHigh = (DWORD)(m_offset >> 32);
									DWORD size;
									if (req->size > 0x40000000) {
										size = 0x40000000;
									} else {
										size = (DWORD)(req->size);
									}
									if (ReadFile((HANDLE)handle, req->data, size, NULL, &m_overlappedRead)) {
										processStreamResult(req.get(), 0, sl_true);
									} else {
										DWORD dwErr = ::GetLastError();
										if (dwErr == ERROR_IO_PENDING) {
											m_requestOperating = req;
										} else {
											processStreamResult(req.get(), 0, sl_true);
										}
									}
								} else {
									processStreamResult(req.get(), req->size, sl_false);
								}
							}
						}
					}
					if (m_requestOperating.isNull()) {
						Ref<AsyncStreamRequest> req;
						if (popWriteRequest(req)) {
							if (req.isNotNull()) {
								if (req->data && req->size) {
									if (req->position >= 0) {
										m_offset = req->position;
									}
									Base::zeroMemory(&m_overlappedWrite, sizeof(m_overlappedWrite));
									m_overlappedWrite.Offset = (DWORD)m_offset;
									m_overlappedWrite.OffsetHigh = (DWORD)(m_offset >> 32);
									DWORD size;
									if (req->size > 0x40000000) {
										size = 0x40000000;
									} else {
										size = (DWORD)(req->size);
									}
									if (WriteFile((HANDLE)handle, req->data, size, NULL, &m_overlappedWrite)) {
										processStreamResult(req.get(), 0, sl_true);
									} else {
										DWORD dwErr = ::GetLastError();
										if (dwErr == ERROR_IO_PENDING) {
											m_requestOperating = req;
										} else {
											processStreamResult(req.get(), 0, sl_true);
										}
									}
								} else {
									processStreamResult(req.get(), req->size, sl_false);
								}
							}
						}
					}
				}

				void onEvent(EventDesc* pev) override
				{
					sl_file handle = getHandle();
					if (handle == SLIB_FILE_INVALID_HANDLE) {
						return;
					}
					OVERLAPPED* pOverlapped = (OVERLAPPED*)(pev->pOverlapped);
					DWORD dwSize = 0;
					sl_bool flagError = sl_false;
					if (!(GetOverlappedResult((HANDLE)handle, pOverlapped, &dwSize, FALSE))) {
						flagError = sl_true;
						close();
					}
					if (dwSize > 0) {
						m_offset += dwSize;
					} else {
						flagError = sl_true;
					}

					Ref<AsyncStreamRequest> req = Move(m_requestOperating);

					if (req.isNotNull()) {
						if (pOverlapped == &m_overlappedRead || pOverlapped == &m_overlappedWrite) {
							processStreamResult(req.get(), dwSize, flagError);
						}
					}

					requestOrder();
				}

				sl_bool isSeekable() override
				{
					return sl_true;
				}

				sl_bool seek(sl_uint64 pos) override
				{
					m_offset = pos;
					return sl_true;
				}

				sl_uint64 getPosition() override
				{
					return m_offset;
				}

			};

		}
	}
	
	Ref<AsyncFile> AsyncFile::create(const AsyncFileParam& param)
	{
		Ref<priv::async::FileInstance> ret = priv::async::FileInstance::create(param);
		if (ret.isNotNull()) {
			return AsyncFile::create(ret.get(), AsyncIoMode::InOut, param.ioLoop);
		}
		return sl_null;
	}

	sl_bool AsyncFileParam::open(const StringParam& _filePath, FileMode mode)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}

		DWORD dwShareMode = (mode & FileMode::Read) ? FILE_SHARE_READ : 0;
		DWORD dwDesiredAccess = 0;
		DWORD dwCreateDisposition = 0;
		DWORD dwFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

		if (mode & FileMode::Write) {
			dwDesiredAccess = GENERIC_WRITE;
			if (mode & FileMode::Read) {
				dwDesiredAccess |= GENERIC_READ;
			}
			if (mode & FileMode::NotCreate) {
				if (mode & FileMode::NotTruncate) {
					dwCreateDisposition = OPEN_EXISTING;
				} else {
					dwCreateDisposition = TRUNCATE_EXISTING;
				}
			} else {
				if (mode & FileMode::NotTruncate) {
					dwCreateDisposition = OPEN_ALWAYS;
				} else {
					dwCreateDisposition = CREATE_ALWAYS;
				}
			}
		} else {
			dwDesiredAccess = GENERIC_READ;
			dwCreateDisposition = OPEN_EXISTING;
		}
		if (mode & FileMode::HintRandomAccess) {
			dwFlags |= FILE_FLAG_RANDOM_ACCESS;
		}

		HANDLE hFile = CreateFileW(
			(LPCWSTR)(filePath.getData())
			, dwDesiredAccess
			, dwShareMode
			, NULL
			, dwCreateDisposition
			, dwFlags
			, NULL
		);

		if (hFile != SLIB_FILE_INVALID_HANDLE) {
			if (mode & FileMode::SeekToEnd) {
				(HandlePtr<File>(hFile))->seekToEnd();
				initialPosition = -1;
			} else {
				initialPosition = 0;
			}
			handle = hFile;
			return sl_true;
		}
		return sl_false;
	}

}

#endif
