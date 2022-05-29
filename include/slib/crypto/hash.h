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

#ifndef CHECKHEADER_SLIB_CRYPTO_HASH
#define CHECKHEADER_SLIB_CRYPTO_HASH

#include "definition.h"

#include "../core/string.h"
#include "../core/memory.h"
#include "../core/mio.h"
#include "../core/scoped_buffer.h"

namespace slib
{
	
	template <class CLASS>
	class SLIB_EXPORT CryptoHash
	{
	public:
		void execute(const void* input, sl_size n, void* output) noexcept
		{
			CLASS& h = *((CLASS*)this);
			h.start();
			h.update(input, n);
			h.finish(output);
		}
		
		static void hash(const void* input, sl_size n, void* output) noexcept
		{
			CLASS h;
			h.start();
			h.update(input, n);
			h.finish(output);
		}
		
		static void hash(const StringData& s, void* output) noexcept
		{
			hash(s.getData(), s.getLength(), output);
		}
		
		static void hash(const Memory& data, void* output) noexcept
		{
			hash(data.getData(), data.getSize(), output);
		}
		
		static Memory hash(const void* input, sl_size n) noexcept
		{
			char v[CLASS::HashSize];
			hash(input, n, v);
			return Memory::create(v, CLASS::HashSize);
		}
		
		static Memory hash(const StringData& s) noexcept
		{
			char v[CLASS::HashSize];
			hash(s.getData(), s.getLength(), v);
			return Memory::create(v, CLASS::HashSize);
		}
		
		static Memory hash(const Memory& data) noexcept
		{
			char v[CLASS::HashSize];
			hash(data.getData(), data.getSize(), v);
			return Memory::create(v, CLASS::HashSize);
		}
		
		void applyMask_MGF1(const void* seed, sl_uint32 sizeSeed, void* _target, sl_uint32 sizeTarget) noexcept
		{
			CLASS* thiz = ((CLASS*)this);
			sl_uint32 n = CLASS::HashSize;
			if (!n) {
				return;
			}
			SLIB_SCOPED_BUFFER(sl_uint8, 128, h, n);
			if (!h) {
				return;
			}
			sl_uint8* target = (sl_uint8*)(_target);
			sl_uint32 i = 0;
			sl_uint8 C[4];
			while (sizeTarget >= n) {
				thiz->start();
				thiz->update(seed, sizeSeed);
				MIO::writeUint32BE(C, i);
				thiz->update(C, 4);
				thiz->finish(h);
				for (sl_uint32 k = 0; k < n; k++) {
					target[k] ^= h[k];
				}
				i++;
				target += n;
				sizeTarget -= n;
			}
			thiz->start();
			thiz->update(seed, sizeSeed);
			MIO::writeUint32BE(C, i);
			thiz->update(C, 4);
			thiz->finish(h);
			for (sl_uint32 k = 0; k < sizeTarget; k++) {
				target[k] ^= h[k];
			}
		}

	};
	
}

#endif
