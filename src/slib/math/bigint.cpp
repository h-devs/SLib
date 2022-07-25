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

#include "slib/math/bigint.h"

#include "slib/core/math.h"
#include "slib/core/variant.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/object_op.h"
#include "slib/core/compile_optimize.h"

#define STACK_BUFFER_SIZE 4096

#define CBIGINT_INT32(o, v) \
	CBigInt o; \
	sl_uint32 _t_m_##o; \
	if (v < 0) { \
		_t_m_##o = -v; \
		o.sign = -1; \
	} else { \
		_t_m_##o = v; \
		o.sign = 1; \
	} \
	o.setUserDataElements((sl_uint32*)&_t_m_##o, 1);

#define CBIGINT_UINT32(o, v) \
	CBigInt o; \
	o.sign = 1; \
	sl_uint32 _t_m_##o = v; \
	o.setUserDataElements((sl_uint32*)&_t_m_##o, 1);

#define CBIGINT_INT64(o, v) \
	CBigInt o; \
	if (v < 0) { \
		o.sign = -1; \
		v = -v; \
	} else { \
		o.sign = 1; \
	} \
	sl_uint32 _t_m_##o[2]; \
	_t_m_##o[0] = (sl_uint32)((sl_uint64)v); \
	_t_m_##o[1] = (sl_uint32)(((sl_uint64)v) >> 32); \
	o.setUserDataElements(_t_m_##o, 2);

#define CBIGINT_UINT64(o, v) \
	CBigInt o; \
	o.sign = 1; \
	sl_uint32 _t_m_##o[2]; \
	_t_m_##o[0] = (sl_uint32)((sl_uint64)v); \
	_t_m_##o[1] = (sl_uint32)(((sl_uint64)v) >> 32); \
	o.setUserDataElements(_t_m_##o, 2);


namespace slib
{
	
	namespace priv
	{
		namespace bigint
		{
			
			SLIB_INLINE static sl_compare_result Compare(const sl_uint32* a, const sl_uint32* b, sl_size n) noexcept
			{
				for (sl_size i = n; i > 0; i--) {
					if (a[i - 1] > b[i - 1]) {
						return 1;
					}
					if (a[i - 1] < b[i - 1]) {
						return -1;
					}
				}
				return 0;
			}
			
			// returns 0, 1 (overflow)
			SLIB_INLINE static sl_uint32 Add(sl_uint32* c, const sl_uint32* a, const sl_uint32* b, sl_size n, sl_uint32 _of) noexcept
			{
				sl_uint32 of = _of;
				for (sl_size i = 0; i < n; i++) {
					sl_uint32 sum = a[i] + of;
					of = sum < of ? 1 : 0;
					sl_uint32 t = b[i];
					sum += t;
					of += sum < t ? 1 : 0;
					c[i] = sum;
				}
				return of;
			}
			
			// returns 0, 1 (overflow)
			SLIB_INLINE static sl_uint32 Add_uint32(sl_uint32* c, const sl_uint32* a, sl_size n, sl_uint32 b) noexcept
			{
				sl_uint32 of = b;
				if (c == a) {
					for (sl_size i = 0; i < n && of; i++) {
						sl_uint32 sum = a[i] + of;
						of = sum < of ? 1 : 0;
						c[i] = sum;
					}
				} else {
					for (sl_size i = 0; i < n; i++) {
						sl_uint32 sum = a[i] + of;
						of = sum < of ? 1 : 0;
						c[i] = sum;
					}
				}
				return of;
			}
			
			// returns 0, 1 (overflow)
			SLIB_INLINE static sl_uint32 Sub(sl_uint32* c, const sl_uint32* a, const sl_uint32* b, sl_size n, sl_uint32 _of) noexcept
			{
				sl_uint32 of = _of;
				for (sl_size i = 0; i < n; i++) {
					sl_uint32 k1 = a[i];
					sl_uint32 k2 = b[i];
					sl_uint32 o = k1 < of ? 1 : 0;
					k1 -= of;
					of = o + (k1 < k2 ? 1 : 0);
					k1 -= k2;
					c[i] = k1;
				}
				return of;
			}
			
			// returns 0, 1 (overflow)
			SLIB_INLINE static sl_uint32 Sub_uint32(sl_uint32* c, const sl_uint32* a, sl_size n, sl_uint32 b) noexcept
			{
				sl_uint32 of = b;
				if (c == a) {
					for (sl_size i = 0; i < n && of; i++) {
						sl_uint32 k = a[i];
						sl_uint32 o = k < of ? 1 : 0;
						k -= of;
						of = o;
						c[i] = k;
					}
				} else {
					for (sl_size i = 0; i < n; i++) {
						sl_uint32 k = a[i];
						sl_uint32 o = k < of ? 1 : 0;
						k -= of;
						of = o;
						c[i] = k;
					}
				}
				return of;
			}
			
			// returns overflow
			SLIB_INLINE static sl_uint32 Mul_uint32(sl_uint32* c, const sl_uint32* a, sl_size n, sl_uint32 b, sl_uint32 o) noexcept
			{
				sl_uint32 of = o;
				for (sl_size i = 0; i < n; i++) {
					sl_uint64 k = a[i];
					k *= b;
					k += of;
					c[i] = (sl_uint32)k;
					of = (sl_uint32)(k >> 32);
				}
				return of;
			}
			
			
			// c = c + a * b
			SLIB_INLINE static sl_uint32 MulAdd_uint32(sl_uint32* c, const sl_uint32* s, sl_size m, const sl_uint32* a, sl_size n, sl_uint32 b, sl_uint32 o) noexcept
			{
				n = Math::min(m, n);
				sl_uint32 of = o;
				sl_size i;
				for (i = 0; i < n; i++) {
					sl_uint64 k = a[i];
					k *= b;
					k += of;
					k += s[i];
					c[i] = (sl_uint32)k;
					of = (sl_uint32)(k >> 32);
				}
				if (c == s) {
					for (i = n; i < m && of; i++) {
						sl_uint32 sum = s[i] + of;
						of = sum < of ? 1 : 0;
						c[i] = sum;
					}
				} else {
					for (i = n; i < m; i++) {
						sl_uint32 sum = s[i] + of;
						of = sum < of ? 1 : 0;
						c[i] = sum;
					}
				}
				return of;
			}
			
			
			// returns remainder
			SLIB_INLINE static sl_uint32 Div_uint32(sl_uint32* q, const sl_uint32* a, sl_size n, sl_uint32 b, sl_uint32 o) noexcept
			{
				sl_size j = n - 1;
				if (q) {
					for (sl_size i = 0; i < n; i++) {
						sl_uint64 k = o;
						k <<= 32;
						k |= a[j];
						q[j] = (sl_uint32)(k / b);
						o = (sl_uint32)(k % b);
						j--;
					}
				} else {
					for (sl_size i = 0; i < n; i++) {
						sl_uint64 k = o;
						k <<= 32;
						k |= a[j];
						o = (sl_uint32)(k % b);
						j--;
					}
				}
				return o;
			}
			
			// shift 0~31 bits
			// returns overflow
			SLIB_INLINE static sl_uint32 ShiftLeft(sl_uint32* c, const sl_uint32* a, sl_size n, sl_uint32 shift, sl_uint32 valueRight) noexcept
			{
				sl_uint32 rs = 32 - shift;
				sl_uint32 of = valueRight >> rs;
				for (sl_size i = 0; i < n; i++) {
					sl_uint32 t = a[i];
					c[i] = (t << shift) | of;
					of = t >> rs;
				}
				return of;
			}
			
			// shift 0~31 bits
			// returns overflow
			SLIB_INLINE static sl_uint32 ShiftRight(sl_uint32* c, const sl_uint32* a, sl_size n, sl_uint32 shift, sl_uint32 valueLeft) noexcept
			{
				sl_uint32 rs = 32 - shift;
				sl_uint32 of = valueLeft << rs;
				for (sl_size i = n; i > 0; i--) {
					sl_uint32 t = a[i - 1];
					c[i - 1] = (t >> shift) | of;
					of = t << rs;
				}
				return of;
			}
			
			SLIB_INLINE static sl_size Mse(const sl_uint32* a, sl_size n) noexcept
			{
				for (sl_size ni = n; ni > 0; ni--) {
					if (a[ni - 1] != 0) {
						return ni;
					}
				}
				return 0;
			}
			
			SLIB_INLINE static sl_size Lse(const sl_uint32* a, sl_size n) noexcept
			{
				for (sl_size ni = 0; ni < n; ni++) {
					if (a[ni] != 0) {
						return ni + 1;
					}
				}
				return 0;
			}
			
			SLIB_INLINE static sl_size MsBytes(const sl_uint32* a, sl_size n) noexcept
			{
				for (sl_size ni = n; ni > 0; ni--) {
					sl_uint32 e = a[ni - 1];
					if (e != 0) {
						for (sl_uint32 nB = 4; nB > 0; nB--) {
							if (((e >> ((nB - 1) << 3)) & 255) != 0) {
								return (((ni - 1) << 2) + nB);
							}
						}
						break;
					}
				}
				return 0;
			}
			
			SLIB_INLINE static sl_size LsBytes(const sl_uint32* a, sl_size n) noexcept
			{
				for (sl_size ni = 0; ni < n; ni++) {
					sl_uint32 e = a[ni];
					if (e != 0) {
						for (sl_uint32 nB = 0; nB < 4; nB++) {
							if (((e >> (nB << 3)) & 255) != 0) {
								return ((ni << 2) + nB + 1);
							}
						}
						break;
					}
				}
				return 0;
			}
			
			SLIB_INLINE static sl_size MsBits(const sl_uint32* a, sl_size n) noexcept
			{
				for (sl_size ni = n; ni > 0; ni--) {
					sl_uint32 e = a[ni - 1];
					if (e != 0) {
						for (sl_uint32 nb = 32; nb > 0; nb--) {
							if (((e >> (nb - 1)) & 1) != 0) {
								return (((ni - 1) << 5) + nb);
							}
						}
						break;
					}
				}
				return 0;
			}
			
			SLIB_INLINE static sl_size LsBits(const sl_uint32* a, sl_size n) noexcept
			{
				for (sl_size ni = 0; ni < n; ni++) {
					sl_uint32 e = a[ni];
					if (e != 0) {
						for (sl_uint32 nb = 0; nb < 32; nb++) {
							if (((e >> nb) & 1) != 0) {
								return ((ni << 5) + nb + 1);
							}
						}
						break;
					}
				}
				return 0;
			}

			static sl_size GetBytesCount(const sl_uint32* elements, sl_size length, sl_bool flagSigned) noexcept
			{
				if (elements) {
					if (flagSigned) {
						return (priv::bigint::MsBits(elements, length) + 8) >> 3;
					} else {
						sl_size n = priv::bigint::MsBytes(elements, length);
						return n ? n : 1;
					}
				}
				return 0;
			}

		}
	}


	SLIB_DEFINE_ROOT_OBJECT(CBigInt)

	SLIB_INLINE void CBigInt::_free() noexcept
	{
		if (elements) {
			if (!m_flagUserData) {
				Base::freeMemory(elements);
			}
			elements = sl_null;
		}
		sign = 1;
		length = 0;
	}


	CBigInt::CBigInt() noexcept
	{
		sign = 1;
		length = 0;
		elements = sl_null;
		m_flagUserData = sl_false;
	}

	CBigInt::~CBigInt() noexcept
	{
		_free();
	}

	void CBigInt::setUserDataElements(sl_uint32* elements, sl_size n) noexcept
	{
		_free();
		if (n > 0) {
			this->elements = elements;
			this->length = n;
			this->m_flagUserData = sl_true;
		}
	}

	sl_int32 CBigInt::makeNegative() noexcept
	{
		sign = -sign;
		return sign;
	}

	void CBigInt::makeBitwiseNot() noexcept
	{
		if (elements) {
			for (sl_size i = 0; i < length; i++) {
				elements[i] = ~(elements[i]);
			}
		}
	}

	sl_bool CBigInt::getBit(sl_size pos) const noexcept
	{
		if (pos < (length << 5)) {
			return ((elements[pos >> 5] >> (pos & 0x1F)) & 1) != 0;
		} else {
			return sl_false;
		}
	}

	sl_bool CBigInt::setBit(sl_size pos, sl_bool bit) noexcept
	{
		if (growLength((pos + 31) >> 5)) {
			sl_size ni = pos >> 5;
			sl_uint32 nb = (sl_uint32)(pos & 0x1F);
			if (bit) {
				elements[ni] |= (((sl_uint32)(1)) << nb);
			} else {
				elements[ni] &= ~(((sl_uint32)(1)) << nb);
			}
			return sl_true;
		}
		return sl_true;
	}

	sl_size CBigInt::getMostSignificantElements() const noexcept
	{
		if (elements) {
			return priv::bigint::Mse(elements, length);
		}
		return 0;
	}

	sl_size CBigInt::getLeastSignificantElements() const noexcept
	{
		if (elements) {
			return priv::bigint::Lse(elements, length);
		}
		return 0;
	}

	sl_size CBigInt::getMostSignificantBytes() const noexcept
	{
		if (elements) {
			return priv::bigint::MsBytes(elements, length);
		}
		return 0;
	}

	sl_size CBigInt::getLeastSignificantBytes() const noexcept
	{
		if (elements) {
			return priv::bigint::LsBytes(elements, length);
		}
		return 0;
	}

	sl_size CBigInt::getMostSignificantBits() const noexcept
	{
		if (elements) {
			return priv::bigint::MsBits(elements, length);
		}
		return 0;
	}

	sl_size CBigInt::getLeastSignificantBits() const noexcept
	{
		if (elements) {
			return priv::bigint::LsBits(elements, length);
		}
		return 0;
	}

	sl_bool CBigInt::isZero() const noexcept
	{
		return !(getMostSignificantElements());
	}

	sl_bool CBigInt::isNotZero() const noexcept
	{
		return getMostSignificantElements() != 0;
	}

	void CBigInt::setZero() noexcept
	{
		sign = 1;
		if (elements) {
			Base::zeroMemory(elements, length << 2);
		}
	}

	CBigInt* CBigInt::allocate(sl_size length) noexcept
	{
		CBigInt* newObject = new CBigInt;
		if (newObject) {
			if (length > 0) {
				sl_uint32* elements = (sl_uint32*)(Base::createMemory(length * 4));
				if (elements) {
					newObject->m_flagUserData = sl_false;
					newObject->length = length;
					newObject->elements = elements;
					Base::zeroMemory(elements, length * 4);
					return newObject;
				}
				delete newObject;
			} else {
				return newObject;
			}
		}
		return sl_null;
	}

	CBigInt* CBigInt::duplicate(sl_size newLength) const noexcept
	{
		CBigInt* ret = allocate(length);
		if (ret) {
			sl_size n = Math::min(length, newLength);
			if (n > 0) {
				Base::copyMemory(ret->elements, elements, n * 4);
			}
			ret->sign = sign;
			ret->length = newLength;
			return ret;
		}
		return sl_null;
	}

	CBigInt* CBigInt::duplicate() const noexcept
	{
		return duplicate(length);
	}

	CBigInt* CBigInt::duplicateCompact() const noexcept
	{
		return duplicate(getMostSignificantElements());
	}

	sl_bool CBigInt::copyAbsFrom(const CBigInt& other) noexcept
	{
		if (this == &other) {
			return sl_true;
		}
		sl_size n = other.getMostSignificantElements();
		if (growLength(n)) {
			if (other.elements) {
				Base::copyMemory(elements, other.elements, n * 4);
				Base::zeroMemory(elements + n, (length - n) * 4);
			} else {
				setZero();
			}
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool CBigInt::copyFrom(const CBigInt& other) noexcept
	{
		if (copyAbsFrom(other)) {
			sign = other.sign;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::compact() noexcept
	{
		return setLength(getMostSignificantElements());
	}

	sl_bool CBigInt::growLength(sl_size newLength) noexcept
	{
		if (length >= newLength) {
			return sl_true;
		}
		sl_uint32* newData = (sl_uint32*)(Base::createMemory(newLength * 4));
		if (newData) {
			if (elements) {
				Base::copyMemory(newData, elements, length * 4);
				Base::zeroMemory(newData + length, (newLength - length) * 4);
				if (!m_flagUserData) {
					Base::freeMemory(elements);
				}
			} else {
				Base::zeroMemory(newData, newLength * 4);
			}
			m_flagUserData = sl_false;
			length = newLength;
			elements = newData;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool CBigInt::setLength(sl_size newLength) noexcept
	{
		if (length < newLength) {
			return growLength(newLength);
		} else if (length == newLength) {
			return sl_true;
		} else {
			if (newLength) {
				sl_uint32* newData = (sl_uint32*)(Base::createMemory(newLength * 4));
				if (newData) {
					if (elements) {
						Base::copyMemory(newData, elements, newLength * 4);
						if (!m_flagUserData) {
							Base::freeMemory(elements);
						}
					}
					m_flagUserData = sl_false;
					length = newLength;
					elements = newData;
					return sl_true;
				} else {
					return sl_false;
				}
			} else {
				if (elements) {
					if (!m_flagUserData) {
						Base::freeMemory(elements);
					}
				}
				length = 0;
				elements = sl_null;
				return sl_true;
			}
		}
	}

	sl_bool CBigInt::setValueFromElements(const sl_uint32* _data, sl_size n) noexcept
	{
		sl_size nd = getMostSignificantElements();
		if (growLength(n)) {
			sl_size i;
			for (i = 0; i < n; i++) {
				elements[i] = _data[i];
			}
			for (; i < nd; i++) {
				elements[i] = 0;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::setBytesLE(const void* _bytes, sl_size nBytes, sl_bool flagSigned) noexcept
	{
		sl_uint8* bytes = (sl_uint8*)_bytes;
		if (flagSigned && nBytes && (bytes[nBytes-1] & 0x80)) {
			// compact negative
			{
				sl_size n;
				for (n = nBytes; n > 0; n--) {
					if (bytes[n - 1] != 0xff) {
						break;
					}
				}
				if (!(bytes[n - 1] & 0x80)) {
					n++;
				}
				if (!n) {
					n = 1;
				}
				nBytes = n;
			}
			setZero();
			sl_size n = (nBytes + 3) >> 2;
			if (growLength(n)) {
				sl_size i = 0;
				for (; i < nBytes; i++) {
					elements[i >> 2] |= ((sl_uint32)((sl_uint8)(~(bytes[i])))) << ((i & 3) << 3);
				}
				priv::bigint::Add_uint32(elements, elements, n, 1);
				sign = -1;
				return sl_true;
			} else {
				return sl_true;
			}
		} else {
			// remove zeros
			{
				sl_size n;
				for (n = nBytes; n > 0; n--) {
					if (bytes[n - 1]) {
						break;
					}
				}
				nBytes = n;
			}
			setZero();
			if (nBytes) {
				if (growLength((nBytes + 3) >> 2)) {
					for (sl_size i = 0; i < nBytes; i++) {
						elements[i >> 2] |= ((sl_uint32)(bytes[i])) << ((i & 3) << 3);
					}
					return sl_true;
				}
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	void CBigInt::setBytesLE(const Memory& mem, sl_bool flagSigned) noexcept
	{
		setBytesLE(mem.getData(), mem.getSize(), flagSigned);
	}

	CBigInt* CBigInt::fromBytesLE(const void* bytes, sl_size nBytes, sl_bool flagSigned) noexcept
	{
		CBigInt* ret = CBigInt::allocate((nBytes + 3) >> 2);
		if (ret) {
			if (ret->setBytesLE(bytes, nBytes, flagSigned)) {
				return ret;
			}
			delete ret;
		}
		return sl_null;
	}

	CBigInt* CBigInt::fromBytesLE(const Memory& mem, sl_bool flagSigned) noexcept
	{
		return fromBytesLE(mem.getData(), mem.getSize(), flagSigned);
	}

	void CBigInt::getBytesLE(void* _bytes, sl_size n, sl_bool flagSigned) const noexcept
	{
		sl_uint8* bytes = (sl_uint8*)_bytes;
		sl_size l = length << 2;
		if (flagSigned && sign < 0) {
			sl_uint8 o = 1;
			if (n <= l) {
				for (sl_size i = 0; i < n; i++) {
					sl_uint8 k = (sl_uint8)((~(elements[i >> 2] >> ((i & 3) << 3))) + o);
					o = k ? 0 : 1;
					bytes[i] = k;
				}
			} else {
				sl_size i = 0;
				for (; i < l; i++) {
					sl_uint8 k = (sl_uint8)((~(elements[i >> 2] >> ((i & 3) << 3))) + o);
					o = k ? 0 : 1;
					bytes[i] = k;
				}
				if (o) {
					for (; i < n; i++) {
						bytes[i] = 0;
					}
				} else {
					for (; i < n; i++) {
						bytes[i] = 0xff;
					}
				}
			}
		} else {
			if (n <= l) {
				for (sl_size i = 0; i < n; i++) {
					bytes[i] = (sl_uint8)(elements[i >> 2] >> ((i & 3) << 3));
				}
			} else {
				sl_size i;
				for (i = 0; i < l; i++) {
					bytes[i] = (sl_uint8)(elements[i >> 2] >> ((i & 3) << 3));
				}
				for (; i < n; i++) {
					bytes[i] = 0;
				}
			}
		}
	}

	Memory CBigInt::getBytesLE(sl_bool flagSigned) const noexcept
	{
		sl_size size = priv::bigint::GetBytesCount(elements, length, flagSigned);
		Memory mem = Memory::create(size);
		if (mem.isNotNull()) {
			sl_uint8* bytes = (sl_uint8*)(mem.getData());
			getBytesLE(bytes, mem.getSize(), flagSigned);
			if (flagSigned && size >= 2 && bytes[size - 1] == 0xff && (bytes[size - 2] & 0x80)) {
				return mem.sub(0, size - 1);
			} else {
				return mem;
			}
		}
		return sl_null;
	}

	sl_bool CBigInt::setBytesBE(const void* _bytes, sl_size nBytes, sl_bool flagSigned) noexcept
	{
		sl_uint8* bytes = (sl_uint8*)_bytes;
		if (flagSigned && nBytes && (*bytes & 0x80)) {
			// compact negative
			{
				sl_size n;
				for (n = 0; n < nBytes; n++) {
					if (bytes[n] != 0xff) {
						break;
					}
				}
				if (!(bytes[n] & 0x80)) {
					n--;
				}
				if (n < nBytes) {
					nBytes -= n;
					bytes += n;
				} else {
					nBytes = 1;
				}
			}
			setZero();
			if (nBytes) {
				sl_size n = (nBytes + 3) >> 2;
				if (growLength(n)) {
					sl_size m = nBytes - 1;
					sl_size i = 0;
					for (; i < nBytes; i++) {
						elements[i >> 2] |= ((sl_uint32)((sl_uint8)(~(bytes[m])))) << ((i & 3) << 3);
						m--;
					}
					priv::bigint::Add_uint32(elements, elements, length, 1);
					sign = -1;
					return sl_true;
				}
			} else {
				return sl_true;
			}
		} else {
			// remove zeros
			{
				sl_size n;
				for (n = 0; n < nBytes; n++) {
					if (bytes[n]) {
						break;
					}
				}
				nBytes -= n;
				bytes += n;
			}
			setZero();
			if (nBytes) {
				if (growLength((nBytes + 3) >> 2)) {
					sl_size m = nBytes - 1;
					for (sl_size i = 0; i < nBytes; i++) {
						elements[i >> 2] |= ((sl_uint32)(bytes[m])) << ((i & 3) << 3);
						m--;
					}
					return sl_true;
				}
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	void CBigInt::setBytesBE(const Memory& mem, sl_bool flagSigned) noexcept
	{
		setBytesBE(mem.getData(), mem.getSize(), flagSigned);
	}

	CBigInt* CBigInt::fromBytesBE(const void* bytes, sl_size nBytes, sl_bool flagSigned) noexcept
	{
		CBigInt* ret = CBigInt::allocate((nBytes + 3) >> 2);
		if (ret) {
			if (ret->setBytesBE(bytes, nBytes, flagSigned)) {
				return ret;
			}
			delete ret;
		}
		return sl_null;
	}

	CBigInt* CBigInt::fromBytesBE(const Memory& mem, sl_bool flagSigned) noexcept
	{
		return fromBytesBE(mem.getData(), mem.getSize(), flagSigned);
	}

	void CBigInt::getBytesBE(void* _bytes, sl_size n, sl_bool flagSigned) const noexcept
	{
		sl_uint8* bytes = (sl_uint8*)_bytes;
		sl_size l = length << 2;
		if (flagSigned && sign < 0) {
			sl_uint8 o = 1;
			if (n <= l) {
				sl_size m = n - 1;
				for (sl_size i = 0; i < n; i++) {
					sl_uint8 k = (sl_uint8)((~(elements[i >> 2] >> ((i & 3) << 3))) + o);
					o = k ? 0 : 1;
					bytes[m] = k;
					m--;
				}
			} else {
				sl_size i;
				sl_size m = n - 1;
				for (i = 0; i < l; i++) {
					sl_uint8 k = (sl_uint8)((~(elements[i >> 2] >> ((i & 3) << 3))) + o);
					o = k ? 0 : 1;
					bytes[m] = k;
					m--;
				}
				if (o) {
					for (; i < n; i++) {
						bytes[m] = 0;
						m--;
					}
				} else {
					for (; i < n; i++) {
						bytes[m] = 0xff;
						m--;
					}
				}
			}
		} else {
			if (n <= l) {
				sl_size m = n - 1;
				for (sl_size i = 0; i < n; i++) {
					bytes[m] = (sl_uint8)(elements[i >> 2] >> ((i & 3) << 3));
					m--;
				}
			} else {
				sl_size i;
				sl_size m = n - 1;
				for (i = 0; i < l; i++) {
					bytes[m] = (sl_uint8)(elements[i >> 2] >> ((i & 3) << 3));
					m--;
				}
				for (; i < n; i++) {
					bytes[m] = 0;
					m--;
				}
			}
		}
	}

	Memory CBigInt::getBytesBE(sl_bool flagSigned) const noexcept
	{
		sl_size size = priv::bigint::GetBytesCount(elements, length, flagSigned);
		Memory mem = Memory::create(size);
		if (mem.isNotNull()) {
			sl_uint8* bytes = (sl_uint8*)(mem.getData());
			getBytesBE(bytes, size, flagSigned);
			if (flagSigned && size >= 2 && *bytes == 0xff && (bytes[1] & 0x80)) {
				return mem.sub(1);
			} else {
				return mem;
			}
		}
		return sl_null;
	}

	sl_bool CBigInt::setValue(sl_int32 v) noexcept
	{
		if (growLength(1)) {
			if (v < 0) {
				elements[0] = -v;
				sign = -1;
			} else {
				elements[0] = v;
				sign = 1;
			}
			Base::zeroMemory(elements + 1, (length - 1) * 4);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	CBigInt* CBigInt::fromInt32(sl_int32 v) noexcept
	{
		CBigInt* ret = allocate(1);
		if (ret) {
			ret->setValue(v);
			return ret;
		}
		return sl_null;
	}

	sl_bool CBigInt::setValue(sl_uint32 v) noexcept
	{
		if (growLength(1)) {
			sign = 1;
			elements[0] = v;
			Base::zeroMemory(elements + 1, (length - 1) * 4);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	CBigInt* CBigInt::fromUint32(sl_uint32 v) noexcept
	{
		CBigInt* ret = allocate(1);
		if (ret) {
			ret->setValue(v);
			return ret;
		}
		return sl_null;
	}

	sl_bool CBigInt::setValue(sl_int64 v) noexcept
	{
		if (growLength(2)) {
			sl_uint64 _v;
			if (v < 0) {
				_v = v;
				sign = -1;
			} else {
				_v = v;
				sign = 1;
			}
			elements[0] = (sl_uint32)(_v);
			elements[1] = (sl_uint32)(_v >> 32);
			Base::zeroMemory(elements + 2, (length - 2) * 4);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	CBigInt* CBigInt::fromInt64(sl_int64 v) noexcept
	{
		CBigInt* ret = allocate(2);
		if (ret) {
			ret->setValue(v);
			return ret;
		}
		return sl_null;
	}

	sl_bool CBigInt::setValue(sl_uint64 v) noexcept
	{
		if (growLength(2)) {
			sign = 1;
			elements[0] = (sl_uint32)(v);
			elements[1] = (sl_uint32)(v >> 32);
			Base::zeroMemory(elements + 2, (length - 2) * 4);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	CBigInt* CBigInt::fromUint64(sl_uint64 v) noexcept
	{
		CBigInt* ret = allocate(2);
		if (ret) {
			ret->setValue(v);
			return ret;
		}
		return sl_null;
	}
	
	sl_int32 CBigInt::getInt32() const noexcept
	{
		if (length) {
			sl_int32 v = elements[0] & 0x7FFFFFFF;
			if (sign > 0) {
				return v;
			} else {
				return -v;
			}
		}
		return 0;
	}
	
	sl_uint32 CBigInt::getUint32() const noexcept
	{
		if (length) {
			return elements[0];
		}
		return 0;
	}
	
	sl_int64 CBigInt::getInt64() const noexcept
	{
		if (length) {
			sl_int64 v = elements[0];
			if (length > 1) {
				v |= ((sl_uint64)elements[1]) << 32;
			}
			if (sign > 0) {
				return v;
			} else {
				return -v;
			}
		}
		return 0;
	}
	
	sl_uint64 CBigInt::getUint64() const noexcept
	{
		if (length) {
			sl_uint64 v = elements[0];
			if (length > 1) {
				v |= ((sl_uint64)elements[1]) << 32;
			}
			return v;
		}
		return 0;
	}

	float CBigInt::getFloat() const noexcept
	{
		return (float)(getDouble());
	}

	double CBigInt::getDouble() const noexcept
	{
		if (length) {
			double ret = (double)(elements[0]);
			const double p = (double)0x10000 * (double)0x10000;
			double k = p;
			for (sl_size i = 1; i < length; i++) {
				sl_uint32 n = elements[i];
				if (n) {
					ret += k * (double)n;
				}
				k *= p;
			}
			return ret;
		}
		return 0;
	}

	String CBigInt::toString(sl_uint32 radix, sl_bool flagUpperCase) const noexcept
	{
		if (radix < 2 || radix > 64) {
			return sl_null;
		}
		sl_size nb = getMostSignificantBits();
		if (!nb) {
			SLIB_RETURN_STRING("0");
		}
		if (radix == 16) {
			sl_size nh = (nb + 3) >> 2;
			sl_size ns;
			if (sign < 0) {
				ns = nh + 1;
			} else {
				ns = nh;
			}
			String ret = String::allocate(ns);
			if (ret.isNotNull()) {
				sl_char8* buf = ret.getData();
				if (sign < 0) {
					buf[0] = '-';
					buf++;
				}
				sl_size ih = nh - 1;
				for (sl_size i = 0; i < nh; i++) {
					sl_size ie = ih >> 3;
					sl_uint32 ib = (sl_uint32)((ih << 2) & 31);
					sl_uint32 vh = (sl_uint32)((elements[ie] >> ib) & 15);
					if (vh < 10) {
						buf[i] = (sl_char8)(vh + 0x30);
					} else {
						buf[i] = (sl_char8)(vh + (flagUpperCase ? 0x37 : 0x57));
					}
					ih--;
				}
			}
			return ret;
		} else {
			const char* pattern = flagUpperCase ? priv::string::g_conv_radixPatternUpper : priv::string::g_conv_radixPatternLower;
			sl_size ne = (nb + 31) >> 5;
			sl_size n = (sl_size)(Math::ceil((nb + 1) / Math::log2((double)radix))) + 1;
			SLIB_SCOPED_BUFFER(sl_uint32, STACK_BUFFER_SIZE, a, ne);
			if (!a) {
				return sl_null;
			}
			SLIB_SCOPED_BUFFER(sl_char8, STACK_BUFFER_SIZE, s, n + 2);
			if (!s) {
				return sl_null;
			}
			s = s + n;
			s[1] = 0;
			Base::copyMemory(a, elements, ne * 4);
			sl_size l = 0;
			for (; ne > 0;) {
				sl_uint32 v = priv::bigint::Div_uint32(a, a, ne, radix, 0);
				ne = priv::bigint::Mse(a, ne);
				if (v < radix) {
					*s = pattern[v];
				} else {
					*s = '?';
				}
				s--;
				l++;
			}
			if (sign < 0) {
				*s = '-';
				s--;
				l++;
			}
			return String(s + 1, l);
		}
	}

	String CBigInt::toString()
	{
		return toString(10);
	}

	String CBigInt::toHexString(sl_bool flagUpperCase) const noexcept
	{
		return toString(16, flagUpperCase);
	}

	sl_bool CBigInt::equals(const CBigInt& other) const noexcept
	{
		const CBigInt& a = *this;
		const CBigInt& b = other;
		if (a.sign != b.sign) {
			return sl_false;
		}
		sl_uint32* pa = a.elements;
		sl_uint32* pb = b.elements;
		sl_size na = a.length;
		sl_size nb = b.length;
		sl_uint32* pc;
		sl_size n1, n2;
		if (na > nb) {
			n1 = nb;
			n2 = na;
			pc = pa;
		} else {
			n1 = na;
			n2 = nb;
			pc = pb;
		}
		sl_size i;
		for (i = 0; i < n1; i++) {
			if (pa[i] != pb[i]) {
				return sl_false;
			}
		}
		for (; i < n2; i++) {
			if (pc[i]) {
				return sl_false;
			}
		}
		return sl_true;
	}
	
	sl_bool CBigInt::equals(sl_int32 v) const noexcept
	{
		if (!v) {
			return isZero();
		}
		const CBigInt& a = *this;
		if (v < 0) {
			if (a.sign > 0) {
				return sl_false;
			}
			v = -v;
		} else {
			if (a.sign < 0) {
				return sl_false;
			}
		}
		sl_uint32* p = a.elements;
		sl_size n = a.length;
		if (n < 1) {
			return sl_false;
		}
		if (p[0] != v) {
			return sl_false;
		}
		for (sl_size i = 1; i < n; i++) {
			if (p[i]) {
				return sl_false;
			}
		}
		return sl_true;
	}
	
	sl_bool CBigInt::equals(sl_uint32 v) const noexcept
	{
		if (!v) {
			return isZero();
		}
		const CBigInt& a = *this;
		if (a.sign < 0) {
			return sl_false;
		}
		sl_uint32* p = a.elements;
		sl_size n = a.length;
		if (n < 1) {
			return sl_false;
		}
		if (p[0] != v) {
			return sl_false;
		}
		for (sl_size i = 1; i < n; i++) {
			if (p[i]) {
				return sl_false;
			}
		}
		return sl_true;
	}
	
	sl_bool CBigInt::equals(sl_int64 v) const noexcept
	{
		if (!v) {
			return isZero();
		}
		const CBigInt& a = *this;
		if (v < 0) {
			if (a.sign > 0) {
				return sl_false;
			}
			v = -v;
		} else {
			if (a.sign < 0) {
				return sl_false;
			}
		}
		sl_uint32 vl = (sl_uint32)v;
		sl_uint32 vh = (sl_uint32)(v >> 32);
		sl_uint32* p = a.elements;
		sl_size n = a.length;
		if (vh) {
			if (n < 2) {
				return sl_false;
			}
			if (p[0] != vl) {
				return sl_false;
			}
			if (p[1] != vh) {
				return sl_false;
			}
			for (sl_size i = 2; i < n; i++) {
				if (p[i]) {
					return sl_false;
				}
			}
		} else {
			if (n < 1) {
				return sl_false;
			}
			if (p[0] != vl) {
				return sl_false;
			}
			for (sl_size i = 1; i < n; i++) {
				if (p[i]) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}
	
	sl_bool CBigInt::equals(sl_uint64 v) const noexcept
	{
		if (!v) {
			return isZero();
		}
		const CBigInt& a = *this;
		if (a.sign < 0) {
			return sl_false;
		}
		sl_uint32 vl = (sl_uint32)v;
		sl_uint32 vh = (sl_uint32)(v >> 32);
		sl_uint32* p = a.elements;
		sl_size n = a.length;
		if (vh) {
			if (n < 2) {
				return sl_false;
			}
			if (p[0] != vl) {
				return sl_false;
			}
			if (p[1] != vh) {
				return sl_false;
			}
			for (sl_size i = 2; i < n; i++) {
				if (p[i]) {
					return sl_false;
				}
			}
		} else {
			if (n < 1) {
				return sl_false;
			}
			if (p[0] != vl) {
				return sl_false;
			}
			for (sl_size i = 1; i < n; i++) {
				if (p[i]) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}
	
	sl_compare_result CBigInt::compareAbs(const CBigInt& other) const noexcept
	{
		const CBigInt& a = *this;
		const CBigInt& b = other;
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (na > nb) {
			return 1;
		} else if (na < nb) {
			return -1;
		}
		return priv::bigint::Compare(a.elements, b.elements, na);
	}

	sl_compare_result CBigInt::compare(const CBigInt& other) const noexcept
	{
		const CBigInt& a = *this;
		const CBigInt& b = other;
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (!na) {
			if (!nb) {
				return 0;
			} else {
				return -b.sign;
			}
		} else {
			if (!nb) {
				return a.sign;
			}
		}
		if (a.sign >= 0 && b.sign < 0) {
			return 1;
		} else if (a.sign < 0 && b.sign >= 0) {
			return -1;
		}
		if (na > nb) {
			return a.sign;
		} else if (na < nb) {
			return -a.sign;
		}
		return priv::bigint::Compare(a.elements, b.elements, na) * a.sign;
	}

	sl_compare_result CBigInt::compare(sl_int32 v) const noexcept
	{
		CBIGINT_INT32(o, v);
		return compare(o);
	}

	sl_compare_result CBigInt::compare(sl_uint32 v) const noexcept
	{
		CBIGINT_UINT32(o, v);
		return compare(o);
	}

	sl_compare_result CBigInt::compare(sl_int64 v) const noexcept
	{
		CBIGINT_INT64(o, v);
		return compare(o);
	}

	sl_compare_result CBigInt::compare(sl_uint64 v) const noexcept
	{
		CBIGINT_UINT64(o, v);
		return compare(o);
	}

	sl_bool CBigInt::addAbs(const CBigInt& a, const CBigInt& b) noexcept
	{
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (!na) {
			if (nb) {
				return copyAbsFrom(b);
			} else {
				setZero();
				return sl_true;
			}
		}
		if (!nb) {
			return copyAbsFrom(a);
		}
		sl_size nd;
		if (&a == this) {
			nd = na;
		} else if (&b == this) {
			nd = nb;
		} else {
			nd = getMostSignificantElements();
		}
		const CBigInt* _p;
		const CBigInt* _q;
		sl_size np, nq;
		if (na > nb) {
			_p = &b;
			np = nb;
			_q = &a;
			nq = na;
		} else {
			_p = &a;
			np = na;
			_q = &b;
			nq = nb;
		}
		const CBigInt& p = *_p;
		const CBigInt& q = *_q;
		if (growLength(nq)) {
			sl_uint32 of = priv::bigint::Add(elements, q.elements, p.elements, np, 0);
			if (of) {
				of = priv::bigint::Add_uint32(elements + np, q.elements + np, nq - np, of);
				if (of) {
					if (growLength(nq + 1)) {
						elements[nq] = of;
						nq++;
					} else {
						return sl_false;
					}
				}
			} else {
				if (elements != q.elements) {
					Base::copyMemory(elements + np, q.elements + np, (nq - np) * 4);
				}
			}
			for (sl_size i = nq; i < nd; i++) {
				elements[i] = 0;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::addAbs(const CBigInt& a, sl_uint32 v) noexcept
	{
		CBIGINT_UINT32(o, v);
		return addAbs(a, o);
	}

	sl_bool CBigInt::addAbs(const CBigInt& a, sl_uint64 v) noexcept
	{
		CBIGINT_UINT64(o, v);
		return addAbs(a, o);
	}

	sl_bool CBigInt::add(const CBigInt& a, const CBigInt& b) noexcept
	{
		if (a.sign * b.sign < 0) {
			if (a.compareAbs(b) >= 0) {
				sign = a.sign;
				return subAbs(a, b);
			} else {
				sign = - a.sign;
				return subAbs(b, a);
			}
		} else {
			sign = a.sign;
			return addAbs(a, b);
		}
	}

	sl_bool CBigInt::add(const CBigInt& a, sl_int32 v) noexcept
	{
		CBIGINT_INT32(o, v);
		return add(a, o);
	}

	sl_bool CBigInt::add(const CBigInt& a, sl_uint32 v) noexcept
	{
		CBIGINT_UINT32(o, v);
		return add(a, o);
	}

	sl_bool CBigInt::add(const CBigInt& a, sl_int64 v) noexcept
	{
		CBIGINT_INT64(o, v);
		return add(a, o);
	}

	sl_bool CBigInt::add(const CBigInt& a, sl_uint64 v) noexcept
	{
		CBIGINT_UINT64(o, v);
		return add(a, o);
	}

	sl_bool CBigInt::add(const CBigInt& o) noexcept
	{
		return add(*this, o);
	}

	sl_bool CBigInt::add(sl_int32 v) noexcept
	{
		return add(*this, v);
	}

	sl_bool CBigInt::add(sl_uint32 v) noexcept
	{
		return add(*this, v);
	}

	sl_bool CBigInt::add(sl_int64 v) noexcept
	{
		return add(*this, v);
	}

	sl_bool CBigInt::add(sl_uint64 v) noexcept
	{
		return add(*this, v);
	}

	sl_bool CBigInt::subAbs(const CBigInt& a, const CBigInt& b) noexcept
	{
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (!nb) {
			return copyAbsFrom(a);
		}
		if (na < nb) {
			return sl_false;
		}
		sl_size nd;
		if (&a == this) {
			nd = na;
		} else if (&b == this) {
			nd = nb;
			if (!(growLength(na))) {
				return sl_false;
			}
		} else {
			nd = getMostSignificantElements();
			if (!(growLength(na))) {
				return sl_false;
			}
		}
		sl_uint32 of = priv::bigint::Sub(elements, a.elements, b.elements, nb, 0);
		if (of) {
			of = priv::bigint::Sub_uint32(elements + nb, a.elements + nb, na - nb, of);
			if (of) {
				return sl_false;
			}
		} else {
			if (elements != a.elements) {
				Base::copyMemory(elements + nb, a.elements + nb, (na - nb) * 4);
			}
		}
		for (sl_size i = na; i < nd; i++) {
			elements[i] = 0;
		}
		return sl_true;
	}

	sl_bool CBigInt::subAbs(const CBigInt& a, sl_uint32 v) noexcept
	{
		CBIGINT_UINT32(o, v);
		return addAbs(a, o);
	}

	sl_bool CBigInt::subAbs(const CBigInt& a, sl_uint64 v) noexcept
	{
		CBIGINT_UINT64(o, v);
		return addAbs(a, o);
	}

	sl_bool CBigInt::sub(const CBigInt& a, const CBigInt& b) noexcept
	{
		if (a.sign * b.sign > 0) {
			if (a.compareAbs(b) >= 0) {
				sign = a.sign;
				return subAbs(a, b);
			} else {
				sign = - a.sign;
				return subAbs(b, a);
			}
		} else {
			sign = a.sign;
			return addAbs(a, b);
		}
	}

	sl_bool CBigInt::sub(const CBigInt& a, sl_int32 v) noexcept
	{
		CBIGINT_INT32(o, v);
		return sub(a, o);
	}

	sl_bool CBigInt::sub(const CBigInt& a, sl_uint32 v) noexcept
	{
		CBIGINT_UINT32(o, v);
		return sub(a, o);
	}

	sl_bool CBigInt::sub(const CBigInt& a, sl_int64 v) noexcept
	{
		CBIGINT_INT64(o, v);
		return sub(a, o);
	}

	sl_bool CBigInt::sub(const CBigInt& a, sl_uint64 v) noexcept
	{
		CBIGINT_UINT64(o, v);
		return sub(a, o);
	}

	sl_bool CBigInt::sub(const CBigInt& o) noexcept
	{
		return sub(*this, o);
	}

	sl_bool CBigInt::sub(sl_int32 v) noexcept
	{
		return sub(*this, v);
	}

	sl_bool CBigInt::sub(sl_uint32 v) noexcept
	{
		return sub(*this, v);
	}

	sl_bool CBigInt::sub(sl_int64 v) noexcept
	{
		return sub(*this, v);
	}

	sl_bool CBigInt::sub(sl_uint64 v) noexcept
	{
		return sub(*this, v);
	}

	sl_bool CBigInt::mulAbs(const CBigInt& a, const CBigInt& b) noexcept
	{
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (!na || !nb) {
			setZero();
			return sl_true;
		}
		sl_size nd;
		if (&a == this) {
			nd = na;
		} else if (&b == this) {
			nd = nb;
		} else {
			nd = getMostSignificantElements();
		}
		sl_size n = na + nb;
		SLIB_SCOPED_BUFFER(sl_uint64, STACK_BUFFER_SIZE, out, n);
		if (!out) {
			return sl_false;
		}
		Base::zeroMemory(out, 8 * n);
		for (sl_size ib = 0; ib < nb; ib++) {
			for (sl_size ia = 0; ia < na; ia++) {
				sl_uint64 c = a.elements[ia];
				c *= b.elements[ib];
				out[ia + ib] += (sl_uint32)c;
				out[ia + ib + 1] += (sl_uint32)(c >> 32);
			}
		}
		sl_uint32 o = 0;
		sl_size i;
		sl_size m = 0;
		for (i = 0; i < n; i++) {
			sl_uint64 c = out[i] + o;
			sl_uint32 t = (sl_uint32)c;
			out[i] = t;
			if (t) {
				m = i;
			}
			o = (sl_uint32)(c >> 32);
		}
		if (growLength(m + 1)) {
			for (i = 0; i <= m; i++) {
				elements[i] = (sl_uint32)(out[i]);
			}
			for (; i < nd; i++) {
				elements[i] = 0;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::mul(const CBigInt& a, const CBigInt& b) noexcept
	{
		sign = a.sign * b.sign;
		return mulAbs(a, b);
	}

	sl_bool CBigInt::mulAbs(const CBigInt& a, sl_uint32 b) noexcept
	{
		sl_size na = a.getMostSignificantElements();
		if (!na || !b) {
			setZero();
			return sl_true;
		}
		sl_size n = na + 1;
		SLIB_SCOPED_BUFFER(sl_uint32, STACK_BUFFER_SIZE, out, n);
		if (!out) {
			return sl_false;
		}
		sl_uint32 o = priv::bigint::Mul_uint32(out, a.elements, na, b, 0);
		if (!o) {
			n = na;
		} else {
			out[n - 1] = o;
		}
		return setValueFromElements(out, n);
	}

	sl_bool CBigInt::mul(const CBigInt& a, sl_int32 v) noexcept
	{
		if (v < 0) {
			sign = -a.sign;
			v = -v;
		} else {
			sign = a.sign;
		}
		return mulAbs(a, v);
	}

	sl_bool CBigInt::mul(const CBigInt& a, sl_uint32 v) noexcept
	{
		return mulAbs(a, v);
	}

	sl_bool CBigInt::mul(const CBigInt& a, sl_int64 v) noexcept
	{
		CBIGINT_INT64(o, v);
		return mul(a, o);
	}

	sl_bool CBigInt::mul(const CBigInt& a, sl_uint64 v) noexcept
	{
		CBIGINT_UINT64(o, v);
		return mul(a, o);
	}

	sl_bool CBigInt::mul(const CBigInt& o) noexcept
	{
		return mul(*this, o);
	}

	sl_bool CBigInt::mul(sl_int32 v) noexcept
	{
		return mul(*this, v);
	}

	sl_bool CBigInt::mul(sl_uint32 v) noexcept
	{
		return mul(*this, v);
	}

	sl_bool CBigInt::mul(sl_int64 v) noexcept
	{
		return mul(*this, v);
	}

	sl_bool CBigInt::mul(sl_uint64 v) noexcept
	{
		return mul(*this, v);
	}

	sl_bool CBigInt::divAbs(const CBigInt& a, const CBigInt& b, CBigInt* quotient, CBigInt* remainder) noexcept
	{
		sl_size nba = a.getMostSignificantBits();
		sl_size nbb = b.getMostSignificantBits();
		if (!nbb) {
			return sl_false;
		}
		if (!nba) {
			if (remainder) {
				remainder->setZero();
			}
			if (quotient) {
				quotient->setZero();
			}
			return sl_true;
		}
		if (nba < nbb) {
			if (remainder) {
				if (!(remainder->copyAbsFrom(a))) {
					return sl_false;
				}
			}
			if (quotient) {
				quotient->setZero();
			}
			return sl_true;
		}
		sl_size na = (nba + 31) >> 5;
		sl_size nb = (nbb + 31) >> 5;
		sl_size nbc = nba - nbb;
		SLIB_SCOPED_BUFFER(sl_uint32, STACK_BUFFER_SIZE, _tmem, (nb + 1) * 31);
		if (!_tmem) {
			return sl_false;
		}
		sl_uint32* tb[32];
		sl_size tl[32];
		{
			sl_uint32 n = (sl_uint32)(Math::min((sl_size)31, nbc));
			tb[0] = (sl_uint32*)(b.elements);
			tl[0] = nb;
			for (sl_uint32 i = 1; i <= n; i++) {
				tb[i] = _tmem + ((i - 1) * (nb + 1));
				tl[i] = (nbb + i + 31) >> 5;
				sl_uint32 o = priv::bigint::ShiftLeft(tb[i], b.elements, nb, i, 0);
				if (o) {
					tb[i][nb] = o;
				}
			}
		}
		SLIB_SCOPED_BUFFER(sl_uint32, STACK_BUFFER_SIZE, rem, na * 2);
		if (!rem) {
			return sl_false;
		}
		Base::copyMemory(rem, a.elements, na * 4);
		sl_uint32* q = rem + na;
		Base::zeroMemory(q, na * 4);
		sl_size nbr = nba;
		sl_size shift = nbc;
		sl_size nq = 0;
		for (sl_size i = 0; i <= nbc; i++) {
			sl_size se = shift >> 5;
			sl_size sb = shift & 31;
			sl_size nbs = nbb + shift;
			if (nbs < nbr || (nbs == nbr && priv::bigint::Compare(rem + se, tb[sb], tl[sb]) >= 0)) {
				if (priv::bigint::Sub(rem + se, rem + se, tb[sb], tl[sb], 0)) {
					rem[se + tl[sb]] = 0;
				}
				q[se] |= (1 << sb);
				if (!nq) {
					nq = se + 1;
				}
				nbr = priv::bigint::MsBits(rem, se + tl[sb]);
			}
			shift--;
		}
		if (quotient) {
			if (!(quotient->setValueFromElements(q, nq))) {
				return sl_false;
			}
		}
		sl_size nr = (nbr + 31) >> 5;
		if (remainder) {
			if (!(remainder->setValueFromElements(rem, nr))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool CBigInt::divAbs(const CBigInt& a, sl_uint32 b, CBigInt* quotient, sl_uint32* remainder) noexcept
	{
		if (!b) {
			return sl_false;
		}
		sl_size na = a.getMostSignificantElements();
		if (!na) {
			if (remainder) {
				*remainder = 0;
			}
			if (quotient) {
				quotient->setZero();
			}
			return sl_true;
		}
		sl_uint32* q;
		if (quotient) {
			quotient->setZero();
			if (quotient->growLength(na)) {
				q = quotient->elements;
			} else {
				return sl_false;
			}
		} else {
			q = sl_null;
		}
		sl_uint32 r = priv::bigint::Div_uint32(q, a.elements, na, b, 0);
		if (remainder) {
			*remainder = r;
		}
		return sl_true;
	}

	sl_bool CBigInt::div(const CBigInt& a, const CBigInt& b, CBigInt* quotient, CBigInt* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		if (divAbs(a, b, quotient, remainder)) {
			if (quotient) {
				if (a.sign < 0) {
					if (flagNonNegativeRemainder) {
						if (quotient->addAbs(*quotient, (sl_uint32)1)) {
							quotient->sign = -b.sign;
						} else {
							return sl_false;
						}
					} else {
						quotient->sign = -b.sign;
					}
				} else {
					quotient->sign = b.sign;
				}
			}
			if (remainder) {
				if (a.sign < 0) {
					if (flagNonNegativeRemainder) {
						if (remainder->subAbs(b, *remainder)) {
							remainder->sign = 1;
						} else {
							return sl_false;
						}
					} else {
						remainder->sign = -1;
					}
				} else {
					remainder->sign = 1;
				}
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::divInt32(const CBigInt& a, sl_int32 b, CBigInt* quotient, sl_int32* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		sl_int32 s;
		sl_uint32 v;
		sl_uint32 r;
		if (b > 0) {
			v = b;
			s = 1;
		} else {
			v = -b;
			s = -1;
		}
		if (divAbs(a, v, quotient, &r)) {
			if (quotient) {
				if (a.sign < 0) {
					if (flagNonNegativeRemainder) {
						if (quotient->addAbs(*quotient, (sl_uint32)1)) {
							quotient->sign = -s;
						} else {
							return sl_false;
						}
					} else {
						quotient->sign = -s;
					}
				} else {
					quotient->sign = s;
				}
			}
			if (remainder) {
				if (a.sign < 0) {
					if (flagNonNegativeRemainder) {
						*remainder = v - r;
					} else {
						*remainder = -((sl_int32)r);
					}
				} else {
					*remainder = r;
				}
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::divUint32(const CBigInt& a, sl_uint32 b, CBigInt* quotient, sl_uint32* remainder) noexcept
	{
		if (divAbs(a, b, quotient, remainder)) {
			if (quotient) {
				if (a.sign < 0) {
					if (quotient->addAbs(*quotient, (sl_uint32)1)) {
						quotient->sign = -1;
					} else {
						return sl_false;
					}
				} else {
					quotient->sign = 1;
				}
			}
			if (remainder) {
				if (a.sign < 0) {
					*remainder = b - *remainder;
				}
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::divInt64(const CBigInt& a, sl_int64 b, CBigInt* quotient, sl_int64* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		CBIGINT_INT64(o, b);
		CBigInt* r;
		if (remainder) {
			r = new CBigInt;
			if (!r) {
				return sl_false;
			}
		} else {
			r = sl_null;
		}
		if (div(a, o, quotient, r, flagNonNegativeRemainder)) {
			if (remainder) {
				*remainder = r->getInt64();
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::divUint64(const CBigInt& a, sl_uint64 b, CBigInt* quotient, sl_uint64* remainder) noexcept
	{
		CBIGINT_UINT64(o, b);
		CBigInt* r;
		if (remainder) {
			r = new CBigInt;
			if (!r) {
				return sl_false;
			}
		} else {
			r = sl_null;
		}
		if (div(a, o, quotient, r)) {
			if (remainder) {
				*remainder = r->getUint64();
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::bitwiseAnd(const CBigInt& a, const CBigInt& b) noexcept
	{
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (!na || !nb) {
			setZero();
			return sl_true;
		}
		sl_size nd;
		if (&a == this) {
			nd = na;
		} else if (&b == this) {
			nd = nb;
		} else {
			nd = getMostSignificantElements();
		}
		sl_size n = SLIB_MIN(na, nb);
		if (growLength(n)) {
			sl_size i;
			for (i = 0; i < n; i++) {
				elements[i] = a.elements[i] & b.elements[i];
			}
			for (i = n; i < nd; i++) {
				elements[i] = 0;
			}
			sign = a.sign;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::bitwiseXor(const CBigInt& a, const CBigInt& b) noexcept
	{
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (!na) {
			if (nb) {
				sign = b.sign;
				return copyFrom(b);
			} else {
				setZero();
				return sl_true;
			}
		}
		if (!nb) {
			sign = a.sign;
			return copyFrom(a);
		}
		sl_size nd;
		if (&a == this) {
			nd = na;
		} else if (&b == this) {
			nd = nb;
		} else {
			nd = getMostSignificantElements();
		}
		const CBigInt* p;
		const CBigInt* q;
		sl_size np, nq;
		if (na > nb) {
			q = &a;
			nq = na;
			p = &b;
			np = nb;
		} else {
			p = &a;
			np = na;
			q = &b;
			nq = nb;
		}
		if (growLength(nq)) {
			sl_size i;
			for (i = 0; i < np; i++) {
				elements[i] = p->elements[i] ^ q->elements[i];
			}
			for (i = np; i < nq; i++) {
				elements[i] = q->elements[i];
			}
			for (i = nq; i < nd; i++) {
				elements[i] = 0;
			}
			sign = a.sign;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool CBigInt::bitwiseOr(const CBigInt& a, const CBigInt& b) noexcept
	{
		sl_size na = a.getMostSignificantElements();
		sl_size nb = b.getMostSignificantElements();
		if (!na) {
			if (nb) {
				return copyFrom(b);
			} else {
				setZero();
				return sl_true;
			}
		}
		if (!nb) {
			return copyFrom(a);
		}
		sl_size nd;
		if (&a == this) {
			nd = na;
		} else if (&b == this) {
			nd = nb;
		} else {
			nd = getMostSignificantElements();
		}
		const CBigInt* p;
		const CBigInt* q;
		sl_size np, nq;
		if (na > nb) {
			q = &a;
			nq = na;
			p = &b;
			np = nb;
		} else {
			p = &a;
			np = na;
			q = &b;
			nq = nb;
		}
		if (growLength(nq)) {
			sl_size i;
			for (i = 0; i < np; i++) {
				elements[i] = p->elements[i] | q->elements[i];
			}
			for (i = np; i < nq; i++) {
				elements[i] = q->elements[i];
			}
			for (i = nq; i < nd; i++) {
				elements[i] = 0;
			}
			sign = a.sign;
			return sl_true;
		}
		return sl_false;
	}

#define DEFINE_CBIGINT_BITWISE_FUNCTIONS(FUNCTION) \
	sl_bool CBigInt::FUNCTION(const CBigInt& a, sl_uint32 v) noexcept \
	{ \
		CBIGINT_UINT32(o, v); \
		return FUNCTION(a, o); \
	} \
	sl_bool CBigInt::FUNCTION(const CBigInt& a, sl_uint64 v) noexcept \
	{ \
		CBIGINT_UINT64(o, v); \
		return FUNCTION(a, o); \
	} \
	sl_bool CBigInt::FUNCTION(const CBigInt& o) noexcept \
	{ \
		return FUNCTION(*this, o); \
	} \
	sl_bool CBigInt::FUNCTION(sl_uint32 v) noexcept \
	{ \
		return FUNCTION(*this, v); \
	} \
	sl_bool CBigInt::FUNCTION(sl_uint64 v) noexcept \
	{ \
		return FUNCTION(*this, v); \
	}

	DEFINE_CBIGINT_BITWISE_FUNCTIONS(bitwiseAnd)
	DEFINE_CBIGINT_BITWISE_FUNCTIONS(bitwiseXor)
	DEFINE_CBIGINT_BITWISE_FUNCTIONS(bitwiseOr)

	sl_bool CBigInt::shiftLeft(const CBigInt& a, sl_size shift) noexcept
	{
		if (!shift) {
			return copyFrom(a);
		}
		sl_size nba = a.getMostSignificantBits();
		sl_size nd;
		if (&a == this) {
			nd = (nba + 31) >> 5;
		} else {
			sign = a.sign;
			nd = getMostSignificantElements();
		}
		sl_size nbt = nba + shift;
		sl_size nt = (nbt + 31) >> 5;
		if (growLength(nt)) {
			sl_size se = shift >> 5;
			sl_uint32 sb = (sl_uint32)(shift & 31);
			if (se > 0 || elements != a.elements) {
				sl_size i;
				for (i = nt; i > se; i--) {
					elements[i - 1] = a.elements[i - 1 - se];
				}
				for (; i > 0; i--) {
					elements[i - 1] = 0;
				}
			}
			if (sb > 0) {
				priv::bigint::ShiftLeft(elements, elements, nt, sb, 0);
			}
			for (sl_size i = nt; i < nd; i++) {
				elements[i] = 0;
			}
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool CBigInt::shiftRight(const CBigInt& a, sl_size shift) noexcept
	{
		if (!shift) {
			return copyFrom(a);
		}
		sl_size nba = a.getMostSignificantBits();
		if (nba <= shift) {
			setZero();
			return sl_true;
		}
		sl_size nd;
		if (&a == this) {
			nd = (nba + 31) >> 5;
		} else {
			sign = a.sign;
			nd = getMostSignificantElements();
		}
		sl_size nbt = nba - shift;
		sl_size nt = (nbt + 31) >> 5;
		if (growLength(nt)) {
			sl_size se = shift >> 5;
			sl_uint32 sb = (sl_uint32)(shift & 31);
			if (se > 0 || elements != a.elements) {
				sl_size i;
				for (i = 0; i < nt; i++) {
					elements[i] = a.elements[i + se];
				}
			}
			if (sb > 0) {
				sl_uint32 l;
				if (nt + se < a.length) {
					l = a.elements[nt + se];
				} else {
					l = 0;
				}
				priv::bigint::ShiftRight(elements, elements, nt, sb, l);
			}
			for (sl_size i = nt; i < nd; i++) {
				elements[i] = 0;
			}
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool CBigInt::shiftLeft(sl_size n) noexcept
	{
		return shiftLeft(*this, n);
	}

	sl_bool CBigInt::shiftRight(sl_size n) noexcept
	{
		return shiftRight(*this, n);
	}

	sl_bool CBigInt::pow(const CBigInt& A, const CBigInt& E, const CBigInt* pM) noexcept
	{
		if (pM) {
			sl_size nM = pM->getMostSignificantElements();
			if (!nM) {
				return sl_false;
			}
		}
		sl_size nbE = E.getMostSignificantBits();
		if (!nbE) {
			if (!(setValue((sl_uint32)1))) {
				return sl_false;
			}
			sign = 1;
			return sl_true;
		}
		if (E.sign < 0) {
			return sl_false;
		}
		sl_size nA = A.getMostSignificantElements();
		if (!nA) {
			setZero();
			return sl_true;
		}
		CBigInt T;
		if (!(T.copyFrom(A))) {
			return sl_false;
		}
		const CBigInt* TE;
		CBigInt t_TE;
		if (this == &E) {
			TE = &t_TE;
			if (!(t_TE.copyFrom(E))) {
				return sl_false;
			}
		} else {
			TE = &E;
		}
		if (!(setValue((sl_uint32)1))) {
			return sl_false;
		}
		for (sl_size ib = 0; ib < nbE; ib++) {
			sl_size ke = ib >> 5;
			sl_uint32 kb = (sl_uint32)(ib & 31);
			if (((TE->elements[ke]) >> kb) & 1) {
				if (!(mul(*this, T))) {
					return sl_false;
				}
				if (pM) {
					if (!(CBigInt::div(*this, *pM, sl_null, this, sl_true))) {
						return sl_false;
					}
				}
			}
			if (!(T.mul(T, T))) {
				return sl_false;
			}
			if (pM) {
				if (!(CBigInt::div(T, *pM, sl_null, &T, sl_true))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	sl_bool CBigInt::pow_mod(const CBigInt& A, const CBigInt& E, const CBigInt& M) noexcept
	{
		return pow(A, E, &M);
	}

	sl_bool CBigInt::pow_mod(const CBigInt& E, const CBigInt& M) noexcept
	{
		return pow(*this, E, &M);
	}

	sl_bool CBigInt::pow(const CBigInt& E) noexcept
	{
		return pow(*this, E);
	}

	sl_bool CBigInt::pow(const CBigInt& A, sl_uint32 E, const CBigInt* pM) noexcept
	{
		CBIGINT_UINT32(o, E);
		return pow(A, o, pM);
	}

	sl_bool CBigInt::pow_mod(const CBigInt& A, sl_uint32 E, const CBigInt& M) noexcept
	{
		return pow(A, E, &M);
	}

	sl_bool CBigInt::pow_mod(sl_uint32 E, const CBigInt& M) noexcept
	{
		return pow(*this, E, &M);
	}

	sl_bool CBigInt::pow(sl_uint32 E) noexcept
	{
		return pow(*this, E);
	}

	namespace priv
	{
		namespace bigint
		{
			
			// Montgomery multiplication: A = A * B * R^-1 mod M
			static sl_bool mont_mul(CBigInt& A, const CBigInt& B, const CBigInt& M, sl_uint32 MI) noexcept
			{
				sl_size nM = M.length;
				sl_size nB = Math::min(nM, B.length);
				
				sl_size nOut = nM * 2 + 1;
				SLIB_SCOPED_BUFFER(sl_uint32, STACK_BUFFER_SIZE, out, nOut);
				if (!out) {
					return sl_false;
				}
				Base::zeroMemory(out, nOut * 4);
				for (sl_size i = 0; i < nM; i++) {
					// T = (T + cB*B + cM*M) / 2^(32*nM)
					sl_uint32 cB = i < A.length ? A.elements[i] : 0;
					sl_uint32 cM = (out[0] + cB * B.elements[0]) * MI;
					priv::bigint::MulAdd_uint32(out, out, nOut, B.elements, nB, cB, 0);
					priv::bigint::MulAdd_uint32(out, out, nOut, M.elements, nM, cM, 0);
					*out = cB;
					nOut--;
					out++;
				}
				if (!(A.setValueFromElements(out, nM + 1))) {
					return sl_false;
				}
				if (A.compareAbs(M) >= 0) {
					if (!(A.subAbs(A, M))) {
						return sl_false;
					}
				}
				return sl_true;
			}
			
			// Montgomery reduction: A = A * R^-1 mod M
			SLIB_INLINE static sl_bool mont_reduction(CBigInt& A, const CBigInt& M, sl_uint32 MI) noexcept
			{
				CBIGINT_UINT32(o, 1);
				return priv::bigint::mont_mul(A, o, M, MI);
			}
			
			struct PowMontgomeryContext
			{
				CBigInt M;
				CBigInt R2;
				CBigInt T;
			};
			
			static sl_bool pow_montgomery(priv::bigint::PowMontgomeryContext& context, CBigInt& ret, const CBigInt& A, const CBigInt& inE, const CBigInt& inM) noexcept
			{
				CBigInt& M = context.M;
				CBigInt& R2 = context.R2;
				CBigInt& T = context.T;
				
				M.copyFrom(inM);
				if (!(M.compact())) {
					return sl_false;
				}
				sl_size nM = M.getMostSignificantElements();
				if (!nM) {
					return sl_false;
				}
				if (M.sign < 0) {
					return sl_false;
				}
				const CBigInt* pE;
				CBigInt _t_E;
				if (&inE == &ret) {
					pE = &_t_E;
					if (!(_t_E.copyFrom(inE))) {
						return sl_false;
					}
				} else {
					pE = &inE;
				}
				const CBigInt& E = *pE;
				sl_size nE = E.getMostSignificantElements();
				if (!nE) {
					if (!(ret.setValue((sl_uint32)1))) {
						return sl_false;
					}
					ret.sign = 1;
					return sl_true;
				}
				if (E.sign < 0) {
					return sl_false;
				}
				sl_size nA = A.getMostSignificantElements();
				if (!nA) {
					ret.setZero();
					return sl_true;
				}
				
				// MI = -(M0^-1) mod (2^32)
				sl_uint32 MI;
				// initialize montgomery
				{
					sl_uint32 M0 = M.elements[0];
					sl_uint32 K = M0;
					K += ((M0 + 2) & 4) << 1;
					for (sl_uint32 i = 32; i >= 8; i /= 2) {
						K *= (2 - (M0 * K));
					}
					MI = 0 - K;
				}
				
				// pre-compute R^2 mod M
				// R = 2^(nM*32)
				{
					if (!(R2.setValue((sl_uint32)1))) {
						return sl_false;
					}
					if (!(R2.shiftLeft(nM * 64))) {
						return sl_false;
					}
					if (!(CBigInt::divAbs(R2, M, sl_null, &R2))) {
						return sl_false;
					}
				}
				
				sl_bool flagNegative = A.sign < 0;
				// T = A * R^2 * R^-1 mod M = A * R mod M
				if (!(CBigInt::divAbs(A, M, sl_null, &T))) {
					return sl_false;
				}
				if (!(mont_mul(T, R2, M, MI))) {
					return sl_false;
				}
				
				// C = R^2 * R^-1 mod M = R mod M
				if (!(ret.copyFrom(R2))) {
					return sl_false;
				}
				if (!(mont_reduction(ret, M, MI))) {
					return sl_false;
				}
				
				sl_size nbE = E.getMostSignificantBits();
				for (sl_size ib = 0; ib < nbE; ib++) {
					sl_size ke = ib >> 5;
					sl_uint32 kb = (sl_uint32)(ib & 31);
					if (((E.elements[ke]) >> kb) & 1) {
						// C = C * T * R^-1 mod M
						if (!(mont_mul(ret, T, M, MI))) {
							return sl_false;
						}
					}
					// T = T * T * R^-1 mod M
					if (!(mont_mul(T, T, M, MI))) {
						return sl_false;
					}
				}
				if (!(mont_reduction(ret, M, MI))) {
					return sl_false;
				}
				if (flagNegative && (E.elements[0] & 1) != 0) {
					ret.sign = -1;
					if (!(ret.add(M))) {
						return sl_false;
					}
				} else {
					ret.sign = 1;
				}
				return sl_true;
			}
			
		}
	}

	
	sl_bool CBigInt::pow_montgomery(const CBigInt& A, const CBigInt& inE, const CBigInt& inM) noexcept
	{
		priv::bigint::PowMontgomeryContext context;
		return priv::bigint::pow_montgomery(context, *this, A, inE, inM);
	}
	
	sl_bool CBigInt::pow_montgomery(const CBigInt& E, const CBigInt& M) noexcept
	{
		return pow_montgomery(*this, E, M);
	}

	sl_bool CBigInt::inverseMod(const CBigInt& A, const CBigInt& M) noexcept
	{
		sl_size nM = M.getMostSignificantElements();
		if (!nM) {
			return sl_false;
		}
		if (M.sign < 0) {
			return sl_false;
		}
		sl_size nA = A.getMostSignificantElements();
		if (!nA) {
			return sl_false;
		}
		CBigInt G;
		if (!(G.gcd(A, M))) {
			return sl_false;
		}
		if (!(G.equals((sl_uint32)1))) {
			return sl_false;
		}
		CBigInt Xa;
		if (!(CBigInt::div(A, M, sl_null, &Xa, sl_true))) {
			return sl_false;
		}
		CBigInt Xb;
		if (!(Xb.copyFrom(M))) {
			return sl_false;
		}
		CBigInt T1;
		if (!(T1.copyFrom(Xa))) {
			return sl_false;
		}
		if (!(T1.elements)) {
			return sl_false;
		}
		CBIGINT_INT32(T1a, 1);
		CBIGINT_INT32(T1b, 0);
		CBigInt T2;
		if (!(T2.copyFrom(M))) {
			return sl_false;
		}
		if (!(T2.elements)) {
			return sl_false;
		}
		CBIGINT_INT32(T2a, 0);
		CBIGINT_INT32(T2b, 1);

		for (;;) {
			while (!(T1.elements[0] & 1)) {
				if (!(T1.shiftRight(1))) {
					return sl_false;
				}
				if ((T1a.elements[0] & 1) || (T1b.elements[0] & 1)) {
					if (!(T1a.add(Xb))) {
						return sl_false;
					}
					if (!(T1b.sub(Xa))) {
						return sl_false;
					}
				}
				if (!(T1a.shiftRight(1))) {
					return sl_false;
				}
				if (!(T1b.shiftRight(1))) {
					return sl_false;
				}
			}
			while (!(T2.elements[0] & 1)) {
				if (!(T2.shiftRight(1))) {
					return sl_false;
				}
				if ((T2a.elements[0] & 1) || (T2b.elements[0] & 1)) {
					if (!(T2a.add(Xb))) {
						return sl_false;
					}
					if (!(T2b.sub(Xa))) {
						return sl_false;
					}
				}
				if (!(T2a.shiftRight(1))) {
					return sl_false;
				}
				if (!(T2b.shiftRight(1))) {
					return sl_false;
				}
			}
			if (T1.compare(T2) >= 0) {
				if (!(T1.sub(T2))) {
					return sl_false;
				}
				if (!(T1a.sub(T2a))) {
					return sl_false;
				}
				if (!(T1b.sub(T2b))) {
					return sl_false;
				}
			} else {
				if (!(T2.sub(T1))) {
					return sl_false;
				}
				if (!(T2a.sub(T1a))) {
					return sl_false;
				}
				if (!(T2b.sub(T1b))) {
					return sl_false;
				}
			}
			if (T1.isZero()) {
				break;
			}
		}
		while (T2a.compare((sl_uint32)0) < 0) {
			if (!(T2a.add(M))) {
				return sl_false;
			}
		}
		while (T2a.compare(M) >= 0) {
			if (!(T2a.sub(M))) {
				return sl_false;
			}
		}
		if (!(copyFrom(T2a))) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool CBigInt::inverseMod(const CBigInt& M) noexcept
	{
		return inverseMod(*this, M);
	}

	sl_int32 CBigInt::checkEulerCriterion(const CBigInt& A, const CBigInt& M) const noexcept
	{
		CBigInt g;

		if (g.equals(1)) {
			return 0;
		}

		CBigInt exp;
		exp.copyFrom(M);
		exp.sub(1);
		exp.shiftRight(1);

		CBigInt a;
		a.copyFrom(A);
		a.pow_montgomery(exp, M);

		if (a.compare(1) > 0) {
			return -1;
		}
		return 1;
	}

	sl_int32 CBigInt::checkEulerCriterion(const CBigInt& M) const noexcept
	{
		return checkEulerCriterion(*this, M);
	}

	sl_bool CBigInt::sqrtMod(const CBigInt& inA, const CBigInt& M) noexcept
	{
		CBigInt A;
		A.copyFrom(inA);
		divAbs(A, M, sl_null, &A);

		if (checkEulerCriterion(A, M) != 1) {
			// A is not quadratic residue!!!
			return sl_false;
		}

		CBigInt P;
		P.copyFrom(M);
		P.sub(1);
		sl_size e = P.getLeastSignificantBits();
		if (!e) {
			return sl_false;
		}
		e--;

		if (e == 1) {
			P.shiftRight(2);
			P.add(1);
			return pow_montgomery(inA, P, M);
		}

		if (e == 2) {
			P.add(1); // tmp <- p
			A.shiftLeft(1);
			CBigInt t;
			divAbs(A, P, sl_null, &t);

			CBigInt q;
			q.copyFrom(P);
			q.shiftRight(3);

			CBigInt b;
			b.copyFrom(t);
			b.pow_montgomery(q, P);

			CBigInt y;
			y.copyFrom(b);
			y.mulAbs(y, b);;
			divAbs(y, P, sl_null, &y);

			t.mulAbs(t, y);
			divAbs(t, P, sl_null, &t);
			t.sub(1);

			A.shiftRight(1);
			A.mulAbs(A, b);
			A.mulAbs(A, t);
			return divAbs(A, P, sl_null, this);
		}

		sl_size nBitsM = M.getMostSignificantBits();
		CBigInt q, y;
		q.copyFrom(M);
		sl_int32 r;
		sl_uint32 i = 2;
		do {
			if (i < 22) {
				y.setValue(i);
			} else {
				y.random(nBitsM - 1);
				if (y.isZero()) {
					y.setValue(i);
				}
			}
			r = checkEulerCriterion(y, q);
			i++;
		} while (r == 1 && i < 82);

		q.shiftRight(e);
		
		CBigInt c, tt;
		c.pow_montgomery(y, q, M);
		tt.pow_montgomery(A, q, M);

		CBigInt _q;
		_q.copyFrom(q);
		_q.add(1);
		_q.shiftRight(1);

		CBigInt& R = *this;
		R.pow_montgomery(A, _q, M);

		sl_size k = 0;
		for (;;) {
			if (tt.equals(1)) {
				return sl_true;
			} else {
				for (sl_size j = 1; j < e; j++) {
					CBigInt exp;
					exp.setValue(1);
					exp.shiftLeft(j);
					CBigInt temp;
					temp.pow_montgomery(tt, exp, M);
					if (temp.equals(1)) {
						k = j;
						break;
					}
				}
			}
			
			CBigInt temp;
			temp.setValue(1);
			temp.shiftLeft(e - k - 1);
			CBigInt bb;
			bb.pow_montgomery(c, temp, M);

			e = k;

			c.copyFrom(bb);
			c.mulAbs(c, bb);
			divAbs(c, M, sl_null, &c);

			tt.mulAbs(tt, c);
			divAbs(tt, M, sl_null, &tt);

			R.mulAbs(R, bb);
			divAbs(R, M, sl_null, &R);
		}
	}

	sl_bool CBigInt::sqrtMod(const CBigInt& M) noexcept
	{
		return sqrtMod(*this, M);
	}

	sl_bool CBigInt::gcd(const CBigInt& inA, const CBigInt& inB) noexcept
	{
		if (&inA == &inB) {
			sign = 1;
			return copyAbsFrom(inA);
		}
		sl_size lbA = inA.getLeastSignificantBits();
		sl_size lbB = inB.getLeastSignificantBits();
		if (!lbA || !lbB) {
			setZero();
			return sl_true;
		}
		sl_size min_p2 = Math::min(lbA - 1, lbB - 1);
		CBigInt A, B;
		if (!(A.shiftRight(inA, min_p2))) {
			return sl_false;
		}
		if (!(B.shiftRight(inB, min_p2))) {
			return sl_false;
		}
		for (;;) {
			lbA = A.getLeastSignificantBits();
			if (!lbA) {
				break;
			}
			if (!(A.shiftRight(lbA - 1))) {
				return sl_false;
			}
			lbB = B.getLeastSignificantBits();
			if (!lbB) {
				break;
			}
			if (!(B.shiftRight(lbB - 1))) {
				return sl_false;
			}
			if (A.compareAbs(B) >= 0) {
				if (!(A.subAbs(A, B))) {
					return sl_false;
				}
				if (!(A.shiftRight(1))) {
					return sl_false;
				}
			} else {
				if (!(B.subAbs(B, A))) {
					return sl_false;
				}
				if (!(B.shiftRight(1))) {
					return sl_false;
				}
			}
		}
		if (!(shiftLeft(B, min_p2))) {
			return sl_false;
		}
		sign = 1;
		return sl_true;
	}

	sl_bool CBigInt::gcd(const CBigInt& B) noexcept
	{
		return gcd(*this, B);
	}

	sl_bool CBigInt::lcm(const CBigInt& inA, const CBigInt& inB) noexcept
	{
		if (!(gcd(inA, inB))) {
			return sl_false;
		}
		CBigInt a, b;
		if (!(CBigInt::div(inA, *this, &a, sl_null, sl_true))) {
			return sl_false;
		}
		if (!(CBigInt::div(inB, *this, &b, sl_null, sl_true))) {
			return sl_false;
		}
		if (!(mul(a))) {
			return sl_false;
		}
		if (!(mul(b))) {
			return sl_false;
		}
		return sl_true;
	}
	
	sl_bool CBigInt::lcm(const CBigInt& B) noexcept
	{
		return lcm(*this, B);
	}

	/*
	 				Miller-Rabin primality test
	 
	 	https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
	 	https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf
	 
	*/
	namespace priv
	{
		namespace bigint
		{
			
			struct ProbablePrimeCheckContext
			{
				CBigInt n1;
				CBigInt d;
				CBigInt a;
				CBigInt n3;
				CBigInt x;
				CBigInt y;
				
				priv::bigint::PowMontgomeryContext contextPowMontgomery;
			};
			
			static sl_bool isProbablePrime(priv::bigint::ProbablePrimeCheckContext& context, const CBigInt& n, sl_uint32 nChecks, sl_bool* pFlagError) noexcept
			{
				if (pFlagError) {
					*pFlagError = sl_false;
				}
#define RETURN_ERROR  if (pFlagError)  *pFlagError = sl_true; return sl_false
				CBigInt& n1 = context.n1;
				CBigInt& d = context.d;
				CBigInt& a = context.a;
				CBigInt& n3 = context.n3;
				CBigInt& x = context.x;
				CBigInt& y = context.y;
				
				// n = (2^r) * d + 1
				if (!(n1.sub(n, 1))) {
					RETURN_ERROR;
				}
				sl_size r = n1.getLeastSignificantBits();
				if (r < 2) {
					RETURN_ERROR;
				}
				r--;
				if (!(d.shiftRight(n1, r))) {
					RETURN_ERROR;
				}
				
				sl_size nBits = n.getMostSignificantBits();
				n3.sub(n, 3); // n-3
				
				for (sl_uint32 i = 0; i < nChecks; i++) {
					// find random a in range [2, n-2]   =>   (random % (n-3)) + 2
					if (!(a.random(nBits))) {
						RETURN_ERROR;
					}
					while (a.compare(n3) >= 0) {
						if (!(a.sub(n3))) {
							RETURN_ERROR;
						}
					}
					if (!(a.add(2))) {
						RETURN_ERROR;
					}
					if (!(priv::bigint::pow_montgomery(context.contextPowMontgomery, x, a, d, n))) {
						RETURN_ERROR;
					}
					if (!(x.equals((sl_uint32)1)) && !(x.equals(n1))) { // x = 1 or x = n - 1 => probably prime
						sl_bool flagPrime = sl_false;
						sl_size k = r;
						while (--k) {
							if (!(y.mul(x, x))) {
								RETURN_ERROR;
							}
							if (!(CBigInt::div(y, n, sl_null, &x, sl_true))) {
								RETURN_ERROR;
							}
							if (x.equals((sl_uint32)1)) {
								// multiple of gcd((a^d mod n) - 1, n)
								return sl_false;
							}
							if (x.equals(n1)) {
								flagPrime = sl_true;
								break;
							}
						}
						if (!flagPrime) {
							// composite
							return sl_false;
						}
					}
				}
				return sl_true;
#undef RETURN_ERROR
			}
			
			// referenced from OpenSSL - https://github.com/openssl/openssl/blob/master/include/openssl/bn.h
			static sl_uint32 getDefaultCheckPrimeCounts(sl_size nBits) noexcept
			{
				if (nBits >= 3747) {
					return 3;
				} else if (nBits >= 1345) {
					return 4;
				} else if (nBits >= 476) {
					return 5;
				} else if (nBits >= 400) {
					return 6;
				} else if (nBits >= 347) {
					return 7;
				} else if (nBits >= 308) {
					return 8;
				} else if (nBits >= 55) {
					return 27;
				} else {
					return 34;
				}
			}
		}
	}
	
	sl_bool CBigInt::isProbablePrime(sl_uint32 nChecks, sl_bool* pFlagError) const noexcept
	{
		if (pFlagError) {
			*pFlagError = sl_false;
		}
		if (sign < 0) {
			return sl_false;
		}
		sl_size n = getMostSignificantElements();
		if (!n) {
			return sl_false;
		}
		sl_uint32 e0 = elements[0];
		if (!(e0 & 1)) {
			return sl_false;
		}
		if (n == 1 && e0 < 4) {
			if (e0 >= 2) {
				return sl_true;
			}
			return sl_false;
		}
		if (nChecks < 1) {
			nChecks = priv::bigint::getDefaultCheckPrimeCounts(getMostSignificantBits());
		}
		priv::bigint::ProbablePrimeCheckContext context;
		return priv::bigint::isProbablePrime(context, *this, nChecks, pFlagError);
	}
	
	sl_bool CBigInt::generatePrime(sl_size nBits) noexcept
	{
		if (nBits < 3) {
			return sl_false;
		}
		sl_uint32 nChecks = priv::bigint::getDefaultCheckPrimeCounts(nBits);
		priv::bigint::ProbablePrimeCheckContext contextCheckPrime;
		for (;;) {
			if (!(random(nBits))) {
				return sl_false;
			}
			elements[0] |= 1;
			if (!(setBit(nBits - 1, sl_true))) {
				return sl_false;
			}
			sl_bool flagError = sl_false;
			if (priv::bigint::isProbablePrime(contextCheckPrime, *this, nChecks, &flagError)) {
				break;
			}
			if (flagError) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool CBigInt::random(sl_size nBits) noexcept
	{
		if (!nBits) {
			setZero();
			return sl_true;
		}
		sl_size nElements = nBits >> 5;
		sl_uint32 nFrontBits = (sl_uint32)(nBits & 31);
		if (nFrontBits) {
			nElements++;
		}
		if (growLength(nElements)) {
			Math::randomMemory(elements, nElements << 2);
			if (nFrontBits) {
				elements[nElements - 1] &= (((sl_uint32)0xFFFFFFFF) >> (32 - nFrontBits));
			}
			for (sl_size i = nElements; i < length; i++) {
				elements[i] = 0;
			}
			return sl_true;
		}
		return sl_false;
	}
	
	sl_size CBigInt::getHashCode() const noexcept
	{
		if (length) {
			return HashBytes(elements, length<<2);
		}
		return 0;
	}

	sl_bool CBigInt::runOperator(sl_uint32 op, Variant& result, const Variant& arg, sl_bool flagThisOnLeft)
	{
#define BIGINT_RUN_UNARY_OP(OP_NAME, OP) \
		case ObjectOperator::OP_NAME: \
			{ \
				CBigInt* thiz = this; \
				result = OP(*((BigInt*)((void*)&thiz))); \
				return sl_true; \
			}

#define BIGINT_RUN_BINARY_OP(OP_NAME, OP) \
		case ObjectOperator::OP_NAME: \
			{ \
				BigInt n = arg.getBigInt(); \
				CBigInt* thiz = this; \
				if (flagThisOnLeft) { \
					result = *((BigInt*)((void*)&thiz)) OP n; \
				} else { \
					result = n OP *((BigInt*)((void*)&thiz)); \
				} \
				return sl_true; \
			}

#define BIGINT_RUN_SHIFT_OP(OP_NAME, OP) \
		case ObjectOperator::OP_NAME: \
			{ \
				sl_uint32 n = arg.getUint32(); \
				CBigInt* thiz = this; \
				if (flagThisOnLeft) { \
					result = *((BigInt*)((void*)&thiz)) OP n; \
					return sl_true; \
				} \
			}

		switch (op) {
			BIGINT_RUN_UNARY_OP(UnaryMinus, -)
			BIGINT_RUN_UNARY_OP(LogicalNot, !)
			BIGINT_RUN_UNARY_OP(BitwiseNot, ~)
			BIGINT_RUN_BINARY_OP(Multiply, *)
			BIGINT_RUN_BINARY_OP(Divide, /)
			BIGINT_RUN_BINARY_OP(Remainder, %)
			BIGINT_RUN_BINARY_OP(Add, +)
			BIGINT_RUN_BINARY_OP(Subtract, -)
			BIGINT_RUN_SHIFT_OP(ShiftLeft, <<)
			BIGINT_RUN_SHIFT_OP(ShiftRight, >>)
			case ObjectOperator::Compare:
				{
					BigInt n = arg.getBigInt();
					CBigInt* thiz = this;
					if (flagThisOnLeft) {
						result = ((BigInt*)((void*)&thiz))->compare(n);
					} else {
						result = n.compare(*((BigInt*)((void*)&thiz)));
					}
					return sl_true;
				}
			BIGINT_RUN_BINARY_OP(Equals, ==)
			BIGINT_RUN_BINARY_OP(BitwiseAnd, &)
			BIGINT_RUN_BINARY_OP(BitwiseXor, ^)
			BIGINT_RUN_BINARY_OP(BitwiseOr, |)
		}
		return sl_false;
	}


	BigInt::BigInt(sl_int32 n) noexcept : ref(CBigInt::fromInt32(n))
	{
	}

	BigInt::BigInt(sl_uint32 n) noexcept : ref(CBigInt::fromUint32(n))
	{
	}

	BigInt::BigInt(sl_int64 n) noexcept : ref(CBigInt::fromInt64(n))
	{
	}

	BigInt::BigInt(sl_uint64 n) noexcept : ref(CBigInt::fromUint64(n))
	{
	}

	BigInt::~BigInt() noexcept
	{
	}

	BigInt BigInt::fromInt32(sl_int32 v) noexcept
	{
		return CBigInt::fromInt32(v);
	}

	BigInt BigInt::fromUint32(sl_uint32 v) noexcept
	{
		return CBigInt::fromUint32(v);
	}

	BigInt BigInt::fromInt64(sl_int64 v) noexcept
	{
		return CBigInt::fromInt64(v);
	}

	BigInt BigInt::fromUint64(sl_uint64 v) noexcept
	{
		return CBigInt::fromUint64(v);
	}

	BigInt BigInt::fromBytesLE(const void* bytes, sl_size nBytes, sl_bool flagSigned) noexcept
	{
		return CBigInt::fromBytesLE(bytes, nBytes, flagSigned);
	}

	BigInt BigInt::fromBytesLE(const Memory& mem, sl_bool flagSigned) noexcept
	{
		return CBigInt::fromBytesLE(mem.getData(), mem.getSize(), flagSigned);
	}

	BigInt BigInt::fromBytesBE(const void* bytes, sl_size nBytes, sl_bool flagSigned) noexcept
	{
		return CBigInt::fromBytesBE(bytes, nBytes, flagSigned);
	}

	BigInt BigInt::fromBytesBE(const Memory& mem, sl_bool flagSigned) noexcept
	{
		return CBigInt::fromBytesBE(mem.getData(), mem.getSize(), flagSigned);
	}

	BigInt BigInt::fromString(const StringParam& str, sl_uint32 radix) noexcept
	{
		BigInt n;
		if (n.parse(str, radix)) {
			return n;
		}
		return sl_null;
	}
	
	BigInt BigInt::fromHexString(const StringParam& str) noexcept
	{
		return fromString(str, 16);
	}

	CBigInt& BigInt::instance() const noexcept
	{
		return *ref;
	}

	BigInt BigInt::duplicate() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->duplicate();
		}
		return sl_null;
	}

	BigInt BigInt::compact() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->duplicateCompact();
		}
		return sl_null;
	}

	sl_size BigInt::getElementsCount() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->length;
		}
		return 0;
	}

	sl_uint32* BigInt::getElements() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->elements;
		}
		return sl_null;
	}

	sl_int32 BigInt::getSign() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->sign;
		}
		return 1;
	}

	sl_bool BigInt::getBit(sl_uint32 pos) const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getBit(pos);
		}
		return sl_false;
	}

	sl_size BigInt::getMostSignificantElements() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getMostSignificantElements();
		}
		return 0;
	}

	sl_size BigInt::getLeastSignificantElements() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getLeastSignificantElements();
		}
		return 0;
	}

	sl_size BigInt::getMostSignificantBytes() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getMostSignificantBytes();
		}
		return 0;
	}

	sl_size BigInt::getLeastSignificantBytes() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getLeastSignificantBytes();
		}
		return 0;
	}

	sl_size BigInt::getMostSignificantBits() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getMostSignificantBits();
		}
		return 0;
	}

	sl_size BigInt::getLeastSignificantBits() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getLeastSignificantBits();
		}
		return 0;
	}

	sl_bool BigInt::isZero() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->isZero();
		}
		return sl_true;
	}

	sl_bool BigInt::isNotZero() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->isNotZero();
		}
		return sl_false;
	}

	void BigInt::getBytesLE(void* buf, sl_size n, sl_bool flagSigned) const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			o->getBytesLE(buf, n, flagSigned);
		} else {
			Base::zeroMemory(buf, n);
		}
	}

	Memory BigInt::getBytesLE(sl_bool flagSigned) const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getBytesLE(flagSigned);
		}
		return sl_null;
	}

	void BigInt::getBytesBE(void* buf, sl_size n, sl_bool flagSigned) const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			o->getBytesBE(buf, n, flagSigned);
		} else {
			Base::zeroMemory(buf, n);
		}
	}

	Memory BigInt::getBytesBE(sl_bool flagSigned) const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getBytesBE(flagSigned);
		}
		return sl_null;
	}

	sl_int32 BigInt::getInt32() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getInt32();
		}
		return 0;
	}

	sl_uint32 BigInt::getUint32() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getUint32();
		}
		return 0;
	}

	sl_int64 BigInt::getInt64() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getInt64();
		}
		return 0;
	}

	sl_uint64 BigInt::getUint64() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getUint64();
		}
		return 0;
	}

	float BigInt::getFloat() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getFloat();
		}
		return 0;
	}

	double BigInt::getDouble() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->getDouble();
		}
		return 0;
	}

	String BigInt::toString(sl_uint32 radix, sl_bool flagUpperCase) const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			return o->toString(radix, flagUpperCase);
		} else {
			SLIB_RETURN_STRING("0");
		}
	}

	String BigInt::toHexString(sl_bool flagUpperCase) const noexcept
	{
		return toString(16, flagUpperCase);
	}

	sl_bool BigInt::equals(const BigInt& other) const noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (a) {
			if (b) {
				return a->equals(*b);
			} else {
				return a->isZero();
			}
		} else {
			if (b) {
				return b->isZero();
			} else {
				return 0;
			}
		}
	}
	
	sl_bool BigInt::equals(sl_int32 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->equals(v);
		} else {
			return !v;
		}
	}
	
	sl_bool BigInt::equals(sl_uint32 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->equals(v);
		} else {
			return !v;
		}
	}
	
	sl_bool BigInt::equals(sl_int64 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->equals(v);
		} else {
			return !v;
		}
	}
	
	sl_bool BigInt::equals(sl_uint64 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->equals(v);
		} else {
			return !v;
		}
	}

	sl_compare_result BigInt::compare(const BigInt& other) const noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (a) {
			if (b) {
				return a->compare(*b);
			} else {
				return a->sign;
			}
		} else {
			if (b) {
				return -(b->sign);
			} else {
				return 0;
			}
		}
	}

	sl_compare_result BigInt::compare(sl_int32 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->compare(v);
		} else {
			if (v > 0) {
				return -1;
			} else if (v < 0) {
				return 1;
			} else {
				return 0;
			}
		}
	}

	sl_compare_result BigInt::compare(sl_uint32 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->compare(v);
		} else {
			return v ? -1 : 0;
		}
	}

	sl_compare_result BigInt::compare(sl_int64 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->compare(v);
		} else {
			if (v > 0) {
				return -1;
			} else if (v < 0) {
				return 1;
			} else {
				return 0;
			}
		}
	}

	sl_compare_result BigInt::compare(sl_uint64 v) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->compare(v);
		} else {
			return v ? -1 : 0;
		}
	}

	BigInt BigInt::add(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (b) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->add(*a, *b)) {
						return r;
					}
					delete r;
				}
			} else {
				return b;
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::add(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (b) {
			if (a) {
				return a->add(*a, *b);
			} else {
				a = b->duplicate();
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::add(const BigInt& A, sl_int32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->add(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromInt32(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::add(sl_int32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->add(*a, v);
			} else {
				a = CBigInt::fromInt32(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::add(const BigInt& A, sl_uint32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->add(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromUint32(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::add(sl_uint32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->add(*a, v);
			} else {
				a = CBigInt::fromUint32(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::add(const BigInt& A, sl_int64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->add(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromInt64(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::add(sl_int64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->add(*a, v);
			} else {
				a = CBigInt::fromInt64(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::add(const BigInt& A, sl_uint64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->add(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromUint64(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::add(sl_uint64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->add(*a, v);
			} else {
				a = CBigInt::fromUint64(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool BigInt::increase() noexcept
	{
		return add((sl_int32)1);
	}

	BigInt BigInt::sub(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (b) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->sub(*a, *b)) {
						return r;
					}
					delete r;
				}
			} else {
				CBigInt* r = b->duplicate();
				if (r) {
					r->makeNegative();
					return r;
				}
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::sub(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (b) {
			if (a) {
				return a->sub(*a, *b);
			} else {
				a = b->duplicate();
				if (a) {
					a->makeNegative();
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::sub(const BigInt& A, sl_int32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->sub(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromInt32(-v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::sub(sl_int32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->sub(*a, v);
			} else {
				a = CBigInt::fromInt32(-v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::sub(const BigInt& A, sl_uint32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->sub(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromInt64(-((sl_int64)v));
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::sub(sl_uint32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->sub(*a, v);
			} else {
				a = CBigInt::fromUint32(v);
				if (a) {
					a->makeNegative();
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::sub(const BigInt& A, sl_int64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->sub(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromInt64(-v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::sub(sl_int64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->sub(*a, v);
			} else {
				a = CBigInt::fromInt64(-v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::sub(const BigInt& A, sl_uint64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->sub(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				CBigInt* r = CBigInt::fromUint64(v);
				if (r) {
					r->makeNegative();
					return r;
				}
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::sub(sl_uint64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->sub(*a, v);
			} else {
				a = CBigInt::fromUint64(v);
				if (a) {
					a->makeNegative();
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool BigInt::decrease() noexcept
	{
		return add((sl_int32)-1);
	}

	void BigInt::makeNegative() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			o->makeNegative();
		}
	}

	BigInt BigInt::negative() const noexcept
	{
		BigInt ret = duplicate();
		ret.makeNegative();
		return ret;
	}

	void BigInt::makeBitwiseNot() const noexcept
	{
		CBigInt* o = ref.ptr;
		if (o) {
			o->makeBitwiseNot();
		}
	}

	BigInt BigInt::bitwiseNot() const noexcept
	{
		BigInt ret = duplicate();
		ret.makeBitwiseNot();
		return ret;
	}

	BigInt BigInt::mul(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			CBigInt* b = B.ref.ptr;
			if (b) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->mul(*a, *b)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::mul(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			CBigInt* b = other.ref.ptr;
			if (b) {
				return a->mul(*a, *b);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::mul(const BigInt& A, sl_int32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			if (v) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->mul(*a, v)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::mul(sl_int32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			if (v) {
				return a->mul(*a, v);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::mul(const BigInt& A, sl_uint32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			if (v) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->mul(*a, v)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::mul(sl_uint32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			if (v) {
				return a->mul(*a, v);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::mul(const BigInt& A, sl_int64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			if (v) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->mul(*a, v)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::mul(sl_int64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			if (v) {
				return a->mul(*a, v);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::mul(const BigInt& A, sl_uint64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			if (v) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->mul(*a, v)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::mul(sl_uint64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			if (v) {
				return a->mul(*a, v);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::div(const BigInt& A, const BigInt& B, BigInt* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (b) {
			if (a) {
				CBigInt* q = new CBigInt;
				if (q) {
					if (remainder) {
						CBigInt* r = new CBigInt;
						if (r) {
							if (CBigInt::div(*a, *b, q, r, flagNonNegativeRemainder)) {
								*remainder = r;
								return q;
							}
							delete r;
						}
					} else {
						if (CBigInt::div(*a, *b, q, sl_null, flagNonNegativeRemainder)) {
							return q;
						}
					}
					delete q;
				}
			}
		}
		if (remainder) {
			remainder->setNull();
		}
		return sl_null;
	}

	sl_bool BigInt::div(const BigInt& other, BigInt* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (b) {
			if (a) {
				if (remainder) {
					CBigInt* r = new CBigInt;
					if (r) {
						if (CBigInt::div(*a, *b, a, r, flagNonNegativeRemainder)) {
							*remainder = r;
							return sl_true;
						}
						delete r;
					}
				} else {
					if (CBigInt::div(*a, *b, a, sl_null, flagNonNegativeRemainder)) {
						return sl_true;
					}
				}
			} else {
				if (remainder) {
					remainder->setNull();
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	BigInt BigInt::divInt32(const BigInt& A, sl_int32 v, sl_int32* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* q = new CBigInt;
				if (q) {
					if (CBigInt::divInt32(*a, v, q, remainder, flagNonNegativeRemainder)) {
						return q;
					}
					delete q;
				}
			}
		}
		if (remainder) {
			*remainder = 0;
		}
		return sl_null;
	}

	sl_bool BigInt::divInt32(sl_int32 v, sl_int32* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				if (CBigInt::divInt32(*a, v, a, remainder, flagNonNegativeRemainder)) {
					return sl_true;
				}
			} else {
				if (remainder) {
					*remainder = 0;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	BigInt BigInt::divUint32(const BigInt& A, sl_uint32 v, sl_uint32* remainder) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* q = new CBigInt;
				if (q) {
					if (CBigInt::divUint32(*a, v, q, remainder)) {
						return q;
					}
					delete q;
				}
			}
		}
		if (remainder) {
			*remainder = 0;
		}
		return sl_null;
	}

	sl_bool BigInt::divUint32(sl_uint32 v, sl_uint32* remainder) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				if (CBigInt::divUint32(*a, v, a, remainder)) {
					return sl_true;
				}
			} else {
				if (remainder) {
					*remainder = 0;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	BigInt BigInt::divInt64(const BigInt& A, sl_int64 v, sl_int64* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* q = new CBigInt;
				if (q) {
					if (CBigInt::divInt64(*a, v, q, remainder, flagNonNegativeRemainder)) {
						return q;
					}
					delete q;
				}
			}
		}
		if (remainder) {
			*remainder = 0;
		}
		return sl_null;
	}

	sl_bool BigInt::divInt64(sl_int64 v, sl_int64* remainder, sl_bool flagNonNegativeRemainder) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				if (CBigInt::divInt64(*a, v, a, remainder, flagNonNegativeRemainder)) {
					return sl_true;
				}
			} else {
				if (remainder) {
					*remainder = 0;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	BigInt BigInt::divUint64(const BigInt& A, sl_uint64 v, sl_uint64* remainder) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* q = new CBigInt;
				if (q) {
					if (CBigInt::divUint64(*a, v, q, remainder)) {
						return q;
					}
					delete q;
				}
			}
		}
		if (remainder) {
			*remainder = 0;
		}
		return sl_null;
	}

	sl_bool BigInt::divUint64(sl_uint64 v, sl_uint64* remainder) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				if (CBigInt::divUint64(*a, v, a, remainder)) {
					return sl_true;
				}
			} else {
				if (remainder) {
					*remainder = 0;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	BigInt BigInt::mod(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (b) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (CBigInt::div(*a, *b, sl_null, r)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	BigInt BigInt::mod_NonNegativeRemainder(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (b) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (CBigInt::div(*a, *b, sl_null, r, sl_true)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}
	
	sl_bool BigInt::mod(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (b) {
			if (a) {
				return CBigInt::div(*a, *b, sl_null, a);
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool BigInt::mod_NonNegativeRemainder(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (b) {
			if (a) {
				return CBigInt::div(*a, *b, sl_null, a, sl_true);
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_int32 BigInt::modInt32(const BigInt& A, sl_int32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			sl_int32 r = 0;
			if (CBigInt::divInt32(*a, v, sl_null, &r)) {
				return r;
			}
		}
		return 0;
	}

	sl_int32 BigInt::modInt32_NonNegativeRemainder(const BigInt& A, sl_int32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			sl_int32 r = 0;
			if (CBigInt::divInt32(*a, v, sl_null, &r, sl_true)) {
				return r;
			}
		}
		return 0;
	}

	sl_uint32 BigInt::modUint32(const BigInt& A, sl_uint32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			sl_uint32 r = 0;
			if (CBigInt::divUint32(*a, v, sl_null, &r)) {
				return r;
			}
		}
		return 0;
	}

	sl_int64 BigInt::modInt64(const BigInt& A, sl_int64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			sl_int64 r;
			if (CBigInt::divInt64(*a, v, sl_null, &r)) {
				return r;
			}
		}
		return 0;
	}

	sl_int64 BigInt::modInt64_NonNegativeRemainder(const BigInt& A, sl_int64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			sl_int64 r;
			if (CBigInt::divInt64(*a, v, sl_null, &r, sl_true)) {
				return r;
			}
		}
		return 0;
	}

	sl_uint64 BigInt::modUint64(const BigInt& A, sl_uint64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			sl_uint64 r;
			if (CBigInt::divUint64(*a, v, sl_null, &r)) {
				return r;
			}
		}
		return 0;
	}

	BigInt BigInt::bitwiseAnd(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			CBigInt* b = B.ref.ptr;
			if (b) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseAnd(*a, *b)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseAnd(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			CBigInt* b = other.ref.ptr;
			if (b) {
				return a->bitwiseAnd(*a, *b);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}
	
	BigInt BigInt::bitwiseAnd(const BigInt& A, sl_uint32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			if (v) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseAnd(*a, v)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseAnd(sl_uint32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			if (v) {
				return a->bitwiseAnd(*a, v);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::bitwiseAnd(const BigInt& A, sl_uint64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (a) {
			if (v) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseAnd(*a, v)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseAnd(sl_uint64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			if (v) {
				return a->bitwiseAnd(*a, v);
			} else {
				a->setZero();
				return sl_true;
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::bitwiseXor(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (b) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseXor(*a, *b)) {
						return r;
					}
					delete r;
				}
			} else {
				return b;
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseXor(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (b) {
			if (a) {
				return a->bitwiseXor(*a, *b);
			} else {
				a = b->duplicate();
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::bitwiseXor(const BigInt& A, sl_uint32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseXor(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromUint32(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseXor(sl_uint32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->bitwiseXor(*a, v);
			} else {
				a = CBigInt::fromUint32(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::bitwiseXor(const BigInt& A, sl_uint64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseXor(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromUint64(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseXor(sl_uint64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->bitwiseXor(*a, v);
			} else {
				a = CBigInt::fromUint64(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::bitwiseOr(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (b) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseOr(*a, *b)) {
						return r;
					}
					delete r;
				}
			} else {
				return b;
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseOr(const BigInt& other) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* b = other.ref.ptr;
		if (b) {
			if (a) {
				return a->bitwiseOr(*a, *b);
			} else {
				a = b->duplicate();
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::bitwiseOr(const BigInt& A, sl_uint32 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseOr(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromUint32(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseOr(sl_uint32 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->bitwiseOr(*a, v);
			} else {
				a = CBigInt::fromUint32(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::bitwiseOr(const BigInt& A, sl_uint64 v) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (v) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->bitwiseOr(*a, v)) {
						return r;
					}
					delete r;
				}
			} else {
				return CBigInt::fromUint64(v);
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::bitwiseOr(sl_uint64 v) noexcept
	{
		CBigInt* a = ref.ptr;
		if (v) {
			if (a) {
				return a->bitwiseOr(*a, v);
			} else {
				a = CBigInt::fromUint64(v);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::shiftLeft(const BigInt& A, sl_size n) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (n) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->shiftLeft(*a, n)) {
						return r;
					}
					delete r;
				}
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::shiftLeft(sl_size n) noexcept
	{
		CBigInt* a = ref.ptr;
		if (n) {
			if (a) {
				return a->shiftLeft(*a, n);
			} else {
				return sl_true;
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}

	BigInt BigInt::shiftRight(const BigInt& A, sl_size n) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (n) {
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->shiftRight(*a, n)) {
						return r;
					}
					delete r;
				}
			}
		} else {
			return a;
		}
		return sl_null;
	}

	sl_bool BigInt::shiftRight(sl_size n) noexcept
	{
		CBigInt* a = ref.ptr;
		if (n) {
			if (a) {
				return a->shiftRight(*a, n);
			} else {
				return sl_true;
			}
		} else {
			return sl_true;
		}
		return sl_false;
	}
	
	BigInt BigInt::abs() const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			if (a->sign > 0) {
				return *this;
			} else {
				return -(*this);
			}
		} else {
			return sl_true;
		}
	}

	BigInt BigInt::pow(const BigInt& A, const BigInt& E, const BigInt* pM) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* e = E.ref.ptr;
		if (!e || e->isZero()) {
			return fromInt32(1);
		}
		if (a) {
			CBigInt* r = new CBigInt;
			if (r) {
				if (pM) {
					if (r->pow(*a, *e, pM->ref.ptr)) {
						return r;
					}
				} else {
					if (r->pow(*a, *e, sl_null)) {
						return r;
					}
				}
				delete r;
			}
		}
		return sl_null;
	}

	sl_bool BigInt::pow(const BigInt& E, const BigInt* pM) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* e = E.ref.ptr;
		if (!e || e->isZero()) {
			if (a) {
				if (a->setValue(1)) {
					return sl_true;
				}
			} else {
				a = CBigInt::fromInt32(1);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			if (a) {
				if (pM) {
					return a->pow(*a, *e, pM->ref.ptr);
				} else {
					return a->pow(*a, *e, sl_null);
				}
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	BigInt BigInt::pow(const BigInt& A, sl_uint32 E, const BigInt* pM) noexcept
	{
		CBigInt* a = A.ref.ptr;
		if (E) {
			if (E == 1) {
				return a;
			}
			if (a) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (pM) {
						if (r->pow(*a, E, pM->ref.ptr)) {
							return r;
						}
					} else {
						if (r->pow(*a, E, sl_null)) {
							return r;
						}
					}
					delete r;
				}
			}
		} else {
			return fromInt32(1);
		}
		return sl_null;
	}

	BigInt BigInt::pow_mod(const BigInt& A, const BigInt& E, const BigInt& M) noexcept
	{
		return pow(A, E, &M);
	}

	sl_bool BigInt::pow_mod(const BigInt& E, const BigInt& M) noexcept
	{
		return pow(E, &M);
	}

	sl_bool BigInt::pow(sl_uint32 E, const BigInt* pM) noexcept
	{
		CBigInt* a = ref.ptr;
		if (E) {
			if (E == 1) {
				return sl_true;
			}
			if (a) {
				if (pM) {
					return a->pow(*a, E, pM->ref.ptr);
				} else {
					return a->pow(*a, E, sl_null);
				}
			}
		} else {
			if (a) {
				if (a->setValue(1)) {
					return sl_true;
				}
			} else {
				a = CBigInt::fromInt32(1);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	BigInt BigInt::pow_mod(const BigInt& A, sl_uint32 E, const BigInt& M) noexcept
	{
		return pow(A, E, &M);
	}

	sl_bool BigInt::pow_mod(sl_uint32 E, const BigInt& M) noexcept
	{
		return pow(E, &M);
	}

	BigInt BigInt::pow_montgomery(const BigInt& A, const BigInt& E, const BigInt& M) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* e = E.ref.ptr;
		CBigInt* m = M.ref.ptr;
		if (!e || e->isZero()) {
			return fromInt32(1);
		} else {
			if (m) {
				if (a) {
					CBigInt* r = new CBigInt;
					if (r) {
						if (r->pow_montgomery(*a, *e, *m)) {
							return r;
						}
						delete r;
					}
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::pow_montgomery(const BigInt& E, const BigInt& M) noexcept
	{
		CBigInt* a = ref.ptr;
		CBigInt* e = E.ref.ptr;
		CBigInt* m = M.ref.ptr;
		if (!e || e->isZero()) {
			if (a) {
				if (a->setValue(1)) {
					return sl_true;
				}
			} else {
				a = CBigInt::fromInt32(1);
				if (a) {
					ref = a;
					return sl_true;
				}
			}
		} else {
			if (m) {
				if (a) {
					return a->pow_montgomery(*a, *e, *m);
				} else {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	BigInt BigInt::inverseMod(const BigInt& A, const BigInt& M) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* m = M.ref.ptr;
		if (a) {
			if (m) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->inverseMod(*a, *m)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	BigInt BigInt::sqrtMod(const BigInt& A, const BigInt& M) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* m = M.ref.ptr;
		if (a) {
			if (m) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->sqrtMod(*a, *m)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	BigInt BigInt::gcd(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (a) {
			if (b) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->gcd(*a, *b)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	BigInt BigInt::lcm(const BigInt& A, const BigInt& B) noexcept
	{
		CBigInt* a = A.ref.ptr;
		CBigInt* b = B.ref.ptr;
		if (a) {
			if (b) {
				CBigInt* r = new CBigInt;
				if (r) {
					if (r->lcm(*a, *b)) {
						return r;
					}
					delete r;
				}
			}
		}
		return sl_null;
	}

	sl_bool BigInt::isProbablePrime(sl_uint32 nChecks, sl_bool* pFlagError) const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->isProbablePrime(nChecks, pFlagError);
		}
		return sl_false;
	}
	
	BigInt BigInt::generatePrime(sl_size nBits) noexcept
	{
		CBigInt* ret = new CBigInt;
		if (ret) {
			if (ret->generatePrime(nBits)) {
				return ret;
			}
			delete ret;
		}
		return sl_null;
	}
	
	BigInt BigInt::random(sl_size nBits) noexcept
	{
		CBigInt* ret = new CBigInt;
		if (ret) {
			if (ret->random(nBits)) {
				return ret;
			}
			delete ret;
		}
		return sl_null;
	}
	
	sl_size BigInt::getHashCode() const noexcept
	{
		CBigInt* a = ref.ptr;
		if (a) {
			return a->getHashCode();
		}
		return 0;
	}

	namespace priv
	{
		namespace bigint
		{
			template <class CT>
			static sl_reg Parse(BigInt* _out, sl_uint32 radix, const CT* sz, sl_size posBegin, sl_size len) noexcept
			{
				if (radix < 2 || radix > 64) {
					return SLIB_PARSE_ERROR;;
				}
				sl_int32 sign;
				sl_size pos = posBegin;
				if (pos < len && sz[pos] == '-') {
					pos++;
					sign = -1;
				} else {
					sign = 1;
				}
				for (; pos < len; pos++) {
					sl_int32 c = (sl_uint32)(sz[pos]);
					if (c != '\t' && c != ' ') {
						break;
					}
				}
				sl_size end = pos;
				const sl_uint8* pattern = radix <= 36 ? priv::string::g_conv_radixInversePatternSmall : priv::string::g_conv_radixInversePatternBig;
				for (; end < len; end++) {
					sl_uint32 c = (sl_uint8)(sz[end]);
					sl_uint32 v = c < 128 ? pattern[c] : 255;
					if (v >= radix) {
						break;
					}
				}
				if (end <= pos) {
					return SLIB_PARSE_ERROR;
				}
				if (!_out) {
					return end;
				}
				CBigInt* output = new CBigInt;
				if (!output) {
					return SLIB_PARSE_ERROR;
				}
				_out->ref = output;
				output->sign = sign;
				if (radix == 16) {
					output->setZero();
					sl_size nh = end - pos;
					sl_size ne = ((nh << 2) + 31) >> 5;
					if (!(output->growLength(ne))) {
						return SLIB_PARSE_ERROR;
					}
					sl_uint32* elements = output->elements;
					sl_size ih = nh - 1;
					for (; pos < end; pos++) {
						sl_uint32 c = (sl_uint8)(sz[pos]);
						sl_uint32 v = c < 128 ? pattern[c] : 255;
						if (v >= radix) {
							break;
						}
						sl_size ie = ih >> 3;
						sl_uint32 ib = (sl_uint32)((ih << 2) & 31);
						elements[ie] |= (v << ib);
						ih--;
					}
					return pos;
				} else {
					sl_size nb = (sl_size)(Math::ceil(Math::log2((double)radix) * len));
					sl_size ne = (nb + 31) >> 5;
					SLIB_SCOPED_BUFFER(sl_uint32, STACK_BUFFER_SIZE, a, ne);
					if (!a) {
						return SLIB_PARSE_ERROR;
					}
					sl_size n = 0;
					for (; pos < end; pos++) {
						sl_uint32 c = (sl_uint8)(sz[pos]);
						sl_uint32 v = c < 128 ? pattern[c] : 255;
						if (v >= radix) {
							break;
						}
						sl_uint32 o = priv::bigint::Mul_uint32(a, a, n, radix, v);
						if (o) {
							a[n] = o;
							n++;
						}
					}
					if (!(output->setValueFromElements(a, n))) {
						return SLIB_PARSE_ERROR;
					}
					return pos;
				}
			}
		}
	}

	SLIB_DEFINE_CLASS_PARSE_INT_MEMBERS(BigInt, priv::bigint::Parse)
	
	BigInt& BigInt::operator=(sl_int32 n) noexcept
	{
		ref = CBigInt::fromInt32(n);
		return *this;
	}

	BigInt& BigInt::operator=(sl_uint32 n) noexcept
	{
		ref = CBigInt::fromUint32(n);
		return *this;
	}

	BigInt& BigInt::operator=(sl_int64 n) noexcept
	{
		ref = CBigInt::fromInt64(n);
		return *this;
	}

	BigInt& BigInt::operator=(sl_uint64 n) noexcept
	{
		ref = CBigInt::fromUint64(n);
		return *this;
	}

	BigInt& BigInt::operator+=(const BigInt& other) noexcept
	{
		add(other);
		return *this;
	}

	BigInt& BigInt::operator+=(sl_int32 v) noexcept
	{
		add(v);
		return *this;
	}

	BigInt& BigInt::operator+=(sl_uint32 v) noexcept
	{
		add(v);
		return *this;
	}

	BigInt& BigInt::operator+=(sl_int64 v) noexcept
	{
		add(v);
		return *this;
	}

	BigInt& BigInt::operator+=(sl_uint64 v) noexcept
	{
		add(v);
		return *this;
	}

	BigInt& BigInt::operator++() noexcept
	{
		increase();
		return *this;
	}

	BigInt BigInt::operator++(int) noexcept
	{
		BigInt r = duplicate();
		increase();
		return r;
	}

	BigInt& BigInt::operator-=(const BigInt& other) noexcept
	{
		sub(other);
		return *this;
	}

	BigInt& BigInt::operator-=(sl_int32 v) noexcept
	{
		sub(v);
		return *this;
	}

	BigInt& BigInt::operator-=(sl_uint32 v) noexcept
	{
		sub(v);
		return *this;
	}

	BigInt& BigInt::operator-=(sl_int64 v) noexcept
	{
		sub(v);
		return *this;
	}

	BigInt& BigInt::operator-=(sl_uint64 v) noexcept
	{
		sub(v);
		return *this;
	}

	BigInt& BigInt::operator--() noexcept
	{
		decrease();
		return *this;
	}

	BigInt BigInt::operator--(int) noexcept
	{
		BigInt r = duplicate();
		decrease();
		return r;
	}

	BigInt& BigInt::operator*=(const BigInt& other) noexcept
	{
		mul(other);
		return *this;
	}

	BigInt& BigInt::operator*=(sl_int32 v) noexcept
	{
		mul(v);
		return *this;
	}

	BigInt& BigInt::operator*=(sl_uint32 v) noexcept
	{
		mul(v);
		return *this;
	}

	BigInt& BigInt::operator*=(sl_int64 v) noexcept
	{
		mul(v);
		return *this;
	}

	BigInt& BigInt::operator*=(sl_uint64 v) noexcept
	{
		mul(v);
		return *this;
	}

	BigInt& BigInt::operator/=(const BigInt& other) noexcept
	{
		div(other);
		return *this;
	}

	BigInt& BigInt::operator/=(sl_int32 v) noexcept
	{
		div(v);
		return *this;
	}

	BigInt& BigInt::operator/=(sl_uint32 v) noexcept
	{
		div(v);
		return *this;
	}

	BigInt& BigInt::operator/=(sl_int64 v) noexcept
	{
		div(v);
		return *this;
	}

	BigInt& BigInt::operator/=(sl_uint64 v) noexcept
	{
		div(v);
		return *this;
	}

	BigInt& BigInt::operator%=(const BigInt& other) noexcept
	{
		mod(other);
		return *this;
	}

	BigInt& BigInt::operator%=(sl_int32 v) noexcept
	{
		*this = modInt32(*this, v);
		return *this;
	}

	BigInt& BigInt::operator<<=(sl_uint32 n) noexcept
	{
		shiftLeft(n);
		return *this;
	}

	BigInt& BigInt::operator>>=(sl_uint32 n) noexcept
	{
		shiftRight(n);
		return *this;
	}
	
	BigInt BigInt::operator-() const noexcept
	{
		return negative();
	}

	BigInt BigInt::operator~() const noexcept
	{
		return bitwiseNot();
	}

	sl_bool BigInt::operator!() const noexcept
	{
		return isZero();
	}

	BigInt::operator sl_bool() const noexcept
	{
		return isNotZero();
	}


#define DEFINE_BIGINT_OP(OP, RET, EXPR, EXPR_FRIEND) \
	RET BigInt::OP(const BigInt& other) const noexcept \
	{ \
		EXPR; \
	} \
	RET BigInt::OP(sl_int32 other) const noexcept \
	{ \
		EXPR; \
	} \
	RET BigInt::OP(sl_uint32 other) const noexcept \
	{ \
		EXPR; \
	} \
	RET BigInt::OP(sl_int64 other) const noexcept \
	{ \
		EXPR; \
	} \
	RET BigInt::OP(sl_uint64 other) const noexcept \
	{ \
		EXPR; \
	} \
	RET OP(sl_int32 other, const BigInt& thiz) noexcept \
	{ \
		EXPR_FRIEND; \
	} \
	RET OP(sl_uint32 other, const BigInt& thiz) noexcept \
	{ \
		EXPR_FRIEND; \
	} \
	RET OP(sl_int64 other, const BigInt& thiz) noexcept \
	{ \
		EXPR_FRIEND; \
	} \
	RET OP(sl_uint64 other, const BigInt& thiz) noexcept \
	{ \
		EXPR_FRIEND; \
	}

	DEFINE_BIGINT_OP(operator==, sl_bool, return equals(other), return thiz.equals(other))
	DEFINE_BIGINT_OP(operator!= , sl_bool, return !(equals(other)), return !(thiz.equals(other)))
	DEFINE_BIGINT_OP(operator>= , sl_bool, return compare(other) >= 0, return thiz.compare(other) <= 0)
	DEFINE_BIGINT_OP(operator<= , sl_bool, return compare(other) <= 0, return thiz.compare(other) >= 0)
	DEFINE_BIGINT_OP(operator> , sl_bool, return compare(other) > 0, return thiz.compare(other) < 0)
	DEFINE_BIGINT_OP(operator<, sl_bool, return compare(other) < 0, return thiz.compare(other) > 0)
	DEFINE_BIGINT_OP(operator+, BigInt, return BigInt::add(*this, other), return BigInt::add(thiz, other))
	DEFINE_BIGINT_OP(operator-, BigInt, return BigInt::sub(*this, other), BigInt ret = BigInt::sub(thiz, other); ret.makeNegative(); return ret)

	DEFINE_BIGINT_OP(operator*, BigInt, return BigInt::mul(*this, other), return BigInt::mul(thiz, other))
	DEFINE_BIGINT_OP(operator/, BigInt, return BigInt::div(*this, other), return BigInt::div(BigInt(other), thiz))

	BigInt BigInt::operator%(const BigInt& other) const noexcept
	{
		return BigInt::mod(*this, other);
	}

	sl_int32 BigInt::operator%(sl_int32 v) const noexcept
	{
		return BigInt::modInt32(*this, v);
	}

	sl_int64 BigInt::operator%(sl_int64 v) const noexcept
	{
		return BigInt::modInt64(*this, v);
	}

	BigInt operator%(sl_int32 v, const BigInt& thiz) noexcept
	{
		return BigInt::mod(BigInt(v), thiz);
	}

	BigInt operator%(sl_uint32 v, const BigInt& thiz) noexcept
	{
		return BigInt::mod(BigInt(v), thiz);
	}

	BigInt operator%(sl_int64 v, const BigInt& thiz) noexcept
	{
		return BigInt::mod(BigInt(v), thiz);
	}

	BigInt operator%(sl_uint64 v, const BigInt& thiz) noexcept
	{
		return BigInt::mod(BigInt(v), thiz);
	}

#define DEFINE_BIGINT_BITWISE_OP(OP, EXPR, EXPR_FRIEND) \
	BigInt BigInt::OP(const BigInt& other) const noexcept \
	{ \
		EXPR; \
	} \
	BigInt BigInt::OP(sl_uint32 other) const noexcept \
	{ \
		EXPR; \
	} \
	BigInt BigInt::OP(sl_uint64 other) const noexcept \
	{ \
		EXPR; \
	} \
	BigInt OP(sl_uint32 other, const BigInt& thiz) noexcept \
	{ \
		EXPR_FRIEND; \
	} \
	BigInt OP(sl_uint64 other, const BigInt& thiz) noexcept \
	{ \
		EXPR_FRIEND; \
	}

	DEFINE_BIGINT_BITWISE_OP(operator&, return BigInt::bitwiseAnd(*this, other), return BigInt::bitwiseAnd(thiz, other))
	DEFINE_BIGINT_BITWISE_OP(operator^, return BigInt::bitwiseXor(*this, other), return BigInt::bitwiseXor(thiz, other))
	DEFINE_BIGINT_BITWISE_OP(operator|, return BigInt::bitwiseOr(*this, other), return BigInt::bitwiseOr(thiz, other))

	BigInt BigInt::operator<<(sl_size n) const noexcept
	{
		return BigInt::shiftLeft(*this, n);
	}

	BigInt BigInt::operator>>(sl_size n) const noexcept
	{
		return BigInt::shiftRight(*this, n);
	}


	Atomic<BigInt>::Atomic(sl_int32 n) noexcept : ref(CBigInt::fromInt32(n))
	{
	}

	Atomic<BigInt>::Atomic(sl_uint32 n) noexcept : ref(CBigInt::fromUint32(n))
	{
	}

	Atomic<BigInt>::Atomic(sl_int64 n) noexcept : ref(CBigInt::fromInt64(n))
	{
	}

	Atomic<BigInt>::Atomic(sl_uint64 n) noexcept : ref(CBigInt::fromUint64(n))
	{
	}

	Atomic<BigInt>::~Atomic() noexcept
	{
	}

	AtomicBigInt& Atomic<BigInt>::operator=(sl_int32 n) noexcept
	{
		ref = CBigInt::fromInt32(n);
		return *this;
	}

	AtomicBigInt& Atomic<BigInt>::operator=(sl_uint32 n) noexcept
	{
		ref = CBigInt::fromUint32(n);
		return *this;
	}

	AtomicBigInt& Atomic<BigInt>::operator=(sl_int64 n) noexcept
	{
		ref = CBigInt::fromInt64(n);
		return *this;
	}

	AtomicBigInt& Atomic<BigInt>::operator=(sl_uint64 n) noexcept
	{
		ref = CBigInt::fromUint64(n);
		return *this;
	}

}
