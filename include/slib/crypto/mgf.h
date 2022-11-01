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

#ifndef CHECKHEADER_SLIB_CRYPTO_MGF
#define CHECKHEADER_SLIB_CRYPTO_MGF

#include "definition.h"

#include "../core/mio.h"
#include "../core/scoped_buffer.h"

/*
	Mask Generation Function (PKCS#1)
*/

namespace slib
{

	template <class HASH>
	class SLIB_EXPORT MGF1
	{
	public:
		static void applyMask(const void* seed, sl_size sizeSeed, void* _target, sl_size sizeTarget) noexcept
		{
			sl_uint32 n = HASH::HashSize;
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
			HASH hash;
			while (sizeTarget >= n) {
				hash.start();
				hash.update(seed, sizeSeed);
				MIO::writeUint32BE(C, i);
				hash.update(C, 4);
				hash.finish(h);
				for (sl_uint32 k = 0; k < n; k++) {
					target[k] ^= h[k];
				}
				i++;
				target += n;
				sizeTarget -= n;
			}
			hash.start();
			hash.update(seed, sizeSeed);
			MIO::writeUint32BE(C, i);
			hash.update(C, 4);
			hash.finish(h);
			for (sl_size k = 0; k < sizeTarget; k++) {
				target[k] ^= h[k];
			}
		}

	};

}

#endif
