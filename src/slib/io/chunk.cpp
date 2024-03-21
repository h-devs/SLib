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

#include "slib/io/chunk.h"

#include "slib/io/async_stream.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/mio.h"

namespace slib
{

	void ChunkIO::readAsync(AsyncStream* stream, const Function<void(AsyncStream*, Memory&, sl_bool flagError)>& callback, sl_uint32 maxSize, sl_uint32 segmentSize, sl_int32 timeout)
	{
		if (maxSize == SLIB_SIZE_MAX) {
			if (!segmentSize) {
				segmentSize = 1024;
			}
		}
		sl_int64 tickEnd = GetTickFromTimeout(timeout);
		stream->readFully(4, [callback, maxSize, segmentSize, tickEnd](AsyncStream* stream, Memory& memSizeField, sl_bool flagError) {
			if (flagError || memSizeField.getSize() != 4) {
				Memory null;
				callback(stream, null, sl_true);
				return;
			}
			sl_uint32 size = MIO::readUint32LE(memSizeField.getData());
			if (!size) {
				Memory null;
				callback(stream, null, sl_false);
				return;
			}
			if (size > maxSize) {
				Memory null;
				callback(stream, null, sl_true);
				return;
			}
			sl_int32 timeout = GetTimeoutFromTick(tickEnd);
			if (!segmentSize || size < segmentSize) {
				stream->readFully(size, [callback](AsyncStream* stream, Memory& data, sl_bool flagError) {
					callback(stream, data, flagError);
				}, timeout);
			} else {
				stream->readFully(size, segmentSize, [size, callback](AsyncStream* stream, MemoryBuffer& buf, sl_bool flagError) {
					if (flagError) {
						Memory null;
						callback(stream, null, sl_true);
						return;
					}
					Memory data = buf.merge();
					if (data.getSize() != size) {
						Memory null;
						callback(stream, null, sl_true);
						return;
					}
					callback(stream, data, sl_false);
				}, timeout);
			}
		}, timeout);
	}

	void ChunkIO::writeAsync(AsyncStream* stream, const Memory& data, const Function<void(AsyncStream*, sl_bool flagError)>& callback, sl_int32 timeout)
	{
		sl_size size = data.getSize();
#ifdef SLIB_ARCH_IS_64BIT
		if (size >> 32) {
			callback(stream, sl_true);
			return;
		}
#endif
		sl_int64 tickEnd = GetTickFromTimeout(timeout);
		sl_uint8 bufSizeField[4];
		MIO::writeUint32LE(bufSizeField, (sl_uint32)size);
		stream->createMemoryAndWrite(bufSizeField, 4, [data, size, callback, tickEnd](AsyncStreamResult& result) {
			if (result.isError() || result.size != 4) {
				callback(result.stream, sl_true);
				return;
			}
			if (!size) {
				callback(result.stream, sl_false);
				return;
			}
			result.stream->write(data, [callback, size](AsyncStreamResult& result) {
				if (result.isError() || result.size != size) {
					callback(result.stream, sl_true);
				} else {
					callback(result.stream, sl_false);
				}
			}, GetTimeoutFromTick(tickEnd));
		}, timeout);
	}

}
