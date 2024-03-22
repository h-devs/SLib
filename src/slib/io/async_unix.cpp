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

#include "async_config.h"

#if defined(SLIB_PLATFORM_IS_UNIX)

#include "slib/io/async_file_stream.h"

#include "slib/core/thread.h"
#include "slib/core/handle_ptr.h"

namespace slib
{

	namespace {

		class FileInstance : public AsyncFileStreamInstance
		{
		public:
			static Ref<FileInstance> create(const AsyncFileStreamParam& param)
			{
				if (param.handle != SLIB_FILE_INVALID_HANDLE) {
					Ref<FileInstance> ret = new FileInstance;
					if (ret.isNotNull()) {
						ret->setHandle(param.handle);
						ret->m_flagCloseOnRelease = param.flagCloseOnRelease;
						return ret;
					} else {
						if (param.flagCloseOnRelease) {
							File::close(param.handle);
						}
					}
				}
				return sl_null;
			}

			void processRead(sl_bool flagError)
			{
				HandlePtr<File> file = getHandle();
				if (file->isNone()) {
					return;
				}
				Ref<AsyncStreamRequest> request = Move(m_requestReading);
				Thread* thread = Thread::getCurrent();
				while (!thread || thread->isNotStopping()) {
					if (request.isNull()) {
						request = getReadRequest();
						if (request.isNull()) {
							return;
						}
					}
					char* data = (char*)(request->data);
					sl_size size = request->size;
					if (data && size) {
						sl_reg n = file->read(data, size, 0);
						if (n > 0) {
							processStreamResult(request.get(), n, flagError ? AsyncStreamResultCode::Unknown : AsyncStreamResultCode::Success);
						} else {
							if (n == SLIB_IO_WOULD_BLOCK) {
								if (flagError) {
									processStreamResult(request.get(), 0, AsyncStreamResultCode::Unknown);
								} else {
									m_requestReading = Move(request);
								}
							} else if (n == SLIB_IO_ENDED) {
								processStreamResult(request.get(), 0, AsyncStreamResultCode::Ended);
							} else {
								processStreamResult(request.get(), 0, AsyncStreamResultCode::Unknown);
							}
							return;
						}
					} else {
						processStreamResult(request.get(), 0, AsyncStreamResultCode::Success);
					}
					request.setNull();
				}
			}

			void processWrite(sl_bool flagError)
			{
				HandlePtr<File> file = getHandle();
				if (file->isNone()) {
					return;
				}
				Ref<AsyncStreamRequest> request = Move(m_requestWriting);
				Thread* thread = Thread::getCurrent();
				while (!thread || thread->isNotStopping()) {
					if (request.isNull()) {
						request = getWriteRequest();
						if (request.isNull()) {
							return;
						}
					}
					char* data = (char*)(request->data);
					sl_size size = request->size;
					if (data && size) {
						for (;;) {
							sl_size sizeWritten = request->sizeWritten;
							sl_reg n = file->write(data + sizeWritten, size - sizeWritten, 0);
							if (n >= 0) {
								request->sizeWritten += n;
								if (request->sizeWritten >= size) {
									request->sizeWritten = 0;
									processStreamResult(request.get(), size, flagError ? AsyncStreamResultCode::Unknown : AsyncStreamResultCode::Success);
									break;
								}
							} else {
								if (n == SLIB_IO_WOULD_BLOCK) {
									if (flagError) {
										request->sizeWritten = 0;
										processStreamResult(request.get(), sizeWritten, AsyncStreamResultCode::Unknown);
									} else {
										m_requestWriting = Move(request);
									}
								} else {
									request->sizeWritten = 0;
									processStreamResult(request.get(), sizeWritten, AsyncStreamResultCode::Unknown);
								}
								return;
							}
						}
					}
					request.setNull();
				}
			}

			void onOrder() override
			{
				processRead(sl_false);
				processWrite(sl_false);
			}

			void onEvent(EventDesc* pev) override
			{
				sl_bool flagProcessed = sl_false;
				if (pev->flagIn) {
					processRead(pev->flagError);
					flagProcessed = sl_true;
				}
				if (pev->flagOut) {
					processWrite(pev->flagError);
					flagProcessed = sl_true;
				}
				if (!flagProcessed) {
					if (pev->flagError) {
						processRead(sl_true);
						processWrite(sl_true);
					}
				}
				requestOrder();
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

}

#endif
