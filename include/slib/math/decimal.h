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

#ifndef CHECKHEADER_SLIB_MATH_DECIMAL
#define CHECKHEADER_SLIB_MATH_DECIMAL

#include "definition.h"

#include "../core/math.h"

namespace slib
{
	
	enum class DecimalValueType
	{
		Normal = 0,
		Infinity = 1,
		NaN = 2
	};

	template <typename SIGNIFICAND_TYPE>
	class SLIB_EXPORT Decimal
	{
	public:
		SIGNIFICAND_TYPE significand;
		sl_int32 exponent;
		DecimalValueType type;
		sl_bool flagNegative;

	public:
		Decimal() noexcept: flagNegative(sl_false), type(DecimalValueType::Normal) {}
		
	public:
		SLIB_CONSTEXPR sl_bool isZero() const
		{
			return type == DecimalValueType::Normal && significand == SIGNIFICAND_TYPE(0);
		}

		void setZero() noexcept
		{
			type = DecimalValueType::Normal;
			significand = 0;
			exponent = 0;
			flagNegative = sl_false;
		}

		sl_compare_result compare(const Decimal& other, sl_int32 log10OfMax) const noexcept
		{
			if (type == DecimalValueType::NaN) {
				if (other.type == DecimalValueType::NaN) {
					return 0;
				} else {
					return 1;
				}
			} else {
				if (other.type == DecimalValueType::NaN) {
					return -1;
				}
			}
			if (isZero()) {
				if (other.isZero()) {
					return 0;
				} else {
					if (other.flagNegative) {
						return 1;
					} else {
						return -1;
					}
				}
			} else {
				if (other.isZero()) {
					if (flagNegative) {
						return -1;
					} else {
						return 1;
					}
				}
			}
			if (flagNegative) {
				if (!(other.flagNegative)) {
					return -1;
				}
				return -_compareAbs(other, log10OfMax);
			} else {
				if (other.flagNegative) {
					return 1;
				}
				return _compareAbs(other, log10OfMax);
			}
		}

		void add(const Decimal& a, const Decimal& b, sl_int32 log10OfMax) noexcept
		{
			if (a.type == DecimalValueType::NaN || b.type == DecimalValueType::NaN) {
				type = DecimalValueType::NaN;
				return;
			}
			if (a.isZero()) {
				*this = b;
				return;
			}
			if (b.isZero()) {
				*this = a;
				return;
			}
			if (a.flagNegative) {
				if (b.flagNegative) {
					_addAbs(a, b, log10OfMax);
					flagNegative = sl_true;
				} else {
					_subtractAbs(b, a, log10OfMax);
				}
			} else {
				if (b.flagNegative) {
					_subtractAbs(a, b, log10OfMax);
				} else {
					_addAbs(a, b, log10OfMax);
					flagNegative = sl_false;
				}
			}
		}

		void multiply(const Decimal& a, const Decimal& b, sl_int32 log10OfMax) noexcept
		{
			if (a.type == DecimalValueType::NaN || b.type == DecimalValueType::NaN) {
				type = DecimalValueType::NaN;
				return;
			}
			if (a.isZero() || b.isZero()) {
				setZero();
				return;
			}
			flagNegative = !(a.flagNegative) != !(b.flagNegative);
			if (a.type == DecimalValueType::Infinity || b.type == DecimalValueType::Infinity) {
				type = DecimalValueType::Infinity;
				return;
			}
			type = DecimalValueType::Normal;
			exponent = a.exponent + b.exponent;
			sl_uint32 la = Math::log10iT(a.significand);
			sl_uint32 lb = Math::log10iT(b.significand);
			if (la + lb + 1 <= (sl_uint32)log10OfMax) {
				significand = a.significand * b.significand;
			} else {
				sl_uint32 k = la + lb - log10OfMax + 1;
				sl_uint32 ka = k * la / (la + lb);
				sl_uint32 kb = k - ka;
				if (ka >= la) {
					ka = la - 1;
				}
				if (kb >= lb) {
					kb = lb - 1;
				}
				SIGNIFICAND_TYPE scaleA;
				Math::pow10iT(scaleA, ka);
				SIGNIFICAND_TYPE scaleB;
				Math::pow10iT(scaleB, kb);
				significand = (a.significand / scaleA) * (b.significand / scaleB);
				exponent += ka + kb;
			}
		}

		void divide(const Decimal& a, const Decimal& b, sl_int32 log10OfMax) noexcept
		{
			if (a.type == DecimalValueType::NaN || b.type == DecimalValueType::NaN) {
				type = DecimalValueType::NaN;
				return;
			}
			if (a.isZero()) {
				if (b.isZero()) {
					type = DecimalValueType::NaN;
				} else {
					setZero();
				}
				return;
			}
			if (b.type == DecimalValueType::Infinity) {
				if (a.type == DecimalValueType::Infinity) {
					type = DecimalValueType::NaN;
				} else {
					setZero();
				}
				return;
			}
			flagNegative = !(a.flagNegative) != !(b.flagNegative);
			if (a.type == DecimalValueType::Infinity || b.isZero()) {
				type = DecimalValueType::Infinity;
				return;
			}
			type = DecimalValueType::Normal;
			exponent = a.exponent - b.exponent;
			sl_int32 la = Math::log10iT(a.significand);
			sl_int32 lb = Math::log10iT(b.significand);
			sl_int32 k = lb + log10OfMax / 3;
			if (k > log10OfMax - 1) {
				k = log10OfMax - 1;
			}
			if (la >= k) {
				significand = a.significand / b.significand;
			} else {
				SIGNIFICAND_TYPE scaleA;
				Math::pow10iT(scaleA, k - la);
				significand = a.significand * scaleA / b.significand;
				exponent -= (k - la);
			}
		}

	private:
		sl_compare_result _compareAbs(const Decimal& other, sl_int32 log10OfMax) const noexcept
		{
			if (type == DecimalValueType::Infinity) {
				if (other.type == DecimalValueType::Infinity) {
					return 0;
				} else {
					return 1;
				}
			} else {
				if (other.type == DecimalValueType::Infinity) {
					return -1;
				}
			}
			sl_compare_result c = Compare<SIGNIFICAND_TYPE>()(significand, other.significand);
			if (exponent == other.exponent) {
				return c;
			}
			sl_int32 diffExponent;
			if (exponent > other.exponent) {
				diffExponent = exponent - other.exponent;
				if (c >= 0 || diffExponent >= log10OfMax) {
					return 1;
				}
			} else {
				diffExponent = other.exponent - exponent;
				if (c <= 0 || diffExponent >= log10OfMax) {
					return -1;
				}
			}
			sl_int32 l1 = Math::log10iT(significand) + exponent;
			sl_int32 l2 = Math::log10iT(other.significand) + other.exponent;
			if (l1 > l2) {
				return 1;
			} else if (l1 < l2) {
				return -1;
			}
			SIGNIFICAND_TYPE p;
			Math::pow10iT(p, diffExponent);
			if (exponent > other.exponent) {
				return Compare<SIGNIFICAND_TYPE>()(significand * p, other.significand);
			} else {
				return Compare<SIGNIFICAND_TYPE>()(significand, other.significand * p);
			}
		}

		void _addAbs(sl_bool flagSubtract, const Decimal& _a, const Decimal& _b, sl_int32 log10OfMax) noexcept
		{
			if (_a.type == DecimalValueType::Infinity) {
				type = DecimalValueType::Infinity;
				if (flagSubtract) {
					flagNegative = sl_false;
				}
				return;
			}
			if (_b.type == DecimalValueType::Infinity) {
				type = DecimalValueType::Infinity;
				if (flagSubtract) {
					flagNegative = sl_true;
				}
				return;
			}
			type = DecimalValueType::Normal;
			sl_int32 la = Math::log10iT(_a.significand);
			sl_int32 lb = Math::log10iT(_b.significand);
			sl_int32 diff = _a.exponent - _b.exponent;
			if (!diff) {
				exponent = _a.exponent;
				_addSignificand(flagSubtract, _a.significand, _b.significand);
				if (!flagSubtract) {
					if (SLIB_MAX(la, lb) >= log10OfMax) {
						exponent++;
						significand /= (sl_uint32)10;
					}
				}
				return;
			}
			const Decimal* pa;
			const Decimal* pb;
			if (diff < 0) {
				pa = &_b;
				pb = &_a;
				sl_int32 temp = lb;
				lb = la;
				la = temp;
				diff = -diff;
			} else {
				pa = &_a;
				pb = &_b;
			}
			const Decimal& a = *pa;
			const Decimal& b = *pb;
			if (diff >= log10OfMax) {
				exponent = a.exponent;
				significand = a.significand;
				if (flagSubtract) {
					flagNegative = sl_false;
				}
			} else {
				sl_bool flagDivide = sl_false;
				sl_int32 na = la + diff;
				sl_int32 k = 0;
				if (flagSubtract) {
					if (na > log10OfMax) {
						flagDivide = sl_true;
						k = na - log10OfMax;
					}
				} else {
					if (na < lb) {
						na = lb;
					}
					if (na >= log10OfMax) {
						flagDivide = sl_true;
						k = na - log10OfMax + 1;
					}
				}
				if (flagDivide) {
					exponent = b.exponent + k;
					SIGNIFICAND_TYPE scaleB;
					Math::pow10iT(scaleB, k);
					if (diff == k) {
						_addSignificand(flagSubtract, a.significand, b.significand / scaleB);
					} else if (diff > k) {
						SIGNIFICAND_TYPE scaleA;
						Math::pow10iT(scaleA, diff - k);
						_addSignificand(flagSubtract, a.significand * scaleA, b.significand / scaleB);
					} else {
						SIGNIFICAND_TYPE scaleA;
						Math::pow10iT(scaleA, k - diff);
						_addSignificand(flagSubtract, a.significand / scaleA, b.significand / scaleB);
					}
				} else {
					exponent = b.exponent;
					SIGNIFICAND_TYPE scaleA;
					Math::pow10iT(scaleA, diff);
					_addSignificand(flagSubtract, a.significand * scaleA, b.significand);
				}
			}
			if (flagSubtract && pa != &_a) {
				flagNegative = !flagNegative;
			}
		}

		void _addAbs(const Decimal& a, const Decimal& b, sl_int32 log10OfMax) noexcept
		{
			_addAbs(sl_false, a, b, log10OfMax);
		}
	
		void _subtractAbs(const Decimal& a, const Decimal& b, sl_int32 log10OfMax) noexcept
		{
			_addAbs(sl_true, a, b, log10OfMax);
		}

		void _addSignificand(sl_bool flagSubtract, const SIGNIFICAND_TYPE& a, const SIGNIFICAND_TYPE& b) noexcept
		{
			if (flagSubtract) {
				if (a >= b) {
					significand = a - b;
					flagNegative = sl_false;
				} else {
					significand = b - a;
					flagNegative = sl_true;
				}
			} else {
				significand = a + b;
			}
		}

	};

}

#endif

