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

#include "slib/math/int128.h"

#include "slib/core/mio.h"
#include "slib/core/string.h"

namespace slib
{

	SLIB_ALIGN(8) const sl_uint64 Uint128::_zero[2] = { 0, 0 };

	sl_uint32 Uint128::getMostSignificantBits() const noexcept
	{
		if (high) {
			return 64 + Math::getMostSignificantBits(high);
		}
		if (low) {
			return Math::getMostSignificantBits(low);
		}
		return 0;
	}

	sl_uint32 Uint128::getLeastSignificantBits() const noexcept
	{
		if (low) {
			return Math::getLeastSignificantBits(low);
		}
		if (high) {
			return 64 + Math::getLeastSignificantBits(high);
		}
		return 0;
	}

	void Uint128::getBytesBE(void* _buf) noexcept
	{
		char* buf = (char*)_buf;
		MIO::writeUint64BE(buf, high);
		MIO::writeUint64BE(buf + 8, low);
	}

	void Uint128::setBytesBE(const void* _buf) noexcept
	{
		const char* buf = (const char*)_buf;
		high = MIO::readUint64BE(buf);
		low = MIO::readUint64BE(buf + 8);
	}

	void Uint128::getBytesLE(void* _buf) noexcept
	{
		char* buf = (char*)_buf;
		MIO::writeUint64LE(buf, low);
		MIO::writeUint64LE(buf + 8, high);
	}

	void Uint128::setBytesLE(const void* _buf) noexcept
	{
		const char* buf = (const char*)_buf;
		low = MIO::readUint64LE(buf);
		high = MIO::readUint64LE(buf + 8);
	}

	Uint128 Uint128::mul64(sl_uint64 a, sl_uint64 b) noexcept
	{
		sl_uint64 high, low;
		Math::mul64(a, b, high, low);
		return Uint128(high, low);
	}

	sl_bool Uint128::div(const Uint128& a, const Uint128& b, Uint128* quotient, Uint128* remainder) noexcept
	{
		if (!(b.high)) {
			if (remainder) {
				remainder->high = 0;
				return div64(a, b.low, quotient, &(remainder->low));
			} else {
				return div64(a, b.low, quotient, sl_null);
			}
		}
		if (a < b) {
			if (remainder) {
				*remainder = a;
			}
			if (quotient) {
				quotient->setZero();
			}
			return sl_true;
		}
		sl_uint32 nba = a.getMostSignificantBits();
		sl_uint32 nbb = b.getMostSignificantBits();
		sl_uint32 shift;
		if (nba >= nbb) {
			shift = nba - nbb;
		} else {
			shift = 0;
		}
		Uint128 r = a;
		Uint128 t = b;
		t <<= shift;
		Uint128 q;
		for (;;) {
			if (r >= t) {
				q |= 1;
				r -= t;
			}
			if (!shift) {
				break;
			}
			q.shiftLeft();
			t.shiftRight();
			shift--;
		}
		if (quotient) {
			*quotient = q;
		}
		if (remainder) {
			*remainder = r;
		}
		return sl_true;
	}

	sl_bool Uint128::div64(const Uint128& a, sl_uint64 b, Uint128* quotient, sl_uint64* remainder) noexcept
	{
		sl_uint64 high = a.high;
		sl_uint64 low = a.low;
		sl_uint64 r;
		if (Math::div128_64(high, low, b, r)) {
			if (quotient) {
				quotient->high = high;
				quotient->low = low;
			}
			if (remainder) {
				*remainder = r;
			}
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool Uint128::div32(const Uint128& a, sl_uint32 b, Uint128* quotient, sl_uint32* remainder) noexcept
	{
		sl_uint64 high = a.high;
		sl_uint64 low = a.low;
		sl_uint32 r;
		if (Math::div128_32(high, low, b, r)) {
			if (quotient) {
				quotient->high = high;
				quotient->low = low;
			}
			if (remainder) {
				*remainder = r;
			}
			return sl_true;
		} else {
			return sl_false;
		}
	}

	Uint128 Uint128::div(const Uint128& other) const noexcept
	{
		Uint128 ret;
		if (div(*this, other, &ret, sl_null)) {
			return ret;
		} else {
			return Uint128::zero();
		}
	}

	Uint128 Uint128::div(sl_uint64 num) const noexcept
	{
		Uint128 ret;
		if (div64(*this, num, &ret, sl_null)) {
			return ret;
		} else {
			return Uint128::zero();
		}
	}

	Uint128 Uint128::div(sl_uint32 num) const noexcept
	{
		Uint128 ret;
		if (div32(*this, num, &ret, sl_null)) {
			return ret;
		} else {
			return Uint128::zero();
		}
	}

	Uint128 Uint128::mod(const Uint128& other) const noexcept
	{
		Uint128 ret;
		if (div(*this, other, sl_null, &ret)) {
			return ret;
		} else {
			return Uint128::zero();
		}
	}

	sl_uint64 Uint128::mod(sl_uint64 num) const noexcept
	{
		sl_uint64 ret;
		if (div64(*this, num, sl_null, &ret)) {
			return ret;
		} else {
			return 0;
		}
	}

	sl_uint32 Uint128::mod(sl_uint32 num) const noexcept
	{
		sl_uint32 ret;
		if (div32(*this, num, sl_null, &ret)) {
			return ret;
		} else {
			return 0;
		}
	}

	void Uint128::shiftRight() noexcept
	{
		low = (high << 63) | (low >> 1);
		high >>= 1;
	}

	void Uint128::shiftLeft() noexcept
	{
		high = (low >> 63) | (high << 1);
		low <<= 1;
	}

	void Uint128::makeNegative() noexcept
	{
		low = -(sl_int64)(low);
		high = -(sl_int64)(high);
		if (low != 0) {
			high--;
		}
	}

	void Uint128::makeBitwiseNot() noexcept
	{
		low = ~low;
		high = ~high;
	}

	namespace priv
	{
		namespace uint128
		{
#ifdef SLIB_ARCH_IS_LITTLE_ENDIAN
			SLIB_ALIGN(8) const sl_uint64 g_pow10_32[] = { SLIB_UINT64(0x85ACEF8100000000), SLIB_UINT64(0x4EE2D6D415B) };
#else
			SLIB_ALIGN(8) const sl_uint64 g_pow10_32[] = { SLIB_UINT64(0x4EE2D6D415B), SLIB_UINT64(0x85ACEF8100000000) };
#endif
		}
	}

	const Uint128& Uint128::pow10_32() noexcept
	{
		return *((Uint128*)((void*)(priv::uint128::g_pow10_32)));

	}

	Uint128 Uint128::pow10(sl_uint32 exponent) noexcept
	{
		if (exponent < 20) {
			return Math::pow10i(exponent);
		}
		if (exponent == 32) {
			return pow10_32();
		}
		if (exponent > SLIB_UINT128_MAX_LOG10I) {
			return 0;
		}
		if (exponent > 32) {
			return pow10_32() * Math::pow10i(exponent - 32);
		} else {
			return mul64(SLIB_POW10_16, Math::pow10i(exponent - 16));
		}
	}

	sl_uint32 Uint128::log10() const noexcept
	{
		if (!high) {
			return Math::log10i(low);
		}
		if (compare(pow10_32()) >= 0) {
			return 32 + (*this / pow10_32()).log10();
		} else {
			return 16 + (*this / SLIB_POW10_16).log10();
		}
	}

	Uint128 Uint128::operator+(const Uint128& other) const noexcept
	{
		Uint128 ret;
		ret.high = high + other.high;
		ret.low = low + other.low;
		if (ret.low < low) {
			ret.high++;
		}
		return ret;
	}

	Uint128 Uint128::operator+(sl_uint64 num) const noexcept
	{
		Uint128 ret;
		ret.high = high;
		ret.low = low + num;
		if (ret.low < num) {
			ret.high++;
		}
		return ret;
	}

	Uint128 operator+(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 ret;
		ret.high = v.high;
		ret.low = v.low + num;
		if (ret.low < num) {
			ret.high++;
		}
		return ret;
	}

	Uint128& Uint128::operator+=(const Uint128& other) noexcept
	{
		high += other.high;
		low += other.low;
		if (low < other.low) {
			high++;
		}
		return *this;
	}

	Uint128& Uint128::operator+=(sl_uint32 num) noexcept
	{
		low += num;
		if (low < num) {
			high++;
		}
		return *this;
	}


	Uint128& Uint128::operator++() noexcept
	{
		low++;
		if (low == 0) {
			high++;
		}
		return *this;
	}

	Uint128 Uint128::operator++(int) noexcept
	{
		Uint128 ret = *this;
		low++;
		if (low == 0) {
			high++;
		}
		return ret;
	}

	Uint128 Uint128::operator-(const Uint128& other) const noexcept
	{
		Uint128 ret;
		ret.high = high - other.high;
		if (low < other.low) {
			ret.high--;
		}
		ret.low = low - other.low;
		return ret;
	}

	Uint128 Uint128::operator-(sl_uint64 num) const noexcept
	{
		Uint128 ret;
		ret.high = high;
		if (low < num) {
			ret.high--;
		}
		ret.low = low - num;
		return ret;
	}

	Uint128 operator-(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 ret;
		ret.high = - (sl_int64)(v.high);
		if (num < v.low) {
			ret.high--;
		}
		ret.low = num - v.low;
		return ret;
	}

	Uint128& Uint128::operator-=(const Uint128& other) noexcept
	{
		high -= other.high;
		if (low < other.low) {
			high--;
		}
		low -= other.low;
		return *this;
	}

	Uint128& Uint128::operator-=(sl_uint64 num) noexcept
	{
		if (low < num) {
			high--;
		}
		low -= num;
		return *this;
	}

	Uint128& Uint128::operator--() noexcept
	{
		if (low == 0) {
			high--;
		}
		low--;
		return *this;
	}

	Uint128 Uint128::operator--(int) noexcept
	{
		Uint128 ret = *this;
		if (low == 0) {
			high--;
		}
		low--;
		return ret;
	}

	Uint128 Uint128::operator*(const Uint128& other) const noexcept
	{
		Uint128 ret;
		Math::mul64(low, other.low, ret.high, ret.low);
		ret.high += (high * other.low);
		ret.high += (low * other.high);
		return ret;
	}

	Uint128 Uint128::operator*(sl_uint64 num) const noexcept
	{
		Uint128 ret;
		Math::mul64(low, num, ret.high, ret.low);
		ret.high += (high * num);
		return ret;
	}

	Uint128 operator*(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 ret;
		Math::mul64(v.low, num, ret.high, ret.low);
		ret.high += (v.high * num);
		return ret;
	}

	Uint128& Uint128::operator*=(const Uint128& other) noexcept
	{
		sl_uint64 ml, mh;
		Math::mul64(low, other.low, mh, ml);
		mh += (high * other.low);
		mh += (low * other.high);
		high = mh;
		low = ml;
		return *this;
	}

	Uint128& Uint128::operator*=(sl_uint64 num) noexcept
	{
		sl_uint64 ml, mh;
		Math::mul64(low, num, mh, ml);
		mh += (high * num);
		high = mh;
		low = ml;
		return *this;
	}

	Uint128 Uint128::operator/(const Uint128& other) const noexcept
	{
		return div(other);
	}

	Uint128 Uint128::operator/(sl_uint64 num) const noexcept
	{
		return div(num);
	}

	Uint128 Uint128::operator/(sl_uint32 num) const noexcept
	{
		return div(num);
	}

	Uint128 operator/(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 a(num);
		return a.div(v);
	}

	Uint128& Uint128::operator/=(const Uint128& other) noexcept
	{
		*this = div(other);
		return *this;
	}

	Uint128& Uint128::operator/=(sl_uint64 num) noexcept
	{
		*this = div(num);
		return *this;
	}

	Uint128& Uint128::operator/=(sl_uint32 num) noexcept
	{
		*this = div(num);
		return *this;
	}

	Uint128 Uint128::operator%(const Uint128& other) const noexcept
	{
		return mod(other);
	}

	Uint128 Uint128::operator%(sl_uint64 num) const noexcept
	{
		return mod(num);
	}

	Uint128 Uint128::operator%(sl_uint32 num) const noexcept
	{
		return mod(num);
	}

	Uint128 operator%(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 a(num);
		return a.mod(v);
	}

	Uint128& Uint128::operator%=(const Uint128& other) noexcept
	{
		*this = mod(other);
		return *this;
	}

	Uint128& Uint128::operator%=(sl_uint64 num) noexcept
	{
		*this = mod(num);
		return *this;
	}

	Uint128& Uint128::operator%=(sl_uint32 num) noexcept
	{
		*this = mod(num);
		return *this;
	}

	Uint128 Uint128::operator&(const Uint128& other) const noexcept
	{
		Uint128 ret;
		ret.high = high & other.high;
		ret.low = low & other.low;
		return ret;
	}

	Uint128 Uint128::operator&(sl_uint64 num) const noexcept
	{
		Uint128 ret;
		ret.high = high;
		ret.low = low & num;
		return ret;
	}

	Uint128 operator&(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 ret;
		ret.high = v.high;
		ret.low = v.low & num;
		return ret;
	}

	Uint128& Uint128::operator&=(const Uint128& other) noexcept
	{
		high &= other.high;
		low &= other.low;
		return *this;
	}

	Uint128& Uint128::operator&=(sl_uint32 num) noexcept
	{
		low &= num;
		return *this;
	}

	Uint128 Uint128::operator|(const Uint128& other) const noexcept
	{
		Uint128 ret;
		ret.high = high | other.high;
		ret.low = low | other.low;
		return ret;
	}

	Uint128 Uint128::operator|(sl_uint64 num) const noexcept
	{
		Uint128 ret;
		ret.high = high;
		ret.low = low | num;
		return ret;
	}

	Uint128 operator|(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 ret;
		ret.high = v.high;
		ret.low = v.low | num;
		return ret;
	}

	Uint128& Uint128::operator|=(const Uint128& other) noexcept
	{
		high |= other.high;
		low |= other.low;
		return *this;
	}

	Uint128& Uint128::operator|=(sl_uint32 num) noexcept
	{
		low |= num;
		return *this;
	}

	Uint128 Uint128::operator^(const Uint128& other) const noexcept
	{
		Uint128 ret;
		ret.high = high ^ other.high;
		ret.low = low ^ other.low;
		return ret;
	}

	Uint128 Uint128::operator^(sl_uint64 num) const noexcept
	{
		Uint128 ret;
		ret.high = high;
		ret.low = low ^ num;
		return ret;
	}

	Uint128 operator^(sl_uint64 num, const Uint128& v) noexcept
	{
		Uint128 ret;
		ret.high = v.high;
		ret.low = v.low ^ num;
		return ret;
	}

	Uint128& Uint128::operator^=(const Uint128& other) noexcept
	{
		high ^= other.high;
		low ^= other.low;
		return *this;
	}

	Uint128& Uint128::operator^=(sl_uint32 num) noexcept
	{
		low ^= num;
		return *this;
	}

	Uint128 Uint128::operator>>(sl_uint32 n) const noexcept
	{
		Uint128 ret;
		if (n) {
			if (n >= 64) {
				ret.low = high >> (n - 64);
				ret.high = 0;
			} else {
				ret.low = (high << (64 - n)) | (low >> n);
				ret.high = high >> n;
			}
		} else {
			ret.low = low;
			ret.high = high;
		}
		return ret;
	}

	Uint128& Uint128::operator>>=(sl_uint32 n) noexcept
	{
		if (n) {
			if (n >= 64) {
				low = high >> (n - 64);
				high = 0;
			} else {
				low = (high << (64 - n)) | (low >> n);
				high >>= n;
			}
		}
		return *this;
	}

	Uint128 Uint128::operator<<(sl_uint32 n) const noexcept
	{
		Uint128 ret;
		if (n) {
			if (n >= 64) {
				ret.high = low << (n - 64);
				ret.low = 0;
			} else {
				ret.high = (low >> (64 - n)) | (high << n);
				ret.low = low << n;
			}
		} else {
			ret.high = high;
			ret.low = low;
		}
		return ret;
	}

	Uint128& Uint128::operator<<=(sl_uint32 n) noexcept
	{
		if (n) {
			if (n >= 64) {
				high = low << (n - 64);
				low = 0;
			} else {
				high = (low >> (64 - n)) | (high << n);
				low <<= n;
			}
		}
		return *this;
	}

	Uint128 Uint128::operator-() const noexcept
	{
		Uint128 ret;
		ret.low = -(sl_int64)(low);
		ret.high = -(sl_int64)(high);
		if (low != 0) {
			ret.high--;
		}
		return ret;
	}

	Uint128 Uint128::operator~() const noexcept
	{
		Uint128 ret;
		ret.low = ~low;
		ret.high = ~high;
		return ret;
	}

	sl_bool Uint128::equals(const Uint128& other) const noexcept
	{
		return high == other.high && low == other.low;
	}

	sl_bool Uint128::equals(sl_uint64 other) const noexcept
	{
		return high == 0 && low == other;
	}

	sl_compare_result Uint128::compare(const Uint128& other) const noexcept
	{
		if (high > other.high) {
			return 1;
		}
		if (high < other.high) {
			return -1;
		}
		return ComparePrimitiveValues(low, other.low);
	}

	sl_compare_result Uint128::compare(const sl_uint64 other) const noexcept
	{
		if (high > 0) {
			return 1;
		}
		return ComparePrimitiveValues(low, other);
	}

	sl_bool operator==(sl_uint64 num, const Uint128& v) noexcept
	{
		return v.equals(num);
	}

	sl_bool operator!=(sl_uint64 num, const Uint128& v) noexcept
	{
		return !(v.equals(num));
	}

	sl_bool operator>=(sl_uint64 num, const Uint128& v) noexcept
	{
		return v.compare(num) <= 0;
	}

	sl_bool operator<=(sl_uint64 num, const Uint128& v) noexcept
	{
		return v.compare(num) >= 0;
	}

	sl_bool operator>(sl_uint64 num, const Uint128& v) noexcept
	{
		return v.compare(num) < 0;
	}

	sl_bool operator<(sl_uint64 num, const Uint128& v) noexcept
	{
		return v.compare(num) > 0;
	}

	sl_size Uint128::getHashCode() const noexcept
	{
		return Rehash64ToSize(high ^ low);
	}

	Uint128 Uint128::fromString(const StringParam& str, sl_uint32 radix) noexcept
	{
		Uint128 ret;
		if (ret.parse(str, radix)) {
			return ret;
		}
		ret.setZero();
		return ret;
	}

	String Uint128::toString(sl_uint32 radix) const noexcept
	{
		if (radix < 2 || radix > 64) {
			return sl_null;
		}
		Uint128 m = *this;
		if (m.isZero()) {
			SLIB_RETURN_STRING("0");
		}
		if (radix == 16) {
			char buf[33];
			buf[32] = 0;
			sl_uint32 posBuf = 32;
			while (m.isNotZero() && posBuf > 0) {
				posBuf--;
				sl_uint32 v = m.low & 15;
				if (v < 10) {
					buf[posBuf] = (sl_char8)(v + 0x30);
				} else {
					buf[posBuf] = (sl_char8)(v + 0x37);
				}
				m >>= 4;
			}
			return String(buf + posBuf, 32 - posBuf);
		} else {
			char buf[129];
			buf[128] = 0;
			sl_uint32 posBuf = 128;
			Uint128 _radix = radix;
			Uint128 r;
			while (m.isNotZero() && posBuf > 0) {
				posBuf--;
				if (div(m, _radix, &m, &r)) {
					sl_uint32 v = (sl_uint32)(r.low);
					if (v < radix) {
						buf[posBuf] = priv::string::g_conv_radixPatternUpper[v];
					} else {
						buf[posBuf] = '?';
					}
				} else {
					buf[posBuf] = '?';
				}
			}
			return String(buf + posBuf, 128 - posBuf);
		}
	}

	Uint128 Uint128::fromHexString(const StringParam& str) noexcept
	{
		return fromString(str, 16);
	}

	String Uint128::toHexString() const noexcept
	{
		return toString(16);
	}

	namespace priv
	{
		namespace uint128
		{

			template <class CT>
			SLIB_INLINE static sl_reg Parse(Uint128* out, sl_uint32 radix, const CT* sz, sl_size posBegin, sl_size len) noexcept
			{
				if (radix < 2 || radix > 64) {
					return SLIB_PARSE_ERROR;
				}
				sl_size pos = posBegin;
				Uint128 m;
				const sl_uint8* pattern = radix <= 36 ? priv::string::g_conv_radixInversePatternSmall : priv::string::g_conv_radixInversePatternBig;
				if (radix == 16) {
					for (; pos < len; pos++) {
						sl_uint32 c = (sl_uint8)(sz[pos]);
						sl_uint32 v = c < 128 ? pattern[c] : 255;
						if (v >= 16) {
							break;
						}
						m <<= 4;
						m |= v;
					}
				} else {
					for (; pos < len; pos++) {
						sl_uint32 c = (sl_uint8)(sz[pos]);
						sl_uint32 v = c < 128 ? pattern[c] : 255;
						if (v >= radix) {
							break;
						}
						m *= radix;
						m += v;
					}
				}
				if (pos == posBegin) {
					return SLIB_PARSE_ERROR;
				}
				if (out) {
					*out = m;
				}
				return pos;
			}

		}
	}

	SLIB_DEFINE_CLASS_PARSE_INT_MEMBERS(Uint128, priv::uint128::Parse)


	template <>
	void Math::pow10iT<Uint128>(Uint128& _out, sl_uint32 exponent) noexcept
	{
		_out = Uint128::pow10(exponent);
	}

	template <>
	sl_uint32 Math::log10iT<Uint128>(const Uint128& v) noexcept
	{
		return v.log10();
	}

}
