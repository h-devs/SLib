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
		static Nullable<Memory> read(READER* reader, sl_size maxSize = SLIB_SIZE_MAX, sl_size segmentSize = 0)
		{
			sl_uint32 size;
			if (!(reader->readUint32(&size, Endian::Little))) {
				return sl_null;
			}
			if (size > maxSize) {
				return sl_null;
			}
			if (size) {
				return reader->readFully(reader, size, segmentSize);
			} else {
				return Memory();
			}
		}

		template <class WRITER>
		static sl_bool write(WRITER* writer, const void* data, sl_size size)
		{
#ifdef SLIB_ARCH_IS_64BIT
			if (size >> 32) {
				return sl_false;
			}
#endif
			if (!(writer->writeUint32((sl_uint32)size, Endian::Little))) {
				return sl_false;
			}
			return writer->writeAllBytes(data, size);
		}

		template <class WRITER>
		static sl_bool write(WRITER* writer, const MemoryView& mem)
		{
			return write(writer, mem.data, mem.size);
		}

		static void readAsync(AsyncStream* stream, const Function<void(Memory&, sl_bool flagError)>& callback, sl_size maxSize = SLIB_SIZE_MAX, sl_size segmentSize = 0);

		static void writeAsync(AsyncStream* stream, const Memory& data, const Function<void(sl_bool flagError)>& callback);

	};

}

#endif
