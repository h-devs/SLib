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

#ifndef CHECKHEADER_SLIB_CORE_ARRAY_TRAITS
#define CHECKHEADER_SLIB_CORE_ARRAY_TRAITS

#include "base.h"
#include "cpp_helper.h"

#include <new>

namespace slib
{
	
	struct SLIB_EXPORT ArrayTraits_Memory
	{
		
		template <class T>
		static void construct(T* dst, sl_reg count) noexcept
		{
			while (count-- > 0) {
				new (dst++) T();
			}
		}
		
		template <class T, class TYPE>
		static void copy_construct(T* dst, const TYPE* src, sl_reg count) noexcept
		{
			while (count-- > 0) {
				new (dst++) T(*(src++));
			}
		}
		
		template <class T, class TYPE>
		static void copy(T* dst, const TYPE* src, sl_reg count) noexcept
		{
			while (count-- > 0) {
				*(dst++) = *(src++);
			}
		}
		
		template <class T>
		static void free(T* dst, sl_reg count) noexcept
		{
			while (count-- > 0) {
				(dst++)->~T();
			}
		}

	};
	
	
	struct ArrayTraits_IndexOf_Base
	{

		template <class T, class VALUE, class EQUALS>
		static sl_reg indexOf(T* data, sl_reg count, const VALUE& value, const EQUALS& equals, sl_reg startIndex) noexcept
		{
			sl_reg ret = -1;
			if (startIndex < 0) {
				startIndex = 0;
			}
			for (sl_reg i = startIndex; i < count; i++) {
				if (equals(data[i], value)) {
					ret = i;
					break;
				}
			}
			return ret;
		}

		template <class T, class VALUE, class EQUALS>
		static sl_reg lastIndexOf(T* data, sl_reg count, const VALUE& value, const EQUALS& equals, sl_reg startIndex) noexcept
		{
			sl_reg ret = -1;
			if (startIndex < 0 || startIndex >= count) {
				startIndex = count - 1;
			}
			for (sl_reg i = startIndex; i >= 0; i--) {
				if (equals(data[i], value)) {
					ret = i;
					break;
				}
			}
			return ret;
		}

	};

	template <class ARG, sl_bool isClass = __is_class(ARG)>
	struct ArrayTraits_IndexOf_Helper;

	template <class EQUALS>
	struct ArrayTraits_IndexOf_Helper<EQUALS, sl_true>
	{
		template <class T, class VALUE>
		static sl_reg indexOf(T* data, sl_reg count, const VALUE& value, const EQUALS& equals) noexcept
		{
			sl_reg ret = -1;
			for (sl_reg i = 0; i < count; i++) {
				if (equals(data[i], value)) {
					ret = i;
					break;
				}
			}
			return ret;
		}
	};

	template <class INDEX>
	struct ArrayTraits_IndexOf_Helper<INDEX, sl_false>
	{
		template <class T, class VALUE>
		static sl_reg indexOf(T* data, sl_reg count, const VALUE& value, INDEX startIndex) noexcept
		{
			return ArrayTraits_IndexOf_Base::indexOf(data, count, value, Equals<T, VALUE>(), (sl_reg)startIndex);
		}
	};

	template <class ARG, sl_bool isClass = __is_class(ARG)>
	struct ArrayTraits_LastIndexOf_Helper;

	template <class EQUALS>
	struct ArrayTraits_LastIndexOf_Helper<EQUALS, sl_true>
	{
		template <class T, class VALUE>
		sl_reg lastIndexOf(T* data, sl_reg count, const VALUE& value, const EQUALS& equals) noexcept
		{
			sl_reg ret = -1;
			for (sl_reg i = count - 1; i >= 0; i--) {
				if (equals(data[i], value)) {
					ret = i;
					break;
				}
			}
			return ret;
		}
	};

	template <class INDEX>
	struct ArrayTraits_LastIndexOf_Helper<INDEX, sl_false>
	{
		template <class T, class VALUE>
		static sl_reg lastIndexOf(T* data, sl_reg count, const VALUE& value, INDEX startIndex) noexcept
		{
			return ArrayTraits_IndexOf_Base::lastIndexOf(data, count, value, Equals<T, VALUE>(), (sl_reg)startIndex);
		}
	};

	struct SLIB_EXPORT ArrayTraits_IndexOf : public ArrayTraits_IndexOf_Base
	{

		using ArrayTraits_IndexOf_Base::indexOf;
		using ArrayTraits_IndexOf_Base::lastIndexOf;

		template <class T, class VALUE, class ARG>
		static sl_reg indexOf(T* data, sl_reg count, const VALUE& value, const ARG& arg) noexcept
		{
			return ArrayTraits_IndexOf_Helper<ARG>::indexOf(data, count, value, arg);
		}

		template <class T, class VALUE, class ARG>
		static sl_reg lastIndexOf(T* data, sl_reg count, const VALUE& value, const ARG& arg) noexcept
		{
			return ArrayTraits_LastIndexOf_Helper<ARG>::lastIndexOf(data, count, value, arg);
		}

	};
	
	
	struct SLIB_EXPORT ArrayTraits_Reverse
	{
		
		template <class T>
		static void reverse(T* data, sl_size count) noexcept
		{
			T* end = data + count;
			sl_reg n = count >> 1;
			while (n-- > 0) {
				T temp(Move(*(--end)));
				*(end) = Move(*data);
				*(data++) = Move(temp);
			}
		}

	};
	
	template <class ELEMENT_TYPE>
	struct SLIB_EXPORT ArrayTraits : public ArrayTraits_Memory, public ArrayTraits_IndexOf, public ArrayTraits_Reverse
	{
	};
	
	
	struct SLIB_EXPORT ArrayTraits_Primitive : public ArrayTraits_IndexOf
	{
		
		template <class T>
		static void construct(T* dst, sl_reg count) noexcept
		{
		}
		
		template <class T>
		static void copy_construct(T* dst, const T* src, sl_reg count) noexcept
		{
			Base::copyMemory(dst, src, ((sl_size)count)*sizeof(T));
		}

		template <class T, class TYPE>
		static void copy_construct(T* dst, const TYPE* src, sl_reg count) noexcept
		{
			while (count-- > 0) {
				*(dst++) = *(src++);
			}
		}
		
		template <class T>
		static void copy(T* dst, const T* src, sl_reg count) noexcept
		{
			Base::copyMemory(dst, src, ((sl_size)count)*sizeof(T));
		}
		
		template <class T, class TYPE>
		static void copy(T* dst, const TYPE* src, sl_reg count) noexcept
		{
			while (count-- > 0) {
				*(dst++) = *(src++);
			}
		}
		
		template <class T>
		static void free(T* dst, sl_reg count) noexcept
		{
		}
		
		template <class T>
		static void reverse(T* data, sl_size count) noexcept
		{
			T temp;
			T* end = data + count;
			sl_reg n = count >> 1;
			while (n-- > 0) {
				temp = *(--end);
				*end = *data;
				*(data++) = temp;
			}
		}

	};
	
	template <> struct ArrayTraits<char> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<signed char> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<unsigned char> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<short> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<unsigned short> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<int> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<unsigned int> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<long> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<unsigned long> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<sl_int64> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<sl_uint64> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<float> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<double> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<sl_char16> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<sl_char32> : public ArrayTraits_Primitive {};
	template <> struct ArrayTraits<sl_bool> : public ArrayTraits_Primitive {};

	
	struct SLIB_EXPORT ArrayTraits_ZeroInit : public ArrayTraits_Memory, public ArrayTraits_IndexOf, public ArrayTraits_Reverse
	{
		
		template <class T>
		static void construct(T* dst, sl_reg count) noexcept
		{
			Base::zeroMemory(dst, ((sl_size)count) * sizeof(T));
		}

	};
	
	class String;
	template <> struct ArrayTraits<String> : public ArrayTraits_ZeroInit {};
	
	class String16;
	template <> struct ArrayTraits<String16> : public ArrayTraits_ZeroInit {};
	
	template <class... TYPES> class Ref;
	template <class T> struct ArrayTraits< Ref<T> > : public ArrayTraits_ZeroInit {};
	
	template <class T> class WeakRef;
	template <class T> struct ArrayTraits< WeakRef<T> > : public ArrayTraits_ZeroInit {};
	
	template <class T> class Array;
	template <class T> struct ArrayTraits< Array<T> > : public ArrayTraits_ZeroInit {};
	
	template <class T> class List;
	template <class T> struct ArrayTraits< List<T> > : public ArrayTraits_ZeroInit {};
	
	template <class KT, class VT, class KEY_COMPARE> class Map;
	template <class KT, class VT, class KEY_COMPARE> struct ArrayTraits< Map<KT, VT, KEY_COMPARE> > : public ArrayTraits_ZeroInit {};
	
	template <class KT, class VT, class HASH, class KEY_COMPARE> class HashMap;
	template <class KT, class VT, class HASH, class KEY_COMPARE> struct ArrayTraits< HashMap<KT, VT, HASH, KEY_COMPARE> > : public ArrayTraits_ZeroInit {};

	class Variant;
	template <> struct ArrayTraits<Variant> : public ArrayTraits_ZeroInit {};
	
}

#endif
