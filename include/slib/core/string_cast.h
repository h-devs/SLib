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

#ifndef CHECKHEADER_SLIB_CORE_STRING_CAST
#define CHECKHEADER_SLIB_CORE_STRING_CAST

#include "cast.h"
#include "string.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <string>
#endif

namespace slib
{

	template <>
	class Cast<String, signed char>
	{
	public:
		signed char operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, unsigned char>
	{
	public:
		unsigned char operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, short>
	{
	public:
		short operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, unsigned short>
	{
	public:
		unsigned short operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, int>
	{
	public:
		int operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, unsigned int>
	{
	public:
		unsigned int operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, long>
	{
	public:
		long operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, unsigned long>
	{
	public:
		unsigned long operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, sl_int64>
	{
	public:
		sl_int64 operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, sl_uint64>
	{
	public:
		sl_uint64 operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, float>
	{
	public:
		float operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String, double>
	{
	public:
		double operator()(const String& v) const noexcept;
	};

#ifdef SLIB_SUPPORT_STD_TYPES
	template <>
	class Cast<String, std::string>
	{
	public:
		std::string operator()(const String& v) const noexcept;
	};
#endif


	template <>
	class Cast<String16, signed char>
	{
	public:
		signed char operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, unsigned char>
	{
	public:
		unsigned char operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, short>
	{
	public:
		short operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, unsigned short>
	{
	public:
		unsigned short operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, int>
	{
	public:
		int operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, unsigned int>
	{
	public:
		unsigned int operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, long>
	{
	public:
		long operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, unsigned long>
	{
	public:
		unsigned long operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, sl_int64>
	{
	public:
		sl_int64 operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, sl_uint64>
	{
	public:
		sl_uint64 operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, float>
	{
	public:
		float operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String16, double>
	{
	public:
		double operator()(const String16& v) const noexcept;
	};

#ifdef SLIB_SUPPORT_STD_TYPES
	template <>
	class Cast<String16, std::u16string>
	{
	public:
		std::u16string operator()(const String16& v) const noexcept;
	};
#endif


	template <>
	class Cast<String32, signed char>
	{
	public:
		signed char operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, unsigned char>
	{
	public:
		unsigned char operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, short>
	{
	public:
		short operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, unsigned short>
	{
	public:
		unsigned short operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, int>
	{
	public:
		int operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, unsigned int>
	{
	public:
		unsigned int operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, long>
	{
	public:
		long operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, unsigned long>
	{
	public:
		unsigned long operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, sl_int64>
	{
	public:
		sl_int64 operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, sl_uint64>
	{
	public:
		sl_uint64 operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, float>
	{
	public:
		float operator()(const String32& v) const noexcept;
	};

	template <>
	class Cast<String32, double>
	{
	public:
		double operator()(const String32& v) const noexcept;
	};

#ifdef SLIB_SUPPORT_STD_TYPES
	template <>
	class Cast<String32, std::u32string>
	{
	public:
		std::u32string operator()(const String32& v) const noexcept;
	};
#endif


	template <class T>
	class Cast<T, String>
	{
	public:
		String operator()(const T& v) const
		{
			return String::from(v);
		}
	};

	template <>
	class Cast<String, String>
	{
	public:
		const String& operator()(const String& v) const
		{
			return v;
		}
	};


	template <class T>
	class Cast<T, String16>
	{
	public:
		String16 operator()(const T& v) const
		{
			return String16::from(v);
		}
	};

	template <>
	class Cast<String16, String16>
	{
	public:
		const String16& operator()(const String16& v) const
		{
			return v;
		}
	};


	template <class T>
	class Cast<T, String32>
	{
	public:
		String32 operator()(const T& v) const
		{
			return String32::from(v);
		}
	};

	template <>
	class Cast<String32, String32>
	{
	public:
		const String32& operator()(const String32& v) const
		{
			return v;
		}
	};


	template <class T>
	class Cast<StringParam, T>
	{
	public:
		T operator()(const StringParam& v) const noexcept
		{
			return Cast<String, T>()(v.toString());
		}
	};

	template <>
	class Cast<StringParam, StringParam>
	{
	public:
		const StringParam& operator()(const StringParam& v) const noexcept
		{
			return v;
		}
	};

	template <>
	class Cast<StringParam, String>
	{
	public:
		String operator()(const StringParam& v) const noexcept;
	};

	template <>
	class Cast<StringParam, String16>
	{
	public:
		String16 operator()(const StringParam& v) const noexcept;
	};

	template <>
	class Cast<StringParam, String32>
	{
	public:
		String32 operator()(const StringParam& v) const noexcept;
	};


	template <class T>
	class Cast<T, StringParam>
	{
	public:
		StringParam operator()(const T& v) const noexcept
		{
			return Cast<T, String>()(v);
		}
	};

	template <>
	class Cast<String, StringParam>
	{
	public:
		StringParam operator()(const String& v) const noexcept;
	};

	template <>
	class Cast<String16, StringParam>
	{
	public:
		StringParam operator()(const String16& v) const noexcept;
	};

	template <>
	class Cast<String32, StringParam>
	{
	public:
		StringParam operator()(const String32& v) const noexcept;
	};

}

#endif
