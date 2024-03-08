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

#include "slib/io/async_file_stream.h"

#include "slib/core/string.h"
#include "slib/core/handle_ptr.h"
#include "slib/platform/win32/windows.h"

namespace slib
{

	namespace {

		class FileInstance : public AsyncFileStreamInstance
		{
		public:
			sl_uint64 m_offset;
			sl_bool m_flagSupportSeeking;

			OVERLAPPED m_overlappedRead;
			OVERLAPPED m_overlappedWrite;

		public:
			FileInstance()
			{
				m_offset = 0;
				m_flagSupportSeeking = sl_false;
			}

		public:
			static Ref<FileInstance> create(const AsyncFileStreamParam& param)
			{
				if (param.handle != SLIB_FILE_INVALID_HANDLE) {
					Ref<FileInstance> ret = new FileInstance();
					if (ret.isNotNull()) {
						ret->setHandle(param.handle);
						ret->m_flagCloseOnRelease = param.flagCloseOnRelease;
						if (param.flagSupportSeeking) {
							ret->m_flagSupportSeeking = sl_true;
							ret->m_offset = param.initialPosition;
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
				if (m_requestReading.isNull()) {
					Ref<AsyncStreamRequest> req = getReadRequest();
					if (req.isNotNull()) {
						if (req->data && req->size) {
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
								processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
							} else {
								DWORD dwErr = ::GetLastError();
								if (dwErr == ERROR_IO_PENDING) {
									m_requestReading = Move(req);
								} else {
									processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
								}
							}
						} else {
							processStreamResult(req.get(), req->size, AsyncStreamResultCode::Success);
						}
					}
				}
				if (m_requestWriting.isNull()) {
					Ref<AsyncStreamRequest> req = getWriteRequest();
					if (req.isNotNull()) {
						if (req->data && req->size) {
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
								processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
							} else {
								DWORD dwErr = ::GetLastError();
								if (dwErr == ERROR_IO_PENDING) {
									m_requestWriting = Move(req);
								} else {
									processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
								}
							}
						} else {
							processStreamResult(req.get(), req->size, AsyncStreamResultCode::Success);
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
				DWORD dwError = ERROR_SUCCESS;
				if (GetOverlappedResult((HANDLE)handle, pOverlapped, &dwSize, FALSE)) {
					if (m_flagSupportSeeking) {
						m_offset += dwSize;
					}
				} else {
					dwError = GetLastError();
					onClose();
				}

				if (pOverlapped == &m_overlappedRead) {
					Ref<AsyncStreamRequest> req = Move(m_requestReading);
					if (req.isNotNull()) {
						if (dwError == ERROR_SUCCESS) {
							processStreamResult(req.get(), dwSize, AsyncStreamResultCode::Success);
						} else if (dwError == ERROR_HANDLE_EOF) {
							processStreamResult(req.get(), 0, AsyncStreamResultCode::Ended);
						} else {
							processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
						}
					}
				} else if (pOverlapped == &m_overlappedWrite) {
					Ref<AsyncStreamRequest> req = Move(m_requestWriting);
					if (req.isNotNull()) {
						if (dwError == ERROR_SUCCESS) {
							processStreamResult(req.get(), dwSize, AsyncStreamResultCode::Success);
						} else {
							processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
						}
					}
				}
				requestOrder();
			}

			sl_bool isSeekable() override
			{
				return m_flagSupportSeeking;
			}

			sl_bool seek(sl_uint64 pos) override
			{
				if (m_flagSupportSeeking) {
					m_offset = pos;
					return sl_true;
				}
				return sl_false;
			}

			sl_uint64 getPosition() override
			{
				return m_offset;
			}

			sl_uint64 getSize() override
			{
				return (HandlePtr<File>(getHandle()))->getSize();
			}

		};

	}

	Ref<AsyncFileStream> AsyncFileStream::create(const AsyncFileStreamParam& param)
	{
		Ref<FileInstance> ret = FileInstance::create(param);
		if (ret.isNotNull()) {
			return AsyncFileStream::create(ret.get(), param.mode, param.ioLoop);
		}
		return sl_null;
	}

	sl_bool AsyncFileStreamParam::openFile(const StringParam& _filePath, FileMode fileMode)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}

		DWORD dwShareMode = (fileMode & FileMode::Read) ? FILE_SHARE_READ : 0;
		DWORD dwDesiredAccess = 0;
		DWORD dwCreateDisposition = 0;
		DWORD dwFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

		if (fileMode & FileMode::Write) {
			dwDesiredAccess = GENERIC_WRITE;
			if (fileMode & FileMode::Read) {
				dwDesiredAccess |= GENERIC_READ;
			}
			if (fileMode & FileMode::NotCreate) {
				if (fileMode & FileMode::NotTruncate) {
					dwCreateDisposition = OPEN_EXISTING;
				} else {
					dwCreateDisposition = TRUNCATE_EXISTING;
				}
			} else {
				if (fileMode & FileMode::NotTruncate) {
					dwCreateDisposition = OPEN_ALWAYS;
				} else {
					dwCreateDisposition = CREATE_ALWAYS;
				}
			}
		} else {
			dwDesiredAccess = GENERIC_READ;
			dwCreateDisposition = OPEN_EXISTING;
		}
		if (fileMode & FileMode::HintRandomAccess) {
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
			handle = hFile;
			flagCloseOnRelease = sl_true;
			if (fileMode & FileMode::SeekToEnd) {
				initialPosition = (HandlePtr<File>(hFile))->getSize();
			} else {
				initialPosition = 0;
			}
			flagSupportSeeking = sl_true;
			if (fileMode & FileMode::Read) {
				if (fileMode & FileMode::Write) {
					mode = AsyncIoMode::InOut;
				} else {
					mode = AsyncIoMode::In;
				}
			} else {
				if (fileMode & FileMode::Write) {
					mode = AsyncIoMode::Out;
				} else {
					mode = AsyncIoMode::None;
				}
			}
			return sl_true;
		}
		return sl_false;
	}

}

#endif
