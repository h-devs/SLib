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

#ifndef CHECKHEADER_SLIB_CORE_UNIQUE_PTR
#define CHECKHEADER_SLIB_CORE_UNIQUE_PTR

#include "definition.h"

namespace slib
{

	namespace priv
	{
		namespace ptr
		{
			
			template <class T>
			class UniqueHelper
			{
			public:
				typedef T* PointerType;
				typedef T ValueType;

				static void free(T* t)
				{
					delete t;
				}
			};

			template <class T>
			class UniqueHelper<T[]>
			{
			public:
				typedef T* PointerType;
				typedef T ValueType;

				static void free(T* t)
				{
					delete[] t;
				}
			};
			
			template <>
			class UniqueHelper<void>
			{
			public:
				typedef void* PointerType;
				typedef int ValueType;

				static void free(void* t)
				{
					delete[] (char*)t;
				}
			};

		}
	}

	template <class T>
	class SLIB_EXPORT UniquePtr
	{
	public:
		typedef typename priv::ptr::UniqueHelper<T>::PointerType PointerType;
		typedef typename priv::ptr::UniqueHelper<T>::ValueType ValueType;

	public:
		PointerType ptr;

	public:
		constexpr UniquePtr() noexcept: ptr(sl_null) {}

		constexpr UniquePtr(sl_null_t) noexcept: ptr(sl_null) {}

		explicit UniquePtr(PointerType _ptr) noexcept: ptr(_ptr) {}

		UniquePtr(const UniquePtr&) = delete;

		UniquePtr(UniquePtr&& other) noexcept
		{
			ptr = other.release();
		}

		~UniquePtr()
		{
			_free();
		}

	public:
		UniquePtr& operator=(sl_null_t)
		{
			reset();
			return *this;
		}

		UniquePtr& operator=(const UniquePtr&) = delete;

		UniquePtr& operator=(UniquePtr&& other) noexcept
		{
			_free();
			ptr = other.release();
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return ptr != sl_null;
		}

		ValueType& operator*() const noexcept
		{
			return *((ValueType*)(void*)ptr);
		}

		PointerType operator->() const noexcept
		{
			return ptr;
		}

		ValueType& operator[](sl_size index) const noexcept
		{
			return ptr[index];
		}

	public:
		PointerType get() const noexcept
		{
			return ptr;
		}

		PointerType release() noexcept
		{
			PointerType ret = ptr;
			ptr = sl_null;
			return ret;
		}

		void reset()
		{
			_free();
			ptr = sl_null;
		}

		void reset(PointerType _ptr)
		{
			_free();
			ptr = _ptr;
		}

	private:
		void _free()
		{
			priv::ptr::UniqueHelper<T>::free(ptr);
		}

	};
	
}

#endif
