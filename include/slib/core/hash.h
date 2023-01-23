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

#ifndef CHECKHEADER_SLIB_CORE_HASH
#define CHECKHEADER_SLIB_CORE_HASH

#include "definition.h"

namespace slib
{

	constexpr sl_uint8 Rehash8(sl_uint8 x)
	{
		return x ^ (x >> 4) ^ (x >> 7);
	}

	constexpr sl_uint16 Rehash16(sl_uint16 x)
	{
		return x ^ (x >> 4) ^ (x >> 7) ^ (x >> 12);
	}

	constexpr sl_uint32 Rehash32(sl_uint32 x)
	{
		return x ^ (x >> 4) ^ (x >> 7) ^ (x >> 12) ^ (x >> 16) ^ (x >> 19) ^ (x >> 20) ^ (x >> 24) ^ (x >> 27);
	}

	constexpr sl_uint64 Rehash64(sl_uint64 x)
	{
		return x ^ (x >> 4) ^ (x >> 7) ^ (x >> 12) ^ (x >> 16) ^ (x >> 19) ^ (x >> 20) ^ (x >> 24) ^ (x >> 27) ^ (x >> 31) ^ (x >> 36) ^ (x >> 40) ^ (x >> 45) ^ (x >> 49) ^ (x >> 52) ^ (x >> 57) ^ (x >> 60);
	}

	constexpr sl_size Rehash(sl_size x)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return Rehash64(x);
#else
		return Rehash32(x);
#endif
	}

	constexpr sl_uint32 Rehash64To32(sl_uint64 x)
	{
		return Rehash32((sl_uint32)(x >> 32) ^ (sl_uint32)x);
	}

	constexpr sl_size Rehash64ToSize(sl_uint64 x)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return Rehash64(x);
#else
		return Rehash64To32(x);
#endif
	}

	constexpr sl_size HashPrimitiveValue(signed char value)
	{
		return Rehash8(value);
	}

	constexpr sl_size HashPrimitiveValue(unsigned char value)
	{
		return Rehash8(value);
	}

	constexpr sl_size HashPrimitiveValue(wchar_t value)
	{
		return Rehash32(value);
	}

	constexpr sl_size HashPrimitiveValue(char16_t value)
	{
		return Rehash16(value);
	}

	constexpr sl_size HashPrimitiveValue(char32_t value)
	{
		return Rehash32(value);
	}

	constexpr sl_size HashPrimitiveValue(short value)
	{
		return Rehash16(value);
	}

	constexpr sl_size HashPrimitiveValue(unsigned short value)
	{
		return Rehash16(value);
	}

	constexpr sl_size HashPrimitiveValue(int value)
	{
		return Rehash32(value);
	}

	constexpr sl_size HashPrimitiveValue(unsigned int value)
	{
		return Rehash32(value);
	}

	constexpr sl_size HashPrimitiveValue(long value)
	{
		return Rehash32((sl_uint32)value);
	}

	constexpr sl_size HashPrimitiveValue(unsigned long value)
	{
		return Rehash32((sl_uint32)value);
	}

	constexpr sl_size HashPrimitiveValue(sl_int64 value)
	{
		return Rehash64ToSize(value);
	}

	constexpr sl_size HashPrimitiveValue(sl_uint64 value)
	{
		return Rehash64ToSize(value);
	}

	SLIB_INLINE static sl_size HashPrimitiveValue(float value) noexcept
	{
		return Rehash32(*(reinterpret_cast<sl_uint32*>(&value)));
	}

	SLIB_INLINE static sl_size HashPrimitiveValue(double value) noexcept
	{
		return Rehash64ToSize(*(reinterpret_cast<sl_uint64*>(&value)));
	}

	template <class T>
	constexpr sl_size HashPrimitiveValue(const T* v)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return Rehash64((sl_uint64)((const void*)v));
#else
		return Rehash32((sl_uint32)((const void*)v));
#endif
	}

	sl_uint32 HashBytes32(const void* buf, sl_size n) noexcept;

	sl_uint64 HashBytes64(const void* buf, sl_size n) noexcept;

	sl_size HashBytes(const void* buf, sl_size n) noexcept;

	template <class T, sl_bool isClass = __is_class(T), sl_bool isEnum = __is_enum(T)>
	class DefaultHasher {};

	template <class T>
	class DefaultHasher<T, sl_true, sl_false>
	{
	public:
		static sl_size hash(const T& a) noexcept
		{
			return a.getHashCode();
		}
	};

	template <class T>
	class DefaultHasher<T, sl_false, sl_true>
	{
	public:
		static sl_size hash(T a) noexcept
		{
#ifdef SLIB_ARCH_IS_64BIT
			return Rehash64((sl_uint64)((sl_int64)a));
#else
			return Rehash32((sl_uint32)((sl_int32)a));
#endif
		}
	};

	template <class T>
	class DefaultHasher<T, sl_false, sl_false>
	{
	public:
		static sl_size hash(T a) noexcept
		{
			return HashPrimitiveValue(a);
		}
	};

	template <class T>
	class Hash
	{
	public:
		sl_size operator()(const T& v) const noexcept
		{
			return DefaultHasher<T>::hash(v);
		}
	};

	template <class T>
	class Hash_IgnoreCase
	{
	public:
		sl_size operator()(const T& v) const noexcept
		{
			return v.getHashCode_IgnoreCase();
		}
	};

}

#endif
