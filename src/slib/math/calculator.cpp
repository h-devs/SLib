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

#include "slib/math/calculator.h"

#include "slib/core/math.h"
#include "slib/core/parse.h"

namespace slib
{

	namespace priv
	{
		namespace calculator
		{

			template <class CHAR>
			SLIB_INLINE static sl_reg ParseNumber(sl_int32& result, const CHAR* str, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CHAR>::Type::parseInt32(10, &result, str, posBegin, posEnd);
			}

			template <class CHAR>
			SLIB_INLINE static sl_reg ParseNumber(sl_int64& result, const CHAR* str, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CHAR>::Type::parseInt64(10, &result, str, posBegin, posEnd);
			}

			template <class CHAR>
			SLIB_INLINE static sl_reg ParseNumber(float& result, const CHAR* str, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CHAR>::Type::parseFloat(&result, str, posBegin, posEnd);
			}

			template <class CHAR>
			SLIB_INLINE static sl_reg ParseNumber(double& result, const CHAR* str, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CHAR>::Type::parseDouble(&result, str, posBegin, posEnd);
			}

			template <class NUMBER, class CHAR>
			static sl_reg Calculate(NUMBER* result, sl_bool* isDivisionByZero, const CHAR* str, sl_size posBegin, sl_size posEnd) noexcept
			{
				sl_bool flagDivisionByZero = sl_false;
				NUMBER accum1 = 0;
				NUMBER accum2 = 0;
				NUMBER value = 0;
				CHAR opAccum1 = 0;
				CHAR opAccum2 = 0;
				sl_size pos = posBegin;
				if (pos >= posEnd) {
					return SLIB_PARSE_ERROR;
				}
				for (;;) {
					while (SLIB_CHAR_IS_WHITE_SPACE(str[pos])) {
						pos++;
						if (pos >= posEnd) {
							return SLIB_PARSE_ERROR;
						}
					}
					sl_bool flagMinusParentheses = sl_false;
					if (str[pos] == '-') {
						sl_size pos2 = pos + 1;
						if (pos2 >= posEnd) {
							return SLIB_PARSE_ERROR;
						}
						while (SLIB_CHAR_IS_WHITE_SPACE(str[pos2])) {
							pos2++;
							if (pos2 >= posEnd) {
								return SLIB_PARSE_ERROR;
							}
						}
						if (str[pos2] == '(') {
							pos = pos2;
							flagMinusParentheses = sl_true;
						}
					}
					if (str[pos] == '(') {
						pos++;
						sl_bool flagDivisionByZeroInner;
						pos = Calculate(&value, &flagDivisionByZeroInner, str, pos, posEnd);
						if (pos == SLIB_PARSE_ERROR) {
							return SLIB_PARSE_ERROR;
						}
						if (flagDivisionByZeroInner) {
							flagDivisionByZero = sl_true;
							value = 0;
						}
						if (pos >= posEnd) {
							return SLIB_PARSE_ERROR;
						}
						while (SLIB_CHAR_IS_WHITE_SPACE(str[pos])) {
							pos++;
							if (pos >= posEnd) {
								return SLIB_PARSE_ERROR;
							}
						}
						if (str[pos] != ')') {
							return SLIB_PARSE_ERROR;
						}
						if (flagMinusParentheses) {
							value = -value;
						}
						pos++;
					} else {
						pos = ParseNumber(value, str, pos, posEnd);
						if (pos == SLIB_PARSE_ERROR) {
							return SLIB_PARSE_ERROR;
						}
					}
					if (opAccum2) {
						if (opAccum2 == '*') {
							accum2 *= value;
						} else if (opAccum2 == '/') {
							if (Math::isAlmostZero(value)) {
								accum2 = 0;
								flagDivisionByZero = sl_true;
							} else {
								accum2 /= value;
							}
						} else {
							return SLIB_PARSE_ERROR;
						}
					} else {
						accum2 = value;
					}
					if (pos < posEnd) {
						while (SLIB_CHAR_IS_WHITE_SPACE(str[pos])) {
							pos++;
							if (pos >= posEnd) {
								break;
							}
						}
					}
					if (pos < posEnd && (str[pos] == '*' || str[pos] == '/')) {
						opAccum2 = str[pos];
						pos++;
					} else {
						if (opAccum1) {
							if (opAccum1 == '+') {
								accum1 += accum2;
							} else if (opAccum1 == '-') {
								accum1 -= accum2;
							} else {
								return SLIB_PARSE_ERROR;
							}
						} else {
							accum1 = accum2;
						}
						if (pos < posEnd && (str[pos] == '+' || str[pos] == '-')) {
							opAccum2 = 0;
							opAccum1 = str[pos];
							pos++;
						} else {
							if (flagDivisionByZero) {
								accum1 = 0;
							}
							if (isDivisionByZero) {
								*isDivisionByZero = flagDivisionByZero;
							}
							if (result) {
								*result = accum1;
							}
							return pos;
						}
					}
				}
				return SLIB_PARSE_ERROR;
			}

			template <class VIEW, class NUMBER>
			static sl_bool CalculateSv(const VIEW& str, NUMBER* result, sl_bool* isDivisionByZero) noexcept
			{
				typename VIEW::Char* data = str.getUnsafeData();
				sl_reg n = str.getUnsafeLength();
				if (n >= 0) {
					return Calculate(result, isDivisionByZero, data, 0, n) == n;
				} else {
					sl_reg ret = Calculate(result, isDivisionByZero, data, 0, SLIB_SIZE_MAX);
					if (ret != SLIB_PARSE_ERROR && !(data[ret])) {
						return sl_true;
					}
					return sl_false;
				}
			}

			template <class NUMBER>
			static sl_bool Calculate(const StringParam& str, NUMBER* result, sl_bool* isDivisionByZero) noexcept
			{
				if (str.isEmpty()) {
					return sl_false;
				}
				if (str.is8BitsStringType()) {
					return CalculateSv(StringData(str), result, isDivisionByZero);
				} else if (str.is16BitsStringType()) {
					return CalculateSv(StringData16(str), result, isDivisionByZero);
				} else {
					return CalculateSv(StringData32(str), result, isDivisionByZero);
				}
			}

		}
	}

	using namespace priv::calculator;

	sl_reg Calculator::calculate(sl_int32* result, sl_bool* isDivisionByZero, const sl_char8* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(sl_int32* result, sl_bool* isDivisionByZero, const sl_char16* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(sl_int32* result, sl_bool* isDivisionByZero, const sl_char32* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, sl_int32* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}

	sl_reg Calculator::calculate(sl_int64* result, sl_bool* isDivisionByZero, const sl_char8* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(sl_int64* result, sl_bool* isDivisionByZero, const sl_char16* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(sl_int64* result, sl_bool* isDivisionByZero, const sl_char32* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, sl_int64* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}

	sl_reg Calculator::calculate(float* result, sl_bool* isDivisionByZero, const sl_char8* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(float* result, sl_bool* isDivisionByZero, const sl_char16* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(float* result, sl_bool* isDivisionByZero, const sl_char32* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, float* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}

	sl_reg Calculator::calculate(double* result, sl_bool* isDivisionByZero, const sl_char8* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(double* result, sl_bool* isDivisionByZero, const sl_char16* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(double* result, sl_bool* isDivisionByZero, const sl_char32* str, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, str, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, double* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}

}
