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

#ifndef CHECKHEADER_SLIB_CORE_SHARED_PTR
#define CHECKHEADER_SLIB_CORE_SHARED_PTR

#include "definition.h"

namespace slib
{
	
	namespace priv
	{
		namespace ptr
		{

			template <class T>
			class SharedObjectContainer
			{
			public:
				T object;
				sl_reg refCount;
				
			public:
				template <class... ARGS>
				SharedObjectContainer(ARGS&&... args): object(Forward<ARGS>(args)...), refCount(1) {}
				
			public:
				sl_reg increaseReference() noexcept
				{
					return Base::interlockedIncrement(&refCount);
				}

				sl_reg decreaseReference() noexcept
				{
					sl_reg nRef = Base::interlockedDecrement(&refCount);
					if (nRef == 0) {
						delete this;
					}
					return nRef;
				}

			};

			extern const void* g_shared_null;
			
		}
	}

	template <class T>
	class SLIB_EXPORT SharedPtr
	{
	public:
		typedef priv::ptr::SharedObjectContainer<T> Container;

	public:
		Container* container;
	
	public:
		SharedPtr() noexcept: container(sl_null) {}

		SharedPtr(SharedPtr&& other) noexcept
		{
			container = other.container;
			other.container = sl_null;
		}
	
		SharedPtr(const SharedPtr& other) noexcept
		{
			Container* o = other.container;
			if (o) {
				o->increaseReference();
			}
			container = o;
		}

		SharedPtr(sl_null_t) noexcept: container(sl_null) {}

		SharedPtr(T&& other) noexcept: container(new Container(Move(other))) {}

		SharedPtr(const T& other) noexcept: container(new Container(other)) {}

		~SharedPtr()
		{
			_free();
		}

	private:
		SharedPtr(Container* _container) noexcept: container(_container) {}

	public:
		template <class... Args>
		static SharedPtr create(Args&&... args)
		{
			return new Container(Forward<Args>(args)...);
		}

		static const SharedPtr& null() noexcept
		{
			return *(reinterpret_cast<SharedPtr const*>(&(priv::ptr::g_shared_null)));
		}
	
		sl_bool isNull() const noexcept
		{
			return !container;
		}

		sl_bool isNotNull() const noexcept
		{
			return container != sl_null;
		}

		void setNull() noexcept
		{
			_free();
		}

	public:
		SharedPtr& operator=(SharedPtr&& other) noexcept
		{
			container = other.container;
			other.container = sl_null;
			return *this;
		}

		SharedPtr& operator=(const SharedPtr& other) noexcept
		{
			_replace(other.container);
			return *this;
		}

		SharedPtr& operator=(sl_null_t) noexcept
		{
			_free();
			return *this;
		}

		SharedPtr& operator=(T&& other) noexcept
		{
			_free();
			container = new Container(Move(other));
			return *this;
		}

		SharedPtr& operator=(const T& other) noexcept
		{
			_free();
			container = new Container(other);
			return *this;
		}

		sl_bool operator==(const SharedPtr& other) const noexcept
		{
			return container == other.container;
		}

		sl_bool operator==(sl_null_t) const noexcept
		{
			return container == sl_null;
		}
	
		sl_bool operator!=(const SharedPtr& other) const noexcept
		{
			return container != other.container;
		}

		sl_bool operator!=(sl_null_t) const noexcept
		{
			return container != sl_null;
		}

		T& operator*() const noexcept
		{
			return container->object;
		}

		T* operator->() const noexcept
		{
			return &(container->object);
		}

		operator sl_bool() const noexcept
		{
			return container != sl_null;
		}
		
	private:
		void _replace(Container* other) noexcept
		{
			if (container) {
				container->decreaseReference();
			}
			if (other) {
				other->increaseReference();
			}
			container = other;
		}
		
		void _free() noexcept
		{
			if (container) {
				container->decreaseReference();
				container = sl_null;
			}
		}

	};
	
}

#endif
