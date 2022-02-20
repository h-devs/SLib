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

#include "slib/math/decimal128.h"

#include "slib/math/decimal.h"
#include "slib/math/int128.h"
#include "slib/core/mio.h"
#include "slib/core/string.h"
#include "slib/core/hash.h"

#define COMBINATION_MASK 0x1f // least significant 5 bits
#define EXPONENT_MASK 0x3fff // least significant 14 bits
#define COMBINATION_INFINITY 30 // Value of combination field for Inf
#define COMBINATION_NAN 31 // Value of combination field for NaN

#define STRING_LENGTH 43
#define STRING_INF "Infinity"
#define STRING_NAN "NaN"
#define DIVISOR_1B 1000000000 // 1 Billion

#define EXPONENT_MAX 6111
#define EXPONENT_MIN -6176
#define EXPONENT_BIAS 6176
#define MAX_DIGITS SLIB_DECIMAL128_MAX_DIGITS
#define LOG10_OF_MAX (MAX_DIGITS-1)

namespace slib
{

	namespace priv
	{
		namespace decimal128
		{

			typedef Decimal<Uint128> DecimalOp;

			SLIB_INLINE static void ToOp(DecimalOp& op, const Decimal128& decimal)
			{
				sl_uint32 high0 = SLIB_GET_DWORD0(decimal.high);
				sl_uint32 high1 = SLIB_GET_DWORD1(decimal.high);

				sl_uint32 combination = (high1 >> 26) & COMBINATION_MASK; // Bits 1-5
				sl_uint32 exponentBiased; // decoded biased exponent (14 bits)
				sl_uint8 significandMsb; // the most signifcant significand bits (50-46)

				op.flagNegative = (sl_bool)(high1 >> 31);

				if (SLIB_UNLIKELY((combination >> 3) == 3)) {
					// Check for special values
					if (combination == COMBINATION_INFINITY) {
						op.type = DecimalValueType::Infinity;
						return;
					} else if (combination == COMBINATION_NAN) {
						op.type = DecimalValueType::NaN;
						return;
					} else {
						exponentBiased = (high1 >> 15) & EXPONENT_MASK;
						significandMsb = 8 + ((high1 >> 14) & 0x1);
					}
				} else {
					significandMsb = (high1 >> 14) & 0x7;
					exponentBiased = (high1 >> 17) & EXPONENT_MASK;
				}

				op.type = DecimalValueType::Normal;
				op.exponent = exponentBiased - EXPONENT_BIAS;
				op.significand.low = decimal.low;
				high1 = (high1 & 0x3fff) + ((significandMsb & 0xf) << 14);
				op.significand.high = SLIB_MAKE_QWORD4(high1, high0);
			}

			SLIB_INLINE static void FromOp(const DecimalOp& op, Decimal128& decimal)
			{
				if (op.type == DecimalValueType::NaN) {
					decimal.setNaN();
					return;
				}
				if (op.type == DecimalValueType::Infinity) {
					decimal.setInfinity(!(op.flagNegative));
					return;
				}
				if (op.isZero()) {
					decimal.setZero();
					return;
				}
				if (op.exponent < EXPONENT_MIN || op.exponent > EXPONENT_MAX) {
					decimal.setInfinity(!(op.flagNegative));
					return;
				}

				sl_uint16 exponentBiased = (sl_uint16)(op.exponent + EXPONENT_BIAS);

				if ((op.significand.high >> 49) & 1) {
					decimal.high = (SLIB_UINT64(0x3) << 61);
					decimal.high |= (sl_uint64)(exponentBiased & 0x3fff) << 47;
					decimal.high |= op.significand.high & SLIB_UINT64(0x7fffffffffff);
				} else {
					decimal.high = (sl_uint64)(exponentBiased & 0x3fff) << 49;
					decimal.high |= op.significand.high & SLIB_UINT64(0x1ffffffffffff);
				}
				decimal.low = op.significand.low;

				if (op.flagNegative) {
					decimal.high |= SLIB_UINT64(0x8000000000000000);
				}

			}

			template <class T>
			static sl_uint32 CopyString(T* dst, const char* src) noexcept
			{
				const char* begin = src;
				while ((*(dst++) = *(src++))) {}
				return (sl_uint32)(src - begin);
			}

			template <class T>
			static sl_compare_result EqualsStringIgnoreCase(const T* s1, sl_size limit, const char* s2) noexcept
			{
				if (limit > 512) {
					limit = 512;
				}
				const char* end = s2 + limit;
				while (s2 < end) {
					T c1 = SLIB_CHAR_LOWER_TO_UPPER(*(s1++));
					char c2 = SLIB_CHAR_LOWER_TO_UPPER(*(s2++));
					if (c1 != c2) {
						return sl_false;
					}
					if (!c1) {
						break;
					}
				}
				return sl_false;
			}

			// Referenced from Mongodb's BSON implementation
			template <class CHAR>
			static sl_uint32 ToString(const Decimal128* dec, CHAR* out)
			{
				CHAR* outStart = out;
				sl_uint32 significand[36] = { 0 };  // the base-10 digits in the significand

				if (((sl_int64)(dec->high)) < 0) { // negative
					*(out++) = '-';
				}

				sl_uint32 high0 = SLIB_GET_DWORD0(dec->high);
				sl_uint32 high1 = SLIB_GET_DWORD1(dec->high);

				sl_uint32 combination = (high1 >> 26) & COMBINATION_MASK; // Bits 1-5
				sl_uint32 exponentBiased; // decoded biased exponent (14 bits)
				sl_uint8 significandMsb; // the most signifcant significand bits (50-46)

				if (SLIB_UNLIKELY((combination >> 3) == 3)) {
					// Check for special values
					if (combination == COMBINATION_INFINITY) {
						return (sl_uint32)(out - outStart + (CopyString(out, STRING_INF) - 1));
					} else if (combination == COMBINATION_NAN) {
						return CopyString(outStart, STRING_NAN) - 1;
					} else {
						exponentBiased = (high1 >> 15) & EXPONENT_MASK;
						significandMsb = 8 + ((high1 >> 14) & 0x1);
					}
				} else {
					significandMsb = (high1 >> 14) & 0x7;
					exponentBiased = (high1 >> 17) & EXPONENT_MASK;
				}

				sl_int32 exponent = exponentBiased - EXPONENT_BIAS; // unbiased exponent

				// Convert the 114-bit binary number to at most 34 decimal digits through modulo and division.
				Uint128 significand128;
				significand128.low = dec->low;
				high1 = (high1 & 0x3fff) + ((significandMsb & 0xf) << 14);
				significand128.high = SLIB_MAKE_QWORD4(high1, high0);

				sl_bool flagZero = sl_false;
				if (significand128.isZero()) {
					flagZero = sl_true;
				} else if (high1 >= (1 << 17)) {
					// The significand is non-canonical or zero.
					flagZero = sl_true;
				} else {
					for (sl_int32 k = 3; k >= 0; k--) {
						sl_uint32 leastDigits = 0;
						Uint128::div32(significand128, DIVISOR_1B, &significand128, &leastDigits);
						// We now have the 9 least significant digits (in base 2). Convert and output to string.
						if (!leastDigits) {
							continue;
						}
						for (sl_int32 j = 8; j >= 0; j--) {
							significand[k * 9 + j] = leastDigits % 10;
							leastDigits /= 10;
						}
					}
				}

				// Output format options:
				//		Scientific - [-]d.dddE(+/-)dd or [-]dE(+/-)dd
				//		Regular    - ddd.ddd

				sl_uint32 nSignificandDigits = 0; // the number of significand digits
				sl_uint32* significandRead = significand; // read pointer into significand

				if (flagZero) {
					nSignificandDigits = 1;
					*significandRead = 0;
				} else {
					nSignificandDigits = 36;
					while (!(*significandRead)) {
						nSignificandDigits--;
						significandRead++;
					}
				}

				// the exponent if scientific notation is used
				sl_int32 exponentScientific = nSignificandDigits - 1 + exponent;

				// The scientific exponent checks are dictated by the string conversion specification and are somewhat arbitrary cutoffs. We must check exponent > 0, because if this is the case, the number has trailing zeros.  However, we *cannot* output these trailing zeros, because doing so would change the precision of the value, and would change stored data if the string converted number is round tripped.
				if (exponentScientific < -6 || exponent > 0) {
					// Scientific format
					*(out++) = *(significandRead++) + '0';
					nSignificandDigits--;
					if (nSignificandDigits) {
						*(out++) = '.';
					}
					for (sl_uint32 i = 0; i < nSignificandDigits && (out - outStart) < 36; i++) {
						*(out++) = *(significandRead++) + '0';
					}
					// Exponent
					*(out++) = 'E';
					if (exponentScientific > 0) {
						*(out++) = '+';
					}
					String strExponentScientific = String::fromInt32(exponentScientific);
					out += CopyString(out, strExponentScientific.getData()) - 1;
				} else {
					// Regular format with no decimal place
					if (exponent >= 0) {
						for (sl_uint32 i = 0; i < nSignificandDigits && (out - outStart) < 36; i++) {
							*(out++) = *(significandRead++) + '0';
						}
					} else {
						sl_int32 nDigitsAfterDecimal = nSignificandDigits + exponent;
						if (nDigitsAfterDecimal > 0) { // non-zero digits before radix
							for (sl_int32 i = 0; i < nDigitsAfterDecimal && (out - outStart) < STRING_LENGTH; i++) {
								*(out++) = *(significandRead++) + '0';
							}
						} else { // leading zero before radix point
							*(out++) = '0';
						}

						*(out++) = '.';
						while (nDigitsAfterDecimal++ < 0) { // add leading zeros after radix
							*(out++) = '0';
						}

						for (sl_int32 i = 0; (i < (sl_int32)nSignificandDigits - SLIB_MAX(nDigitsAfterDecimal - 1, 0)) && (out - outStart) < STRING_LENGTH; i++) {
							*(out++) = *(significandRead++) + '0';
						}
					}
				}

				return (sl_uint32)(out - outStart);
			}

			template <class CHAR>
			static sl_bool FromString(Decimal128* dec, const CHAR* input, sl_size& len)
			{
				if (!len) {
					return sl_false;
				}
				if (len > 0x10000000) {
					len = 0x10000000;
				}
				const CHAR* str = input;
				const CHAR* strEnd = input + len;

				sl_bool bNegative = sl_false;
				bool bIncludeNegative = sl_false;
				CHAR ch = *str;
				if (ch == '+' || ch == '-') {
					if (ch == '-') {
						bNegative = sl_true;
					}
					bIncludeNegative = sl_true;
					str++;
					if (str >= strEnd) {
						return sl_false;
					}
					ch = *str;
				}

				// Check for Infinity or NaN
				if (!SLIB_CHAR_IS_DIGIT(ch) && ch != '.') {
					sl_uint32 n = 0;
					if (EqualsStringIgnoreCase(str, strEnd - str, "infinity")) {
						n = 8;
						dec->setInfinity(!bNegative);
					} else if (EqualsStringIgnoreCase(str, strEnd - str, "inf")) {
						n = 3;
						dec->setInfinity(!bNegative);
					} else if (EqualsStringIgnoreCase(str, strEnd - str, "nan")) {
						n = 3;
						dec->setNaN();
					}
					if (n) {
						len = str + n - input;
						if (str + n < strEnd) {
							if (!(str[n]) || SLIB_CHAR_IS_WHITE_SPACE(str[n])) {
								return sl_true;
							}
						} else {
							return sl_true;
						}
					}
					return sl_false;
				}

				// Read digits
				sl_bool bSawDecimal = sl_false;
				sl_bool bFoundNonZero = sl_false;
				sl_uint32 posFirstNonZero = 0;

				sl_uint32 digits[MAX_DIGITS] = { 0 };
				sl_uint32* digitsInsert = digits; // Insertion pointer for digits
				sl_uint16 nDigitsStored = 0;
				sl_uint32 nDigitsRead = 0;
				sl_uint32 nDigits = 0; // Total number of digits (no leading zeros)
				sl_uint32 nDigitsAfterDecimal = 0;

				while ((SLIB_CHAR_IS_DIGIT(ch) || ch == '.')) {
					if (ch == '.') {
						if (bSawDecimal) {
							return sl_false;
						}
						bSawDecimal = sl_true;
						str++;
						if (str >= strEnd) {
							ch = 0;
							break;
						}
						ch = *str;
						continue;
					}
					if (nDigitsStored < 34) {
						if (ch != '0' || bFoundNonZero) {
							if (!bFoundNonZero) {
								posFirstNonZero = nDigitsRead;
							}
							bFoundNonZero = sl_true;
							*(digitsInsert++) = ch - '0'; // Only store 34 digits
							nDigitsStored++;
						}
					}
					if (bFoundNonZero) {
						nDigits++;
					}
					if (bSawDecimal) {
						nDigitsAfterDecimal++;
					}
					nDigitsRead++;
					str++;
					if (str >= strEnd) {
						ch = 0;
						break;
					}
					ch = *str;
				}

				if (bSawDecimal && !nDigitsRead) {
					return sl_false;
				}

				// Read exponent if exists
				sl_int32 exponent = 0;
				if (ch == 'e' || ch == 'E') {
					str++;
					if (str >= strEnd) {
						return sl_false;
					}
					if (*str == '+') {
						str++;
					}
					if (str >= strEnd) {
						return sl_false;
					}
					sl_reg nRead = StringTypeFromCharType<CHAR>::Type::parseInt32(10, &exponent, str);
					if (nRead == SLIB_PARSE_ERROR) {
						return sl_false;
					}
					str += nRead;
				}

				// Done reading input. Find first non-zero digit in digits
				sl_uint32 posFirstDigit = 0; // The index of the first non-zero digit
				sl_uint32 posLastDigit = 0; // The index of the last digit
				sl_uint32 nSignificantDigits = 0; // Total number of significant digits * (no leading or trailing zero)

				if (!nDigitsStored) {
					// value is zero
					posFirstDigit = 0;
					posLastDigit = 0;
					digits[0] = 0;
					nDigits = 1;
					nDigitsStored = 1;
					nSignificantDigits = 0;
				} else {
					posLastDigit = nDigitsStored - 1;
					nSignificantDigits = nDigits;
					// Mark trailing zeros as non-significant
					while (input[posFirstNonZero + nSignificantDigits - 1 + bIncludeNegative + bSawDecimal] == '0') {
						nSignificantDigits--;
					}
				}

				// Normalization of exponent: Correct exponent based on radix position, and shift significand as needed to represent user input

				// Overflow prevention
				if (exponent <= (sl_int32)nDigitsAfterDecimal && (sl_int32)nDigitsAfterDecimal - exponent > (1 << 14)) {
					exponent = EXPONENT_MIN;
				} else {
					exponent -= nDigitsAfterDecimal;
				}

				// Attempt to normalize the exponent
				while (exponent > EXPONENT_MAX) {
					// Shift exponent to significand and decrease
					posLastDigit++;
					if (posLastDigit - posFirstDigit > MAX_DIGITS) {
						// The exponent is too great to shift into the significand
						if (!nSignificantDigits) {
							// Value is zero, we are allowed to clamp the exponent
							exponent = EXPONENT_MAX;
							break;
						}
						return sl_false;
					}
					exponent--;
				}

				while (exponent < EXPONENT_MIN || nDigitsStored < nDigits) {
					//Shift last digit
					if (posLastDigit == 0) {
						// underflow is not allowed, but zero clamping is
						if (!nSignificantDigits) {
							exponent = EXPONENT_MIN;
							break;
						}
						return sl_false;
					}
					if (nDigitsStored < nDigits) {
						if (input[nDigits - 1 + bIncludeNegative + bSawDecimal] != '0' && nSignificantDigits) {
							return sl_false;
						}
						nDigits--; // adjust to match digits not stored
					} else {
						if (digits[posLastDigit]) {
							// Inexact rounding is not allowed
							return sl_true;
						}
						posLastDigit--; // adjust to round
					}

					if (exponent < EXPONENT_MAX) {
						exponent++;
					} else {
						return sl_false;
					}
				}

				// Round
				// We've normalized the exponent, but might still need to round
				if (posLastDigit - posFirstDigit + 1 < nSignificantDigits) {
					// There are non-zero digits after posLastDigit that need rounding. We round to nearest, ties to even
					CHAR digitRound = input[posFirstNonZero + posLastDigit + bIncludeNegative + bSawDecimal + 1] - '0';
					if (digitRound) {
						// Inexact (non-zero) rounding is not allowed
						return sl_false;
					}
				}

				// Encode significand
				sl_uint64 significandHigh = 0; // The high 17 digits of the significand
				sl_uint64 significandLow = 0; // The low 17 digits of the significand

				if (!nSignificantDigits) {
					// read a zero 
					significandHigh = 0;
					significandLow = 0;
				} else if (posLastDigit - posFirstDigit < 17) {
					sl_uint32 index = posFirstDigit;
					significandLow = digits[index++];
					for (; index <= posLastDigit; index++) {
						significandLow *= 10;
						significandLow += digits[index];
						significandHigh = 0;
					}
				} else {
					sl_uint32 index = posFirstDigit;
					significandHigh = digits[index++];
					for (; index <= posLastDigit - 17; index++) {
						significandHigh *= 10;
						significandHigh += digits[index];
					}
					significandLow = digits[index++];
					for (; index <= posLastDigit; index++) {
						significandLow *= 10;
						significandLow += digits[index];
					}
				}

				{
					sl_uint64 t = significandLow;
					Math::mul64(significandHigh, SLIB_UINT64(100000000000000000), significandHigh, significandLow);
					significandLow += t;
					if (significandLow < t) {
						significandHigh++;
					}
				}

				sl_uint16 exponentBiased = (exponent + (sl_int16)EXPONENT_BIAS);

				// Encode combination, exponent, and significand.
				if ((significandHigh >> 49) & 1) {
					// Encode '11' into bits 1 to 3
					dec->high = (SLIB_UINT64(0x3) << 61);
					dec->high |= (sl_uint64)(exponentBiased & 0x3fff) << 47;
					dec->high |= significandHigh & SLIB_UINT64(0x7fffffffffff);
				} else {
					dec->high = (sl_uint64)(exponentBiased & 0x3fff) << 49;
					dec->high |= significandHigh & SLIB_UINT64(0x1ffffffffffff);
				}
				dec->low = significandLow;

				// Encode sign
				if (bNegative) {
					dec->high |= SLIB_UINT64(0x8000000000000000);
				}

				len = str - input;
				return sl_true;
			}

			template <class CHAR>
			SLIB_INLINE static sl_reg Parse(Decimal128* _out, const CHAR *sz, sl_size posBegin, sl_size posEnd) noexcept
			{
				if (posEnd <= posBegin) {
					return SLIB_PARSE_ERROR;
				}
				sl_size len = posEnd - posBegin;
				if (FromString(_out, sz + posBegin, len)) {
					return posBegin + len;
				}
				return SLIB_PARSE_ERROR;
			}

		}
	}

	using namespace priv::decimal128;

	SLIB_ALIGN(8) const sl_uint64 Decimal128::_zero[2] = { 0, 0 };

	const Decimal128& Decimal128::infinity() noexcept
	{
		static sl_uint64 v[] = { 0x7800000000000000, 0};
		return *((Decimal128*)((void*)v));
	}

	const Decimal128& Decimal128::negativeInfinity() noexcept
	{
		static sl_uint64 v[] = { 0xF800000000000000, 0 };
		return *((Decimal128*)((void*)v));
	}

	const Decimal128& Decimal128::NaN() noexcept
	{
		static sl_uint64 v[] = { 0x7C00000000000000, 0 };
		return *((Decimal128*)((void*)v));
	}

	sl_bool Decimal128::isInfinity() const noexcept
	{
		sl_uint32 high1 = SLIB_GET_DWORD1(high);
		sl_uint32 combination = (high1 >> 26) & COMBINATION_MASK;
		if (combination == COMBINATION_INFINITY) {
			return sl_true;
		}
		return sl_false;
	}

	void Decimal128::setInfinity(sl_bool flagPositive) noexcept
	{
		if (flagPositive) {
			high = SLIB_UINT64(0x7800000000000000);
		} else {
			high = SLIB_UINT64(0xF800000000000000);
		}
		low = 0;
	}

	sl_bool Decimal128::isPositiveInfinity() const noexcept
	{
		return isInfinity() && isPositive();
	}

	void Decimal128::setPositiveInfinity() noexcept
	{
		high = SLIB_UINT64(0x7800000000000000);
		low = 0;
	}

	sl_bool Decimal128::isNegativeInfinity() const noexcept
	{
		return isInfinity() && isNegative();
	}

	void Decimal128::setNegativeInfinity() noexcept
	{
		high = SLIB_UINT64(0xF800000000000000);
		low = 0;
	}

	sl_bool Decimal128::isNaN() const noexcept
	{
		sl_uint32 high1 = SLIB_GET_DWORD1(high);
		sl_uint32 combination = (high1 >> 26) & COMBINATION_MASK;
		if (combination == COMBINATION_NAN) {
			return sl_true;
		}
		return sl_false;
	}

	void Decimal128::setNaN() noexcept
	{
		high = SLIB_UINT64(0x7C00000000000000);
		low = 0;
	}

	void Decimal128::getBytesBE(void* _buf) noexcept
	{
		char* buf = (char*)_buf;
		MIO::writeUint64BE(buf, high);
		MIO::writeUint64BE(buf + 8, low);
	}

	void Decimal128::setBytesBE(const void* _buf) noexcept
	{
		const char* buf = (const char*)_buf;
		high = MIO::readUint64BE(buf);
		low = MIO::readUint64BE(buf + 8);
	}

	void Decimal128::getBytesLE(void* _buf) noexcept
	{
		char* buf = (char*)_buf;
		MIO::writeUint64LE(buf, low);
		MIO::writeUint64LE(buf + 8, high);
	}

	void Decimal128::setBytesLE(const void* _buf) noexcept
	{
		const char* buf = (const char*)_buf;
		low = MIO::readUint64LE(buf);
		high = MIO::readUint64LE(buf + 8);
	}

	Decimal128 Decimal128::fromString(const StringParam& str) noexcept
	{
		Decimal128 ret;
		if (ret.parse(str)) {
			return ret;
		}
		ret.setNaN();
		return ret;
	}

	sl_compare_result Decimal128::compare(const Decimal128& other) const noexcept
	{
		DecimalOp op1, op2;
		ToOp(op1, *this);
		ToOp(op2, other);
		return op1.compare(op2, LOG10_OF_MAX);
	}

	sl_bool Decimal128::equals(const Decimal128& other) const noexcept
	{
		return !(compare(other));
	}

	sl_size Decimal128::getHashCode() const noexcept
	{
		return Rehash64ToSize(high ^ low);
	}

	String Decimal128::toString() const noexcept
	{
		char str[STRING_LENGTH];
		sl_uint32 n = ToString(this, str);
		return String(str, n);
	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(Decimal128, Parse)

	Decimal128 Decimal128::operator+(const Decimal128& other) const noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op.add(op1, op2, LOG10_OF_MAX);
		Decimal128 ret;
		FromOp(op, ret);
		return ret;
	}

	Decimal128& Decimal128::operator+=(const Decimal128& other) noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op.add(op1, op2, LOG10_OF_MAX);
		FromOp(op, *this);
		return *this;
	}

	Decimal128 Decimal128::operator-(const Decimal128& other) const noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op2.flagNegative = !(op2.flagNegative);
		op.add(op1, op2, LOG10_OF_MAX);
		Decimal128 ret;
		FromOp(op, ret);
		return ret;
	}

	Decimal128& Decimal128::operator-=(const Decimal128& other) noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op2.flagNegative = !(op2.flagNegative);
		op.add(op1, op2, LOG10_OF_MAX);
		FromOp(op, *this);
		return *this;
	}

	Decimal128 Decimal128::operator*(const Decimal128& other) const noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op.multiply(op1, op2, LOG10_OF_MAX);
		Decimal128 ret;
		FromOp(op, ret);
		return ret;
	}

	Decimal128& Decimal128::operator*=(const Decimal128& other) noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op.multiply(op1, op2, LOG10_OF_MAX);
		FromOp(op, *this);
		return *this;
	}

	Decimal128 Decimal128::operator/(const Decimal128& other) const noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op.divide(op1, op2, LOG10_OF_MAX);
		Decimal128 ret;
		FromOp(op, ret);
		return ret;
	}

	Decimal128& Decimal128::operator/=(const Decimal128& other) noexcept
	{
		DecimalOp op1, op2, op;
		ToOp(op1, *this);
		ToOp(op2, other);
		op.divide(op1, op2, LOG10_OF_MAX);
		FromOp(op, *this);
		return *this;
	}

	Decimal128 Decimal128::operator-() const noexcept
	{
		return Decimal128(high ^ SLIB_UINT64(0x8000000000000000), low);
	}

}
