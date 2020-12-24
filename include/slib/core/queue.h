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

#ifndef CHECKHEADER_SLIB_CORE_QUEUE
#define CHECKHEADER_SLIB_CORE_QUEUE

#include "definition.h"

#include "linked_list.h"

namespace slib
{
	
	template < class T, class CONTAINER = CLinkedList<T> >
	class SLIB_EXPORT Queue : public CONTAINER
	{
	public:
		template <class... ARGS>
		sl_bool push_NoLock(ARGS&&... args) noexcept
		{
			return this->pushBack_NoLock(Forward<ARGS>(args)...) != sl_null;
		}

		template <class... ARGS>
		sl_bool push(ARGS&&... args) noexcept
		{
			return this->pushBack(Forward<ARGS>(args)...);
		}

		sl_bool pushAll(const Queue<T, CONTAINER>* other) noexcept
		{
			return this->pushBackAll(other);
		}

		sl_bool pop_NoLock(T* _out = sl_null) noexcept
		{
			return this->popFront_NoLock(_out);
		}

		sl_bool pop(T* _out = sl_null) noexcept
		{
			return this->popFront(_out);
		}

		Queue<T, CONTAINER>* duplicate_NoLock() const noexcept
		{
			CONTAINER* container = CONTAINER::duplicate_NoLock();
			return static_cast<Queue<T, CONTAINER>*>(container);
		}

		Queue<T, CONTAINER>* duplicate() const noexcept
		{
			CONTAINER* container = CONTAINER::duplicate();
			return static_cast<Queue<T, CONTAINER>*>(container);
		}

	};
	
	template < class T, class CONTAINER = CLinkedList<T> >
	class SLIB_EXPORT Stack : public CONTAINER
	{
	public:
		template <class... ARGS>
		sl_bool push_NoLock(ARGS&&... args) noexcept
		{
			return this->pushBack_NoLock(Forward<ARGS>(args)...) != sl_null;
		}

		template <class... ARGS>
		sl_bool push(ARGS&&... args) noexcept
		{
			return this->pushBack(Forward<ARGS>(args)...);
		}

		sl_bool pushAll(const Stack<T, CONTAINER>* other) noexcept
		{
			return this->pushBackAll(other);
		}

		sl_bool pop_NoLock(T* _out = sl_null) noexcept
		{
			return this->popBack_NoLock(_out);
		}

		sl_bool pop(T* _out = sl_null) noexcept
		{
			return this->popBack(_out);
		}

		Stack<T, CONTAINER>* duplicate_NoLock() const noexcept
		{
			CONTAINER* container = CONTAINER::duplicate_NoLock();
			return static_cast<Stack<T, CONTAINER>*>(container);
		}

		Stack<T, CONTAINER>* duplicate() const noexcept
		{
			CONTAINER* container = CONTAINER::duplicate();
			return static_cast<Stack<T, CONTAINER>*>(container);
		}

	};
	
	template <class T>
	using LinkedQueue = Queue< T, CLinkedList<T> >;
	
	template <class T>
	using LinkedStack = Stack< T, CLinkedList<T> >;

}

#endif
