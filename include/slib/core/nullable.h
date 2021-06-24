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

#ifndef CHECKHEADER_SLIB_CORE_NULLABLE
#define CHECKHEADER_SLIB_CORE_NULLABLE

#include "compare.h"
#include "hash.h"

namespace slib
{
	
	template <class T>
	class Nullable
	{
	public:
		Nullable(): flagNull(sl_true) {}
		
		Nullable(const Nullable& other): flagNull(other.flagNull), value(other.value) {}
		
		Nullable(Nullable&& other): flagNull(Move(other.flagNull)), value(Move(other.value)) {}
		
		Nullable(sl_null_t): flagNull(sl_true) {}
		
		template <class OTHER>
		Nullable(const Nullable<OTHER>& other): flagNull(other.flagNull), value(other.value) {}
		
		template <class... ARGS>
		Nullable(ARGS... args): flagNull(sl_false), value(Forward<ARGS...>(args...)) {}

	public:
		operator T const&() const noexcept
		{
			return value;
		}

		operator T&() noexcept
		{
			return value;
		}
		
		Nullable& operator=(const Nullable& other)
		{
			flagNull = other.flagNull;
			value = other.value;
			return *this;
		}
		
		Nullable& operator=(Nullable&& other)
		{
			flagNull = other.flagNull;
			value = Move(other.value);
			return *this;
		}
		
		Nullable& operator=(sl_null_t)
		{
			flagNull = sl_true;
			value = T();
			return *this;
		}
		
		template <class OTHER>
		Nullable& operator=(const Nullable<OTHER>& other)
		{
			flagNull = other.flagNull;
			value = other.value;
			return *this;
		}
		
		template <class ARG>
		Nullable& operator=(ARG&& arg)
		{
			flagNull = sl_false;
			value = Forward<ARG>(arg);
			return *this;
		}
		
	public:
		constexpr sl_bool isNull() const
		{
			return flagNull;
		}
		
		constexpr sl_bool isNotNull() const
		{
			return !flagNull;
		}
		
		void setNull()
		{
			flagNull = sl_true;
			value = T();
		}

		T const& get() const noexcept
		{
			return value;
		}
		
		T& get() noexcept
		{
			return value;
		}
		
	public:
		sl_compare_result compare(const Nullable& other) const noexcept
		{
			if (flagNull) {
				if (other.flagNull) {
					return 0;
				} else {
					return -1;
				}
			} else {
				if (other.flagNull) {
					return 1;
				} else {
					return Compare<T>()(value, other.value);
				}
			}
		}

		sl_compare_result compare(const T& other) const noexcept
		{
			if (flagNull) {
				return -1;
			} else {
				return Compare<T>()(value, other.value);
			}
		}

		constexpr sl_compare_result compare(sl_null_t) const
		{
			return flagNull ? 0 : 1;
		}

		sl_bool equals(const Nullable& other) const noexcept
		{
			if (flagNull) {
				return other.flagNull;
			} else {
				if (other.flagNull) {
					return sl_false;
				} else {
					return Equals<T>()(value, other.value);
				}
			}
		}

		sl_bool equals(const T& other) const noexcept
		{
			if (flagNull) {
				return sl_false;
			} else {
				return Equals<T>()(value, other.value);
			}
		}

		constexpr sl_bool equals(sl_null_t) const
		{
			return flagNull;
		}

		sl_size getHashCode() const noexcept
		{
			if (flagNull) {
				return 0;
			} else {
				return Hash<T>()(value);
			}
		}
	
	public:		
		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS
		
	public:
		T value;
		sl_bool flagNull;
		
	};
	
}

#endif
