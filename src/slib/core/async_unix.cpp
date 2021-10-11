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

#include "slib/core/async_file.h"

#include "slib/core/handle_ptr.h"

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
				
			public:
				FileInstance()
				{
				}

			public:
				static Ref<FileInstance> create(const AsyncFileParam& param)
				{
					if (param.handle != SLIB_FILE_INVALID_HANDLE) {
						Ref<FileInstance> ret = new FileInstance;
						if (ret.isNotNull()) {
							ret->setHandle(param.handle);
							ret->m_flagCloseOnRelease = param.flagCloseOnRelease;
							if (param.initialPosition >= 0) {
								if ((HandlePtr<File>(param.handle))->seek(param.initialPosition, SeekPosition::Begin)) {
									return ret;
								}
							} else {
								return ret;
							}
						} else {
							if (param.flagCloseOnRelease) {
								File::close(param.handle);
							}
						}
					}
					return sl_null;
				}
				
				void processRequests(sl_bool flagError)
				{
					HandlePtr<File> file = getHandle();
					if (file.isNone()) {
						return;
					}

					Ref<AsyncStreamRequest> request = Move(m_requestOperating);
					if (request.isNull()) {
						popReadRequest(request);
						if (request.isNull()) {
							popWriteRequest(request);
							if (request.isNull()) {
								return;
							}
						}
					}
					
					sl_bool flagRead = request->flagRead;
					char* data = (char*)(request->data);
					sl_size size = request->size;
					if (data && size) {
						if (request->position >= 0) {
							if (!(file->seek(request->position, SeekPosition::Begin))) {
								processStreamResult(request.get(), 0, sl_false);
								return;
							}
						}
						for (;;) {
							sl_reg n;
							sl_size sizeProcessed = request->sizeWritten;
							if (flagRead) {
								sizeProcessed = 0;
								n = file->read(data, size);
							} else {
								sizeProcessed = request->sizeWritten;
								n = file->write(data + sizeProcessed, size - sizeProcessed);
							}
							if (n > 0) {
								if (flagRead) {
									processStreamResult(request.get(), n, flagError);
									return;
								} else {
									request->sizeWritten += n;
									if (request->sizeWritten >= size) {
										request->sizeWritten = 0;
										processStreamResult(request.get(), size, flagError);
										return;
									}
								}
							} else {
								if (n == SLIB_IO_WOULD_BLOCK) {
									if (flagError) {
										request->sizeWritten = 0;
										processStreamResult(request.get(), sizeProcessed, sl_true);
									} else {
										m_requestOperating = Move(request);
									}
								} else {
									request->sizeWritten = 0;
									processStreamResult(request.get(), sizeProcessed, sl_true);
								}
								return;
							}
						}
					}

				}
				
				void onOrder() override
				{
					processRequests(sl_false);
				}
				
				void onEvent(EventDesc* pev) override
				{
					if (pev->flagIn || pev->flagOut || pev->flagError) {
						processRequests(pev->flagError);
					}
					requestOrder();
				}

				sl_bool isSeekable() override
				{
					return sl_true;
				}

				sl_bool seek(sl_uint64 pos) override
				{
					return (HandlePtr<File>(getHandle()))->seek(pos, SeekPosition::Begin);
				}

				sl_uint64 getPosition() override
				{
					return (HandlePtr<File>(getHandle()))->getPosition();
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

	sl_bool AsyncFileParam::open(const StringParam& filePath, FileMode mode)
	{
		File file = File::open(filePath, mode);
		if (file.isOpened()) {
			file.setNonBlocking();
			handle = file.release();
			return sl_true;
		}
		return sl_false;
	}

}

#endif
