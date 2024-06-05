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

#include "slib/doc/file_type.h"

#include "slib/core/memory.h"
#include "slib/graphics/image.h"

namespace slib
{

	FileType FileTypeHelper::get(const void* _mem, sl_size size)
	{
		FileType type = Image::getFileType(_mem, size);
		if (type != FileType::Unknown) {
			return type;
		}
		sl_uint8* mem = (sl_uint8*)_mem;
		if (size >= 4 && *mem == 0x50 && mem[1] == 0x4B && mem[2] == 0x03 && mem[3] == 0x04) {
			return FileType::ZIP;
		}
		if (size >= 6 && *mem == 0x52 && mem[1] == 0x61 && mem[2] == 0x72 && mem[3] == 0x21 && mem[4] == 0x1A && mem[5] == 0x07) {
			return FileType::RAR;
		}
		if (size >= 4 && *mem == '%' && mem[1] == 'P' && mem[2] == 'D' && mem[3] == 'F') {
			return FileType::PDF;
		}
		if (size >= 4 && *mem == 0x4D && mem[1] == 0x5A) {
			return FileType::EXE;
		}
		if (size >= 4 && *mem == 0x7F && mem[1] == 'E' && mem[2] == 'L' && mem[3] == 'F') {
			return FileType::ELF;
		}
		if (size >= 4 && *mem == 0xCF && mem[1] == 0xFA && mem[2] == 0xED && mem[3] == 0xFE) {
			return FileType::MachO;
		}
		return FileType::Unknown;
	}

	FileType FileTypeHelper::get(const MemoryView& mem)
	{
		return get(mem.data, mem.size);
	}

}
