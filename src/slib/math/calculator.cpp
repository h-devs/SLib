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
			
			template <class CT>
			SLIB_INLINE static sl_reg ParseNumber(sl_int32& result, const CT* sz, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CT>::Type::parseInt32(10, &result, sz, posBegin, posEnd);
			}
			
			template <class CT>
			SLIB_INLINE static sl_reg ParseNumber(sl_int64& result, const CT* sz, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CT>::Type::parseInt64(10, &result, sz, posBegin, posEnd);
			}
			
			template <class CT>
			SLIB_INLINE static sl_reg ParseNumber(float& result, const CT* sz, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CT>::Type::parseFloat(&result, sz, posBegin, posEnd);
			}
			
			template <class CT>
			SLIB_INLINE static sl_reg ParseNumber(double& result, const CT* sz, sl_size posBegin, sl_size posEnd) noexcept
			{
				return StringTypeFromCharType<CT>::Type::parseDouble(&result, sz, posBegin, posEnd);
			}

			template <class RT, class CT>
			static sl_reg Calculate(RT* result, sl_bool* isDivisionByZero, const CT* sz, sl_size posBegin, sl_size posEnd) noexcept
			{
				sl_bool flagDivisionByZero = sl_false;
				RT accum1 = 0;
				RT accum2 = 0;
				RT value = 0;
				CT opAccum1 = 0;
				CT opAccum2 = 0;
				sl_size pos = posBegin;
				if (pos >= posEnd) {
					return SLIB_PARSE_ERROR;
				}
				for (;;) {
					while (SLIB_CHAR_IS_WHITE_SPACE(sz[pos])) {
						pos++;
						if (pos >= posEnd) {
							return SLIB_PARSE_ERROR;
						}
					}
					sl_bool flagMinusParentheses = sl_false;
					if (sz[pos] == '-') {
						sl_size pos2 = pos + 1;
						if (pos2 >= posEnd) {
							return SLIB_PARSE_ERROR;
						}
						while (SLIB_CHAR_IS_WHITE_SPACE(sz[pos2])) {
							pos2++;
							if (pos2 >= posEnd) {
								return SLIB_PARSE_ERROR;
							}
						}
						if (sz[pos2] == '(') {
							pos = pos2;
							flagMinusParentheses = sl_true;
						}
					}
					if (sz[pos] == '(') {
						pos++;
						sl_bool flagDivisionByZeroInner;
						pos = Calculate(&value, &flagDivisionByZeroInner, sz, pos, posEnd);
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
						while (SLIB_CHAR_IS_WHITE_SPACE(sz[pos])) {
							pos++;
							if (pos >= posEnd) {
								return SLIB_PARSE_ERROR;
							}
						}
						if (sz[pos] != ')') {
							return SLIB_PARSE_ERROR;
						}
						if (flagMinusParentheses) {
							value = -value;
						}
						pos++;
					} else {
						pos = ParseNumber(value, sz, pos, posEnd);
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
						while (SLIB_CHAR_IS_WHITE_SPACE(sz[pos])) {
							pos++;
							if (pos >= posEnd) {
								break;
							}
						}
					}
					if (pos < posEnd && (sz[pos] == '*' || sz[pos] == '/')) {
						opAccum2 = sz[pos];
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
						if (pos < posEnd && (sz[pos] == '+' || sz[pos] == '-')) {
							opAccum2 = 0;
							opAccum1 = sz[pos];
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

			template <class RT>
			static sl_bool Calculate(const StringParam& _str, RT* result, sl_bool* isDivisionByZero) noexcept
			{
				if (_str.isEmpty()) {
					return sl_false;
				}
				if (_str.is8()) {
					StringData str(_str);
					sl_char8* data = str.getUnsafeData();
					sl_reg n = str.getUnsafeLength();
					if (n >= 0) {
						return Calculate(result, isDivisionByZero, data, 0, n) == n;
					} else {
						sl_reg ret = Calculate(result, isDivisionByZero, data, 0, SLIB_SIZE_MAX);
						if (ret != SLIB_PARSE_ERROR && data[ret] == 0) {
							return sl_true;
						}
						return sl_false;
					}
				} else {
					StringData16 str(_str);
					sl_char16* data = str.getUnsafeData();
					sl_reg n = str.getUnsafeLength();
					if (n >= 0) {
						return Calculate(result, isDivisionByZero, data, 0, n) == n;
					} else {
						sl_reg ret = Calculate(result, isDivisionByZero, data, 0, SLIB_SIZE_MAX);
						if (ret != SLIB_PARSE_ERROR && data[ret] == 0) {
							return sl_true;
						}
						return sl_false;
					}
				}
			}

		}
	}

	using namespace priv::calculator;

	sl_reg Calculator::calculate(sl_int32* result, sl_bool* isDivisionByZero, const sl_char8* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}

	sl_reg Calculator::calculate(sl_int32* result, sl_bool* isDivisionByZero, const sl_char16* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, sl_int32* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}
	
	sl_reg Calculator::calculate(sl_int64* result, sl_bool* isDivisionByZero, const sl_char8* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}
	
	sl_reg Calculator::calculate(sl_int64* result, sl_bool* isDivisionByZero, const sl_char16* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, sl_int64* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}

	sl_reg Calculator::calculate(float* result, sl_bool* isDivisionByZero, const sl_char8* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}
	
	sl_reg Calculator::calculate(float* result, sl_bool* isDivisionByZero, const sl_char16* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, float* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}

	sl_reg Calculator::calculate(double* result, sl_bool* isDivisionByZero, const sl_char8* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}
	
	sl_reg Calculator::calculate(double* result, sl_bool* isDivisionByZero, const sl_char16* sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return Calculate(result, isDivisionByZero, sz, posBegin, posEnd);
	}

	sl_bool Calculator::calculate(const StringParam& str, double* result, sl_bool* isDivisionByZero) noexcept
	{
		return Calculate(str, result, isDivisionByZero);
	}
	
}
