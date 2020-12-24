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

#ifndef CHECKHEADER_SLIB_CORE_SCOPED
#define CHECKHEADER_SLIB_CORE_SCOPED

#include "definition.h"

#include "base.h"
#include "new_helper.h"

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT ScopedPtr
	{
	public:
		ScopedPtr() : ptr(sl_null)
		{
		}

		ScopedPtr(T* _ptr) : ptr(_ptr)
		{
		}

		~ScopedPtr()
		{
			release();
		}

	public:
		void release()
		{
			delete ptr;
			ptr = sl_null;
		}

		sl_bool isNull()
		{
			return ptr == sl_null;
		}

		sl_bool isNotNull()
		{
			return ptr != sl_null;
		}

		T& operator*()
		{
			return *(ptr);
		}

		T* operator->()
		{
			return ptr;
		}

	public:
		T* ptr;

	};

	template <class T>
	class SLIB_EXPORT ScopedPtrNew : public ScopedPtr<T>
	{
	public:
		ScopedPtrNew(): ScopedPtr<T>(new T)
		{
		}

	public:
		T& operator*()
		{
			return *(this->ptr);
		}

		T* operator->()
		{
			return this->ptr;
		}

	};

	template <class T>
	class SLIB_EXPORT ScopedArray
	{
	public:
		ScopedArray() : data(sl_null), count(0)
		{
		}

		ScopedArray(T* _data, sl_size _count): data(_data), count(_count)
		{
		}

		ScopedArray(sl_size _count)
		{
			data = NewHelper<T>::create(_count);
			if (data) {
				count = _count;
			} else {
				count = 0;
			}
		}
	
		~ScopedArray()
		{
			release();
		}
	
	public:
		void release()
		{
			if (data) {
				NewHelper<T>::free(data, count);
				data = sl_null;
			}
			count = 0;
		}
	
		sl_bool isNull()
		{
			return data == sl_null;
		}
	
		sl_bool isNotNull()
		{
			return data != sl_null;
		}
	
		T& operator[](sl_size index)
		{
			return data[index];
		}

		T* operator+(sl_size offset)
		{
			return data + offset;
		}

	public:
		T* data;
		sl_size count;

	};

#define SLIB_SCOPED_ARRAY(TYPE, NAME, COUNT) \
	ScopedArray<TYPE> _scoped_array__##NAME(COUNT); \
	TYPE* NAME = _scoped_array__##NAME.data;

	
	template <class T, sl_size countStack>
	class SLIB_EXPORT ScopedBuffer
	{
	public:
		ScopedBuffer(sl_size _count)
		{
			if (_count < countStack) {
				data = stack;
			} else {
				data = NewHelper<T>::create(_count);
			}
			count = _count;
		}

		~ScopedBuffer()
		{
			release();
		}

	public:
		void release()
		{
			if (data) {
				if (data != stack) {
					NewHelper<T>::free(data, count);
				}
				data = sl_null;
			}
			count = 0;
		}

		sl_bool isNull()
		{
			return data == sl_null;
		}

		sl_bool isNotNull()
		{
			return data != sl_null;
		}

		T& operator[](sl_size index)
		{
			return data[index];
		}

		T* operator+(sl_size offset)
		{
			return data + offset;
		}

	public:
		T* data;
		sl_size count;

	private:
		T stack[countStack];

	};

#define SLIB_SCOPED_BUFFER(TYPE, STACK, NAME, COUNT) \
	ScopedBuffer<TYPE, STACK> _scoped_buf__##NAME(COUNT); \
	TYPE* NAME = _scoped_buf__##NAME.data;
	
	
	class ScopedCounter
	{
	public:
		ScopedCounter(sl_reg* p)
		{
			count = p;
			Base::interlockedIncrement(p);
		}

		~ScopedCounter()
		{
			Base::interlockedDecrement(count);
		}

	public:
		sl_reg* count;

	};

}

#endif
