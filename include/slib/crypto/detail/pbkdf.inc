/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

namespace slib
{
	
	template <class HASH>
	void PBKDF1<HASH>::generateKey(
		const void* password, sl_uint32 lenPassword,
		const void* salt, sl_uint32 lenSalt,
		sl_uint32 nIteration,
		void* _outDK, sl_uint32 lenDK)
	{
		char t[HASH::HashSize];
		char* outDK = (char*)_outDK;

		HASH hash;
		hash.start();
		hash.update(password, lenPassword);
		hash.update(salt, lenSalt);
		hash.finish(t);
		
		sl_uint32 i;
		for (i = 1; i < nIteration; i++) {
			hash.start();
			hash.update(t, HASH::HashSize);
			hash.finish(t);
		}
		for (i = 0; i < lenDK; i++) {
			outDK[i] = t[i];
		}
	}

	template <class KEYED_HASH>
	void PBKDF2<KEYED_HASH>::generateKey(
		const void* password, sl_uint32 lenPassword,
		const void* salt, sl_uint32 lenSalt,
		sl_uint32 nIteration,			
		void* _outDK, sl_size lenDK)
	{
		char f[KEYED_HASH::HashSize];
		char u[KEYED_HASH::HashSize];

		sl_uint8 bi[4] = {0, 0, 0, 1};
		char* outDK = (char*)_outDK;
		char* endDK = outDK + lenDK;

		KEYED_HASH h;
		sl_uint32 i = 1;
		sl_uint32 p = 0;
		sl_uint32 j, k;

		for (;;) {
			h.start(password, lenPassword);
			h.update(salt, lenSalt);
			h.update(bi, 4);
			h.finish(u);
			for (j = 0; j < KEYED_HASH::HashSize; j++) {
				f[j] = u[j];
			}
			for (k = 1; k < nIteration; k++) {
				h.start(password, lenPassword);
				h.update(u, KEYED_HASH::HashSize);
				h.finish(u);
				for (j = 0; j < KEYED_HASH::HashSize; j++) {
					f[j] ^= u[j];
				}
			}
			if (outDK + (sl_size)(KEYED_HASH::HashSize) >= endDK) {
				Base::copyMemory(outDK, f, endDK - outDK);
				break;
			} else {
				Base::copyMemory(outDK, f, KEYED_HASH::HashSize);
			}
			outDK += KEYED_HASH::HashSize;
			i++;
			bi[0] = (sl_uint8)(i >> 24);
			bi[1] = (sl_uint8)(i >> 16);
			bi[2] = (sl_uint8)(i >> 8);
			bi[3] = (sl_uint8)(i);
		}
	}

}
