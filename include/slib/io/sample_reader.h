/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_IO_SAMPLE_READER
#define CHECKHEADER_SLIB_IO_SAMPLE_READER

#include "definition.h"

namespace slib
{

	class SLIB_EXPORT SampleReader
	{
	private:
		sl_uint8* current;
		sl_uint8* end;
		sl_uint32 bitPos;
		sl_uint32 bitsPerSample;

	public:
		SampleReader(const void* input, sl_size size, sl_uint32 _bitsPerSample)
		{
			current = (sl_uint8*)input;
			end = current + size;
			bitPos = 0;
			bitsPerSample = _bitsPerSample;
		}

	public:
		sl_bool read(sl_uint32& _out)
		{
			if (current >= end) {
				return sl_false;
			}
			sl_uint32 remain = 8 - bitPos;
			if (bitsPerSample <= remain) {
				_out = (*current >> (remain - bitsPerSample)) & ((1 << bitsPerSample) - 1);
				bitPos += bitsPerSample;
				if (bitPos >= 8) {
					bitPos = 0;
					current++;
				}
			} else {
				sl_uint32 nBits;
				if (bitPos) {
					_out = *current & ((1 << remain) - 1);
					bitPos = 0;
					current++;
					nBits = bitsPerSample - remain;
				} else {
					_out = 0;
					nBits = bitsPerSample;
				}
				sl_uint32 nBytes = nBits >> 3;
				if (current + nBytes <= end) {
					for (sl_uint32 i = 0; i < nBytes; i++) {
						_out = (_out << 8) | *current;
						current++;
					}
				} else {
					return sl_false;
				}
				nBits &= 7;
				if (nBits) {
					if (current >= end) {
						return sl_false;
					}
					_out = (_out << nBits) | (*current >> (8 - nBits));
					bitPos = nBits;
				}
			}
			return sl_true;
		}

	};

}

#endif
