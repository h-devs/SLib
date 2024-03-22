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

#ifndef CHECKHEADER_SLIB_IO_CHUNK
#define CHECKHEADER_SLIB_IO_CHUNK

#include "definition.h"

#include "../core/memory.h"
#include "../core/nullable.h"
#include "../core/endian.h"
#include "../core/function.h"
#include "../core/timeout.h"

/*
	Chunk structure

	-----------------------------
		Chunk Length (4 Bytes, Little Endian)
	-----------------------------
		Chunk Data
	-----------------------------

*/

namespace slib
{

	class AsyncStream;

	class SLIB_EXPORT ChunkIO
	{
	public:
		template <class READER>
		static Nullable<Memory> read(READER* reader, sl_uint32 maxSize = 0xffffffff, sl_size segmentSize = 0, sl_int32 timeout = -1)
		{
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			sl_uint8 bufSize[4];
			if (reader->readFully(bufSize, 4, timeout) != 4) {
				return sl_null;
			}
			sl_uint32 size = MIO::readUint32LE(bufSize);
			if (size > maxSize) {
				return sl_null;
			}
			if (!size) {
				return Memory();
			}
			Memory ret = reader->readFully(size, segmentSize, GetTimeoutFromTick(tickEnd));
			if (ret.getSize() == size) {
				return ret;
			}
			return sl_null;
		}

		template <class WRITER>
		static sl_bool write(WRITER* writer, const void* data, sl_size size, sl_int32 timeout = -1)
		{
#ifdef SLIB_ARCH_IS_64BIT
			if (size >> 32) {
				return sl_false;
			}
#endif
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			sl_uint8 bufSize[4];
			MIO::writeUint32LE(bufSize, (sl_uint32)size);
			if (writer->writeFully(bufSize, 4, timeout) != 4) {
				return sl_false;
			}
			if (!size) {
				return sl_true;
			}
			return writer->writeFully(data, size, GetTimeoutFromTick(tickEnd)) == size;
		}

		template <class WRITER>
		static sl_bool write(WRITER* writer, const MemoryView& mem, sl_int32 timeout = -1)
		{
			return write(writer, mem.data, mem.size, timeout);
		}

		static void readAsync(AsyncStream* stream, const Function<void(AsyncStream*, Memory&, sl_bool flagError)>& callback, sl_uint32 maxSize = 0xffffffff, sl_uint32 segmentSize = 0, sl_int32 timeout = -1);

		static void writeAsync(AsyncStream* stream, const Memory& data, const Function<void(AsyncStream*, sl_bool flagError)>& callback, sl_int32 timeout = -1);

	};

}

#endif
